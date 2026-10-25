#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
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

LexRAScript::~LexRAScript()
{
    FreeGrammar(grammar);
}

void SCI_METHOD LexRAScript::Release()
{
    delete this;
}

int SCI_METHOD LexRAScript::Version() const
{
    return Scintilla::lvRelease5;
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

    COLORREF bgColor = ::SendMessage(nppData._nppHandle, NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR, 0, 0);

    bool darkModeEnabled = ::SendMessageW(nppData._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0);

    if (grammar == nullptr || darkModeEnabled != grammarDarkMode)
    {
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
        // line states are indexes into the old grammar's states, so restyle everything
        FreeGrammar(grammar);
        grammar = LoadGrammar(configDoc.RootElement()->FirstChildElement("RAScript"), darkModeEnabled);
        grammarDarkMode = darkModeEnabled;
        startPos = 0;
        lengthDoc = pAccess->Length();
    }

    // Style whole lines, starting from the state the line before ended in
    const Sci_Position length = pAccess->Length();
    const Sci_Position firstLine = pAccess->LineFromPosition(startPos);
    const Sci_Position start = pAccess->LineStart(firstLine);
    const Sci_Position rangeEnd = static_cast<Sci_Position>(startPos) + lengthDoc;
    const Sci_Position lastLine = pAccess->LineFromPosition(rangeEnd > start ? rangeEnd - 1 : start);
    Sci_Position end = pAccess->LineStart(lastLine + 1);
    if (end > length)
    {
        end = length;
    }
    if (end <= start)
    {
        return;
    }
    std::string text(end - start, '\0');
    pAccess->GetCharRange(&text[0], start, end - start);
    std::vector<int> styles;
    std::vector<int> lineStates;
    ParseLines(grammar, text, firstLine > 0 ? styler.GetLineState(firstLine - 1) : 0, styles, lineStates);
    for (size_t i = 0; i < lineStates.size(); i++)
    {
        styler.SetLineState(firstLine + static_cast<Sci_Position>(i), lineStates[i]);
    }

    StyleContext sc(start, end - start, initStyle, styler);
    std::unordered_map<int, bool> stylesUpdated;
    // styles[p - start] is the style of byte p. SetState colors everything since
    // the last SetState with the previous style, then starts the new one here;
    // Complete colors the last stretch, up to the end of the range
    for (; sc.More(); sc.Forward())
    {
        int style = styles[sc.currentPos - start];
        if (!stylesUpdated.count(style))
        {
            // setting a style redraws the whole view, so only when the background changed
            if (static_cast<COLORREF>(::SendMessage(curScintilla, SCI_STYLEGETBACK, style, 0)) != bgColor)
            {
                ::SendMessage(curScintilla, SCI_STYLESETBACK, style, (LPARAM)bgColor);
            }
            stylesUpdated[style] = true;
        }
        sc.SetState(style);
    }
    sc.Complete();
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