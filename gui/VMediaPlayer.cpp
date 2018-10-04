#include "VMediaPlayer.h"
#include "vlc.h"
#include <VLCQtCore/Audio.h>
#include <VLCQtCore/Media.h>
#include <VLCQtCore/Video.h>

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QTime>
#include <QThread>
#include <QApplication>

VMediaPlayer::VMediaPlayer( VlcInstance *instance ) : VlcMediaPlayer( instance ), _media( 0 ), startTime( 0 )
{
    connect( this, SIGNAL( stateChanged() ),
             this, SLOT( onStateChange() ) );
//  connect( this, SIGNAL( stopped() ),
//           this, SLOT( closeFile() ) );
    connect( this, SIGNAL( playing() ), this, SLOT( onPlaying() ) );
    connect( this, SIGNAL( stopped() ), this, SLOT( onStopped() ) );
}

VMediaPlayer::~VMediaPlayer()
{
    closeFile();
}

void VMediaPlayer::closeFile()
{
    if ( _media ) delete _media;
    _media = 0;
}

void VMediaPlayer::openFile( QString fileName, VlcVideoDelegate& widget )
{
    QFileInfo info( fileName );
    if ( !info.exists() )
    {
        qDebug() << "openMedia: file error";
        return;
    }
    fileName = info.canonicalFilePath();
    closeFile();

    setVideoWidget( &widget );
    _media = new VlcMedia( fileName, true, VLC::_instance );

    _fileName = fileName;

    this->open( _media );

    QTime waitTime = QTime::currentTime().addSecs( 2 );
    int sleep = 10;
    while ( QTime::currentTime() < waitTime )
    {
        qApp->processEvents();
        if ( state() != Vlc::Idle ) break;
        QThread::msleep( sleep );
        sleep *= 1.5;
    }
    video()->setAspectRatio( Vlc::R_16_9 );
    provideAspectRatio();
    provideAudioTrackList();
    // Ended or Error check?
}


const QStringList& VMediaPlayer::getSupportedSubsList()
{
    static const QStringList data = QStringList() << "aqt" << "cvd" << "dks" << "jss" << "sub" << "ttxt" <<
       "mpl" << "pjs" << "psb" << "rt" << "smi" << "ssf" << "srt" << "ssa" << "svcd" << "usf" << "idx" << "txt";
    return data;
}

void VMediaPlayer::setSubtitleFileV( QString fileName )
{
    static QStringList supportedList = getSupportedSubsList();
    foreach( QString ext, supportedList )
    {
        if ( fileName.toLower().endsWith( ext ) )
        {
            video()->setSubtitleFile( QDir::toNativeSeparators( fileName ) );
            return;
        }
    }
    video()->setSubtitle( -1 );
}

void VMediaPlayer::setSubtitleDelay( quint64 delay ) { video()->setSubtitleDelay( delay ); }

void VMediaPlayer::setFontSize( int fontSize )
{
    switch ( fontSize )
    {
    case -2: fontSize = 20; break;
    case -1: fontSize = 18; break;
    default:
    case 0: fontSize = 16; break;
    case 1: fontSize = 11; break;
    case 2: fontSize = 7; break;
    }
    VLC::resetInstance( QStringList( QString( "--freetype-rel-fontsize=%1" ).arg( fontSize ) ) );
}

void VMediaPlayer::setMute( bool mute ) { if ( audio()->getMute() != mute ) audio()->toggleMute(); }
void VMediaPlayer::setTime( int time ) { VlcMediaPlayer::setTime( time ); }
void VMediaPlayer::setStartTime( int time ) { startTime = time; qDebug() << "VLC start time =" << time; }
void VMediaPlayer::setVolume( int volume ) { audio()->setVolume( volume ); }

void VMediaPlayer::setPause( bool setPause )
{
    switch ( state() )
    {
    case Vlc::Playing:
        if ( setPause ) this->pause();
        break;
    case Vlc::Paused:
        if ( !setPause ) this->resume();
        break;
    default:
        break;
    }

}

void VMediaPlayer::nextAspectRatio()
{
    switch ( video()->aspectRatio() )
    {
    case Vlc::Original: video()->setAspectRatio( Vlc::R_16_9 ); break;
    case Vlc::R_16_9: video()->setAspectRatio( Vlc::R_4_3 ); break;
    case Vlc::R_4_3: video()->setAspectRatio( Vlc::R_185_100 ); break;
    case Vlc::R_185_100: video()->setAspectRatio( Vlc::R_235_100 ); break;
    default:
    case Vlc::R_235_100: video()->setAspectRatio( Vlc::Original ); break;
    }
    provideAspectRatio();
}

void VMediaPlayer::provideAspectRatio()
{
    switch ( video()->aspectRatio() )
    {
    case Vlc::Original: emit newAspectRatio( "Original" ); break;
    case Vlc::R_16_9: emit newAspectRatio( "16:9" ); break;
    case Vlc::R_4_3: emit newAspectRatio( "4:3" ); break;
    case Vlc::R_185_100: emit newAspectRatio( "185" ); break;
    case Vlc::R_235_100: emit newAspectRatio( "235" ); break;
    default: emit newAspectRatio( "Unknown" ); break;
    }
}

void VMediaPlayer::setAudioTrack( int track )
{
    if ( audio()->trackIds().contains( track ) ) audio()->setTrack( track );
    else qDebug() << this->metaObject()->className() << __FUNCTION__ << "No such track" << track << "in" << audio()->trackIds();
}

void VMediaPlayer::provideAudioTrackList()
{
    if ( !audio() || !audio()->trackCount() )
    {
        QTimer::singleShot( 500, this, SLOT( provideAudioTrackList() ) );
        qDebug() << this->metaObject()->className() << __FUNCTION__ << "delay!";
        return;
    }
    QStringList strings = audio()->trackDescription();
    QList<int> ids = audio()->trackIds();
    TIndexedString list;
    while ( !ids.isEmpty() )
    {
        QString name = "Track";
        if ( !strings.isEmpty() ) name = strings.takeFirst();
        const int tid = ids.takeFirst();
        if ( tid == -1 ) continue;
        list.insert( name, tid );
    }
    audio()->setTrack( list.last() ); 

    emit newAudioTrackList( list, audio()->track() );
}


void VMediaPlayer::onStateChange()
{
    QString text;
    switch ( state() )
    {
    case Vlc::Buffering: text = "Buffering"; break;
    case Vlc::Playing: text = "Playing"; break;
    case Vlc::Paused: text = "Paused"; break;
    case Vlc::Idle: text = "Idle"; break;
    case Vlc::Opening: text = "Opening"; break;
    case Vlc::Stopped: text = "Stopped"; break;
    case Vlc::Ended: text = "Ended"; break;
    case Vlc::Error: text = "Error"; break;
    }
    if ( text.size() ) emit stateText( text );
//  qDebug() << "VLC State" << text;
    emit stateImage( "" );
}

void VMediaPlayer::onPlaying() { if ( startTime ) VlcMediaPlayer::setTime( startTime ); startTime = 0; }
void VMediaPlayer::onStopped() { startTime = 0; }

