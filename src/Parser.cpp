#include "boost/regex.hpp" // IWYU pragma: keep
#include "tinyxml2.h"
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>

// The kinds of TextMate rule: a single regex (match), a region (begin and end), a
// list of rules (patterns), and an include of another list
enum class Kind
{
    Match,
    BeginEnd,
    List,
    Include
};

struct Rule
{
    bool initialized = false;
    Kind kind = Kind::List;
    int style = 0;

    // match for a Match rule, begin and end for a BeginEnd rule
    std::string match;
    std::string begin;
    std::string end;

    // Optional <Capture index="n" lightStyle="..." darkStyle="..." /> children:
    // (capture group, style) pairs, so one rule can style each group differently
    std::vector<std::pair<int, int>> captures;

    // A BeginEnd rule's group styles for its begin and end matches: <BeginCapture>
    // and <EndCapture>, or <Capture> for both when a side has none (TextMate's
    // beginCaptures, endCaptures and captures)
    std::vector<std::pair<int, int>> beginCaptures;
    std::vector<std::pair<int, int>> endCaptures;

    // A BeginEnd rule's child <Rule>s and <Include>s, matched between its begin and
    // end, or a List rule's rules
    std::vector<Rule> patterns;

    // An <Include ref="#name" /> or <Include ref="$self" />: the rules of a
    // <Repository> <Rule>, or the top-level list, in its place
    std::string include;
};

std::vector<std::pair<int, int>> getCaptures(tinyxml2::XMLElement *e, const char *element, const std::string &modeKey)
{
    std::vector<std::pair<int, int>> captures;
    for (tinyxml2::XMLElement *g = e->FirstChildElement(element); g != NULL; g = g->NextSiblingElement(element))
    {
        int index;
        int groupStyle;
        if (g->QueryIntAttribute("index", &index) == tinyxml2::XML_SUCCESS &&
            g->QueryIntAttribute(modeKey.c_str(), &groupStyle) == tinyxml2::XML_SUCCESS)
        {
            captures.push_back(std::make_pair(index, groupStyle));
        }
    }
    return captures;
}

std::vector<Rule> getRules(tinyxml2::XMLElement *parent, const std::string &modeKey);

// A <Rule>'s kind follows from its attributes, like a TextMate rule: match makes a
// Match rule, begin and end a BeginEnd rule, and neither a List of its child
// rules
Rule getRule(tinyxml2::XMLElement *e, const std::string &modeKey)
{
    Rule t;
    const char *matchAttr = e->Attribute("match");
    const char *beginAttr = e->Attribute("begin");
    const char *endAttr = e->Attribute("end");
    std::vector<std::pair<int, int>> captures = getCaptures(e, "Capture", modeKey);
    // A Match rule's own style is only optional when its captures have styles. A
    // BeginEnd rule's is always optional: without one, it takes its parent's style
    int style = 0;
    tinyxml2::XMLError eResult = e->QueryIntAttribute(modeKey.c_str(), &style);
    t.style = style;
    if (matchAttr != NULL)
    {
        if (eResult == tinyxml2::XML_SUCCESS || !captures.empty())
        {
            t.initialized = true;
            t.kind = Kind::Match;
            t.match = matchAttr;
            t.captures = captures;
        }
        return t;
    }
    if (beginAttr != NULL && endAttr != NULL)
    {
        t.initialized = true;
        t.kind = Kind::BeginEnd;
        t.begin = beginAttr;
        t.end = endAttr;
        t.beginCaptures = getCaptures(e, "BeginCapture", modeKey);
        t.endCaptures = getCaptures(e, "EndCapture", modeKey);
        if (t.beginCaptures.empty())
        {
            t.beginCaptures = captures;
        }
        if (t.endCaptures.empty())
        {
            t.endCaptures = captures;
        }
        t.patterns = getRules(e, modeKey);
        return t;
    }
    if (beginAttr == NULL && endAttr == NULL)
    {
        t.initialized = true;
        t.kind = Kind::List;
        t.patterns = getRules(e, modeKey);
    }
    return t;
}

std::vector<Rule> getRules(tinyxml2::XMLElement *parent, const std::string &modeKey)
{
    std::vector<Rule> rules;
    for (tinyxml2::XMLElement *e = parent->FirstChildElement(); e != NULL; e = e->NextSiblingElement())
    {
        std::string element = e->Name();
        if (element == "Rule")
        {
            Rule rule = getRule(e, modeKey);
            if (rule.initialized)
            {
                rules.push_back(rule);
            }
        }
        else if (element == "Include" && e->Attribute("ref") != NULL)
        {
            Rule include;
            include.initialized = true;
            include.kind = Kind::Include;
            include.include = e->Attribute("ref");
            rules.push_back(include);
        }
    }
    return rules;
}

