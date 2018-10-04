#ifndef __VPNINTEGRATION_H_INCL__
    #define __VPNINTEGRATION_H_INCL__

    #include <QString>
    #include <QBasicTimer>
    #include <QLocalServer>
    #include <QPointer>
    #include <QList>

class QLocalSocket;

class TVpnIntegration
{
private:
    friend class TVpnIntegrationClient;
    static bool isVpnInstalled();
    static QString getVpnExecutable();

public:
    // VPN side
    static void setVpnInstalled( bool installed );

};

class TVpnIntegrationServer : public QObject
{
    Q_OBJECT

public:
    TVpnIntegrationServer( QObject *parent = 0 );
    ~TVpnIntegrationServer();
    bool listen();
    bool isConnected();

    public slots:
    void onNotifyConnectionStatus( bool nowConnected );
    void onAuthFailed(); 

    private slots:
    void onServerConnect();
    void onSocketReadyRead();

    signals:
    void connectCommand();
    void disconnectCommand();

protected:
    void sendStatus();
    void sendStatus( QLocalSocket *socket );
    void sendMessage( QLocalSocket *socket, const QByteArray& data );
    void sendMessage( const QByteArray& data );
    bool connected = false;
    QPointer<QLocalServer> server;
    QList<QPointer<QLocalSocket>> socketList;
};



#endif // __VPNINTEGRATION_H_INCL__
