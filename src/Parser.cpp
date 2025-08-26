#include <string>

#include "DebugUtils.h"
#include "PluginInterface.h"
#include "tinyxml2.h"

extern NppData nppData;

void ParseFile(tinyxml2::XMLElement *config, int *styles, std::string /* text */)
{
    styles[0] = 2;
    for (tinyxml2::XMLElement *e = config->FirstChildElement("Token"); e != NULL; e = e->NextSiblingElement("Token"))
    {
        std::string wmName = e->Attribute("name");
        DBUG(wmName.c_str());
    }
}