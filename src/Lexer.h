#pragma once

#ifndef LEXER_H
#define LEXER_H

#include "ILexer.h"
#include "LexerModule.h"
#include "DefaultLexer.h"
#include "SciLexer.h"

using namespace Lexilla;

class LexerTemplate : public DefaultLexer
{

public:
    LexerTemplate() : DefaultLexer("LexerTemplate", SCLEX_AUTOMATIC)
    {
    }
    virtual ~LexerTemplate() {}

    static ILexer5 *LexerFactory();

    // ILexer
    void SCI_METHOD Release();
    int SCI_METHOD Version() const;
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, Scintilla::IDocument *pAccess);
};

#endif