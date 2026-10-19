#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include "tinyxml2.h"

int *ParseFile(tinyxml2::XMLElement *, bool, int, std::string);

#endif // PARSER_HPP