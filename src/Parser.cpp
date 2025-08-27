#include <string>
#include <regex>

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
};

Token getToken(tinyxml2::XMLElement *e)
{
    Token t;
    t.initialized = false;
    const char *nameAttr = e->Attribute("name");
    const char *typeAttr = e->Attribute("type");
    int style;
    tinyxml2::XMLError eResult = e->QueryIntAttribute("style", &style);
    if (nameAttr != NULL && typeAttr != NULL && eResult == tinyxml2::XML_SUCCESS)
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

int *ParseFile(tinyxml2::XMLElement *config, int length, std::string /*text*/)
{
    int *styles = new int[length];
    for (int i = 0; i < length; i++)
    {
        styles[i] = 0;
    }
    Token *tokens = new Token[config->ChildElementCount()];
    int len = 0;
    for (tinyxml2::XMLElement *e = config->FirstChildElement("Token"); e != NULL; e = e->NextSiblingElement("Token"))
    {
        Token token = getToken(e);
        if (token.initialized)
        {
            tokens[len] = token;
            len++;
        }
    }
    for (int i = 0; i < len; i++)
    {
        Token token = tokens[i];
        if (token.type == "NORMAL")
        {
            // std::regex pattern(token.regex.c_str());
            // std::string::const_iterator start = text.begin();
            // std::string::const_iterator end = text.end();
            // std::smatch m;
            // while (std::regex_search(start, end, m, pattern))
            // {
            //     int numGroups = m.size();
            //     int strLen = m.length();
            //     int pos = m.position();
            //     if (numGroups > 1) // regex contains groups, need to parse each group and stylize them appropriately
            //     {
            //         for (int j = 1; j <= numGroups; j++)
            //         {
            //             for (int k = 1; k <= m[j].length(); k++)
            //             {
            //                 styles[m.position(j) + k] = token.style;
            //             }
            //         }
            //     }
            //     else // regex contains no groups, just stylize the matched string
            //     {
            //         for (int j = 1; j <= strLen; j++)
            //         {
            //             styles[pos + j] = token.style;
            //         }
            //     }
            //     start = m[0].second;
            // }
        }
    }
    delete[] tokens;
    return styles;
}