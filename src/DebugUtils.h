#ifndef DEBUGUTILS_H
#define DEBUGUTILS_H

#include <iostream>
#include <sstream>

#ifdef DEBUG
#define DBUG(msg)            \
    std::wostringstream os_; \
    os_ << msg;              \
    OutputDebugStringW(os_.str().c_str());
#else
#define DBUG(msg)
#endif

#endif // DEBUGUTILS_H