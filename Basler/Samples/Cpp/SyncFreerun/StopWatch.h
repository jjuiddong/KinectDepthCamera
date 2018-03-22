
// the clock code is platform dependent
#if defined(CIH_WIN_BUILD)

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

#elif defined(CIH_LINUX_BUILD)

#include <time.h>

class StopWatch
{
public:
    void reset()
    {
        clock_gettime( CLOCK_REALTIME, &t0 );
    }

    double get( bool doReset = false )
    {
        clock_gettime( CLOCK_REALTIME, &t1 );
        if ( doReset )
            reset();
        return difftime( t1, t0 );
    }

private:
    timespec t0, t1;

    static double difftime( const timespec& t1, const timespec& t0 )
    {
        return ( double ) ( t1.tv_sec - t0.tv_sec ) + 1.0e-9 * ( t1.tv_nsec - t0.tv_nsec );
    }


};
#else
#   error unknown platform
#endif

