#pragma once
#include <string>

#ifdef _DEBUG
    #define TORRENT_DEBUG
    #ifdef WIN32
        #define USE_DEBUG(...)              DbgPrint( __VA_ARGS__)
extern "C" void DbgPrint( const char *FormatString, ... );
    #else
        #include <syslog.h>
        #define USE_DEBUG(...)              syslog( LOG_NOTICE, __VA_ARGS__ )
    #endif
#endif

#ifndef USE_DEBUG
    #define USE_DEBUG(...)
#endif

extern bool CleanOnExit;

bool isExistsAndNotEmpty( std::string filePath );
bool removeFile( std::string filePath );
bool tempDirValid();
std::string getTempDir();
std::string getUniqueTempExeName();
void cleanTempDir();
void checkTempDir();
void setTempDir( std::string newDir, bool cleanOnExit );

// this is the format of windows_version
// xx xx xx
// |  |  |
// |  |  + service pack version
// |  + minor version
// + major version
unsigned int getWinVersion();

void replaceAll( std::string& str, const std::string& from, const std::string& to );
