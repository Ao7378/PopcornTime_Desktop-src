#include "TorHelpers.h"
#include "hostApp.h"
#include "defaults.h"
#include "lib.h"

#include <QObject>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QSslSocket>
#include <QElapsedTimer>
#include <QFile>


DownloadNotifyThread::DownloadNotifyThread( QObject *parent ) : QThread( parent ), tId( 0 )
{ }

DownloadNotifyThread::~DownloadNotifyThread() { stopOnTorrent(); }

void DownloadNotifyThread::startOnTorrent( int torrentId )
{
    stopOnTorrent();
    tId = torrentId;
    start();
}

void DownloadNotifyThread::stopOnTorrent()
{
    if ( !isRunning() ) return;
    requestInterruption();
    wait();
}

void DownloadNotifyThread::run()
{
    dlInfo info;
    int progress = 0;
    int hadPieces = 0;
    int pieceBytes = 0;
    QByteArray url;
    url.resize( 2000 );
    bool haveUrl = false;
    forever
    {
        msleep(TORRENT_INFO_UPDATE_DELAY_MS);
        if ( QThread::currentThread()->isInterruptionRequested() ) return;
        info = *torrentGetInfo( tId );
        if ( haveUrl )
        {
            emit downloadInfoChanged( DownloadInfo( info ) );
        }
        else
        {
            if ( ( haveUrl = torrentGetFileUrl( tId, url.data(), url.size() ) ) == true )
            {
                QString theUrl = QString( url ); //::fromUtf8
                theUrl.replace( "\\", "/" );
                theUrl.replace( "\"", "" );
                qDebug() << "torrentDataAvailable" << theUrl << "raw" << theUrl.toUtf8().toHex();
                emit torrentDataAvailable( theUrl );
            }
            else
            {
//              qDebug() << "pieces" << info.piecesDone << "of" << info.piecesTotal;
                if ( hadPieces < info.piecesStartupDone )
                {
                    hadPieces = info.piecesStartupDone;
                    pieceBytes = 0;
                }
                float ByteRatio;
                if ( !info.piecesStartup ) ByteRatio = 0;
                else
                {
                    ByteRatio = 1 - info.piecesStartupDone / info.piecesStartup;
                    if ( ByteRatio < 0 ) ByteRatio = 0.1f;
                }

                pieceBytes += info.downloadRateBs / ( 1000 / TORRENT_INFO_UPDATE_DELAY_MS ) * ByteRatio; 
                const int currentProgress = info.pieceSize ? ( pieceBytes + info.piecesStartupDone * info.pieceSize ) * 100 / ( info.pieceSize * info.piecesStartup ) : 0;
                if ( currentProgress > progress ) progress = currentProgress;
                if ( progress > 100 ) progress = 100;
                else if ( progress == 0 && ( info.seeders || info.peers ) ) progress = 3;
                QString msg = progress > 3 ? TK_DOWNLOADING : TK_LOOKING_FOR_PEERS;
                emit torrentProgressAvailable( progress, info.downloadRateBs / 1000, info.seeders, info.peers, msg );
//              qDebug() << "torrentProgressAvailable:" << progress << "%," << info.downloadRateBs / 1000 << "B/s, seeds/peers:" << info.seeders << info.peers << msg;
            }
        }
    }
}



TorrentDownloadThread::TorrentDownloadThread( QObject *parent, TorrentInfo& tInfo ) :
   QThread( parent ), m_info( tInfo )
{
    connect( this, SIGNAL( finished() ), this, SLOT( deleteLater() ) );
    start();
}

TorrentDownloadThread::~TorrentDownloadThread() { abort(); }

void TorrentDownloadThread::abort()
{
    if ( !isRunning() ) return;
    requestInterruption();
    wait();
}

void TorrentDownloadThread::onTorrentFileDownloaded( QByteArray torrentFile )
{
    data = torrentFile;
    emit gotMetadata();
}

