#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <windows.h>

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

Document getDocumentText(HWND curScintilla)
{
    Document d;
    d.len = 0;
    d.text = "\0";
    long nLen = ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0);
    char *buffer = new char[nLen + 1];
    ::SendMessage(curScintilla, SCI_GETTEXT, nLen + 1, (LPARAM)buffer);
    d.len = nLen;
    d.text = buffer;
    delete[] buffer;
    return d;
}

void SCI_METHOD LexRAScript::Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, Scintilla::IDocument *pAccess)
{
    LexAccessor styler(pAccess);
    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1)
    {
        return;
    }
    HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
    Document d = getDocumentText(curScintilla);
    // I don't know, this might be really fucking bad.
    // I'm forcing scintilla to restyle the entire document for each change,
    // but I can't get the document to fully style after closing/opening because its only partially updating
    startPos = 0;
    lengthDoc = d.len;
    StyleContext sc(startPos, lengthDoc, initStyle, styler);

    COLORREF bgColor = ::SendMessage(nppData._nppHandle, NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR, 0, 0);

    bool darkModeEnabled = ::SendMessageW(nppData._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0);

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

    int *styles = ParseFile(configNode, darkModeEnabled, d.len, d.text);
    std::unordered_map<int, bool> stylesUpdated;
    int found = 0;
    for (;; sc.Forward())
    {
        if (sc.currentPos < lenDef)
        {
            int style = styles[sc.currentPos];
            if (found < 10)
            {
                if (style != 0)
                {
                    found++;
                }
            }
            if (!stylesUpdated.count(style))
            {
                ::SendMessage(curScintilla, SCI_STYLESETBACK, style, (LPARAM)bgColor);
                stylesUpdated[style] = true;
            }
            sc.ChangeState(style);
        }
        sc.SetState(0); // Needed for some reason, do not remove
        if (!sc.More())
        {
            break;
        }
    }
    sc.Complete();
    delete[] styles;
    styles = nullptr;
    styler.Flush();
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

extern "C" __declspec(dllexport) Scintilla::ILexer5 *SCI_METHOD CreateLexer(const char *name)
{
    if (strcmp(name, "RAScript") == 0)
    {
        return LexRAScript::LexerFactory();
    }
    return nullptr;
}