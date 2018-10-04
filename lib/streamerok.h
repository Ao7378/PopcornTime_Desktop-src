#pragma once

#define TORRENT_NO_ASSERTS 1
//#define TORRENT_NO_DEPRECATE 1
#define TORRENT_DISK_STATS 1
#define NO_MONGOOSE_WEB_SERVER 1

#include "lib.h"
#include <boost/unordered_map.hpp>
#include "libtorrent/session.hpp"
#ifdef NO_MONGOOSE_WEB_SERVER
    #include <boost/thread.hpp>
#endif

struct tAux
{
    tAux( libtorrent::torrent_handle hndl, int index, long long size );
    long long prioritize(long long needSeekToThisPiece);
	long long updateDeadlines(int downloadedPieceIndex, long long needSeekToThisPiece);
    bool canGetUrl();
    bool isFinished();
    libtorrent::torrent_handle handle;
    int fileIndex;
    long long fileSize;
    int pieceLength;
    int fileFirstPiece;
    int fileLastPiece;
    int startupPiecesCountAtStart;
    int startupPiecesCountAtEnd;
    std::string fileUri;
    std::string filePath;

	int fileFirstPieceForDeadlines;
	int piecesCount;
	int pieceForUpdateDeadline;
	//Number of the first piece of a file of all the pieces of the torrent
	int fileVeryFirstPiece;
	bool prebufferingEnded;
	std::string piecesDeadlines;
};

typedef boost::unordered_map<int, tAux> torrentMap;

class Streamerok
{
public:
    Streamerok();
    ~Streamerok();

    // returns 0 or 1 on disable/enable, negative for error
    int setProxy( const char *host = 0, int port = 0, const char *user = 0, const char *pass = 0 );
    // returns positive integer for torrent, negative for error
    int downloadTorrent( const char *url, const char *fileName = 0 );
    int importTorrent( const char *torrentBody, int bodyLength, const char *fileName = 0 );
    dlInfo getTorrentInfo( int torrentId );
    bool getFileUrl( int torrentId, char *buffer, int length );
    tAux* getAuxByUriPath( const char *uri );
    tAux* getAuxByIndex( int torrentId );
    int getIndexByUriPath( const char *uri );
    NetUsage getNetworkUsage();
    virtual bool deleteTorrent( int torrentId, bool deleteCompleted = true );
    tAux* getAux( int torrentId );
    int getNextId()
    {
        if ( nextId + 1 == INT_MAX ) nextId = 0;
        return ++nextId;
    }

    static Streamerok *sPtr;
    static int countPieces( libtorrent::bitfield& pieces, int first, int last );
	int seekInFile(long long offset, int torrentID);
	bool isRangeDownloaded( long long begin, long long end, int torrentId);

private:
    std::string getFileUrl( int torrentId );
//  std::string getFileRelPath( int torrentId );
    static void peridoicCallback();
    bool handle_alert( libtorrent::alert *a );
    int getFileIndex( libtorrent::torrent_handle& h, std::string fileName = std::string() );
    int addTorrent( libtorrent::add_torrent_params& p, std::string fileName );

    int nextId;
    libtorrent::session ses;
	long long needSeekToThisPiece;
#ifdef NO_MONGOOSE_WEB_SERVER
    friend void threadFunction();
    boost::thread thread;
#endif
    torrentMap tMap;

//  std::string proxyHostPort;
};

extern "C" int getTorrentIndex( const char *uri );
extern "C" unsigned long long getFileSize( int torrentId );
extern "C" unsigned long long getFileDownloaded( int torrentId );

