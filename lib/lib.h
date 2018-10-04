#ifndef __StreamerokLib__
    #define __StreamerokLib__
// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the LIB_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// LIB_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

    #include <string>

    #ifdef _WIN32
        #ifdef LIB_EXPORTS
            #define LIB_API __declspec(dllexport)
        #elif _MSC_VER
            #define LIB_API __declspec(dllimport)
        #else
            #define LIB_API
        #endif
    #else
        #define LIB_API
    #endif

struct dlInfo
{
    long long downloadRateBs;
    long long downloaded;
    long long total;
    long long piecesDone;
    long long piecesTotal;
    long long pieceSize;
    long long seeders;
    long long peers;
};

struct NetUsage
{
    long long uploadRateBs;
    long long downloadRateBs;
    long long uploaded;
    long long downloaded;
};

// To be called once before any other function. If URL is set, returns 1. Returns 0 if not.
// To disable updating simply provide NULL pointer, or pointer to zero char.
extern "C" LIB_API int setUpdateUrl( const char *url );

// Calling of any function implicitly calls torrentInit if uninitialized.
extern "C" LIB_API bool torrentInit( const char *downloadDir = 0, int cleanOnExit = 0 );
// Called on DLL Detach as well. Runs downloaded update right before shutdown.
extern "C" LIB_API void torrentDeInit( void );

extern "C" LIB_API int torrentSetProxy( const char *host = 0, int port = 0, const char *user = 0, const char *pass = 0 );

extern "C" LIB_API int torrentAddStart( const char *url, const char *fileName = 0 );
extern "C" LIB_API int torrentAddImport( const char *torrentBody, int bodyLength, const char *fileName = 0 );
extern "C" LIB_API const dlInfo* torrentGetInfo( int torrentId );
extern "C" LIB_API bool torrentGetFileUrl( int torrentId, char *buffer, int length );
extern "C" LIB_API const NetUsage* torrentGetNetUsage( void );
extern "C" LIB_API bool torrentStopDelete( int torrentId );

#endif
