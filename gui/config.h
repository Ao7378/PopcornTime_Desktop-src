#ifndef __CONFIG_H_INCL__
    #define __CONFIG_H_INCL__

    #include <QString>
    #include <QByteArray>
    #include <QList>
    #include <QSettings>
    #include <QThread>
    #include "commontypes.h"

class AppConfig;

class ProxyCheckerThread : public QThread
{
    Q_OBJECT

public:
    ProxyCheckerThread( AppConfig *AppConfig, TProxyList& proxyList );
    ~ProxyCheckerThread();

    public slots:
    void abort();
    
    signals:
    void proxyResult( int workingProxy );

protected:
    void run();

private:
    TProxyList proxies;
};

class AppConfig : public QSettings
{
    Q_OBJECT

public:
    // Constructor
    AppConfig();
    ~AppConfig();
    void importJson( QString cfg, bool setSettings );

    public slots:
    void connectProxy();
    void disconnectProxy();

    void haveProxyResult( int workingProxy );

    signals:
    void newProxySettings( ProxySettings );

private:
    bool processJson( QByteArray json );
    QList<ProxySettings> proxies;
    int proxyIndex;
    ProxyCheckerThread *thread;
};

#endif // __CONFIG_H_INCL__