// Converts UTF-8 to UTF-16 so the rules' regexes can run as boost::wregex, whose
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
    std::vector<const Rule *> rules;
    std::vector<int> offsets;
    std::vector<int> markCounts;
    boost::wregex combined;
};

// What is matched inside an open BeginEnd rule: its patterns (one Scanner) and its
// end, searched on its own so it can match an empty string ($ or a lookahead)
struct Region
{
    Scanner patterns;
    boost::wregex end;
    int endMarkCount;
};

// A Match rule is matched by its match, a BeginEnd rule by its begin
Scanner buildScanner(const std::vector<const Rule *> &rules)
{
    Scanner scanner;
    std::wstring combined;
    int group = 1;
    for (size_t i = 0; i < rules.size(); i++)
    {
        std::wstring regex = toUtf16(rules[i]->kind == Kind::Match ? rules[i]->match : rules[i]->begin);
        scanner.rules.push_back(rules[i]);
        scanner.offsets.push_back(group);
        scanner.markCounts.push_back(static_cast<int>(boost::wregex(regex).mark_count()));
        group += 1 + scanner.markCounts.back();
        combined += (combined.empty() ? L"(" : L"|(") + regex + L")";
    }
    if (!scanner.rules.empty())
    {
        scanner.combined = boost::wregex(combined);
    }
    return scanner;
}

// The top-level rules ($self) and the <Repository>'s named <Rule>s
struct RuleLists
{
    std::vector<Rule> self;
    std::map<std::string, std::vector<Rule>> repository;
};

// The rules to scan for a list: each <Include> or List rule replaced in place by
// the rules it holds. A list already being expanded is skipped, so includes can't
// loop, and a BeginEnd rule's patterns are their own list, expanded for its region
void flatten(const std::vector<Rule> &list, const RuleLists &lists, std::set<const std::vector<Rule> *> &expanding,
             std::vector<const Rule *> &rules)
{
    expanding.insert(&list);
    for (size_t i = 0; i < list.size(); i++)
    {
        if (list[i].kind == Kind::List)
        {
            flatten(list[i].patterns, lists, expanding, rules);
            continue;
        }
        if (list[i].kind != Kind::Include)
        {
            rules.push_back(&list[i]);
            continue;
        }
        const std::vector<Rule> *included = NULL;
        if (list[i].include == "$self")
        {
            included = &lists.self;
        }
        else if (list[i].include.size() > 1 && list[i].include[0] == '#' &&
                 lists.repository.count(list[i].include.substr(1)))
        {
            included = &lists.repository.at(list[i].include.substr(1));
        }
        if (included != NULL && !expanding.count(included))
        {
            flatten(*included, lists, expanding, rules);
        }
    }
    expanding.erase(&list);
}

std::vector<const Rule *> flatten(const std::vector<Rule> &list, const RuleLists &lists)
{
    std::set<const std::vector<Rule> *> expanding;
    std::vector<const Rule *> rules;
    flatten(list, lists, expanding, rules);
    return rules;
}

void buildRegions(const std::vector<const Rule *> &rules, const RuleLists &lists,
                  std::map<const Rule *, Region> &regions)
{
    for (size_t i = 0; i < rules.size(); i++)
    {
        if (rules[i]->kind != Kind::BeginEnd || regions.count(rules[i]))
        {
            continue;
        }
        std::vector<const Rule *> patterns = flatten(rules[i]->patterns, lists);
        Region &region = regions[rules[i]];
        region.patterns = buildScanner(patterns);
        region.end = boost::wregex(toUtf16(rules[i]->end));
        region.endMarkCount = static_cast<int>(region.end.mark_count());
        buildRegions(patterns, lists, regions);
    }
}

// The BeginEnd rules open at the end of a line, carried to the next line,
// each with the style its text gets (its own, or its parent's when it has none)
struct OpenRegion
{
    const Rule *rule;
    int style;
};

void styleCaptures(const std::vector<std::pair<int, int>> &captures, const boost::wsmatch &m, int offset, int markCount,
                   std::wstring::const_iterator begin, std::vector<int> &lineStyles)
{
    for (size_t g = 0; g < captures.size(); g++)
    {
        int j = captures[g].first;
        if (j < 0 || j > markCount || !m[offset + j].matched)
        {
            continue;
        }
        for (ptrdiff_t k = m[offset + j].first - begin; k < m[offset + j].second - begin; k++)
        {
            lineStyles[k] = captures[g].second;
        }
    }
}

void styleMatch(const Rule &rule, const boost::wsmatch &m, int offset, int markCount,
                std::wstring::const_iterator begin, std::vector<int> &lineStyles)
{
    if (rule.style != 0)
    {
        for (ptrdiff_t k = m[offset].first - begin; k < m[offset].second - begin; k++)
        {
            lineStyles[k] = rule.style;
        }
    }
    styleCaptures(rule.captures, m, offset, markCount, begin, lineStyles);
}

