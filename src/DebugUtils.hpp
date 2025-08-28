#ifndef DEBUGUTILS_H
#define DEBUGUTILS_H

#ifdef DEBUG
#ifndef DBUG

#include <sstream>

#define DBUG(s)                                \
    {                                          \
        std::wostringstream os_;               \
        os_ << s;                              \
        OutputDebugStringW(os_.str().c_str()); \
    }
#endif // DBUG
#else
#ifndef DBUG
#define DBUG(s) \
    {           \
    }
#endif // DBUG
#endif // DEBUG

#endif // DEBUGUTILS_H