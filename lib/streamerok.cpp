#include "../common/common.h"
#include "streamerok.h"

#include "urlencode.h"
#ifndef NO_MONGOOSE_WEB_SERVER
    #include "SocketHandler.h"
#else
void threadFunction();
#endif
#include "libtorrent/alert_types.hpp"
#include "libtorrent/magnet_uri.hpp"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>

#ifdef _DEBUG
    #include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
    #include <boost/algorithm/hex.hpp>
#endif

#define FILEPATH_IN_URL	1
#define USE_DEADLINES 1

#ifdef WIN32
const char NATIVE_PATH_SEPARATOR[] = "\\";
#else
const char NATIVE_PATH_SEPARATOR[] = "/";
#endif

const int PRIORITY_PIECES_COUNT_MIN = 5;
const int MIN_STARTUP_PIECES_START = 5;
const int MIN_STARTUP_PIECES_END = 4;
const int METADATA_TIMEOUT_S = 120;


using namespace libtorrent;

struct dlInfoExt : public dlInfo
{
    dlInfoExt()
    {
        downloadRateBs = 0;
        downloaded = 0;
        total = 0;
        piecesDone = 0;
        piecesTotal = 0;
        pieceSize = 0;
        seeders = 0;
        peers = 0;
    }
};

struct NetUsageExt : public NetUsage
{
    NetUsageExt( long long ulRate = 0, long long dlRate = 0, long long ulTotal = 0, long long dlTotal = 0 )
    {
        uploadRateBs = ulRate;
        downloadRateBs = dlRate;
        uploaded = ulTotal;
        downloaded = dlTotal;
    }
};


void peridoicCallback();

Streamerok *Streamerok::sPtr = 0;



tAux::tAux( libtorrent::torrent_handle hndl, int index, long long size )
   : handle( hndl ), fileIndex( index ), fileSize( size ), prebufferingEnded(false)
{
	//torrent_info ti = handle.get_torrent_info(); //get_torrent_info() is deprecated
	boost::intrusive_ptr<const libtorrent::torrent_info> intrPtrTI = handle.torrent_file();
    fileFirstPiece = intrPtrTI->map_file( fileIndex, 0, 1 ).piece;
	fileVeryFirstPiece = fileFirstPiece;
    fileLastPiece = intrPtrTI->map_file( fileIndex, fileSize - 1, 1 ).piece;
    pieceLength = intrPtrTI->piece_length();
	fileFirstPieceForDeadlines = fileFirstPiece;
	//
    /*const int*/ piecesCount = fileLastPiece - fileFirstPiece + 1;
	for(int i = 0; i < fileLastPiece + 1 ; i++)
	{
		//USE_DEBUG( "piecesDeadlines push_back 'f': %i\n", i );
		piecesDeadlines.push_back('f');
	}
	USE_DEBUG( "fileFirstPiece: %i\n", fileFirstPiece );
	USE_DEBUG( "fileLastPiece: %i\n", fileLastPiece );
	USE_DEBUG( "pieceLength: %i\n", pieceLength );
	USE_DEBUG( "piecesCount: %i\n", piecesCount );
#ifdef USE_DEADLINES
	//handle.set_sequential_download(true);
	for ( int i = fileFirstPiece; i < piecesCount; ++i )
	{
		//USE_DEBUG( "Prioritize pieces to 0 for deadlines: %i\n", i );
		handle.piece_priority(i, 1);
	}
	pieceForUpdateDeadline = 1;
	////torrent::request_time_critical_piece() will working for deadlines, if you're lucky...
#endif
	
	
    startupPiecesCountAtStart = static_cast<int>( ceil( float( piecesCount ) / 200 ) + 0.5 );
    if ( startupPiecesCountAtStart < MIN_STARTUP_PIECES_START ) startupPiecesCountAtStart = MIN_STARTUP_PIECES_START;
    for ( int i = fileFirstPiece; i < fileFirstPiece + startupPiecesCountAtStart && i < fileLastPiece; ++i )
	{
#ifdef USE_DEADLINES
		//handle.piece_priority(i, 1);
		piecesDeadlines.at(i) = 't';
		handle.set_piece_deadline(i, pieceForUpdateDeadline*10,  torrent_handle::alert_when_available);
		pieceForUpdateDeadline++;
		USE_DEBUG( "set_piece_deadline first pieces: %i\n", i );
#endif
#ifdef USE_PRIORITIES
		handle.piece_priority( i, 7 );
		USE_DEBUG( "Prioritize first pieces: %i\n", i );
#endif
	}
    startupPiecesCountAtEnd = static_cast<int>( ceil( float( piecesCount ) / 400 ) + 0.5 );
    if ( startupPiecesCountAtEnd < MIN_STARTUP_PIECES_END ) startupPiecesCountAtEnd = MIN_STARTUP_PIECES_END;
    for ( int i = fileLastPiece; i > fileLastPiece - startupPiecesCountAtEnd && i > fileFirstPiece; --i )
    {
#ifdef USE_DEADLINES
		//handle.piece_priority(i, 1);
		piecesDeadlines.at(i) = 't';
		handle.set_piece_deadline(i, pieceForUpdateDeadline*10,  torrent_handle::alert_when_available);
		pieceForUpdateDeadline++;
		USE_DEBUG( "set_piece_deadline last pieces: %i\n", i );
#endif
#ifdef USE_PRIORITIES
        handle.piece_priority( i, 7 );
        USE_DEBUG( "Prioritize last pieces: %i\n", i );
#endif
    }
	pieceForUpdateDeadline = fileFirstPiece + startupPiecesCountAtStart;

#ifdef USE_DEADLINES
	//for ( int i = MIN_STARTUP_PIECES_START; i < piecesCount - MIN_STARTUP_PIECES_END; i++ )
 //   {
	//	handle.set_piece_deadline(i, pieceForUpdateDeadline*1000,  torrent_handle::alert_when_available);
	//	pieceForUpdateDeadline++;
	//	USE_DEBUG( "set_piece_deadline other pieces: %i\n", i );
 //   }
#endif	
    //USE_DEBUG( "Startp pieces: %i first, %i last\n", startupPiecesCountAtStart, startupPiecesCountAtEnd );
#ifdef USE_PRIORITIES
    prioritize(-1);
#endif
//  handle.piece_priority( fileLastPiece, 7 );
//  handle.piece_priority( fileLastPiece - 1, 7 );

}

