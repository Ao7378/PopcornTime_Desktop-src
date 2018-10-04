#include "config.h"
#include "defaults.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QEventLoop>



ProxyCheckerThread::ProxyCheckerThread( AppConfig *AppConfig, TProxyList& proxyList ) :
   QThread( AppConfig ), proxies( proxyList )
{
    QObject::connect( this,       SIGNAL( proxyResult( int ) ),
                      AppConfig,  SLOT( haveProxyResult( int ) ) );
    QObject::connect( this,       SIGNAL( finished() ),
                      this,       SLOT( deleteLater() ) );
    start();
}

ProxyCheckerThread::~ProxyCheckerThread() { abort(); }

void ProxyCheckerThread::abort()
{
    if ( !isRunning() ) return;
    requestInterruption();
    wait();
}

void ProxyCheckerThread::run()
{
    qDebug() << "checking proxies";
    QNetworkAccessManager netMan;
    ProxySettings sets;
    const QNetworkRequest req = QNetworkRequest( QUrl( STARTUP_URL ) );
    QTimer timer;
    timer.setSingleShot( true );
    timer.setInterval( PROXY_CHECK_TIMEOUT_MS );
    QEventLoop loop;
    QObject::connect( &timer, SIGNAL( timeout() ), &loop, SLOT( quit() ) );
    for ( int i = 0; i < proxies.size(); ++i )
    {
        sets = proxies[i];
        qDebug() << "checking proxy" << i << sets.host;
        if ( !sets.isValid() ) continue;
        netMan.setProxy( sets.networkProxy() );
        QNetworkReply *reply = netMan.get( req );
        QObject::connect( reply, SIGNAL( finished() ), &loop, SLOT( quit() ) );
        timer.start();
        loop.exec();
        if ( QThread::currentThread()->isInterruptionRequested() ) return;
        if ( timer.isActive() )
        {
            if ( reply->error() == QNetworkReply::NoError )
            {
                qDebug() << "proxy ok" << reply->bytesAvailable();
                emit proxyResult( i );
                return;
            }
            qDebug() << "error";
        }
        else qDebug() << "timeout";

        timer.stop();
        delete reply;
    }
    qDebug() << "all failed";
    emit proxyResult( -1 );
}








AppConfig::AppConfig() :
#ifdef Q_OS_MAC
   QSettings( COMPANY_NAME_MAC, APP_NAME ),
#else
   QSettings( APP_NAME, APP_NAME ),
#endif
   proxies( TProxyList() ), proxyIndex( -1 ), thread( 0 )
{
    QString cfg = QSettings::value( SETTINGS_KEY_CONFIG ).toString();
    if ( cfg.isEmpty() ) qDebug() << "No stored config";
    else importJson( cfg, false );
}

AppConfig::~AppConfig()
{
    if ( thread ) delete thread;
}


void AppConfig::importJson( QString cfg, bool setSettings )
{
    QByteArray decoded = QByteArray::fromBase64( cfg.toLatin1() );
//  qDebug() << "decoded" << decoded.size() << decoded;

    if ( setSettings && processJson( decoded ) ) QSettings::setValue( "info", cfg );
}

void AppConfig::connectProxy()
{
    if ( proxyIndex < -1 ) proxyIndex = 0;
    if ( !thread ) thread = new ProxyCheckerThread( this, proxies );
}

void AppConfig::disconnectProxy()
{
    proxyIndex = -1;
    emit newProxySettings( ProxySettings() );
}

void AppConfig::haveProxyResult( int workingProxy )
{
    qDebug() << "haveProxyResult" << workingProxy;
    thread = 0;
    proxyIndex = workingProxy;
    if ( workingProxy < 0 || workingProxy > proxies.size() ) emit newProxySettings( ProxySettings() );
    else emit newProxySettings( proxies[workingProxy] );
}

bool AppConfig::processJson( QByteArray json )
{
    QJsonParseError err;
    QJsonDocument jDoc = QJsonDocument::fromJson( json, &err );
    if ( jDoc.isNull() )
    {
        qDebug() << "JSON error" << err.errorString();
        return false;
    }
    QJsonArray proxyList = jDoc.object().value( "proxy_setting" ).toObject().value( "proxyList" ).toArray();

    while ( proxyList.size() )
    {
        ProxySettings sets;
        const QJsonObject item = proxyList.takeAt( 0 ).toObject();

        sets.host = item["host"].toString().toLatin1();

        if ( !item["enabled"].toBool( false ) || sets.host.isEmpty() ) continue;

        sets.port = item["port"].toInt( 1080 );
        sets.user = item["user"].toString().toLatin1();
        sets.pass = item["pass"].toString().toLatin1();

        proxies += sets;

        qDebug() << sets.host << sets.port << sets.user << sets.pass;
    }

    return true;
}



