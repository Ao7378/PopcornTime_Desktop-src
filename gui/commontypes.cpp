#include "commontypes.h"
#include "lib.h"

#include <QCryptographicHash>
#include <QBuffer>
#include <QFileInfo>

#include <QJsonArray>
#include <QJsonDocument>

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#include <quazip/quagzipfile.h>
#include <quazip/quazipnewinfo.h>

#include <QTemporaryFile>
#include <QDir>
#include <QUrlQuery>

#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <QSharedPointer>

SubtitleItem::SubtitleItem( const TorrentInfo& torrentInfo, QString theLang, QByteArray theAbbr, QUrl theUrl )
   : m_language( theLang ), m_langAbbr( theAbbr ), m_url( theUrl )
{
    m_torrentDesc = torrentInfo.getTorrentDesc();
}


void SubtitleItem::setOriginalName( QByteArray name )
{
    if ( name.startsWith( "\"" ) && name.endsWith( "\"" ) ) name = name.mid( 1, name.size() - 2 );
    if ( name.startsWith( "'" ) && name.endsWith( "'" ) ) name = name.mid( 1, name.size() - 2 );
    m_zipName = name;
}

bool SubtitleItem::setZipData( const QByteArray& data )
{
    if ( data.isEmpty() ) return false;
    m_zipData = data;

#ifdef _DEBUGGING
    if ( data.size() < 100 ) qDebug() << __FUNCTION__ << "zipData: " << data;
#endif

    QCryptographicHash hash( QCryptographicHash::Md5 );
    hash.addData( data );
    m_zipMD5 = hash.result().toHex().toLower();

    QSharedPointer<QIODevice> file;
    QSharedPointer<QTemporaryFile> tmpFile;

    QSharedPointer<QBuffer> buf;
    QSharedPointer<QuaZip> zip;

    QFileInfo zipInfo( m_zipName.size() ? m_zipName : m_url.path() );
    const QString zipName = zipInfo.fileName();
    const QString originalName = zipInfo.completeBaseName();
    const bool isGzip = zipName.endsWith( "gz", Qt::CaseInsensitive );
//  qDebug() << __FUNCTION__ << getTorrentHash() << language() << "zipName: " << zipName << "origName:" << originalName;

    if ( isGzip )
    {
        tmpFile.reset( new QTemporaryFile( QDir::tempPath() + QDir::separator() + originalName + "_XXXXXX.gz" ) );
        if ( !tmpFile->open() )
        {
            qDebug() << __FUNCTION__ << getTorrentHash() << language() << "Can't create temp file: " << tmpFile->fileName();
            return false;
        }
//      qDebug() << __FUNCTION__ << getTorrentHash() << language() << "Temp file: " << tmpFile->fileName();
        tmpFile->write( data );
        tmpFile->close();

        file.reset( new QuaGzipFile( tmpFile->fileName() ) );
        m_fileName = originalName;
    }
    else
    {
        buf.reset( new QBuffer( &m_zipData ) );
        zip.reset( new QuaZip( buf.data() ) );

        if ( !zip->open( QuaZip::mdUnzip ) )
        {
            qDebug() << __FUNCTION__ << getTorrentHash() << language() << "Can't open ZIP file: ";
            return false;
        }

        QList<QuaZipFileInfo> fileInfos = zip->getFileInfoList();
        if ( fileInfos.isEmpty() )
        {
            qDebug() << __FUNCTION__ << getTorrentHash() << language() << "zip is empty";
            return false;
        }


        file.reset( new QuaZipFile( zip.data() ) );
        if ( fileInfos.size() == 1 )
        {
            QFileInfo fi( zip->getFileNameList().first() );
            m_fileName = fi.fileName();
            zip->goToFirstFile();
            //      qDebug() << __FUNCTION__ << getTorrentHash() << language() << "zip has single file: " << m_fileName;
        }
        else
        {
            quint32 largest = 0;
            foreach( const QuaZipFileInfo& info, fileInfos )
            {
                QFileInfo fi( info.name );
                if ( m_fileName.endsWith( "srt", Qt::CaseInsensitive ) )
                {
                    //              qDebug() << __FUNCTION__ << getTorrentHash() << language() << "zip file checked (have .srt): " << info.name << info.uncompressedSize;
                    if ( !fi.suffix().endsWith( "srt", Qt::CaseInsensitive ) || info.uncompressedSize <= largest ) continue;
                }
                else
                {
                    //              qDebug() << __FUNCTION__ << getTorrentHash() << language() << "zip file checked: " << info.name << info.uncompressedSize;
                    if ( info.uncompressedSize <= largest && !fi.suffix().endsWith( "srt", Qt::CaseInsensitive ) ) continue;
                }
                largest = info.uncompressedSize;
                zip->setCurrentFile( info.name );
                m_fileName = info.name;
                //          qDebug() << __FUNCTION__ << getTorrentHash() << language() << "zip file selected: " << m_fileName << largest;
            }
        }

        if ( !zip->hasCurrentFile() )
        {
            qDebug() << __FUNCTION__ << getTorrentHash() << language() << "no file is opened within zip";
            return false;
        }
    }



    if ( !file->open( QIODevice::ReadOnly ) )
    {
        qDebug() << __FUNCTION__ << getTorrentHash() << language() << "Can't open archive";
        return false;
    }

    m_data = file->readAll();
    file->close();
    if ( !zip.isNull() ) zip->close();

    if ( m_data.isEmpty() ) return false;
    hash.reset();
    hash.addData( m_data );
    m_dataMD5 = hash.result().toHex().toLower();
    return true;
}