long long tAux::updateDeadlines(int downloadedPieceIndex, long long needSeekToThisPiece)
{
	if ( !handle.is_valid() ) return needSeekToThisPiece;
	bool startupBegin = false;
	bool startupEnd = false;

	if(needSeekToThisPiece != -1)
	{
		USE_DEBUG( "needSeekToThisPiece %i\n", (int)needSeekToThisPiece );
		fileFirstPieceForDeadlines = (int)needSeekToThisPiece;
		handle.clear_piece_deadlines();
		for(unsigned int i = 0; i < piecesDeadlines.size(); i++)
			piecesDeadlines.at(i) = 'f';
		needSeekToThisPiece = -1;
	}

	for ( int i = fileFirstPiece; !startupBegin && i < fileFirstPiece + startupPiecesCountAtStart && i < fileLastPiece; ++i ) 
	{
		if ( !handle.have_piece( i ) )
		{
			USE_DEBUG( "Waiting for first piece: %i\n", i );
//			return;
			startupBegin = true;
			break;
		}
	}
    for ( int i = fileLastPiece; !startupEnd && i > fileLastPiece - startupPiecesCountAtEnd && i > fileFirstPiece; --i )
	{
		if ( !handle.have_piece( i ) )
	    {
	        USE_DEBUG( "Waiting for last piece: %i\n", i );
	//      return;
	        startupEnd = true;
			break;
	    }
	}
	//handle.reset_piece_deadline(downloadedPieceIndex);
	piecesDeadlines.at(downloadedPieceIndex) = 'f';
	//handle.piece_priority( downloadedPieceIndex, 0 );
	USE_DEBUG( "downloadedPieceIndex: %i\n", downloadedPieceIndex );
	if(!startupEnd && !startupBegin)
	{
		int piecesToSet = PRIORITY_PIECES_COUNT_MIN;
		for ( int i = fileFirstPieceForDeadlines; i < fileLastPiece && piecesToSet; ++i )
		{
		    if ( handle.have_piece( i ) ) continue;
			else if(piecesDeadlines.at(i) == 'f')
			{
				handle.set_piece_deadline(i, pieceForUpdateDeadline*10,  torrent_handle::alert_when_available);
				pieceForUpdateDeadline++;
				USE_DEBUG( "set_piece_deadline piece %i  \n", i );
				piecesDeadlines.at(i) = 't';
				--piecesToSet;
			}
			else
				--piecesToSet;
		}
	
	}


	//handle.set_piece_deadline(pieceForUpdateDeadline, pieceForUpdateDeadline*1000,  torrent_handle::alert_when_available);
	//pieceForUpdateDeadline++;
	return needSeekToThisPiece;
}


