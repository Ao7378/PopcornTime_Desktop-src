#ifndef __CHROMECAST_H_INCL__
    #define __CHROMECAST_H_INCL__

    #include <QObject>
    #include <QByteArray>
    #include <QProcess>
    #include <QList>
    #include <QStringListModel>
    #include <QListWidget>
    #include <QHostAddress>
    #include <QUrl>
    #include <QFrame>
    #include <QNetworkAccessManager>
    #include <QNetworkReply>
    #include <QBasicTimer>
    #include <QTimer>


class ListWidget : public QListWidget
{
    Q_OBJECT

public:
    QSize sizeHint() const
    {
        QSize sz;
        sz.setHeight( QListWidget::sizeHint().height() );
        sz.setWidth( sizeHintForColumn( 0 ) );
        return sz;//contentsSize();
    }

};

class Frame : public QFrame
{
    Q_OBJECT

public:
    explicit Frame( QWidget *parent = 0 ) : QFrame( parent, Qt::Popup ) { setFocusPolicy( Qt::ClickFocus ); hide(); }

protected:
    void focusOutEvent( QFocusEvent *event ) { (void) event; close(); }
};

enum CAST_DEVICETYPE
{
    CAST_DEVICETYPE_INVALID = 0,
    CAST_DEVICETYPE_CHROMECAST,
    CAST_DEVICETYPE_AIRPLAY,
    CAST_DEVICETYPE_DLNA,
    CAST_DEVICETYPE_LOCAL
};

const char *const CAST_DEVICETYPE_STRING[] =
{
    "",
    "googlecast",
    "airplay",
    "dlna",
    ""
};

const char *const CAST_DEVICETYPE_USER_STRING[] =
{
    "",
    "Google Cast",
    "AirPlay",
    "DLNA",
    ""
};

struct CastDevice
{
public:
    CastDevice();
    CastDevice( const QJsonObject& obj );
    bool operator==( const CastDevice& src ) { return src.type == type && src.name == name && src.ip == ip; }
    bool isValid() const { return type != CAST_DEVICETYPE_INVALID && name.size() && !ip.isNull() && url.size(); }
    bool isChromeCast() const { return type == CAST_DEVICETYPE_CHROMECAST; }
    CAST_DEVICETYPE type;

public:
    QByteArray name;
    QHostAddress ip;
    QByteArray url;
};


class QPushButton;
class GlyphButton;
class QMovie;
class QListWidgetItem;
class NjsProcess;

class ChromeCast : public QObject
{
    Q_OBJECT

public:
    enum State
    {
           Idle = 0,
           Opening,
           Buffering,
           Playing,
           Paused,
    };

    ChromeCast( QWidget *parent = 0, GlyphButton *castBtn = 0 );
    void setButton( GlyphButton *btn );
    State state() { return m_state; }
    static const QStringList& getSupportedSubsList();

    public slots:
    void buttonClicked();
    void stop();
    void pause();
    void setTime( int );
    void setStartTime( int );
    void setMute( bool );
    void setPause( bool );
    void setVolume( int );
    void setSubtitleFileV( QString fileName );
    void setFontSize( int );
    void setMediaFile( QString filename ) { mediaFile = filename; }

    private slots:
    void frameChange( int frame );
    void parseNjsData( QByteArray key, QByteArray value );
    void discoveryStopped( int exitCode, QProcess::ExitStatus exitStatus );
    void serverStopped( int exitCode, QProcess::ExitStatus exitStatus );
    void onListItemClicked( QListWidgetItem *item );
    void onReqestFinish();
    void onVolumeTimer();

    signals:
    void lengthChanged( int );
    void timeChanged( int );
    void stopped();
    void opening();
    void playing();
    void paused();
    void deviceSelected();

    void stateText( const QString& text );
    void stateImage( QString resourcePng );
    void castingSwitched( bool casting );
    void stateChanged();

protected:
    virtual void timerEvent( QTimerEvent * );

private:
    void parseDiscovery( QByteArray data );
    bool addDevice( const CastDevice& device );
    void parseMStat( QByteArray data );
    void startDiscovery( bool crashed );
    void handleCastingSwitch( bool switchedToEnable );
    QNetworkReply* issueCommand( QByteArray command, QByteArray args = "" );
    void setState( State newState );
    bool isPlaying() { return m_state == Playing || m_state == Buffering; }

    QNetworkAccessManager netMan;
    QTimer volumeTimer;
    QTimer discoveryRestartTimer;
    int lastVolume;
    NjsProcess *discoveryProc;
//  int discoveryRestartCounter;
    NjsProcess *castProc;
    Frame *popup;
    ListWidget *devicesView;
    GlyphButton *m_castBtn;
    QMovie *btnMovie;
    QList<CastDevice> devs;
    QStringListModel model;
    QUrl serverUrl;
    CastDevice currentDevice;
    bool isCasting;
    float totalLength;
    QBasicTimer stateRefreshtimer;
    State m_state;
    QString subtitleFile;
    QString mediaFile;
    int fontSize;
    volatile int startTime;
    int currentTime;
};

#endif // __CHROMECAST_H_INCL__
