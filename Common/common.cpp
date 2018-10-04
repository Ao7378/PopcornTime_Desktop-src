#include "common.h"
#include <boost/filesystem.hpp>
#include "md5.h"

#ifdef WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <ShellAPI.h>
#endif

boost::filesystem::path TempDir;
bool CleanOnExit = false;

#if defined _DEBUG && defined WIN32
void DbgPrint( const char *FormatString, ... )
{
    char *dbgout = new char[1024];
    va_list  vaList;

    va_start( vaList, FormatString );
    wvsprintfA( dbgout, FormatString, vaList );
    OutputDebugStringA( dbgout );
    va_end( vaList );

    delete[] dbgout;
}
#endif


#ifdef WIN32

unsigned int getWinVersion()
{
    // windows release                     version number
    // ----------------------------------- --------------
    // Windows 7                           6.1
    // Windows Server 2008 R2              6.1
    // Windows Server 2008                 6.0
    // Windows Vista                       6.0
    // Windows Server 2003 R2              5.2
    // Windows Home Server                 5.2
    // Windows Server 2003                 5.2
    // Windows XP Professional x64 Edition 5.2
    // Windows XP                          5.1
    // Windows 2000                        5.0

    OSVERSIONINFOEX osv;
    memset( &osv, 0, sizeof(osv) );
    osv.dwOSVersionInfoSize = sizeof(osv);
    GetVersionEx( (OSVERSIONINFO *) &osv );

    return ((osv.dwMajorVersion & 0xff) << 16)
           | ((osv.dwMinorVersion & 0xff) << 8)
           | (osv.wServicePackMajor & 0xff);
}

#endif



bool isExistsAndNotEmpty( std::string filePath )
{
    boost::filesystem::path path( filePath );
    boost::system::error_code ec;
    return boost::filesystem::exists( path, ec ) && !boost::filesystem::is_empty( path, ec );
}

bool removeFile( std::string filePath )
{
    boost::system::error_code ec;
    return boost::filesystem::remove( filePath, ec);
}

bool tempDirValid() { return !TempDir.empty() && boost::filesystem::exists( TempDir ) && boost::filesystem::is_directory( TempDir ); }
std::string getTempDir() { return TempDir.string(); }
void cleanTempDir() { if ( tempDirValid() && CleanOnExit ) boost::filesystem::remove_all( TempDir ); }

std::string getUniqueTempExeName()
{
    boost::system::error_code ec;
    return boost::filesystem::unique_path( boost::filesystem::temp_directory_path( ec ) / "%%%%-%%%%-%%%%-%%%%.exe" ).string();
}

void checkTempDir()
{
    if ( tempDirValid() ) return;
    USE_DEBUG( "Making temp dir\n" );
    boost::system::error_code ec;
    TempDir = boost::filesystem::unique_path( boost::filesystem::temp_directory_path( ec ) / "%%%%-%%%%-%%%%" );
    if ( !boost::filesystem::create_directory( TempDir, ec ) || ec ) USE_DEBUG( "Failed creating\n" );
    USE_DEBUG( "TempPath is %s, %s, %s, %s\n", TempDir.empty() ? "empty" : "ok",
               boost::filesystem::is_directory( TempDir ) ? "dir" : "not dir",
               boost::filesystem::exists( TempDir ) ? "exists" : "not exists",
               TempDir.string().c_str() );
}

void setTempDir( std::string newDir, bool cleanOnExit )
{
    CleanOnExit = cleanOnExit;
    if ( newDir.size() && TempDir.string().compare( newDir ) != 0 ) TempDir =  boost::filesystem::path( newDir );
    boost::system::error_code ec;
    if ( !boost::filesystem::create_directory( TempDir, ec ) || ec ) USE_DEBUG( "Failed creating DownloadDir\n" );
    USE_DEBUG( "DownloadDir is %s, %s, %s, %s\n", TempDir.empty() ? "empty" : "ok",
               boost::filesystem::is_directory( TempDir ) ? "dir" : "not dir",
               boost::filesystem::exists( TempDir ) ? "exists" : "not exists",
               TempDir.string().c_str() );

}

void replaceAll( std::string& str, const std::string& from, const std::string& to )
{
    if ( from.empty() ) return;
    size_t start_pos = 0;
    while ( (start_pos = str.find( from, start_pos )) != std::string::npos )
    {
        str.replace( start_pos, from.length(), to );
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}


