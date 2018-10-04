#ifndef __MEDIAPLAYER_H_INCL__
    #define __MEDIAPLAYER_H_INCL__

    #include <VLCQtCore/MediaPlayer.h>
    #include <QString>
    #include <commontypes.h>

class VMediaPlayer : public VlcMediaPlayer
{
    Q_OBJECT

public:
    VMediaPlayer( VlcInstance *instance );
    ~VMediaPlayer();
    QString getFileName() { return _fileName; }
    static const QStringList& getSupportedSubsList();

    public slots:
    void closeFile();
    void openFile( QString fileName, VlcVideoDelegate& widget );

    void setSubtitleFileV( QString fileName );
    void setSubtitleDelay( quint64 delay );
    void setFontSize( int fontSize );
    void setMute( bool mute );
    void setTime( int time );
    void setStartTime( int );
    void setVolume( int volume );
    void setPause( bool );
    void nextAspectRatio();
    void provideAspectRatio();
    void setAudioTrack( int ); 
    void provideAudioTrackList();

    private slots:
    void onStateChange();
    void onPlaying();
    void onStopped();

    signals:
    void stateText( const QString& text );
    void stateImage( QString resourcePng );
    void newAspectRatio( QString );
    void newAudioTrackList( TIndexedString, int active );

private:
    VlcMedia *_media;
    QString _fileName;
    int startTime;
};

#endif // __MEDIAPLAYER_H_INCL__
