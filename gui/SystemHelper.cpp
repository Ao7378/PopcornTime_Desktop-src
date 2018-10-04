#include "SystemHelper.h"

#if defined WIN32 || defined _WIN32

    #include "Windows.h"

bool SystemHelper::disableSleep()
{
    SetThreadExecutionState( ES_CONTINUOUS | ES_DISPLAY_REQUIRED );
    return true;
}

bool SystemHelper::restoreSleep()
{
    SetThreadExecutionState( ES_CONTINUOUS );
    return true;
}

#elif defined  __APPLE__

    #include <IOKit/pwr_mgt/IOPMLib.h>

IOPMAssertionID assertionID;

bool SystemHelper::disableSleep()
{
    static CFStringRef reasonForActivity = CFSTR( "PopcornTime active" );
    if ( assertionID ) return true;

    IOReturn success = IOPMAssertionCreateWithName( kIOPMAssertionTypeNoDisplaySleep,
                                                   kIOPMAssertionLevelOn,
                                                   reasonForActivity,
                                                   &assertionID );
    return success == kIOReturnSuccess;
}

bool SystemHelper::restoreSleep()
{
    if ( !assertionID ) return false;

    IOReturn success = IOPMAssertionRelease( assertionID );
    assertionID = 0;

    return success == kIOReturnSuccess;
}

extern "C" {

#if defined(__MACH__) && !defined(CLOCK_REALTIME)
#include <sys/time.h>
#define CLOCK_REALTIME 0
// clock_gettime is not implemented on older versions of OS X (< 10.12).
// If implemented, CLOCK_REALTIME will have already been defined.
int clock_gettime(int /*clk_id*/, struct timespec* t) {
    struct timeval now;
    int rv = gettimeofday(&now, NULL);
    if (rv) return rv;
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
#endif

}

#endif