void SubtitleItem::setData( const QByteArray& data, QByteArray name )
{
    if ( data.isEmpty() ) return;
    m_data = data;

    QCryptographicHash hash( QCryptographicHash::Md5 );
    hash.addData( data );
    m_dataMD5 = hash.result().toHex().toLower();

    if ( name.startsWith( "\"" ) && name.endsWith( "\"" ) ) name = name.mid( 1, name.size() - 2 );
    if ( name.startsWith( "'" ) && name.endsWith( "'" ) ) name = name.mid( 1, name.size() - 2 );
    m_fileName = name;
}

bool SubtitleItem::canDisplay() const
{
    return m_data.size();
}

}

QString SubtitleItem::zipFileName() const
{
    if ( m_zipName.isEmpty() ) return m_url.fileName();
    return m_zipName;
}

SubtitleItem::operator QString() const
{
    return m_langAbbr + " " + m_dataMD5 + " " + ( m_zipData.size() < 300 ? m_zipData : QString::number( m_data.size() ) );
}



DownloadInfo::DownloadInfo() :
   downloadRateBs( 0 ),
   downloaded( 0 ), total( 0 ),
   piecesDone( 0 ), piecesTotal( 0 ), piecesStartup( 0 ), pieceSize( 0 ),
   seeders( 0 ), peers( 0 ), piecesStartupAtEnd( 0 ), piecesStartupDone( 0 )
{ }

DownloadInfo::DownloadInfo( const DownloadInfo& info ) :
   downloadRateBs( info.downloadRateBs ),
   downloaded( info.downloaded ), total( info.total ),
   piecesDone( info.piecesDone ), piecesTotal( info.piecesTotal ), piecesStartup( info.piecesStartup ), pieceSize( info.pieceSize ),
   seeders( info.seeders ), peers( info.peers ), piecesStartupAtEnd( info.piecesStartupAtEnd ), piecesStartupDone( info.piecesStartupDone )
{ }

