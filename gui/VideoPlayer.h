#ifndef __VIDEO_PLAYER_H_INCL__
    #define __VIDEO_PLAYER_H_INCL__

    #include <QString>
    #include <QObject>
    #include <QSharedPointer>
    #include <QPointer>
    #include <QTimer>
    #include "commontypes.h"

    #include <QtAV/VideoRenderer.h>
    #include <QtAV/AVPlayer.h>
    #include <QtAV/SubtitleFilter.h>
    #include "TorrentWrapper.h"

class TorrentIODevice;


class VideoPlayer : public QObject
{
    Q_OBJECT

    VideoPlayer( QObject *parent = 0 );

public:
    ~VideoPlayer();

    static QSharedPointer<VideoPlayer> PlayerObject();

    void setVideoWidget( QtAV::VideoRenderer *widget );
    void setTorrentWrapper( TorrentWrapper *wrapper );
    static const QStringList& getSupportedSubsList();
#ifndef NO_TORRENT_IO_WRAPPER
    TorrentIODevice* getTorrentDevice() { return torrentIODev.data(); }
#endif

    public slots:
    void openFile( QString fileName );
    void open();
    void closeFile();

    void setFontSize( int fontSize );
    void stop();
    void setSubtitleFileV( QString fileName );
    void setSubtitleDelay( quint64 delay );
    void setMute( bool mute );
    void setTime( int time );
    void setVolume( int volume );
    void setPause( bool = true );
    void pause();
    void resume();
    void setAudioTrack( int );
    void provideAudioTrackList();

    private slots:
    void onDurationChanged( qint64 );
    void onPositionChanged( qint64 );

    void onStateChange();
    void onInternalAudioTracksChanged( const QVariantList& tracks );
    void onInternalSubtitleTracksChanged( const QVariantList& tracks );

    void resetPlayer();

    signals:
    void opening();
    void playing();
    void stopped();
    void paused();
    void end();
    void lengthChanged( int );
    void timeChanged( int );
    void stateText( const QString& text );
    void stateImage( QString resourcePng );
    void stateChanged();
    void newAspectRatio( QString );
    void newAudioTrackList( TIndexedString, int active );

private:
    static QThread thread;
    static QSharedPointer<VideoPlayer> object;
    static QtAV::VideoRenderer *videoWidget;

#ifndef NO_TORRENT_IO_WRAPPER
    QSharedPointer<TorrentIODevice> torrentIODev;
#endif
    QSharedPointer<QtAV::AVPlayer> _player;
    QSharedPointer<QtAV::SubtitleFilter> _subtitleFilter;
    QPointer<TorrentWrapper> torrentWrapper;
    QString videoFileName;
    QString currentSubtitleFile;
    int subsFontPointSize = 26;
    qreal subsDelay = 0;
    QtAV::AVPlayer::State oldState = QtAV::AVPlayer::StoppedState;
    QtAV::MediaStatus oldMediaStatus = QtAV::MediaStatus::NoMedia;
};

#endif // __VIDEO_PLAYER_H_INCL__
