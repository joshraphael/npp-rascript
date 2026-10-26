#ifndef LEXRASCRIPT_HPP
#define LEXRASCRIPT_HPP

#include <cstdint> // IWYU pragma: keep

#include "ILexer.h"
#include "LexerModule.h" // IWYU pragma: keep

#include "DefaultLexer.h"
#include "SciLexer.h"

class LexRAScript : public Lexilla::DefaultLexer
{
public:
    LexRAScript() : DefaultLexer("RAScript", SCLEX_AUTOMATIC) {}
    virtual ~LexRAScript();

    static Scintilla::ILexer5 *LexerFactory();

    // ILexer
    void SCI_METHOD Release() override;
    int SCI_METHOD Version() const override;
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle,
                        Scintilla::IDocument *pAccess) override;

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