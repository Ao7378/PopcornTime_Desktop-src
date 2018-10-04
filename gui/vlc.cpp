#include <QApplication>
#include <QObject>
#include <QDebug>

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QStringList>

#include <VLCQtCore/Instance.h>
#include <VLCQtCore/Common.h>
#include <VLCQtWidgets/WidgetVideo.h>

#include "vlc.h"
#include "VMediaPlayer.h"

VlcInstance *VLC::_instance = 0;
VMediaPlayer *VLC::_player = 0;
QString VLC::defaultFile;
VLC *VLC::object;


VLC::VLC( QObject *parent ) : QObject( parent )
{
    object = this;
    resetInstance( QStringList() );
//  _instance = new VlcInstance( VlcCommon::args(), parent );
//  _player = new VlcMediaPlayer( _instance );

#ifdef _DEBUGGING
    {
        QStringList nameFilter;
        nameFilter << "*.avi" << "*.mkv" << "*.mp4";
        QDir directory( QApplication::applicationDirPath() );
        QStringList list = directory.entryList( nameFilter );
        if ( list.size() )
        {
            defaultFile = QDir::cleanPath( QApplication::applicationDirPath() + QDir::separator() + list.first() );
        }
    }
    if ( !QFile::exists( defaultFile ) )
    {
        QStringList nameFilter;
        nameFilter << "*.avi" << "*.mkv" << "*.mp4";
#if defined WIN32 || defined _WIN32
        QDir directory( "C:\\" );
#else
        QDir directory( QDir::homePath() );
#endif
        QStringList list = directory.entryList( nameFilter );
        if ( list.size() )
        {
            defaultFile = QDir::cleanPath( directory.absolutePath() + QDir::separator() + list.first() );
        }
    }
    qDebug() << "File selected" << defaultFile;

    if ( !QFile::exists( defaultFile ) ) defaultFile = QFileDialog::getOpenFileName( 0, "Open file", QDir::homePath(), "Multimedia files(*)" );

    if ( !QFileInfo::exists( defaultFile ) )
    {
        defaultFile.clear();
        return;
    }
#endif
}

bool VLC::hasDebugFile() { return QFile::exists( defaultFile ); }

void VLC::resetInstance( QStringList params )
{
    params += "--no-video-on-top";
    params += "--avcodec-fast";
#ifdef _DEBUGGING
#else
    params += "--quiet";
#endif

//  qDebug() << "resetInstance" << VlcCommon::args() + params;
    if ( _player ) delete _player;
    if ( _instance ) delete _instance;
    _instance = new VlcInstance( VlcCommon::args() + params, object );
    _player = new VMediaPlayer( _instance );
    emit object->mediaPlayerReplaced();
}

VLC::~VLC()
{
    delete _player;
    _player = 0;

    delete _instance;
    _instance = 0;
}

