#include "VideoPlayer.h"
#include "defaults.h"
#include <QApplication>
#include <QObject>
#include <QBuffer>
#include <QThread>

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QStringList>
#include <QTimer>

#include <QtAV/QtAV>

#ifndef NO_TORRENT_IO_WRAPPER
    #include "TorrentIODevice.h"
#endif

QThread VideoPlayer::thread;
QSharedPointer<VideoPlayer> VideoPlayer::object;
QtAV::VideoRenderer *VideoPlayer::videoWidget = 0;

VideoPlayer::VideoPlayer( QObject *parent ) : QObject( parent )
#ifndef NO_TORRENT_IO_WRAPPER
   , torrentIODev( new TorrentIODevice( this ))
#endif
{
}

VideoPlayer::~VideoPlayer()
{
    thread.quit();
    thread.wait( 1000 );
    thread.terminate();
}

QSharedPointer<VideoPlayer> VideoPlayer::PlayerObject()
{
    if ( !object )
    {
        object.reset( new VideoPlayer());
        object->moveToThread( &thread );
        QTimer::singleShot( 0, object.data(), SLOT( resetPlayer() ) );
        thread.start();
    }
    return object;
}

void VideoPlayer::setFontSize( int fontSize )
{
    subsFontPointSize = (fontSize + 2) * 4 + 18;
    if ( _subtitleFilter )
    {
        QFont font = _subtitleFilter->font();
        font.setPointSize( subsFontPointSize );
        _subtitleFilter->setFont( font );
    }
}

void VideoPlayer::stop()
{
    closeFile();
    emit stopped();
}

void VideoPlayer::setSubtitleFileV( QString fileName )
{
    if ( currentSubtitleFile == fileName ) return;
    qDebug() << this->metaObject()->className() << __FUNCTION__ << QDir::toNativeSeparators( fileName );
    currentSubtitleFile = fileName;
    _player->uninstallFilter( _subtitleFilter.data() );
    _subtitleFilter.reset( new QtAV::SubtitleFilter( _player.data() ));
    _player->installFilter( _subtitleFilter.data() );
    if ( QFileInfo::exists( fileName ) )
    {
        _subtitleFilter->setFile( fileName );
    }
}

void VideoPlayer::setSubtitleDelay( quint64 delay )
{
    subsDelay = delay / 1000000;
    if ( _subtitleFilter ) _subtitleFilter->setDelay( subsDelay );
}

void VideoPlayer::setMute( bool mute ) { _player->audio()->setMute( mute ); }
void VideoPlayer::setTime( int time )
{
    _player->setPosition( time );
}

void VideoPlayer::setVolume( int volume ) { _player->audio()->setVolume( qreal( volume ) / 100 ); }

void VideoPlayer::setPause( bool pause )
{
    if ( _player->state() == QtAV::AVPlayer::State::StoppedState ) open();
    else _player->pause( pause );
}

void VideoPlayer::pause() { if ( _player->state() != QtAV::AVPlayer::State::StoppedState ) _player->pause( true ); }
void VideoPlayer::resume() { if ( _player->state() != QtAV::AVPlayer::State::StoppedState ) _player->pause( false ); }

void VideoPlayer::setAudioTrack( int track ) { _player->setAudioStream( track ); }
void VideoPlayer::provideAudioTrackList() { onInternalAudioTracksChanged( _player->internalAudioTracks() ); }


void VideoPlayer::onDurationChanged( qint64 time ) { emit lengthChanged( int( time ) ); }
void VideoPlayer::onPositionChanged( qint64 time ) { emit timeChanged( int( time ) ); }

void VideoPlayer::onStateChange()
{
    QString status;
    if ( oldMediaStatus != _player->mediaStatus() )
    {
        switch ( _player->mediaStatus() )
        {
        case QtAV::MediaStatus::NoMedia: status = tr( "No media" ); break;
        case QtAV::MediaStatus::InvalidMedia: status = tr( "Invalid meida" ); break;
        case QtAV::MediaStatus::BufferingMedia: status = tr( "Buffering..." ); break;
        case QtAV::MediaStatus::BufferedMedia: status = tr( "Buffered" ); break;
        case QtAV::MediaStatus::LoadingMedia: status = tr( "Loading..." ); emit opening(); break;
        case QtAV::MediaStatus::LoadedMedia: status = tr( "Loaded" ); break;
        case QtAV::MediaStatus::StalledMedia: status = tr( "Stalled" ); break;
        default: status = QString(); break;
        }
        oldMediaStatus = _player->mediaStatus();
    }

    if ( oldState != _player->state() )
    {
        switch ( _player->state() )
        {
        case QtAV::AVPlayer::State::StoppedState:
            status += tr( " Stopped" );
            resetPlayer();
            break;
        case QtAV::AVPlayer::State::PlayingState:
            status += tr( " Playing" );
            if ( oldState == QtAV::AVPlayer::State::PausedState ) emit playing();
            break;
        case QtAV::AVPlayer::State::PausedState:
            status += tr( " Paused" );
            emit paused();
            break;
        }
        oldState = _player->state();
    }

    emit stateImage( "" );
}

