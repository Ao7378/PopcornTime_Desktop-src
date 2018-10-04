#include "init_wrapper.h"
#include "cefclient/cefclient.h"

#ifdef OS_WIN
    #include <windows.h>
    #include <tchar.h>

namespace
{

typedef BOOL( WINAPI *LPFN_ISWOW64PROCESS )(HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;

}  // namespace

// Detect whether the operating system is a 64-bit.
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms684139%28v=vs.85%29.aspx
BOOL IsWow64()
{
    BOOL bIsWow64 = FALSE;

    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
       GetModuleHandle( TEXT( "kernel32" ) ),
       "IsWow64Process" );

    if ( NULL != fnIsWow64Process )
    {
        if ( !fnIsWow64Process( GetCurrentProcess(), &bIsWow64 ) )
        {
            // handle error
        }
    }
    return bIsWow64;
}
#endif


CefInitWrapper::CefInitWrapper( int argc, char *argv[] )
{
    initResultCode = CefInit( argc, argv );
    if ( initResultCode >= 0 ) return;
#ifdef OS_WIN
    CefLoadPlugins( IsWow64() ); // Load flash system plug-in on Windows.
#endif
}

CefInitWrapper::~CefInitWrapper() { CefQuit(); }

