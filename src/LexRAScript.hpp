#ifndef LEXRASCRIPT_HPP
#define LEXRASCRIPT_HPP

#include <stdint.h>
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
    virtual ~LexRAScript();

    static ILexer5 *LexerFactory();

    // ILexer
    void SCI_METHOD Release();
    int SCI_METHOD Version() const;
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, Scintilla::IDocument *pAccess);

private:
    // RAScript.xml, loaded once and kept for the light or dark styles it was loaded with
    struct Grammar *grammar = nullptr;
    bool grammarDarkMode = false;

    // The view, background color and styleGeneration the backgrounds were last set for
    void *backgroundsView = nullptr;
    unsigned long backgroundsColor = 0;
    int backgroundsGeneration = -1;
};

#endif // LEXRASCRIPT_HPP