long long tAux::prioritize(long long needSeekToThisPiece)
{
    if ( !handle.is_valid() ) return needSeekToThisPiece;
    bool startup = false;

	if(needSeekToThisPiece != -1)
	{
		USE_DEBUG( "needSeekToThisPiece %i\n", (int)needSeekToThisPiece );
		fileFirstPiece = (int)needSeekToThisPiece;
		needSeekToThisPiece = -1;
	}

    for ( int i = fileFirstPiece; !startup && i < fileFirstPiece + startupPiecesCountAtStart && i < fileLastPiece; ++i ) if ( !handle.have_piece( i ) )
    {
        USE_DEBUG( "Waiting for first piece: %i\n", i );
//      return;
        startup = true;
    }
    for ( int i = fileLastPiece; !startup && i > fileLastPiece - startupPiecesCountAtEnd && i > fileFirstPiece; --i ) if ( !handle.have_piece( i ) )
    {
        USE_DEBUG( "Waiting for last piece: %i\n", i );
//      return;
        startup = true;
    }



//  USE_DEBUG( "Prioritize\n" );
    int piecesToSet = PRIORITY_PIECES_COUNT_MIN;
    for ( int i = fileFirstPiece; i < fileLastPiece && piecesToSet; ++i )
    {
        if ( handle.have_piece( i ) ) continue;
        const int currPrio = handle.piece_priority( i );
        int prioToSet = 0;

        if ( currPrio == 7 ) prioToSet = 0;
        else if ( startup && currPrio == 0 ) prioToSet = 1;
        else if ( !startup ) prioToSet = 7;

        if ( prioToSet )
        {
            handle.piece_priority( i, prioToSet );
            USE_DEBUG( "Prioritize piece %i prio %i \n", i, prioToSet );
        }
        --piecesToSet;
    }
	return needSeekToThisPiece;
}

bool tAux::canGetUrl()
{
    if ( !handle.is_valid() ) return false;
    const bitfield pieces = handle.status( torrent_handle::query_pieces ).pieces;
    for ( int i = fileFirstPiece; i < fileFirstPiece + startupPiecesCountAtStart; ++i ) if ( !pieces.get_bit( i ) )
        {
            //USE_DEBUG( "Checking for startup - false, first piece %i\n", i );
            return false;
        }
    for ( int i = fileLastPiece; i > fileLastPiece - startupPiecesCountAtEnd; --i ) if ( !pieces.get_bit( i ) )
        {
            //USE_DEBUG( "Checking for startup - false, last piece %i\n", i );
            return false;
        }
    return true;
}

bool tAux::isFinished()
{
    if ( !handle.is_valid() ) return false;
    bitfield pieces = handle.status( torrent_handle::query_pieces ).pieces;
    for ( int i = fileFirstPiece; i < fileLastPiece; ++i ) if ( !handle.have_piece( i ) ) return false;
    return true;
}

Streamerok::Streamerok()
   : nextId( 0 ), ses( fingerprint( "UT", 2, 2, 1, 0 ) // 3,4,2,0,
                       , std::pair<int, int>( 40500, 40599 )
                       , 0
                       , session::add_default_plugins
                       , alert::progress_notification )
#ifdef NO_MONGOOSE_WEB_SERVER
   , needSeekToThisPiece(-1), thread( &threadFunction ) 
#endif
{
    ses.start_lsd();
    ses.start_dht();
    session_settings sets = ses.settings();


#ifdef WIN32
    // windows XP has a limit on the number of
    // simultaneous half-open TCP connections
    // here's a table:

    // windows version       half-open connections limit
    // --------------------- ---------------------------
    // XP sp1 and earlier    infinite
    // earlier than vista    8
    // vista sp1 and earlier 5
    // vista sp2 and later   infinite

    unsigned int winVer = getWinVersion();
    if ( winVer >= 0x060100 )
    {
        // windows 7 and up doesn't have a half-open limit
    }
    else if ( winVer >= 0x060002 )
    {
        // on vista SP 2 and up, there's no limit
    }
    else if ( winVer >= 0x060000 )
    {
        // on vista the limit is 5 (in home edition)
        sets.peer_connect_timeout = 3;
    }
    else if ( winVer >= 0x050102 )
    {
        // on XP SP2 the limit is 10
        sets.peer_connect_timeout = 3;
    }
    else
    {
        // before XP SP2, there was no limit
    }
#endif

//  sets.cache_expiry = 5;
//  sets.peer_connect_timeout = 3;
//  sets.connections_limit = 80;
//  sets.connection_speed = 20;
//  sets.download_rate_limit = 0;

//  sets.anonymous_mode = true;
//  proxy_settings s;
//  s.hostname = "127.0.0.1";
//  s.port = 1080;
//  s.type = libtorrent::proxy_settings::socks5;
//  ses.set_proxy( s );
    sets.announce_to_all_trackers = true;
    sets.announce_to_all_tiers = true;
    sets.prefer_udp_trackers = false;
    sets.max_peerlist_size = 0;


#ifdef _DEBUG
//  sets.tracker_completion_timeout = 20;
//  sets.tracker_receive_timeout = 5;
//  sets.stop_tracker_timeout = 3;
    ses.set_alert_mask( alert::progress_notification
                        | libtorrent::alert::error_notification
                        | libtorrent::alert::status_notification
//                      | libtorrent::alert::storage_notification
//                      | libtorrent::alert::peer_notification
                        | libtorrent::alert::tracker_notification
                        | read_piece_alert::alert_type);
#endif

    sets.user_agent = "BTWebClient/2210(25031)"; //25154

    ses.set_settings( sets );
#ifndef NO_MONGOOSE_WEB_SERVER
    SocketHandler::setDocumentRoot( getTempDir() );
    SocketHandler::startThread( &peridoicCallback );
#endif
}