DownloadInfo::DownloadInfo( const dlInfo& info ) :
   downloadRateBs( info.downloadRateBs ),
   downloaded( info.downloaded ), total( info.total ),
   piecesDone( info.piecesDone ), piecesTotal( info.piecesTotal ), piecesStartup( info.piecesStartup ), pieceSize( info.pieceSize ),
   seeders( info.seeders ), peers( info.peers ), piecesStartupAtEnd( info.piecesStartupAtEnd ), piecesStartupDone( info.piecesStartupDone )
{ }


TorrentInfo::TorrentInfo( const QJsonObject& jObj )
{
    json = jObj;

    title = jObj.value( "title" ).toString();
    QJsonObject tor = jObj.value( "torrent" ).toObject();
    url = tor.value( "url" ).toString();
    magnet = tor.value( "magnet" ).toString();
    fileName = tor.value( "file" ).toString();
    updateTorrentHash();
}

TorrentInfo::TorrentInfo( QString iurl,
                          QString imagnet,
                          QString ifileName,
                          QString isubtitles_url,
                          QString iTitle,
                          QByteArray ilocale,
                          QByteArray iquality )
   : index( -1 ), url( iurl ), magnet( imagnet ), fileName( ifileName ), title( iTitle )
{
    (void) iquality; ( void ) isubtitles_url; ( void ) ilocale;
    updateTorrentHash();
}


QJsonValue TorrentInfo::getJsonValue( const QString& key ) const { return json.value( key ); }

QJsonObject TorrentInfo::getTorrentDesc() const
{
    QJsonObject result;
    const QString imdb = getImdb();
    const QByteArray hash = getTorrentHash().toLower();

    if ( imdb.isEmpty() || hash.isEmpty() ) return result;

    result["imdb"] = QJsonValue( imdb );
    result["torrentHash"] = QJsonValue( QString( hash ) );
    result["filename"] = fileName;
    result["image"] = getJsonValue( "image" );
    result["title"] = getJsonValue( "title" );
    result["api"] = 1;
    if ( getJsonValue( "season_id" ).isDouble() && getJsonValue( "episode_id" ).isDouble() )
    {
        result["season"] = getJsonValue( "season_id" );
        result["episode"] = getJsonValue( "episode_number" );
        result["eimdb"] = getJsonValue( "eimdb" );
    }
    else result["year"] = getJsonValue( "year" );
    return result;
}


void TorrentInfo::updateTorrentHash()
{
    static const QRegularExpression torrentHashMagnetMatcher( "\\burn:btih:([A-Fa-f\\d]{40})\\b" );
    static const QRegularExpression torrentHashMatcher( "\\b([A-Fa-f\\d]{40})\\b" );
    QRegularExpressionMatch match;
    if ( ( match = torrentHashMagnetMatcher.match( magnet ) ).hasMatch() )
    {
        torrentHash = match.captured( 1 ).toLatin1().toLower();
        qDebug() << __FUNCTION__ << "from magnet" << torrentHash;
    }
    else if ( ( match = torrentHashMatcher.match( url ) ).hasMatch() )
    {
        torrentHash = match.captured( 1 ).toLatin1();
        qDebug() << __FUNCTION__ << "from torrent url" << torrentHash;
    }
}

TorrentInfo TorrentInfo::fromUrl( const QUrl url, QString fileName )
{
    if ( url.scheme() == "magnet" ) return TorrentInfo( "", url.toString(), fileName, "", QUrlQuery( url ).queryItemValue( "dn", QUrl::FullyDecoded ) ); 
    if ( url.scheme() == "http" || url.scheme() == "https" ) return TorrentInfo( url.toString(), "", fileName, "" );
    return TorrentInfo();
}




void registerMetaTypes()
{
    qRegisterMetaType<DownloadInfo>( "DownloadInfo" );
#ifdef USE_SOCKS5
    qRegisterMetaType<ProxySettings>( "ProxySettings" );
#endif
    qRegisterMetaType<TorrentInfo>( "TorrentInfo" );
    qRegisterMetaType<SubtitleItem>( "SubtitleItem" );
    qRegisterMetaType<TIndexedString>( "TIndexedString" );
}
