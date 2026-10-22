#include <string>
#include <utility>
#include <vector>

#include "boost/regex.hpp"
#include "PluginInterface.h"
#include "tinyxml2.h"

#include "DebugUtils.hpp"

extern NppData nppData;

struct Token
{
    bool initialized;
    std::string name;
    std::string type;
    int style;

    // Can only have start/end or regex based on type
    std::string start;
    std::string end;
    std::string regex;

    // Optional <Group index="n" lightStyle="..." darkStyle="..." /> children:
    // (capture group, style) pairs, so one token can style each group differently
    std::vector<std::pair<int, int>> groups;
};

Token getToken(tinyxml2::XMLElement *e, std::string modeKey)
{
    Token t;
    t.initialized = false;
    const char *nameAttr = e->Attribute("name");
    const char *typeAttr = e->Attribute("type");
    std::vector<std::pair<int, int>> groups;
    for (tinyxml2::XMLElement *g = e->FirstChildElement("Group"); g != NULL; g = g->NextSiblingElement("Group"))
    {
        int index;
        int groupStyle;
        if (g->QueryIntAttribute("index", &index) == tinyxml2::XML_SUCCESS && g->QueryIntAttribute(modeKey.c_str(), &groupStyle) == tinyxml2::XML_SUCCESS)
        {
            groups.push_back(std::make_pair(index, groupStyle));
        }
    }
    // A token's own style is only optional when its groups have styles
    int style = 0;
    tinyxml2::XMLError eResult = e->QueryIntAttribute(modeKey.c_str(), &style);
    if (nameAttr != NULL && typeAttr != NULL && (eResult == tinyxml2::XML_SUCCESS || !groups.empty()))
    {
        std::string name = nameAttr;
        std::string type = typeAttr;
        if (type == "NORMAL")
        {
            const char *regexAttr = e->Attribute("regex");
            if (regexAttr != NULL)
            {
                t.initialized = true;
                t.name = name;
                t.type = type;
                t.style = style;
                t.regex = regexAttr;
                t.groups = groups;
                return t;
            }
        }
        if (type == "DELIMETER")
        {
            const char *startAttr = e->Attribute("start");
            const char *endAttr = e->Attribute("end");
            if (startAttr != NULL && endAttr != NULL)
            {
                t.initialized = true;
                t.name = name;
                t.type = type;
                t.style = style;
                t.start = startAttr;
                t.end = endAttr;
                return t;
            }
        }
    }
    return t;
}

// Converts UTF-8 to UTF-16 so the token regexes can run as boost::wregex, whose
// Windows character classes (\w, [[:alpha:]]) know non-ASCII letters like é and
// Ω. Scintilla colors bytes, not UTF-16 units, so byteOf records the byte each
// UTF-16 unit came from, plus one last entry for the end of the text.
std::wstring toUtf16(const std::string &text, std::vector<int> &byteOf)
{
    std::wstring wide;
    const int size = static_cast<int>(text.size());
    for (int i = 0, length; i < size; i += length)
    {
        // a UTF-8 character's first byte says how many bytes it has; the rest
        // must be continuation bytes (10xxxxxx), or the first byte is invalid
        const unsigned char first = static_cast<unsigned char>(text[i]);
        length = first < 0xC0 ? 1 : first < 0xE0 ? 2 : first < 0xF0 ? 3 : 4;
        if (length > size - i)
        {
            length = size - i; // the text ends partway through the character
        }
        for (int k = 1; k < length; k++)
        {
            if ((static_cast<unsigned char>(text[i + k]) & 0xC0) != 0x80)
            {
                length = k;
                break;
            }
        }
        wchar_t units[4];
        const int count = MultiByteToWideChar(CP_UTF8, 0, &text[i], length, units, 4);
        for (int u = 0; u < count; u++)
        {
            wide += units[u];
            byteOf.push_back(i);
        }
    }
    byteOf.push_back(size);
    return wide;
}

std::wstring toUtf16(const std::string &text)
{
    std::vector<int> byteOf;
    return toUtf16(text, byteOf);
}

void styleMatch(const Token &token, const boost::wsmatch &m, std::wstring::const_iterator begin, std::vector<int> &stylePerUnit)
{
    int numGroups = static_cast<int>(m.size());
    std::vector<std::pair<int, int>> groups = token.groups;
    if (groups.empty())
    {
        for (int j = numGroups > 1 ? 1 : 0; j < numGroups; j++)
        {
            groups.push_back(std::make_pair(j, token.style));
        }
    }
    for (size_t g = 0; g < groups.size(); g++)
    {
        int j = groups[g].first;
        if (j < 0 || j >= numGroups || !m[j].matched)
        {
            continue;
        }
        for (ptrdiff_t k = m[j].first - begin; k < m[j].second - begin; k++)
        {
            stylePerUnit[k] = groups[g].second;
        }
    }
}