Streamerok::~Streamerok()
{
#ifdef NO_MONGOOSE_WEB_SERVER
    thread.interrupt();
    thread.join();
#else
    SocketHandler::stop();
    Sleep( 100 );
#endif
}

int Streamerok::setProxy( const char *host, int port, const char *user, const char *pass )
{
    session_settings sets = ses.settings();

    proxy_settings s;
    std::string proxyHostPort;
//  proxyHostPort.clear();
    sets.anonymous_mode = false;

    if ( !host || !*host || port < 1 ) s.type = libtorrent::proxy_settings::none;
    else
    {
        sets.anonymous_mode = true;
        s.hostname = host;
        s.port = port;
        if ( !user || !*user ) s.type = libtorrent::proxy_settings::socks5;
        else
        {
            s.type = libtorrent::proxy_settings::socks5_pw;
            s.username = user;
            s.password = pass;
        }

        proxyHostPort = std::string( host ) + ":" + boost::lexical_cast<std::string>( port );
        USE_DEBUG( "Proxy set to %s\n", proxyHostPort.c_str() );
    }

    ses.set_proxy( s );
    ses.set_settings( sets );
    return proxyHostPort.size();
}

int Streamerok::downloadTorrent( const char *url, const char *fileName )
{
    if ( !url ) return -8;
    add_torrent_params p;
    std::string uri( url );

    if ( uri.find( "magnet:" ) == std::string::npos ) p.url = url;
    else
    {
        USE_DEBUG( "Loading Magnet %s", url );
        error_code ec;
        libtorrent::parse_magnet_uri( uri, p, ec );
        if ( ec )
        {
            USE_DEBUG( "Magnet add error: %s\n", ec.message().c_str() );
            return -8;
        }
    }
    return addTorrent( p, fileName ? fileName : "" );
}

int Streamerok::importTorrent( const char *torrentBody, int bodyLength, const char *fileName )
{
    if ( !torrentBody || !bodyLength ) return -1; //WEBREQUEST_ERROR_BAD_URL;

    error_code ec;

    boost::intrusive_ptr<torrent_info> t = new torrent_info( torrentBody, bodyLength, ec );
    if ( ec )
    {
        USE_DEBUG( "Torrent add error: %s\n", ec.message().c_str() );
        return -8;
    }

    add_torrent_params p;
    p.ti = t;
    return addTorrent( p, fileName ? fileName : "" );
}

int Streamerok::seekInFile(long long offset, int torrentId)
{
	tAux *ta = Streamerok::sPtr->getAux( torrentId );
	if(ta->fileSize < offset)
	{
		USE_DEBUG("seekInFile incorrect offset, return -1");
		return -1;
	}
	//USE_DEBUG("pieceLength %i", ta->pieceLength);
#ifdef USE_PRIORITIES
	for(int i = 0; i < ta->piecesCount; i++)
	{
		ta->handle.piece_priority(i, 0);
	}
	needSeekToThisPiece = ta->prioritize(offset/ta->pieceLength - 1);
#endif
#ifdef USE_DEADLINES
	USE_DEBUG("seekInFile seek to piece %i", int(offset/ta->pieceLength + ta->fileVeryFirstPiece));
	needSeekToThisPiece = ta->updateDeadlines(0,static_cast<int>(offset/ta->pieceLength) + ta->fileVeryFirstPiece);
#endif
	return 0;
}

bool Streamerok::isRangeDownloaded( long long begin, long long end, int torrentId)
{
	tAux *ta = Streamerok::sPtr->getAux( torrentId );
	if(ta->fileSize < begin || ta->fileSize < end) 
	{
		USE_DEBUG("isRangeDownloaded incorrect range, return false");
		return false;
	}

	int firstPeaceNumberInRequestedRange = static_cast<int>(begin/ta->pieceLength) + ta->fileVeryFirstPiece;
	int lastPeaceNumberInRequestedRange = static_cast<int>(end/ta->pieceLength) + ta->fileVeryFirstPiece;
	USE_DEBUG("isRangeDownloaded requested pieces range is from %i to %i", firstPeaceNumberInRequestedRange, lastPeaceNumberInRequestedRange);
	for(int i = firstPeaceNumberInRequestedRange; i <= lastPeaceNumberInRequestedRange; i++)
	{
		if ( !ta->handle.have_piece( i ) )
		{
			USE_DEBUG("isRangeDownloaded piece %i isn't downloaded, return false", i);
			return false;
		}
		else
		{
			USE_DEBUG("isRangeDownloaded piece %i is downloaded", i);
		}
	}
	USE_DEBUG("isRangeDownloaded requested range is fully downloaded, return true");
	return true;
}

