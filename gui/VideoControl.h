#ifndef __VIDEOCONTROL_H_INCL__
    #define __VIDEOCONTROL_H_INCL__

    #ifdef Q_OS_MAC
        #define NO_ASPECT_RATIO     1
    #endif


    #include <QFrame>
    #include <QTimer>
    #include <QSlider>
    #include "commontypes.h"

class SubComboBox;
class VariantComboBox;
class Subtitler;
class QComboBox;
class TTrackComboBox;
class QPushButton;
class QDoubleSpinBox;


class VolumeSlider : public QSlider
{
    Q_OBJECT

public:
    VolumeSlider() : QSlider( Qt::Horizontal ) { }

    public slots:
    void provide() { emit valueChanged( value() ); }

protected:
    virtual void paintEvent( QPaintEvent *event );

};

class SeekControl;
class QSlider;
class GlyphButton;
class QPushButton;

class QLabel;

class VideoControl : public QFrame
{
    Q_OBJECT

public:
    VideoControl( QWidget *parent = 0 );
    ~VideoControl();
    QPushButton* fullscreenButton();
    GlyphButton* castButton() { return m_castBtn; }

    bool panelsVisible();
    void updateZoom();

    signals:
    void volumeSet( int volume );
    void volumeChanged( int );
    void newImage( QString resourcePng );
    void switchedtoCast();
    void switchedtoVLC();
    void mouseEnter();
    void mouseLeave();
    void timeOnRewind( int time );
    void subtitleDelay( quint64 delay );

    private slots:
    void showSubsMenu();
    void volumeUp();
    void volumeDown();

    public slots:
    void uiShow();
    void uiHide();

    void setVolume( int volume );
    void pauseToggleReq();
    void pauseToggled( bool paused );

    void clearSubtitles();
    void startSubtitles( TorrentInfo info );

    void updateDownloadStatus( DownloadInfo info );

    void reconnectMediaPlayer();
    void castSwitch( bool casting );
    void provideControlSettings();
    void updateLabel( const QString& newText );
    void updateAspectRatio( QString );

protected:
    virtual void leaveEvent( QEvent * );
    virtual void enterEvent( QEvent * );

    QFrame *m_hidableFrame;
    SeekControl *m_seek;
    VolumeSlider *m_vol;
    QLabel *m_statusLabel;
    GlyphButton *m_pauseBtn;
    GlyphButton *m_stopBtn;
    GlyphButton *m_muteBtn;
    GlyphButton *m_fullscrBtn;
    GlyphButton *m_castBtn;
    GlyphButton *m_subsBtn;
    GlyphButton *m_setsBtn;
    QDoubleSpinBox *m_subsDelay;
#ifndef NO_ASPECT_RATIO
    QAction *m_aspectAction;
#endif
    SubComboBox *m_subs;
    VariantComboBox *m_subsVars;
    Subtitler *m_subtitler;
    TTrackComboBox *m_tracks;
    QList<QMetaObject::Connection> connList;
    QPointer<QObject> currentPlayer;
    QPointer<QObject> localPlayer;
    QTimer subtitleDelayTimer;
    QPointer<QObject> cast;
};

#endif // __VIDEOCONTROL_H_INCL__
