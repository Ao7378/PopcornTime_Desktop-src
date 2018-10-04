#include "TorrentWrapper.h"
#include "defaults.h"
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>

#include "lib.h"
#include "SystemHelper.h"


// TODO: your implementation here

TorrentWrapper::TorrentWrapper( QObject *parent ) : QObject( parent ),
   m_thread( new DownloadNotifyThread( this ) ),
   m_netMan( new QNetworkAccessManager( this ) )
{
    QObject::connect( m_thread, SIGNAL( torrentProgressAvailable( int, int, int, int, QString ) ),
                      this, SIGNAL( torrentProgressAvailable( int, int, int, int, QString ) ) );
    QObject::connect( m_thread, SIGNAL( torrentDataAvailable( QString ) ), this, SIGNAL( torrentDataAvailable( QString ) ) );
    QObject::connect( m_thread, SIGNAL( downloadInfoChanged( DownloadInfo ) ),
                      this, SIGNAL( downloadInfoChanged( DownloadInfo ) ) );
}


TorrentWrapper::~TorrentWrapper()
{
    qDebug( "torrentDeInit start" );
    torrentDeInit();
    qDebug( "torrentDeInit end" );
}

bool TorrentWrapper::isDownloaded( quint64 start, quint64 end, int index )
{
    if ( index < 0 ) index = activeTorrent.index;
    return isRangeDownloaded( start, end, index );
}

void TorrentWrapper::onCheckDownloaded( bool& result, quint64 start, quint64 end, int index )
{
    if ( activeTorrent.isValid() ) result = isDownloaded( start, end, index );
}


void TorrentWrapper::seek( quint64 start ) { seek( start, -1 ); }
void TorrentWrapper::seek( quint64 start, int index )
{
    if ( !activeTorrent.isValid() ) return;
    if ( index < 0 ) index = activeTorrent.index;
    seekInFile( start, index );
}


void TorrentWrapper::init( QString downloadDir, bool cleanOnExit )
{
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << downloadDir.toUtf8().toHex();
    torrentInit( downloadDir.toUtf8().constData(), cleanOnExit ? 1 : 0 );
}

#ifdef USE_SOCKS5
void TorrentWrapper::setProxy( ProxySettings proxy )
{
    qDebug() << "TorrentWrapper: setting proxy";
    if ( proxy.isValid() ) torrentSetProxy( proxy.host.constData(), proxy.port, proxy.user.constData(), proxy.pass.constData() );
    else torrentSetProxy();
}
#endif

void TorrentWrapper::get( TorrentInfo info )
{
#ifdef _DEBUGGING
#endif
    torrentProgressAvailable( 0, 0, 0, 0, TK_GETTING_METADATA );
    auto dl = new TorrentDownloadThread( this, info );
    QObject::connect( dl, SIGNAL( torrentDownloaded( TorrentInfo ) ),
                      this, SLOT( onMetadataDownloaded( TorrentInfo ) ) );
    QObject::connect( dl, SIGNAL( getMetadataError( TorrentInfo ) ),
                      this, SLOT( onTorrentMetadataError( TorrentInfo ) ) );

    activeTorrentPreloaded = false;
}

void TorrentWrapper::cancel( int index )
{
    TorrentInfo currentTorrent = activeTorrent;
    activeTorrent = TorrentInfo();
    //  if ( !index ) qDebug() << "cancel Torrent: no index";
    //  else qDebug() << "cancel Torrent" << index;
    if ( index < 0 ) index = currentTorrent.index;

    if ( !activeTorrentPreloaded ) emit preloadCancelled( currentTorrent );
    else emit torrentLoadCancelled( currentTorrent );

    m_thread->stopOnTorrent();
    activeTorrentPreloaded = false;

    if ( activeTorrentIds.contains( index ) ) activeTorrentIds.removeOne( index );
    if ( activeTorrentIds.isEmpty() ) SystemHelper::restoreSleep();
    torrentStopDelete( index );
}


void TorrentWrapper::onMetadataDownloaded( TorrentInfo tInfo )
{
    activeTorrent = tInfo;
//  qDebug() << "torrentDownloaded finish" << activeTorrent.index << activeTorrent.subtitleUrlJson;
    if ( activeTorrent.index < 0 )
    {
        qDebug() << __FUNCTION__ << "getMetadataError";
        emit torrentProgressAvailable( 0, -1, 0, 0, TK_ERROR_GETTING_METADATA );
        emit getMetadataError( tInfo );
        return;
    }

    m_thread->startOnTorrent( activeTorrent.index );

    emit leechingStarted( tInfo.index );
    emit newMovieName( tInfo.title );

    emit startSubs( tInfo );

    if ( activeTorrentIds.isEmpty() ) SystemHelper::disableSleep();
    activeTorrentIds << activeTorrent.index;
}

void TorrentWrapper::onTorrentMetadataError( TorrentInfo tInfo )
{
    qDebug() << __FUNCTION__ << "getMetadataError" + QString::number( tInfo.index );
    emit torrentProgressAvailable( 0, -1, 0, 0, QString( TK_ERROR_GETTING_METADATA ) + "(" + QString::number( tInfo.index ) + ")" );
    emit getMetadataError( tInfo );
}