void scanLine(const std::wstring &line, const std::vector<const Token *> &scanned, const std::vector<boost::wregex> &patterns, std::vector<int> &lineStyles)
{
    const std::wstring::const_iterator begin = line.begin();
    std::vector<boost::wsmatch> next(patterns.size());
    std::vector<bool> searched(patterns.size(), false);
    std::vector<bool> found(patterns.size(), false);
    for (ptrdiff_t p = 0; p < static_cast<ptrdiff_t>(line.size());)
    {
        int best = -1;
        for (size_t t = 0; t < patterns.size(); t++)
        {
            if (!searched[t] || (found[t] && next[t][0].first - begin < p))
            {
                boost::match_flag_type flags = boost::match_not_null | boost::match_not_dot_newline; // . never matches \n, like Oniguruma
                if (p > 0)
                {
                    flags |= boost::match_prev_avail;
                }
                found[t] = boost::regex_search(begin + p, line.cend(), next[t], patterns[t], flags);
                searched[t] = true;
            }
            if (found[t] && (best < 0 || next[t][0].first < next[best][0].first))
            {
                best = static_cast<int>(t);
            }
        }
        if (best < 0)
        {
            break;
        }
        styleMatch(*scanned[best], next[best], begin, lineStyles);
        p = next[best][0].second - begin; // never empty, so the scan always moves forward
    }
}

int *ParseFile(tinyxml2::XMLElement *config, bool darkModeEnabled, int length, std::string text)
{
    std::string modeKey = "lightStyle";
    if (darkModeEnabled)
    {
        modeKey = "darkStyle";
    }
    int *styles = new int[length];
    for (int i = 0; i < length; i++)
    {
        styles[i] = 0;
    }
    Token *tokens = new Token[config->ChildElementCount()];
    int len = 0;
    for (tinyxml2::XMLElement *e = config->FirstChildElement("Token"); e != NULL; e = e->NextSiblingElement("Token"))
    {
        Token token = getToken(e, modeKey);
        if (token.initialized)
        {
            tokens[len] = token;
            len++;
        }
    }
    // The regexes run on UTF-16 text (see toUtf16), so styles are painted by
    // UTF-16 unit first and copied to the document's bytes at the end
    std::vector<int> byteOf;
    std::wstring wideText = toUtf16(text, byteOf);
    std::vector<int> stylePerUnit(wideText.size(), 0);
    
    std::vector<const Token *> scanned;
    std::vector<boost::wregex> patterns;
    for (int i = 0; i < len; i++)
    {
        if (tokens[i].type == "NORMAL")
        {
            scanned.push_back(&tokens[i]);
            patterns.push_back(boost::wregex(toUtf16(tokens[i].regex)));
        }
    }
    // Like TextMate, each line is matched on its own: its text without the line
    // break, plus a "\n" whose style is given to the real line break
    const ptrdiff_t size = static_cast<ptrdiff_t>(wideText.size());
    for (ptrdiff_t lineStart = 0; lineStart < size;)
    {
        ptrdiff_t lineEnd = lineStart;
        while (lineEnd < size && wideText[lineEnd] != L'\r' && wideText[lineEnd] != L'\n')
        {
            lineEnd++;
        }
        ptrdiff_t breakEnd = lineEnd;
        if (breakEnd < size && wideText[breakEnd] == L'\r')
        {
            breakEnd++;
        }
        if (breakEnd < size && wideText[breakEnd] == L'\n' && (breakEnd == lineEnd || wideText[lineEnd] == L'\r'))
        {
            breakEnd++;
        }
        std::wstring line = wideText.substr(lineStart, lineEnd - lineStart) + L"\n";
        std::vector<int> lineStyles(line.size(), 0);
        scanLine(line, scanned, patterns, lineStyles);
        const ptrdiff_t lineLength = lineEnd - lineStart;
        for (ptrdiff_t k = 0; k < lineLength; k++)
        {
            stylePerUnit[lineStart + k] = lineStyles[k];
        }
        for (ptrdiff_t k = lineEnd; k < breakEnd; k++)
        {
            stylePerUnit[k] = lineStyles[lineLength];
        }
        lineStart = breakEnd > lineEnd ? breakEnd : size;
    }
    // Copy each UTF-16 unit's style to the bytes it came from (byteOf[i] up to
    // byteOf[i + 1]), so styles[b] is the style of byte b
    for (size_t i = 0; i < stylePerUnit.size(); i++)
    {
        for (int b = byteOf[i]; stylePerUnit[i] != 0 && b < byteOf[i + 1]; b++)
        {
            styles[b] = stylePerUnit[i];
        }
    }
    delete[] tokens;
    return styles;
}