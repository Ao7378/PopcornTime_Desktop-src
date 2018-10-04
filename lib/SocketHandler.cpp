#include "SocketHandler.h"
#include "mongoose.h"
#include <atlcore.h>
#include <algorithm>
#include "../Common/common.h"
#include "boost/lexical_cast.hpp"

const int DEFAULT_PORT = 19765;
const int MIN_PORT = 20000;
const int MAX_PORT = 40000;


DWORD SocketHandler::threadId = 0;
std::string SocketHandler::documentRoot;
volatile bool SocketHandler::canLoop = true;
volatile int SocketHandler::listenPort = -1;

void SocketHandler::startThread( callbackFn func )
{
    CreateThread(
       NULL, 0,
       (LPTHREAD_START_ROUTINE) &SocketHandler::run,
       func,
       0,
       &threadId
       );
}

std::string SocketHandler::getUrlBase()
{
    return std::string( "http://localhost:" ) + boost::lexical_cast<std::string>( listenPort ) + "/";
}

void SocketHandler::run( LPVOID lpStatus )
{
    const callbackFn fPtr = reinterpret_cast<callbackFn>( lpStatus );
    struct mg_server *server = mg_create_server( NULL );


    const int buflen = 32;
    char buf[buflen];
    const char *result;

    int currentPort = DEFAULT_PORT;
    _snprintf( buf, buflen, "127.0.0.1:%i", currentPort );

    while ( result = mg_set_option( server, "listening_port", buf ) )
    {
        USE_DEBUG( "Error \"%s\" opening http listen socket %s\n", result, buf );
        Sleep( 100 );
        currentPort = rand() % ( MAX_PORT - MIN_PORT ) + MIN_PORT;
        _snprintf( buf, buflen, "127.0.0.1:%i", currentPort );
    }
    listenPort = currentPort;

    mg_set_option( server, "document_root", documentRoot.c_str() );
    mg_set_option( server, "extra_mime_types", ".mp4=video/mp4" );

    USE_DEBUG( "Started on %s with root at %s\n",
               mg_get_option( server, "listening_port" ),
               mg_get_option( server, "document_root" ) );
    canLoop = true;

    time_t lastTime = time( NULL );
    while ( canLoop )
    {
        mg_poll_server( server, 100 );

        const time_t current_time = time( NULL );
        if ( fPtr && lastTime < current_time  )
        {
            lastTime = current_time;
            fPtr();
        }
//      Sleep( 20 );
//      USE_DEBUG( "." );
    }
    USE_DEBUG( "Webserver shuts down...\n" );
    mg_destroy_server( &server );
    USE_DEBUG( "Webserver is shut down.\n" );
}

