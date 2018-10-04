// dllmain.cpp : Defines the entry point for the DLL application.
#include "../Common/common.h"
#include "streamerok.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                      )
{
    switch ( ul_reason_for_call )
    {
    case DLL_PROCESS_ATTACH:
        USE_DEBUG( "DLL Process Attach, %s Load\n", lpReserved ? "Static" : "Dynamic" );
        break;
    case DLL_PROCESS_DETACH:
        torrentDeInit();
        USE_DEBUG( "DLL Process Detach, %s\n", lpReserved ? "Process terminated" : "FreeLibrary or load fail" );
        break;
    case DLL_THREAD_ATTACH:
//      USE_DEBUG( "DLL Thread Attach\n" );
        break;
    case DLL_THREAD_DETACH:
//      USE_DEBUG( "DLL Thread Detach\n" );
        break;
    }
    return TRUE;
}