int Streamerok::addTorrent( libtorrent::add_torrent_params& p, std::string fileName )
{
    torrent_handle h;

    p.save_path = tempDirValid() ? getTempDir() : std::string( "." );
    p.storage_mode = libtorrent::storage_mode_allocate;
    int index = getNextId();

    error_code ec;
    h = ses.add_torrent( p, ec );

    if ( ec )
    {
        USE_DEBUG( "Torrent add error: %s\n", ec.message().c_str() );
        return -10;
    }

    time_t timeout = time( NULL ) + METADATA_TIMEOUT_S;
    boost::chrono::milliseconds dura( 500 );
    while ( time( NULL ) < timeout && h.is_valid() && !h.status( 0 ).has_metadata ) boost::this_thread::sleep_for( dura );

    if ( !h.is_valid() ) return -11;
    if ( !h.status().has_metadata )
    {
        USE_DEBUG( "Torrent metadata download timeout\n" );
        ses.remove_torrent( h );
        return -12;
    }

    int longestIndex = -1;
    size_type longestSize = 0;

    try
    {
        longestIndex = getFileIndex( h, fileName );
        if ( longestIndex < 0 )
        {
            USE_DEBUG( "no file selected\n" );
            deleteTorrent( index );
            return -9;
        }
		//longestSize = h.get_torrent_info().file_at( longestIndex ).size; //get_torrent_info() is deprecated
		longestSize = h.torrent_file()->file_at( longestIndex ).size;
    }
    catch (...)
    {
        USE_DEBUG( "Exception on getFileIndex\n" );
        deleteTorrent( index );
        return -9;
    }

    USE_DEBUG( "torrent added.\n" );
    tMap.insert( torrentMap::value_type( index, tAux( h, longestIndex, longestSize ) ) );
    return index;
}


int Streamerok::getFileIndex( libtorrent::torrent_handle& h, std::string fileName )
{
    if ( !h.is_valid() || !h.status( 0 ).has_metadata ) return -1;
    //torrent_info ti = h.get_torrent_info(); //get_torrent_info() is deprecated
	boost::intrusive_ptr<const libtorrent::torrent_info> intrPtrTI = h.torrent_file();
    if ( !intrPtrTI->is_valid() ) return -1;

    std::vector<boost::uint8_t> filePrio;
    size_type longestSize = 0;
    int longestIndex = -1;

    for ( int i = 0; i < intrPtrTI->num_files(); ++i )
    {
        h.file_priority( i, 0 );
        const file_entry file = intrPtrTI->file_at( i );
#ifdef WIN32
        USE_DEBUG( "[%i] %08I64d %s\n", i, file.size, file.path.c_str() );
#else
        USE_DEBUG( "[%i] %08lld %s\n", i, file.size, file.path.c_str() );
#endif
        if ( file.pad_file || longestSize == std::numeric_limits<int64_t>::max() ) continue;
        if ( file.size > longestSize )
        {
            longestSize = file.size;
            longestIndex = i;
        }
        if ( fileName.size() && file.path.find( fileName ) != std::string::npos )
        {
            longestIndex = i;
            longestSize = std::numeric_limits<int64_t>::max();
        }
    }
    if ( longestIndex >= intrPtrTI->num_files() ) return -1;
//  filePrio[longestIndex] = 1;
#ifdef WIN32
    USE_DEBUG( "selected [%i] %08I64d %s\n", longestIndex, intrPtrTI->file_at( longestIndex ).size, intrPtrTI->file_at( longestIndex ).path.c_str() );
#else
    USE_DEBUG( "selected [%i] %08lld %s\n", longestIndex, intrPtrTI->file_at( longestIndex ).size, intrPtrTI->file_at( longestIndex ).path.c_str() );
#endif
    return longestIndex;
}




tAux* Streamerok::getAux( int torrentId )
{
    torrentMap::iterator it = tMap.find( torrentId );
    if ( it == tMap.end() ) return 0;
    tAux *t = &it->second;
    if ( t && t->handle.is_valid() ) return t;
    tMap.erase( it );
    return 0;
}

