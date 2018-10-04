#ifndef __HOSTAPP_H_INCL__
    #define __HOSTAPP_H_INCL__

    #include <QObject>
    #include <QSettings>
    #include <QJsonObject>
    #include <QBasicTimer>
    #include <QPointer>
    #include <QRect>

    #include "commontypes.h"
    #include "config.h"
    #include "TorHelpers.h"

class QGraphicsWebView;
class QWebFrame;
class QWidget;
class TorrentWrapper;
class TVpnIntegrationClient;

class HostApp : public QObject
{
    Q_OBJECT

public:
    HostApp( QObject *jsObject, QWidget *parent );
    ~HostApp();
    TorrentWrapper* torrentEngine();

    int getVolume();

    public slots:
    void setVolume( int volume );
    void setAnimations( bool );

    // JS callback slots
    void onJsMessage( const QString& name, const QVariantList& args );

    void sendWinAction( QString str );
    void getTorrent( QString session );
    void getTorrent( QJsonValue jVal );
    QString getTorrent( QString url, QString fileName, QString subtitles_url );
    void url_request( QString url );
    void cancelTorrent( QString ATorrentId );

    bool vpn_isConnected();
    void vpn_connect();
    void vpn_disconnect();

    void setConfig( QString config );
    void setConfig();
    void setConfigVars( QString vars );
    QString getTempPath();
    QString setTempPath();
    QString setTempPath( QString callback );
    void OpenTempDir();
    void setCleanOnExit( QString isClear );
    int getUserFontSize() { return fontSize; }
    void setUserFontSize( QString size );

    void openBrowser( QString url );

    void closePlayer();

    void settingsLoad( QString callback );
    void settingsSave( QString data );
    void checkVpnInstalled();
    private slots:
    void attachObject();
    void cancelTorrent( int index );
    void cancelTorrent();

    void onSubThumbReply( QString url, QString data );
    void PopupUrl( QString url );
    // JS control slots
    void updateTorrentProgress( int progressPeers, int dlSpeed, int seeds, int peers, QString msg );
    void vpnConnected();
    void vpnDisconnected();
    void hideLoading();
    void setTorrentIndex( int index );
    void vpnAuthFailed( bool email );

    signals:
    void commandedMinimize();
    void toggledMaximize();
    void commandedClose();
    void commandedClosePlayer();
    void fontSizeChanged( int relativeSize );
    void subThumbRequest( QString url );
    void initCompleted();

protected:
    virtual void timerEvent( QTimerEvent * );

private:
    int slotIndex( QString name );
    bool executeJs( QString script, QObject *javaScriptObject = 0 );

    QSharedPointer<TorrentWrapper> torrent;
    QThread torrentThread;
    QObject *jsObj;
    QString downloadDir;
    bool cleanOnExit;
    int fontSize;
    QBasicTimer vpnCheckTimer;
    TVpnIntegrationClient *vpn;
};


#endif // __HOSTAPP_H_INCL__
