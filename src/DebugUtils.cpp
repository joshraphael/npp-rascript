#include <windows.h>
#include <sstream>

#include "DebugUtils.h"

void DBUG(std::string outputString)
{
#ifdef DEBUG
    std::wostringstream output;
    output << outputString.c_str();
    OutputDebugStringW(output.str().c_str());
#endif
}