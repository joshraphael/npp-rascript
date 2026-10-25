#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include "tinyxml2.h"

struct Grammar;

Grammar *LoadGrammar(tinyxml2::XMLElement *, bool);
void FreeGrammar(Grammar *);
// Styles whole lines of text (UTF-8, line breaks included) starting from a
// line state: one style per byte, and the state at the end of each line
void ParseLines(Grammar *, const std::string &, int, std::vector<int> &, std::vector<int> &);
int *ParseFile(tinyxml2::XMLElement *, bool, int, std::string);

#endif // PARSER_HPP
