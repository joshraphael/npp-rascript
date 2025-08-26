#ifndef LEXRASCRIPT_HPP
#define LEXRASCRIPT_HPP

#include "ILexer.h"
#include "LexerModule.h"
#include "DefaultLexer.h"
#include "SciLexer.h"

using namespace Lexilla;

class LexRAScript : public DefaultLexer
{

public:
    LexRAScript() : DefaultLexer("RAScript", SCLEX_AUTOMATIC)
    {
    }
    virtual ~LexRAScript() {}

    static ILexer5 *LexerFactory();

    // ILexer
    void SCI_METHOD Release();
    int SCI_METHOD Version() const;
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, Scintilla::IDocument *pAccess);
};

#endif // LEXRASCRIPT_HPP