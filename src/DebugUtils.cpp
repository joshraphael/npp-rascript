#include <windows.h>
#include <sstream>

#include "DebugUtils.h"

#ifdef DEBUG
void DBUG(std::string outputString)
{
    std::wostringstream output;
    output << outputString.c_str();
    OutputDebugStringW(output.str().c_str());
}
#else
void DBUG(std::string) {}
#endif