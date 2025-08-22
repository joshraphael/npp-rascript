#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <string_view>
#include <iostream>
#include <sstream>
#include <assert.h>

#include "Lexer.h"

#include "PluginInterface.h"
#include "LexAccessor.h"
#include "StyleContext.h"

extern NppData nppData;
const int RASCRIPT_STYLE_DEFAULT = 0;

#ifdef DEBUG
#define DBUG(msg)            \
    std::wostringstream os_; \
    os_ << msg;              \
    OutputDebugStringW(os_.str().c_str());
#else
#define DBUG(msg)
#endif

Scintilla::ILexer5 *LexerTemplate::LexerFactory()
{
    return new LexerTemplate();
}

LexerFactoryFunction GetLexerFactoryByIndex(int index)
{
    return (index == 0) ? LexerTemplate::LexerFactory : nullptr;
}

void SCI_METHOD LexerTemplate::Release()
{
    delete this;
}

int SCI_METHOD LexerTemplate::Version() const
{
    return Scintilla::lvRelease5;
}

struct Document
{
    int len;
    std::string text;
};

Document getDocumentText()
{
    Document d;
    d.len = 0;
    d.text = "\0";
    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1)
        return d;
    HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
    long nLen = ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0);
    char *buffer = new char[nLen + 1];
    ::SendMessage(curScintilla, SCI_GETTEXT, nLen + 1, (LPARAM)buffer);
    d.len = nLen;
    d.text = buffer;
    delete buffer;
    return d;
}

void SCI_METHOD LexerTemplate::Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, Scintilla::IDocument *pAccess)
{
    LexAccessor styler(pAccess);

    StyleContext sc(startPos, lengthDoc, RASCRIPT_STYLE_DEFAULT, styler);

    for (;; sc.Forward())
    {
        if (!sc.More())
        {
            break;
        }
    }
    Document d = getDocumentText();
    DBUG("Last Letter Typed: " << d.text[d.len - 1] << "\n");
    sc.Complete();
}

extern LexerFactoryFunction GetLexerFactoryByIndex(int index);

extern "C" __declspec(dllexport) inline int __stdcall GetLexerCount()
{
    return 1;
}

extern "C" __declspec(dllexport) inline void __stdcall GetLexerName(unsigned int index, char *name, int buflength)
{
    if (index == 0)
    {
        strncpy_s(name, buflength, "RAScript", _TRUNCATE);
        name[buflength - 1] = '\0';
    }
}

extern "C" __declspec(dllexport) inline void __stdcall GetLexerStatusText(unsigned int index, WCHAR *desc, int buflength)
{
    if (index == 0)
    {
        wcsncpy_s(desc, buflength, L"RAScript file", _TRUNCATE);
        desc[buflength - 1] = L'\0';
    }
}

extern "C" __declspec(dllexport) inline LexerFactoryFunction __stdcall GetLexerFactory(unsigned int index)
{
    return GetLexerFactoryByIndex(index);
}

extern "C" __declspec(dllexport) inline void *__stdcall CreateLexer(const char * /* name */)
{
    return LexerTemplate::LexerFactory();
}