int TorrentDownloadThread::getAddTorrentFile()
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot( true );
    timer.setInterval( TORRENT_FILE_DOWNLOAD_TIMEOUT_MS );
    QObject::connect( &timer, SIGNAL( timeout() ), &loop, SLOT( quit() ) );
    QObject::connect( this, SIGNAL( gotMetadata() ), &loop, SLOT( quit() ) );
    QObject::connect( Requester::getInstance(), SIGNAL( haveTorrentFile( QByteArray ) ), this, SLOT( onTorrentFileDownloaded( QByteArray ) ) );
    QMetaObject::invokeMethod( Requester::getInstance(), qPrintable( "getTorrentFile" ), Q_ARG( QString, m_info.url ) );

    timer.start();
    loop.exec();
    QObject::disconnect( Requester::getInstance(), SIGNAL( haveTorrentFile( QByteArray ) ), this, SLOT( onTorrentFileDownloaded( QByteArray ) ) );

    int result = -20;

    if ( timer.isActive() && data.size() )
    {
        qDebug() << this->metaObject()->className() << __FUNCTION__ << "Torrent response read" << data.left( 50 ) << "...";
        result = torrentAddImport( data.constData(), data.size(), m_info.fileName.toLocal8Bit().constData() );
    }
    else qDebug() << this->metaObject()->className() << __FUNCTION__ << "timeout or error";
    timer.stop();
    return result;
}

void TorrentDownloadThread::run()
{
//  qDebug() << __FUNCTION__ << "loading" << m_info.url << m_info.magnet;
    m_info.index = -30;

    QUrl url( m_info.url );
    if ( url.isValid() )
    {
        if ( !url.isLocalFile() ) m_info.index = getAddTorrentFile();
        else
        {
            QFile file( url.toLocalFile() );
            QByteArray buffer;
            if ( file.open( QIODevice::ReadOnly ) && !( buffer = file.readAll() ).isEmpty() )
            {
                m_info.index = torrentAddImport( buffer.constData(), buffer.size(), m_info.fileName.toLatin1() );
                qDebug() << this->metaObject()->className() << __FUNCTION__ << "local file load " << m_info.index;
            }
        }
    }

    if ( m_info.index < 0 && !m_info.magnet.isEmpty() )
    {
        qDebug() << this->metaObject()->className() << __FUNCTION__ << "torrent failed, getting magnet" << m_info.url << m_info.magnet;
        m_info.index = torrentAddStart( m_info.magnet.toLatin1(), m_info.fileName.toLatin1() );
    }

    if ( m_info.index < 0 ) emit getMetadataError( m_info );
    else emit torrentDownloaded( m_info );
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << "load end: " << bool( m_info.index >= 0 ) << m_info.getTorrentDesc();
}

Requester *Requester::instance_ = 0;

Requester::Requester() : manager_( new QNetworkAccessManager( this ) ), thread_(), torrentReply_( 0 )
{
    QObject::connect( &thread_, SIGNAL( started() ), this, SLOT( sslInit() ) );
    moveToThread( &thread_ );
    if ( !instance_ ) instance_ = this;
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << QThread::currentThread();
    thread_.start();
}

void Requester::sslInit()
{
    QElapsedTimer myTimer;
    myTimer.start();
    volatile bool supports = QSslSocket::supportsSsl();
    qint64 elapsed = myTimer.elapsed();

    qDebug() << "elapsed time to call supportsSSL = " << supports << elapsed << " msec.";
//  qDebug() << "Support SSL:  " << QSslSocket::supportsSsl()
//      << "\nLib Version Number: " << QSslSocket::sslLibraryVersionNumber()
//      << "\nLib Version String: " << QSslSocket::sslLibraryVersionString()
//      << "\nLib Build Version Number: " << QSslSocket::sslLibraryBuildVersionNumber()
//      << "\nLib Build Version String: " << QSslSocket::sslLibraryBuildVersionString();
}

