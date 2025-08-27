#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <string_view>
#include <assert.h>

#include "ILexer.h"
#include "LexAccessor.h"
#include "tinyxml2.h"
#include "PluginInterface.h"

#include "DebugUtils.hpp"
#include "LexRAScript.hpp"
#include "Parser.hpp"
#include "StyleContext.h"

extern NppData nppData;

Scintilla::ILexer5 *LexRAScript::LexerFactory()
{
    return new LexRAScript();
}

void SCI_METHOD LexRAScript::Release()
{
    delete this;
}

int SCI_METHOD LexRAScript::Version() const
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
    {
        return d;
    }
    HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
    long nLen = ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0);
    char *buffer = new char[nLen + 1];
    ::SendMessage(curScintilla, SCI_GETTEXT, nLen + 1, (LPARAM)buffer);
    d.len = nLen;
    d.text = buffer;
    delete[] buffer;
    return d;
}

void SCI_METHOD LexRAScript::Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int /* initStyle */, Scintilla::IDocument *pAccess)
{
    LexAccessor styler(pAccess);
    StyleContext sc(startPos, lengthDoc, -1, styler);
    Document d = getDocumentText();

    COLORREF bgColor = ::SendMessage(nppData._nppHandle, NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR, 0, 0);
    // std::stringstream backgroundColor;
    // backgroundColor << std::hex << bgColor;
    // std::string str_value = backgroundColor.str();
    // DBUG(L"backgroundColor");
    // DBUG(str_value.c_str());

    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1)
    {
        return;
    }
    HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

    TCHAR configPath[MAX_PATH];
    ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);
    std::wstring configFile(configPath);
    std::wstring config = L"\\RAScript.xml";
    configFile = configFile + config;
    const std::string finalConfigFilePath(configFile.begin(), configFile.end());
    tinyxml2::XMLDocument configDoc;
    tinyxml2::XMLError eResult = configDoc.LoadFile(finalConfigFilePath.c_str());
    if (eResult != tinyxml2::XML_SUCCESS)
    {
        return;
    }
    tinyxml2::XMLElement *configNode = configDoc.RootElement()->FirstChildElement("RAScript");
    Sci_PositionU lenDef = d.len;

    int *styles = ParseFile(configNode, d.len, d.text);

    for (;; sc.Forward())
    {
        if (sc.currentPos < lenDef)
        {
            int style = styles[sc.currentPos];
            ::SendMessage(curScintilla, SCI_STYLESETBACK, style, (LPARAM)bgColor);
            sc.ChangeState(styles[sc.currentPos]);
        }
        sc.SetState(0);
        if (!sc.More())
        {
            break;
        }
    }
    sc.Complete();
    delete[] styles;
    styles = nullptr;
}

extern "C" __declspec(dllexport) int SCI_METHOD GetLexerCount()
{
    return 1;
}

extern "C" __declspec(dllexport) void SCI_METHOD GetLexerName(unsigned int index, char *name, int buflength)
{
    if (index == 0)
    {
        strncpy_s(name, buflength, "RAScript", _TRUNCATE);
        name[buflength - 1] = '\0';
    }
}

extern "C" __declspec(dllexport) void SCI_METHOD GetLexerStatusText(unsigned int index, WCHAR *desc, int buflength)
{
    if (index == 0)
    {
        wcsncpy_s(desc, buflength, L"RAScript file", _TRUNCATE);
        desc[buflength - 1] = L'\0';
    }
}

extern "C" __declspec(dllexport) LexerFactoryFunction SCI_METHOD GetLexerFactory(unsigned int index)
{
    return (index == 0) ? LexRAScript::LexerFactory : nullptr;
}

extern "C" __declspec(dllexport) Scintilla::ILexer5 *SCI_METHOD CreateLexer(const char * /* name */)
{
    return LexRAScript::LexerFactory();
}