void fill(std::vector<int> &lineStyles, ptrdiff_t from, ptrdiff_t to, int style)
{
    for (ptrdiff_t k = from; k < to; k++)
    {
        lineStyles[k] = style;
    }
}

void scanLine(const std::wstring &line, const Scanner &top, const std::map<const Rule *, Region> &regions,
              std::vector<OpenRegion> &open, boost::wsmatch &m, boost::wsmatch &endMatch, std::vector<int> &lineStyles)
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
        const Scanner &scanner = open.empty() ? top : regions.at(open.back().rule).patterns;
        // rules never match an empty string, so the scan always moves forward
        bool found = !scanner.rules.empty() &&
                     boost::regex_search(begin + p, line.cend(), m, scanner.combined, flags | boost::match_not_null);
        if (!open.empty())
        {
            // inside a region, its end wins a tie with its patterns
            const Region &region = regions.at(open.back().rule);
            if (boost::regex_search(begin + p, line.cend(), endMatch, region.end, flags) &&
                (!found || endMatch[0].first <= m[0].first))
            {
                fill(lineStyles, p, endMatch[0].second - begin, style);
                styleCaptures(open.back().rule->endCaptures, endMatch, 0, region.endMarkCount, begin, lineStyles);
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
        const Rule &rule = *scanner.rules[t];
        fill(lineStyles, p, m[0].first - begin, style);
        if (rule.kind == Kind::BeginEnd)
        {
            OpenRegion region = {&rule, rule.style != 0 ? rule.style : style};
            fill(lineStyles, m[0].first - begin, m[0].second - begin, region.style);
            styleCaptures(rule.beginCaptures, m, scanner.offsets[t], scanner.markCounts[t], begin, lineStyles);
            open.push_back(region);
        }
        else
        {
            fill(lineStyles, m[0].first - begin, m[0].second - begin, style);
            styleMatch(rule, m, scanner.offsets[t], scanner.markCounts[t], begin, lineStyles);
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
    RuleLists lists;
    Scanner top;
    std::map<const Rule *, Region> regions;
    std::vector<std::vector<OpenRegion>> states;
};

Grammar *LoadGrammar(tinyxml2::XMLElement *config, bool darkModeEnabled)
{
    std::string modeKey = darkModeEnabled ? "darkStyle" : "lightStyle";
    Grammar *grammar = new Grammar();
    grammar->lists.self = getRules(config, modeKey);
    tinyxml2::XMLElement *repository = config->FirstChildElement("Repository");
    for (tinyxml2::XMLElement *r = repository != NULL ? repository->FirstChildElement("Rule") : NULL; r != NULL;
         r = r->NextSiblingElement("Rule"))
    {
        // like a TextMate repository entry, a rule with match or begin/end is a list
        // of just itself
        Rule rule = getRule(r, modeKey);
        if (r->Attribute("name") != NULL && rule.initialized)
        {
            grammar->lists.repository[r->Attribute("name")] =
                rule.kind == Kind::List ? rule.patterns : std::vector<Rule>(1, rule);
        }
    }
    std::vector<const Rule *> topRules = flatten(grammar->lists.self, grammar->lists);
    grammar->top = buildScanner(topRules);
    buildRegions(topRules, grammar->lists, grammar->regions);
    grammar->states.push_back(std::vector<OpenRegion>());
    return grammar;
}

void FreeGrammar(Grammar *grammar)
{
    delete grammar;
}

void addStyles(const std::vector<Rule> &rules, std::set<int> &styles)
{
    for (size_t i = 0; i < rules.size(); i++)
    {
        styles.insert(rules[i].style);
        const std::vector<std::pair<int, int>> *lists[] = {&rules[i].captures, &rules[i].beginCaptures,
                                                           &rules[i].endCaptures};
        for (size_t l = 0; l < 3; l++)
        {
            for (size_t g = 0; g < lists[l]->size(); g++)
            {
                styles.insert((*lists[l])[g].second);
            }
        }
        addStyles(rules[i].patterns, styles);
    }
}

std::vector<int> GrammarStyles(Grammar *grammar)
{
    std::set<int> styles;
    styles.insert(0);
    addStyles(grammar->lists.self, styles);
    for (std::map<std::string, std::vector<Rule>>::const_iterator r = grammar->lists.repository.begin();
         r != grammar->lists.repository.end();
         ++r)
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
            same = state[k].rule == open[k].rule && state[k].style == open[k].style;
        }
        if (same)
        {
            return static_cast<int>(i);
        }
    }
    grammar.states.push_back(open);
    return static_cast<int>(grammar.states.size() - 1);
}

void ParseLines(Grammar *grammar, const std::string &text, int state, std::vector<int> &styles,
                std::vector<int> &lineStates)
{
    styles.assign(text.size(), 0);
    lineStates.clear();
    if (grammar->top.rules.empty())
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
