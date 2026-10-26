#ifndef PARSER_HPP
#define PARSER_HPP

#include "tinyxml2.h"
#include <string>
#include <vector>

struct Grammar;

Grammar *LoadGrammar(tinyxml2::XMLElement *, bool);
void FreeGrammar(Grammar *);
// Every style a grammar's tokens can give text, and 0 for unstyled text
std::vector<int> GrammarStyles(Grammar *);
// Styles whole lines of text (UTF-8, line breaks included) starting from a
// line state: one style per byte, and the state at the end of each line
void ParseLines(Grammar *, const std::string &, int, std::vector<int> &, std::vector<int> &);

#endif // PARSER_HPP
