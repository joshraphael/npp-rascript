#include <fstream>
#include <iostream>
#include <string>

#include "menuCmdID.h"
#include "PluginInterface.h"

#include "Config.hpp"
#include "DebugUtils.hpp"
#include "LexRAScript.hpp"
#include "RAScript.hpp"

const int nbFunc = 2;
FuncItem funcItem[nbFunc];
NppData nppData;
TCHAR configPath[MAX_PATH];
const TCHAR NPP_PLUGIN_NAME[] = TEXT("RAScript");

void pluginInit(HANDLE /*hModule*/)
{
}

void pluginCleanUp()
{
}

void commandMenuInit()
{
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);
	std::wstring configFile(configPath);
	std::wstring config = L"\\RAScript.xml";
	configFile = configFile + config;
	const std::string finalConfigFilePath(configFile.begin(), configFile.end());
	std::ofstream outFile(finalConfigFilePath);

	if (outFile.is_open())
	{
		outFile << config_contents;
		outFile.close();
	}
	else
	{
		DBUG("Error opening file: " << finalConfigFilePath.c_str());
	}

	setCommand(0, TEXT("Hello Notepad++"), Test, NULL, false);
}

void commandMenuCleanUp()
{
}

bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit)
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
	try
	{

		switch (reasonForCall)
		{
		case DLL_PROCESS_ATTACH:
			pluginInit(hModule);
			break;

		case DLL_PROCESS_DETACH:
			pluginCleanUp();
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;
		}
	}
	catch (...)
	{
		return FALSE;
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
	case NPPN_SHUTDOWN:
	{
		commandMenuCleanUp();
		break;
	}
	case NPPN_DARKMODECHANGED:
	{
		DBUG("DARK MODE CHANGED");
		int which = -1;
		::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
		if (which != -1)
		{
			HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
			::SendMessage(curScintilla, SCI_COLOURISE, 0, -1); // restyle entire document
		}
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