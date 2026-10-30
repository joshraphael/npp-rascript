#include "RAScript.hpp"
#include "DebugUtils.hpp"
#include "PluginInterface.h"
#include <cstdio>
#include <string>

// Notepad++ refuses a plugin without at least one menu item
const int nbFunc = 1;
FuncItem funcItem[nbFunc];
NppData nppData;
int styleGeneration = 0;
HINSTANCE dllModule = NULL;
const TCHAR NPP_PLUGIN_NAME[] = TEXT("RAScript");

void commandMenuInit()
{
    TCHAR configPath[MAX_PATH];
    ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);
    // a wide path, so a config folder with non-ASCII characters (a user name like José) works
    std::wstring configFile = std::wstring(configPath) + L"\\RAScript.xml";
    // RAScript.xml is built into the DLL, so the DLL is all an install needs
    HRSRC resource = FindResourceW(dllModule, L"RASCRIPT_XML", RT_RCDATA);
    HGLOBAL loaded = resource != NULL ? LoadResource(dllModule, resource) : NULL;
    const void *contents = loaded != NULL ? LockResource(loaded) : NULL;
    if (contents == NULL)
    {
        DBUG(L"RAScript.xml resource not found");
    }
    else
    {
        // binary mode, so the file is written byte for byte as released
        FILE *outFile = _wfopen(configFile.c_str(), L"wb");
        if (outFile != NULL)
        {
            fwrite(contents, 1, SizeofResource(dllModule, resource), outFile);
            fclose(outFile);
        }
        else
        {
            DBUG(L"Error opening file: " << configFile);
        }
    }

    setCommand(0, TEXT("Hello Notepad++"), Test, NULL, false);
}

bool setCommand(size_t index, const TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit)
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

void Test()
{
    ::MessageBox(NULL, TEXT("Test"), TEXT("Test"), MB_OK);
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID /*lpReserved*/)
{
    if (reasonForCall == DLL_PROCESS_ATTACH)
    {
        dllModule = static_cast<HINSTANCE>(hModule);
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
    nppData = notpadPlusData;
    commandMenuInit();
}

extern "C" __declspec(dllexport) const TCHAR *getName()
{
    return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem *getFuncsArray(int *nbF)
{
    *nbF = nbFunc;
    return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
    switch (notifyCode->nmhdr.code)
    {
        case NPPN_DARKMODECHANGED:   // dark mode activated
        case NPPN_BUFFERACTIVATED:   // change tabs
        case NPPN_WORDSTYLESUPDATED: // style config change
        {
            styleGeneration++; // Notepad++ may have reset the styles' backgrounds
            int which = -1;
            ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
            if (which != -1)
            {
                HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
                ::SendMessage(curScintilla, SCI_COLOURISE, 0, -1); // restyle entire document
            }
            break;
        }
        default:
            return;
    }
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT /*Message*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif // UNICODE