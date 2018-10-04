// lib.cpp : Defines the exported functions for the DLL application.
//


#include "lib.h"
#include "limits.h"
//#include "SocketHandler.h"
#include "../Common/common.h"
#include "streamerok.h"

dlInfo gDlInfo;
NetUsage gNnetUsage;


bool torrentInit( const char *downloadDir, int cleanOnExit )
{
#if !defined( WIN32 ) && defined( _DEBUG )
    openlog("streamerok", 0, LOG_USER);
#endif
    if ( downloadDir && *downloadDir ) setTempDir( downloadDir, cleanOnExit != 0 );
    checkTempDir();
    if ( !Streamerok::sPtr )
    {
        Streamerok::sPtr = new Streamerok;
        USE_DEBUG( "Lib init\n" );
    }

    return tempDirValid();
}


void torrentDeInit()
{
    try
    {
        if ( Streamerok::sPtr )
        {
            delete Streamerok::sPtr;
            Streamerok::sPtr = 0;
            cleanTempDir();
        }
    }
    catch (...)
    {
        USE_DEBUG( "Streamerok stop error catached" );
    }
    USE_DEBUG( "Lib deinit\n" );
}



int setUpdateUrl( const char *url )
{
    return true;
}


int torrentSetProxy( const char *host, int port, const char *user, const char *pass )
{
    torrentInit();
    return Streamerok::sPtr->setProxy( host, port, user, pass );
}

int torrentAddStart( const char *url, const char *fileName )
{
    if ( !url ) return -1; //WEBREQUEST_ERROR_BAD_URL;
    torrentInit();
    return Streamerok::sPtr->downloadTorrent( url, fileName );
}

int torrentAddImport( const char *torrentBody, int bodyLength, const char *fileName )
{
    if ( !torrentBody ) return -8;
    torrentInit();
    return Streamerok::sPtr->importTorrent( torrentBody, bodyLength, fileName );

}

const dlInfo* torrentGetInfo( int torrentId )
{
    torrentInit();
    gDlInfo = Streamerok::sPtr->getTorrentInfo( torrentId );
#ifdef WIN32
    USE_DEBUG( "Bytes: %I64d of %I64d, speed %I64d B/s\n", gDlInfo.downloaded, gDlInfo.total, gDlInfo.downloadRateBs );
#else
    USE_DEBUG( "Bytes: %lld of %lld, speed %lld B/s\n", gDlInfo.downloaded, gDlInfo.total, gDlInfo.downloadRateBs );
#endif

    return &gDlInfo;
}

bool torrentGetFileUrl( int torrentId, char *buffer, int length )
{
    torrentInit();
    return Streamerok::sPtr->getFileUrl( torrentId, buffer, length );
}


const NetUsage* torrentGetNetUsage()
{
    torrentInit();
    gNnetUsage = Streamerok::sPtr->getNetworkUsage();
    return &gNnetUsage;
}

bool torrentStopDelete( int torrentId )
{
    torrentInit();
    return Streamerok::sPtr->deleteTorrent( torrentId, CleanOnExit );
}