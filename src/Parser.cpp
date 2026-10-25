#include <map>
#include <set>
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

    // A DELIMETER's child <Token>s and <Include>s, matched between its start and end
    std::vector<Token> children;

    // An <Include ref="#name" /> or <Include ref="$self" /> (type INCLUDE): the
    // tokens of a <Repository> <Rule>, or the top-level list, in its place
    std::string include;

    // A DELIMETER's group styles for its start and end matches: <StartGroup> and
    // <EndGroup>, or <Group> for both when a side has none (TextMate's
    // beginCaptures, endCaptures and captures)
    std::vector<std::pair<int, int>> startGroups;
    std::vector<std::pair<int, int>> endGroups;
};

std::vector<std::pair<int, int>> getGroups(tinyxml2::XMLElement *e, const char *element, const std::string &modeKey)
{
    std::vector<std::pair<int, int>> groups;
    for (tinyxml2::XMLElement *g = e->FirstChildElement(element); g != NULL; g = g->NextSiblingElement(element))
    {
        int index;
        int groupStyle;
        if (g->QueryIntAttribute("index", &index) == tinyxml2::XML_SUCCESS && g->QueryIntAttribute(modeKey.c_str(), &groupStyle) == tinyxml2::XML_SUCCESS)
        {
            groups.push_back(std::make_pair(index, groupStyle));
        }
    }
    return groups;
}

std::vector<Token> getTokens(tinyxml2::XMLElement *parent, const std::string &modeKey);

Token getToken(tinyxml2::XMLElement *e, std::string modeKey)
{
    Token t;
    t.initialized = false;
    const char *nameAttr = e->Attribute("name");
    const char *typeAttr = e->Attribute("type");
    std::vector<std::pair<int, int>> groups = getGroups(e, "Group", modeKey);
    // A NORMAL token's own style is only optional when its groups have styles. A
    // DELIMETER's is always optional: without one, it takes its parent's style
    int style = 0;
    tinyxml2::XMLError eResult = e->QueryIntAttribute(modeKey.c_str(), &style);
    if (nameAttr != NULL && typeAttr != NULL)
    {
        std::string name = nameAttr;
        std::string type = typeAttr;
        if (type == "NORMAL" && (eResult == tinyxml2::XML_SUCCESS || !groups.empty()))
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
                t.startGroups = getGroups(e, "StartGroup", modeKey);
                t.endGroups = getGroups(e, "EndGroup", modeKey);
                if (t.startGroups.empty())
                {
                    t.startGroups = groups;
                }
                if (t.endGroups.empty())
                {
                    t.endGroups = groups;
                }
                t.children = getTokens(e, modeKey);
                return t;
            }
        }
    }
    return t;
}

