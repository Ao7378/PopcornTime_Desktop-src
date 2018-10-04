#pragma once

#include <vector>
#include <string>
#include <windows.h>

typedef std::vector<HANDLE> HANDLES;
typedef void (*const callbackFn)( void );

struct mg_connection;

class SocketHandler
{
public:
    static void startThread( callbackFn func = 0 );
    static bool isRunning() { return threadId ? true : false; }
    static void stop() { canLoop = false; }
    static std::string getUrlBase();
    static void setDocumentRoot( std::string dir ) { documentRoot = dir; }

private:
    static void run( LPVOID lpStatus );

    static DWORD threadId;
    static std::string documentRoot;
    static volatile int listenPort;
    static volatile bool canLoop;
};