dlInfo Streamerok::getTorrentInfo( int torrentId )
{
    dlInfoExt result;

    tAux *tInf = getAux( torrentId );
    if ( !tInf ) return result;

    result.total = tInf->fileSize;

    torrent_handle *t = &tInf->handle;

    torrent_status ts = t->status( torrent_handle::query_pieces );
    result.downloadRateBs = ts.download_payload_rate;
    result.seeders = ts.num_seeds;
    result.peers = ts.num_peers;
    result.piecesDone = ts.num_pieces;
    result.piecesStartup = tInf->startupPiecesCountAtStart + tInf->startupPiecesCountAtEnd;
    //torrent_info ti = t->get_torrent_info(); // get_torrent_info() is deprecated
	boost::intrusive_ptr<const libtorrent::torrent_info> intrPtrTI = t->torrent_file();
    result.piecesTotal = tInf->fileLastPiece - tInf->fileFirstPiece + 1;
    result.pieceSize = intrPtrTI->piece_length();
    result.downloaded = tInf->fileSize == ts.total_wanted_done ? tInf->fileSize : countPieces( ts.pieces, tInf->fileFirstPiece, tInf->fileLastPiece ) * intrPtrTI->piece_length();
    result.piecesStartupAtEnd = tInf->startupPiecesCountAtEnd;
    result.piecesStartupDone = countPieces( ts.pieces, tInf->fileFirstPiece, tInf->fileFirstPiece + tInf->startupPiecesCountAtStart ) +
       countPieces( ts.pieces, tInf->fileLastPiece - tInf->startupPiecesCountAtEnd, tInf->fileLastPiece );

//  const int first = tInf->fileFirstPiece;
//  const int last = tInf->fileLastPiece;
    //USE_DEBUG( "File pieces %i of %i; %i.%i.%i.%i.%i...%i.%i.%i.%i.%i\n",
    //           ts.num_pieces, tInf->fileLastPiece - tInf->fileFirstPiece + 1,
    //           ts.pieces.get_bit( first ), ts.pieces.get_bit( first + 1 ), ts.pieces.get_bit( first + 2 ), ts.pieces.get_bit( first + 3 ), ts.pieces.get_bit( first + 4 ),
    //           ts.pieces.get_bit( last - 4 ), ts.pieces.get_bit( last - 3 ), ts.pieces.get_bit( last - 2 ), ts.pieces.get_bit( last - 1 ), ts.pieces.get_bit( last )
    //          );
    return result;
}

//std::string Streamerok::getFileRelPath( int torrentId )
//{
//    std::string path;
//    tInfo *tInf = getHandle( torrentId );
//    if ( !tInf ) return path;
//
//    torrent_handle *t = &tInf->handle;
//    torrent_info ti = t->get_torrent_info(); // get_torrent_info() is deprecated
//    for ( int i = 0; i < ti.num_files(); ++i )
//    {
//        if ( t->file_priority( i ) == 0 ) continue;
//        const file_entry file = ti.file_at( i );
//        path = file.path;
//        break;
//    }
//    if ( !isExistsAndNotEmpty( getTempDir() + "\\" + path ) ) return std::string();
//    return path;
//}

std::string Streamerok::getFileUrl( int torrentId )
{
	//USE_DEBUG("in getFileUrl ");
    tAux *tInf = getAux( torrentId );
    if ( !tInf ) return std::string();

    if ( tInf->fileUri.size() )
    {
      //USE_DEBUG( "GetUrl: already made\n" );
        return tInf->fileUri;
    }

    if ( !tInf->canGetUrl())
    {
     // USE_DEBUG( "GetUrl: no pieces\n" );
        return std::string();
    }

    torrent_handle *t = &tInf->handle;
    if ( !t || !t->is_valid() )
    {
      //USE_DEBUG( "GetUrl: torrent is invalid\n" );
        return std::string();
    }
    //torrent_info ti = t->get_torrent_info(); //get_torrent_info() is deprecated
	boost::intrusive_ptr<const libtorrent::torrent_info> intrPtrTI = t->torrent_file();
    const file_entry file = intrPtrTI->file_at( tInf->fileIndex );
    std::string path = file.path;

    if ( !path.size() )
    {
        //USE_DEBUG( "GetUrl: torrent is invalid\n" );
        return std::string();
    }
    if ( !isExistsAndNotEmpty( getTempDir() + NATIVE_PATH_SEPARATOR + path ) )
    {
        //USE_DEBUG( "GetUrl: path is invalid %s%s%s\n", getTempDir().c_str(), NATIVE_PATH_SEPARATOR, path.c_str() );
        return std::string();
    }
    tInf->filePath = path;
#ifdef WIN32
    replaceAll( tInf->filePath, NATIVE_PATH_SEPARATOR, "/" );
#endif
    //USE_DEBUG( "URL before encoding %s\n", path.c_str() );
#ifdef FILEPATH_IN_URL
    path.insert( 0, "\"" + getTempDir() + NATIVE_PATH_SEPARATOR );
    path += "\"";
#else
    path = urlEncode( path );
    path.insert( 0, SocketHandler::getUrlBase() );
#endif
    tInf->fileUri = path;
#ifdef _DEBUG
    boost::filesystem::detail::utf8_codecvt_facet utf8;
    USE_DEBUG( "URL made %s (%s)\n", path.c_str(), boost::algorithm::hex( path ).c_str() );
#endif
    return path;
}