std::vector<Token> getTokens(tinyxml2::XMLElement *parent, const std::string &modeKey)
{
    std::vector<Token> tokens;
    for (tinyxml2::XMLElement *e = parent->FirstChildElement(); e != NULL; e = e->NextSiblingElement())
    {
        std::string element = e->Name();
        if (element == "Token")
        {
            Token token = getToken(e, modeKey);
            if (token.initialized)
            {
                tokens.push_back(token);
            }
        }
        else if (element == "Include" && e->Attribute("ref") != NULL)
        {
            Token include;
            include.initialized = true;
            include.style = 0;
            include.type = "INCLUDE";
            include.include = e->Attribute("ref");
            tokens.push_back(include);
        }
    }
    return tokens;
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

struct Scanner
{
    std::vector<const Token *> tokens;
    std::vector<int> offsets;
    std::vector<int> markCounts;
    boost::wregex combined;
};

// What is matched inside an open DELIMETER: its children (one Scanner) and its
// end, searched on its own so it can match an empty string ($ or a lookahead)
struct Region
{
    Scanner children;
    boost::wregex end;
    int endMarkCount;
};

// A NORMAL token is matched by its regex, a DELIMETER by its start
Scanner buildScanner(const std::vector<const Token *> &tokens)
{
    Scanner scanner;
    std::wstring combined;
    int group = 1;
    for (size_t i = 0; i < tokens.size(); i++)
    {
        std::wstring regex = toUtf16(tokens[i]->type == "NORMAL" ? tokens[i]->regex : tokens[i]->start);
        scanner.tokens.push_back(tokens[i]);
        scanner.offsets.push_back(group);
        scanner.markCounts.push_back(static_cast<int>(boost::wregex(regex).mark_count()));
        group += 1 + scanner.markCounts.back();
        combined += (combined.empty() ? L"(" : L"|(") + regex + L")";
    }
    if (!scanner.tokens.empty())
    {
        scanner.combined = boost::wregex(combined);
    }
    return scanner;
}

// The top-level token list ($self) and the <Repository>'s named <Rule>s
struct TokenLists
{
    std::vector<Token> self;
    std::map<std::string, std::vector<Token>> repository;
};

// The tokens to scan for a list: each <Include> replaced in place by the tokens
// of the list it names. A list already being expanded is skipped, so includes
// can't loop, and a DELIMETER's children are their own list, expanded for its
// region
void flatten(const std::vector<Token> &list, const TokenLists &lists, std::set<const std::vector<Token> *> &expanding, std::vector<const Token *> &tokens)
{
    expanding.insert(&list);
    for (size_t i = 0; i < list.size(); i++)
    {
        if (list[i].type != "INCLUDE")
        {
            tokens.push_back(&list[i]);
            continue;
        }
        const std::vector<Token> *included = NULL;
        if (list[i].include == "$self")
        {
            included = &lists.self;
        }
        else if (list[i].include.size() > 1 && list[i].include[0] == '#' && lists.repository.count(list[i].include.substr(1)))
        {
            included = &lists.repository.at(list[i].include.substr(1));
        }
        if (included != NULL && !expanding.count(included))
        {
            flatten(*included, lists, expanding, tokens);
        }
    }
    expanding.erase(&list);
}

std::vector<const Token *> flatten(const std::vector<Token> &list, const TokenLists &lists)
{
    std::set<const std::vector<Token> *> expanding;
    std::vector<const Token *> tokens;
    flatten(list, lists, expanding, tokens);
    return tokens;
}

void buildRegions(const std::vector<const Token *> &tokens, const TokenLists &lists, std::map<const Token *, Region> &regions)
{
    for (size_t i = 0; i < tokens.size(); i++)
    {
        if (tokens[i]->type != "DELIMETER" || regions.count(tokens[i]))
        {
            continue;
        }
        std::vector<const Token *> children = flatten(tokens[i]->children, lists);
        Region &region = regions[tokens[i]];
        region.children = buildScanner(children);
        region.end = boost::wregex(toUtf16(tokens[i]->end));
        region.endMarkCount = static_cast<int>(region.end.mark_count());
        buildRegions(children, lists, regions);
    }
}

// The begin/end tokens open at the end of a line, carried to the next line,
// each with the style its text gets (its own, or its parent's when it has none)
struct OpenRegion
{
    const Token *token;
    int style;
};

void styleGroups(const std::vector<std::pair<int, int>> &groups, const boost::wsmatch &m, int offset, int markCount, std::wstring::const_iterator begin, std::vector<int> &lineStyles)
{
    for (size_t g = 0; g < groups.size(); g++)
    {
        int j = groups[g].first;
        if (j < 0 || j > markCount || !m[offset + j].matched)
        {
            continue;
        }
        for (ptrdiff_t k = m[offset + j].first - begin; k < m[offset + j].second - begin; k++)
        {
            lineStyles[k] = groups[g].second;
        }
    }
}

void styleMatch(const Token &token, const boost::wsmatch &m, int offset, int markCount, std::wstring::const_iterator begin, std::vector<int> &lineStyles)
{
    if (token.groups.empty())
    {
        for (int j = markCount > 0 ? 1 : 0; j <= markCount; j++)
        {
            for (ptrdiff_t k = m[offset + j].first - begin; m[offset + j].matched && k < m[offset + j].second - begin; k++)
            {
                lineStyles[k] = token.style;
            }
        }
        return;
    }
    styleGroups(token.groups, m, offset, markCount, begin, lineStyles);
}

void fill(std::vector<int> &lineStyles, ptrdiff_t from, ptrdiff_t to, int style)
{
    for (ptrdiff_t k = from; k < to; k++)
    {
        lineStyles[k] = style;
    }
}

void scanLine(const std::wstring &line, const Scanner &top, const std::map<const Token *, Region> &regions, std::vector<OpenRegion> &open, boost::wsmatch &m, boost::wsmatch &endMatch, std::vector<int> &lineStyles)
{
    const std::wstring::const_iterator begin = line.begin();
    const ptrdiff_t size = static_cast<ptrdiff_t>(line.size());
    for (ptrdiff_t p = 0; p < size;)
    {
        boost::match_flag_type flags = boost::match_not_dot_newline; // . never matches \n, like Oniguruma
        if (p > 0)
        {
            flags |= boost::match_prev_avail;
        }
        const int style = open.empty() ? 0 : open.back().style;
        const Scanner &scanner = open.empty() ? top : regions.at(open.back().token).children;
        // tokens never match an empty string, so the scan always moves forward
        bool found = !scanner.tokens.empty() && boost::regex_search(begin + p, line.cend(), m, scanner.combined, flags | boost::match_not_null);
        if (!open.empty())
        {
            // inside a region, its end wins a tie with its children
            const Region &region = regions.at(open.back().token);
            if (boost::regex_search(begin + p, line.cend(), endMatch, region.end, flags) && (!found || endMatch[0].first <= m[0].first))
            {
                fill(lineStyles, p, endMatch[0].second - begin, style);
                styleGroups(open.back().token->endGroups, endMatch, 0, region.endMarkCount, begin, lineStyles);
                open.pop_back();
                p = endMatch[0].second - begin;
                continue;
            }
        }
        if (!found)
        {
            fill(lineStyles, p, size, style);
            break;
        }
        size_t t = 0;
        while (!m[scanner.offsets[t]].matched)
        {
            t++;
        }
        const Token &token = *scanner.tokens[t];
        fill(lineStyles, p, m[0].first - begin, style);
        if (token.type == "DELIMETER")
        {
            OpenRegion region = {&token, token.style != 0 ? token.style : style};
            fill(lineStyles, m[0].first - begin, m[0].second - begin, region.style);
            styleGroups(token.startGroups, m, scanner.offsets[t], scanner.markCounts[t], begin, lineStyles);
            open.push_back(region);
        }
        else
        {
            fill(lineStyles, m[0].first - begin, m[0].second - begin, style);
            styleMatch(token, m, scanner.offsets[t], scanner.markCounts[t], begin, lineStyles);
        }
        p = m[0].second - begin;
    }
}

// A grammar loaded from RAScript.xml, compiled once and reused for every Lex
// call. states holds every distinct stack of open regions seen at the end of a
// line, so a line's state (Scintilla's line state) is an index into it, and 0 is
// no open regions
struct Grammar
{
    TokenLists lists;
    Scanner top;
    std::map<const Token *, Region> regions;
    std::vector<std::vector<OpenRegion>> states;
};

Grammar *LoadGrammar(tinyxml2::XMLElement *config, bool darkModeEnabled)
{
    std::string modeKey = darkModeEnabled ? "darkStyle" : "lightStyle";
    Grammar *grammar = new Grammar();
    grammar->lists.self = getTokens(config, modeKey);
    tinyxml2::XMLElement *repository = config->FirstChildElement("Repository");
    for (tinyxml2::XMLElement *r = repository != NULL ? repository->FirstChildElement("Rule") : NULL; r != NULL; r = r->NextSiblingElement("Rule"))
    {
        if (r->Attribute("name") != NULL)
        {
            grammar->lists.repository[r->Attribute("name")] = getTokens(r, modeKey);
        }
    }
    std::vector<const Token *> topTokens = flatten(grammar->lists.self, grammar->lists);
    grammar->top = buildScanner(topTokens);
    buildRegions(topTokens, grammar->lists, grammar->regions);
    grammar->states.push_back(std::vector<OpenRegion>());
    return grammar;
}

void FreeGrammar(Grammar *grammar)
{
    delete grammar;
}

void addStyles(const std::vector<Token> &tokens, std::set<int> &styles)
{
    for (size_t i = 0; i < tokens.size(); i++)
    {
        styles.insert(tokens[i].style);
        const std::vector<std::pair<int, int>> *lists[] = {&tokens[i].groups, &tokens[i].startGroups, &tokens[i].endGroups};
        for (size_t l = 0; l < 3; l++)
        {
            for (size_t g = 0; g < lists[l]->size(); g++)
            {
                styles.insert((*lists[l])[g].second);
            }
        }
        addStyles(tokens[i].children, styles);
    }
}

std::vector<int> GrammarStyles(Grammar *grammar)
{
    std::set<int> styles;
    styles.insert(0);
    addStyles(grammar->lists.self, styles);
    for (std::map<std::string, std::vector<Token>>::const_iterator r = grammar->lists.repository.begin(); r != grammar->lists.repository.end(); ++r)
    {
        addStyles(r->second, styles);
    }
    return std::vector<int>(styles.begin(), styles.end());
}

int stateOf(Grammar &grammar, const std::vector<OpenRegion> &open)
{
    for (size_t i = 0; i < grammar.states.size(); i++)
    {
        const std::vector<OpenRegion> &state = grammar.states[i];
        bool same = state.size() == open.size();
        for (size_t k = 0; same && k < open.size(); k++)
        {
            same = state[k].token == open[k].token && state[k].style == open[k].style;
        }
        if (same)
        {
            return static_cast<int>(i);
        }
    }
    grammar.states.push_back(open);
    return static_cast<int>(grammar.states.size() - 1);
}

void ParseLines(Grammar *grammar, const std::string &text, int state, std::vector<int> &styles, std::vector<int> &lineStates)
{
    styles.assign(text.size(), 0);
    lineStates.clear();
    if (grammar->top.tokens.empty())
    {
        return;
    }
    std::vector<OpenRegion> open;
    if (state > 0 && state < static_cast<int>(grammar->states.size()))
    {
        open = grammar->states[state];
    }
    // The regexes run on UTF-16 text (see toUtf16), so styles are painted by
    // UTF-16 unit first and copied to the document's bytes at the end
    std::vector<int> byteOf;
    std::wstring wideText = toUtf16(text, byteOf);
    std::vector<int> stylePerUnit(wideText.size(), 0);
    // Like TextMate, each line is matched on its own: its text without the line
    // break, plus a "\n" whose style is given to the real line break
    const ptrdiff_t size = static_cast<ptrdiff_t>(wideText.size());
    std::wstring line;
    std::vector<int> lineStyles;
    boost::wsmatch m;
    boost::wsmatch endMatch;
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
        line.assign(wideText, lineStart, lineEnd - lineStart);
        line += L'\n';
        lineStyles.assign(line.size(), 0);
        scanLine(line, grammar->top, grammar->regions, open, m, endMatch, lineStyles);
        lineStates.push_back(stateOf(*grammar, open));
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
}

int *ParseFile(tinyxml2::XMLElement *config, bool darkModeEnabled, int length, std::string text)
{
    Grammar *grammar = LoadGrammar(config, darkModeEnabled);
    std::vector<int> styles;
    std::vector<int> lineStates;
    ParseLines(grammar, text, 0, styles, lineStates);
    FreeGrammar(grammar);
    int *result = new int[length];
    for (int i = 0; i < length; i++)
    {
        result[i] = i < static_cast<int>(styles.size()) ? styles[i] : 0;
    }
    return result;
}
