#include "TorrentIODevice.h"
#include "TorrentWrapper.h"
#include "defaults.h"
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
TorrentIODevice::TorrentIODevice( QObject *parent ) : QIODevice( parent )
{
}

TorrentIODevice::~TorrentIODevice()
{
    emit buffered();
    close();
}

void TorrentIODevice::setFileName( const QString& fileName )
{
    close();
    file.setFileName( fileName );
}

QString TorrentIODevice::getFileName()
{
    return file.fileName();
}

void TorrentIODevice::bindTorrentWrapper( TorrentWrapper *torrent )
{
    QObject::connect( torrent, &TorrentWrapper::torrentLoadCancelled, this, &TorrentIODevice::close );
    QObject::connect( this, &TorrentIODevice::checkDownloaded, torrent, &TorrentWrapper::onCheckDownloaded, Qt::BlockingQueuedConnection );
    QObject::connect( this, SIGNAL( seekRequest( quint64 ) ), torrent, SLOT( seek( quint64 ) ) );
}


bool TorrentIODevice::open() { return open( ReadOnly ); }
bool TorrentIODevice::open( OpenMode mode )
{
    if ( mode != ReadOnly )
    {
        return false;
    }
    const bool res = file.open( mode );
    if ( res ) QIODevice::open( mode );
    return res;
}

void TorrentIODevice::close()
{
    abortWait();
    QIODevice::close();
    file.close();
}

bool TorrentIODevice::isSequential() const { return false; }
bool TorrentIODevice::isWritable() const { return false; }

qint64 TorrentIODevice::readData( char *data, qint64 maxSize )
{
    if ( abort ) return 0;
    bool isDownloaded = false;
    emit checkDownloaded( isDownloaded, file.pos(), file.pos() + maxSize );

    if ( !isDownloaded )
    {
        emit bufferUnderrun();
        emit seekRequest( file.pos() );
        QMutexLocker ml( &mutex );

        while ( !isDownloaded && !abort )
        {

            QThread::msleep( TORRENTIO_BUFFERING_CHECK_INTERVAL_MS );
            if ( !isOpen() )
            {
                TORRENTIO_DEBUG << "Exited event loop and !isOpen";
                return 0;
            }
            emit checkDownloaded( isDownloaded, file.pos(), file.pos() + maxSize );
        }

        if ( abort )
        {
            abort = false;
            TORRENTIO_DEBUG << "aborted";
            return 0;
        }

        emit buffered();
    }

    const qint64 res = file.read( data, maxSize );
    return res;
}

qint64 TorrentIODevice::writeData( const char *data, qint64 maxSize )
{
    (void) data; (void) maxSize;
    return 0;
}

qint64 TorrentIODevice::pos() const
{
    return file.pos();
}

qint64 TorrentIODevice::size() const
{
    return file.size();
}
bool TorrentIODevice::seek( qint64 pos )
{
    return file.seek( pos );
}


void TorrentIODevice::abortWait()
{
    if ( mutex.tryLock( 0 ) ) mutex.unlock();
    else
    {
        abort = true;
    }
}