bool Streamerok::getFileUrl( int torrentId, char *buffer, int length )
{
    if ( !buffer || !length )
    {
        //USE_DEBUG( "getFileUrl buffer error\n" );
        return false;
    }
    *buffer = 0;
    const unsigned int len = length;
    const std::string path = getFileUrl( torrentId );
    if ( !path.size() || path.size() >= len ) return false;
    buffer[path.size()] = 0;
    USE_DEBUG( "getFileUrl have URL\n" );
    return path.copy( buffer, path.size() ) == path.size();
}

tAux* Streamerok::getAuxByUriPath( const char *uri )
{
    if ( !uri || !*uri  ) return 0;
    std::string sUri;
    if ( *uri == '/' ) sUri = std::string( uri + 1 );
    else sUri = std::string( uri );
//  USE_DEBUG( "Searching for uri %s\n", sUri.c_str() );
    BOOST_FOREACH( torrentMap::value_type & v, tMap )
    {
        if ( v.second.filePath.find( sUri ) == std::string::npos )
        {
//          USE_DEBUG( "Skipping file with uri %s\n", v.second.filePath.c_str() );
            continue;
        }
//      USE_DEBUG( "torrent found index %i\n", v.first );
        return &v.second;

//      std::string filePath = getFileRelPath( v.first );
//      if ( !filePath.size() ) continue;
//      replaceAll( filePath, NATIVE_PATH_SEPARATOR, "/" );
//      if ( filePath.find( sUri ) == std::string::npos )
//      {
//          USE_DEBUG( "Skipping file with uri %s\n", filePath.c_str() );
//          continue;
//      }
//      USE_DEBUG( "torrent found index %i\n", v.first );
//      return getTorrentInfo( v.first );
    }
    USE_DEBUG( "torrent not found\n" );
    return 0;
}

int Streamerok::getIndexByUriPath( const char *uri )
{
    if ( !uri || !*uri  ) return -1;
    std::string sUri;
    if ( *uri == '/' ) sUri = std::string( uri + 1 );
    else sUri = std::string( uri );
    USE_DEBUG( "Searching for uri %s\n", sUri.c_str() );
    BOOST_FOREACH( torrentMap::value_type & v, tMap )
    {
        if ( v.second.filePath.find( sUri ) == std::string::npos )
        {
            USE_DEBUG( "Skipping file with uri %s\n", v.second.filePath.c_str() );
            continue;
        }
        USE_DEBUG( "torrent found index %i\n", v.first );
        return v.first;

    }
    USE_DEBUG( "torrent not found\n" );
    return -2;

}

NetUsage Streamerok::getNetworkUsage()
{
    session_status ss = ses.status();
    return NetUsageExt( ss.upload_rate, ss.download_rate, ss.total_upload, ss.total_download );
}

bool Streamerok::deleteTorrent( int torrentId, bool deleteCompleted )
{
    torrentMap::iterator it = tMap.find( torrentId );
    if ( it == tMap.end() )
    {
        USE_DEBUG( "Can't find data for torrent index %i\n", torrentId );
        return false;
    }
    torrent_handle *t = &it->second.handle;
    const bool hadTorrent = t && t->is_valid();
    if ( hadTorrent )
    {
        try
        {
            ses.remove_torrent( *t, deleteCompleted || !it->second.isFinished() ? session::delete_files : 0 );
        }
        catch (...)
        {
            USE_DEBUG( "Torrent removal error, index %i\n", torrentId );
        }
    }
    else USE_DEBUG( "Cant't associate torrent index %i\n", torrentId );
    tMap.erase( it );
    USE_DEBUG( "Torrent %i removed\n", torrentId );
    return hadTorrent;
}

int getTorrentIndex( const char *uri )
{
    if ( !Streamerok::sPtr ) return -3;
    return Streamerok::sPtr->getIndexByUriPath( uri );
}

unsigned long long getFileSize( int torrentId )
{
    if ( !Streamerok::sPtr ) return 0;
    tAux *ta = Streamerok::sPtr->getAux( torrentId );
    if ( !ta ) return 0;
    return ta->fileSize;
}

int Streamerok::countPieces( libtorrent::bitfield& pieces, int first, int last )
{
    int piecesDone = 0;
    for ( int i = first; i <= last; ++i ) if ( pieces.get_bit( i ) ) ++piecesDone;
        else break;
    return piecesDone;
}