Requester::~Requester()
{
    if ( instance_ == this ) instance_ = 0;
    manager_->deleteLater();
    thread_.quit();
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << QThread::currentThread() <<
    thread_.wait( 500 );
}

void Requester::getSubtitleThumb( QString url )
{
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << QThread::currentThread() << url;
    QUrl qurl( url );
    if ( !qurl.isValid() ) return;

    QNetworkReply *reply = manager_->get( QNetworkRequest( qurl ) );
    connect( reply, SIGNAL( finished() ), this, SLOT( onSubtitleThumbReqestFinished() ) );
}

void Requester::getUrl( QString url )
{
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << QThread::currentThread();
    QNetworkReply *reply = manager_->get( QNetworkRequest( QUrl( url ) ) );
    connect( reply, SIGNAL( finished() ), sender(), SLOT( onReqestFinished() ) );
    connect( reply, SIGNAL( downloadProgress( qint64, qint64 ) ), sender(), SLOT( onDownloadProgress( qint64, qint64 ) ) );
//  qDebug() << this->metaObject()->className() << __FUNCTION__;
}

QNetworkReply* Requester::post( const QNetworkRequest& request, const QByteArray& data ) { return manager_->post( request, data ); }
QNetworkReply* Requester::post( const QNetworkRequest& request, QHttpMultiPart *multiPart ) { return manager_->post( request, multiPart ); }

void Requester::getTorrentFile( QString url )
{
    qDebug() << this->metaObject()->className() << __FUNCTION__ << QThread::currentThread() << url;
    QUrl qurl( url );
    if ( !qurl.isValid() ) return;
    if ( torrentReply_ )
    {
        qDebug() << this->metaObject()->className() << __FUNCTION__ << "old reply found";
        QNetworkReply *reply = qobject_cast<QNetworkReply *>( torrentReply_ );
        if ( reply )
        {
            qDebug() << this->metaObject()->className() << __FUNCTION__ << "removing old reply" << reply;
            disconnect( reply, SIGNAL( finished() ), this, SLOT( onTorrentFileReqestFinished() ) );
            reply->abort();
            reply->deleteLater();
        }
    }
    torrentReply_ = manager_->get( QNetworkRequest( qurl ) );
    connect( torrentReply_, SIGNAL( finished() ), this, SLOT( onTorrentFileReqestFinished() ) );
}

void Requester::onSubtitleThumbReqestFinished()
{
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << QThread::currentThread();
    QString url, body;
    QNetworkReply *reply = dynamic_cast<QNetworkReply *>( sender() );
    if ( reply )
    {
        url = reply->url().toString();
        if ( !reply->error() ) body = reply->readAll();
        //  else qDebug() << this->metaObject()->className() << __FUNCTION__ << "url_request finished with error " << reply->error();
        reply->deleteLater();
    }
    emit haveSubtitleThumb( url, body );
}

void Requester::onTorrentFileReqestFinished()
{
    qDebug() << this->metaObject()->className() << __FUNCTION__ << QThread::currentThread();
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
    if ( !reply ) return;
    if ( reply == torrentReply_ )
    {
        if ( !reply->error() )
        {
            int statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
            switch ( statusCode )
            {
            case 301:
            case 302:
            case 307:
                qDebug() << this->metaObject()->className() << __FUNCTION__ << "Redirected: " << reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
                torrentReply_ = manager_->get( QNetworkRequest( reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl() ) );
                connect( torrentReply_, SIGNAL( finished() ), this, SLOT( onTorrentFileReqestFinished() ) );
                reply->deleteLater();
                return;

            case 200:
                emit haveTorrentFile( reply->readAll() );
                break;
            }
        }
        else
        {
            qDebug() << this->metaObject()->className() << __FUNCTION__ << "getTorrentFile finished with error " << reply->error();
            emit haveTorrentFile( "" );
        }
    }
    reply->deleteLater();
    torrentReply_ = 0;
}