void VideoPlayer::onInternalAudioTracksChanged( const QVariantList& tracks )
{

    TIndexedString list;
    foreach( auto track, tracks )
    {
        const auto map = track.toMap();

        const int tid = map.value( "id" ).toInt();
        if ( tid == -1 ) continue;

        QString name = map.value( "language" ).toString();
        if ( name.isEmpty() || name == "und" ) name = "Track " + QString::number( tid + 1 );
        list.insert( name, tid );
    }
    if ( list.size() ) _player->setAudioStream( list.last() );

    emit newAudioTrackList( list, _player->currentAudioStream() );
}

void VideoPlayer::onInternalSubtitleTracksChanged( const QVariantList& tracks )
{
    qDebug() << this->metaObject()->className() << __FUNCTION__ << tracks;
}



void VideoPlayer::setVideoWidget( QtAV::VideoRenderer *widget )
{
    videoWidget = widget;
    if ( videoWidget && _player ) _player->setRenderer( videoWidget );
}

void VideoPlayer::setTorrentWrapper( TorrentWrapper *wrapper )
{
    torrentWrapper = wrapper;
#ifndef NO_TORRENT_IO_WRAPPER
    if ( wrapper && _player ) torrentIODev->bindTorrentWrapper( torrentWrapper.data() );
#endif
}

const QStringList& VideoPlayer::getSupportedSubsList()
{
    static const QStringList data = QStringList() << "aqt" << "cvd" << "dks" << "jss" << "sub" << "ttxt" <<
       "mpl" << "pjs" << "psb" << "rt" << "smi" << "ssf" << "srt" << "ssa" << "svcd" << "usf" << "idx" << "txt";
    return data;
}

void VideoPlayer::openFile( QString fileName )
{
    if ( !fileName.isEmpty() )
    {
        videoFileName = fileName;
#ifndef NO_TORRENT_IO_WRAPPER
        if ( torrentIODev->isOpen() ) torrentIODev->close();
        torrentIODev->setFileName( videoFileName );
#endif
    }
    open();
}

void VideoPlayer::open()
{
#ifndef NO_TORRENT_IO_WRAPPER
    if ( !torrentIODev )
    {
        torrentIODev.reset( new TorrentIODevice());
        if ( torrentWrapper ) torrentIODev->bindTorrentWrapper( torrentWrapper );
        return;
    }
    if ( torrentIODev->isOpen() ) torrentIODev->close();
    if ( torrentIODev->getFileName().isEmpty() ) torrentIODev->setFileName( videoFileName );
    if ( !QFileInfo::exists( torrentIODev->getFileName() ) )
    {
        return;
    }
    _player->setIODevice( torrentIODev.data() );
    if ( torrentIODev->isOpen() || torrentIODev->open() )
    {
        QTimer::singleShot( 500, _player.data(), SLOT( play() ) ); // _player ->play();
    }
#else
    _player->play( videoFileName );
#endif
}

void VideoPlayer::closeFile()
{
    if ( !_player ) return;
    if ( _subtitleFilter ) _subtitleFilter.reset();
#ifndef NO_TORRENT_IO_WRAPPER
    torrentIODev->close();
#endif
    _player->stop();
}


void VideoPlayer::resetPlayer()
{
    _player.reset( new QtAV::AVPlayer());
#ifndef NO_TORRENT_IO_WRAPPER
    torrentIODev.reset( new TorrentIODevice() );
    if ( torrentWrapper ) torrentIODev->bindTorrentWrapper( torrentWrapper );
#endif
    oldState = QtAV::AVPlayer::StoppedState;

    QObject::connect( _player.data(), SIGNAL( started() ), this, SIGNAL( playing() ) );
    QObject::connect( _player.data(), SIGNAL( stopped() ), this, SIGNAL( stopped() ) );
    QObject::connect( _player.data(), SIGNAL( durationChanged( qint64 ) ), this, SLOT( onDurationChanged( qint64 ) ) );
    QObject::connect( _player.data(), SIGNAL( positionChanged( qint64 ) ), this, SLOT( onPositionChanged( qint64 ) ) );
    QObject::connect( _player.data(), SIGNAL( stateChanged( QtAV::AVPlayer::State ) ), this, SLOT( onStateChange() ) );
    QObject::connect( _player.data(), SIGNAL( stateChanged( QtAV::AVPlayer::State ) ), this, SIGNAL( stateChanged() ) );
    QObject::connect( _player.data(), SIGNAL( mediaStatusChanged( QtAV::MediaStatus ) ), this, SLOT( onStateChange() ) );
    QObject::connect( _player.data(), SIGNAL( internalAudioTracksChanged( const QVariantList& ) ), this, SLOT( onInternalAudioTracksChanged( const QVariantList& ) ) );
#ifndef NO_TORRENT_IO_WRAPPER
    QObject::connect( _player.data(), SIGNAL( stopped() ), torrentIODev.data(), SLOT( abortWait() ) );
#endif


    if ( videoWidget && _player ) _player->setRenderer( videoWidget );
}

