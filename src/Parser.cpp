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
    std::string style;

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
    if (nameAttr != NULL && typeAttr != NULL)
    {
        std::string name = nameAttr;
        std::string type = typeAttr;
        if (type == "NORMAL")
        {
            const char *regexAttr = e->Attribute("regex");
            const char *styleAttr = e->Attribute("style");
            if (regexAttr != NULL && styleAttr != NULL)
            {
                t.initialized = true;
                t.name = name;
                t.type = type;
                t.regex = regexAttr;
                t.style = styleAttr;
                return t;
            }
        }
        if (type == "DELIMETER")
        {
            const char *startAttr = e->Attribute("start");
            const char *endAttr = e->Attribute("end");
            const char *styleAttr = e->Attribute("style");
            if (startAttr != NULL && endAttr != NULL && styleAttr != NULL)
            {
                t.initialized = true;
                t.name = name;
                t.type = type;
                t.start = startAttr;
                t.end = endAttr;
                t.style = styleAttr;
                return t;
            }
        }
    }
    return t;
}

int *ParseFile(tinyxml2::XMLElement *config, int length, std::string text)
{
    int *styles = new int[length];
    for (int i = 0; i < length; i++)
    {
        styles[i] = -1;
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
            std::regex pattern(token.regex.c_str());
            auto begin = std::sregex_iterator{text.begin(), text.end(), pattern};
            auto end = std::sregex_iterator();
            for (std::sregex_iterator match = begin; match != end; ++match)
            {
                DBUG(match->position() << " : " << match->position() + match->length());
            }
        }
    }
    delete[] tokens;
    return styles;
}