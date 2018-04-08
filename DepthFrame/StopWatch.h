//
// the clock code is platform dependent
//
#pragma once

#include <windows.h>

class StopWatch
{
public:
    StopWatch()
    {
        ::QueryPerformanceFrequency(&timerFrequency);
    }

    void reset()
    {
        ::QueryPerformanceCounter(&t0);
    }

    double get( bool doReset = false )
    {
        LARGE_INTEGER t1;
        ::QueryPerformanceCounter(&t1);

        const __int64 oldTicks = ((__int64)t0.HighPart << 32) + (__int64)t0.LowPart;
        const __int64 newTicks = ((__int64)t1.HighPart << 32) + (__int64)t1.LowPart;
        const long double timeDifference = (long double) (newTicks - oldTicks);
        const long double ticksPerSecond = (long double) (((__int64)timerFrequency.HighPart << 32)
            + (__int64)timerFrequency.LowPart);

        if ( doReset )
            reset();

        return (double)(timeDifference / ticksPerSecond);
    }

private:
    LARGE_INTEGER t0;
    LARGE_INTEGER timerFrequency;
};

