#ifndef RASCRIPT_HPP
#define RASCRIPT_HPP

#include "PluginInterface.h"
#include <windows.h>

void commandMenuInit();
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk = NULL, bool check0nInit = false);
void Test();

#endif // RASCRIPT_HPP