unsigned long long getFileDownloaded( int torrentId )
{
    if ( !Streamerok::sPtr ) return 0;
    tAux *ta = Streamerok::sPtr->getAux( torrentId );
    if ( !ta ) return 0;

    torrent_status ts = ta->handle.status( torrent_handle::query_pieces );
    if ( ta->fileSize == ts.total_wanted_done ) return ta->fileSize;
    return Streamerok::countPieces( ts.pieces, ta->fileFirstPiece, ta->fileLastPiece ) * ta->pieceLength;
}

bool Streamerok::handle_alert( libtorrent::alert *a )
{
	if(read_piece_alert* p = alert_cast<read_piece_alert>( a ))
	{
		USE_DEBUG( "read_piece_alert %i\n", p->piece);
        BOOST_FOREACH( torrentMap::value_type & v, tMap )
        {
            if ( v.second.handle != p->handle ) continue;
			needSeekToThisPiece = v.second.updateDeadlines(p->piece, needSeekToThisPiece);
            //USE_DEBUG( "Reprioritize\n" );
            return true;
        }
		return true;
	}
    else if ( piece_finished_alert *p = alert_cast<piece_finished_alert>( a ) )
    {
        //USE_DEBUG( "Piece %i finish alert\n", p->piece_index );
#ifdef USE_PRIORITIES
        BOOST_FOREACH( torrentMap::value_type & v, tMap )
        {
            if ( v.second.handle != p->handle ) continue;
            needSeekToThisPiece = v.second.prioritize(needSeekToThisPiece);
            USE_DEBUG( "Reprioritize\n" );
            return true;
        }
#else
        ( void ) p;
#endif
        return true;
    }
    else if ( file_completed_alert *p = alert_cast<file_completed_alert>( a ) )
    {
#ifdef USE_PRIORITIES
        BOOST_FOREACH( torrentMap::value_type & v, tMap )
        {
            if ( v.second.handle != p->handle ) continue;
            if ( v.second.isFinished() ) return true;
            USE_DEBUG( "Reprioritize on file finished\n" );
            needSeekToThisPiece = v.second.prioritize(needSeekToThisPiece);
            return true;
        }
#else
        ( void ) p;
#endif
        return true;
    }
    else if ( torrent_checked_alert *p = alert_cast<torrent_checked_alert>( a ) )
    {
#ifdef USE_PRIORITIES
        BOOST_FOREACH( torrentMap::value_type & v, tMap )
        {
            if ( v.second.handle != p->handle ) continue;
            if ( v.second.isFinished() ) return true;
            USE_DEBUG( "Reprioritize on file checked\n" );
            needSeekToThisPiece = v.second.prioritize(needSeekToThisPiece);
            return true;
        }
#else
        ( void ) p;
#endif
        return true;
    }
#ifdef _DEBUG
    else if ( anonymous_mode_alert *p = alert_cast<anonymous_mode_alert>( a ) )
    {
        (void) p;
        //USE_DEBUG( "Anonymous mode alert, tracker %s\n", p->str.c_str() );
        return true;
    }
    else if ( alert_cast<block_finished_alert>( a )
              || alert_cast<block_downloading_alert>( a )
              || alert_cast<state_update_alert>( a )
             ) return true;
    else if ( a )
    {
        (void) p;
       // USE_DEBUG( "Alert type %i '%s'\n", a->category(), a->message().c_str() );
        return true;
    }
#endif
    return false;
}


void Streamerok::peridoicCallback()
{
    if ( !Streamerok::sPtr ) return;
    libtorrent::session *ses = &Streamerok::sPtr->ses;
    ses->post_torrent_updates();

    std::deque<alert *> alerts;
    ses->pop_alerts( &alerts );
    for ( std::deque<alert *>::iterator i = alerts.begin(), end( alerts.end() ); i != end; ++i )
    {
        TORRENT_TRY
        {
            if ( !sPtr->handle_alert( *i ) )
            {
//              libtorrent::alert const *a = *i;
//              USE_DEBUG( "Alert not handled %s\n", a->message().c_str() );
            }
        }
           TORRENT_CATCH( std::exception & e )
        {
            (void) e;
            USE_DEBUG( "alert exception\n" );
        }

        delete *i;
    }
    alerts.clear();
}

void threadFunction()
{
//  USE_DEBUG( "Thread is started\n" );
    for (;;)
    {
        try
        {
            Streamerok::peridoicCallback();
            boost::this_thread::sleep( boost::posix_time::milliseconds( 1000 ) );
        }
        catch (boost::thread_interrupted&)
        {
            break;
        }
    }
//          USE_DEBUG( "Thread is stopped\n" );
}

