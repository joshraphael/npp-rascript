#ifndef DEBUGUTILS_H
#define DEBUGUTILS_H

// DBUG writes to the debug output (OutputDebugString) in Debug builds (_DEBUG:
// the Debug configurations in RAScript.vcxproj, or `make install-debug`) and
// compiles to nothing otherwise. Under Wine, `make open-debug` shows it in the
// terminal, or `make debug` in DebugView. A Release build (NDEBUG) refuses to
// compile with it, so it can't ship by accident
#ifdef _DEBUG
#ifdef NDEBUG
#error "_DEBUG must not be defined in a Release build"
#endif
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
#endif // _DEBUG

#include <cstring>
#include <ostream>
#include <utility>
#include <vector>
#include <windows.h>

// Records how long the steps of some code take: mark(name) adds the time since
// the previous mark under that name, and printing it in a DBUG line writes each
// name=milliseconds and the total. Names are string literals. Does nothing
// unless _DEBUG and DBUG are defined, and the DBUG line itself compiles away
//
//     TimingLog timing;
//     ...
//     timing.mark("parse");
//     DBUG("Lex lines=" << lines << " | " << timing << "\n");
class TimingLog
{
public:
    TimingLog()
    {
#if defined(_DEBUG) && defined(DBUG)
        start = last = now();
#endif
    }

    void mark([[maybe_unused]] const char *name)
    {
#if defined(_DEBUG) && defined(DBUG)
        double t = now();
        for (size_t i = 0; i < laps.size(); i++)
        {
            if (std::strcmp(laps[i].first, name) == 0)
            {
                laps[i].second += t - last;
                last = t;
                return;
            }
        }
        laps.push_back(std::make_pair(name, t - last));
        last = t;
#endif
    }

    friend std::wostream &operator<<(std::wostream &os, const TimingLog &timing)
    {
        for (size_t i = 0; i < timing.laps.size(); i++)
        {
            os << timing.laps[i].first << "=" << timing.laps[i].second << " ";
        }
        return os << "total=" << now() - timing.start << "ms";
    }

private:
    static double now()
    {
        LARGE_INTEGER frequency;
        LARGE_INTEGER counter;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&counter);
        return counter.QuadPart * 1000.0 / frequency.QuadPart;
    }

    double start = 0;
    double last = 0;
    std::vector<std::pair<const char *, double>> laps;
};

#endif // DEBUGUTILS_H
