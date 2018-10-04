#include "vpnIntegration.h"
#include <QSettings>
#include <QApplication>
#ifdef Q_OS_WIN
    #include <windows.h>
    #include <shellapi.h>
#else
    #include <QProcess>
#endif
#include <QFileInfo>
#include <QDir>
#include <QLocalSocket>
#include <QDebug>

#ifdef Q_OS_WIN
const char VPN_SOCKET_NAME[] = "121D9A1F-431B-04A7-58ED-2FBE2BCECE56";
#else
const char VPN_SOCKET_NAME[] = "/var/tmp/121D9A1F-431B-04A7-58ED-2FBE2BCECE56";
#endif

const char VPN_STATUS_CONNECTED[] = "Connected";
const char VPN_STATUS_DISCONNECTED[] = "Disconnected";
const char VPN_STATUS_EXITED[] = "Exited";
const char VPN_STATUS_ACKNOWLEDGED[] = "Acknowledged";
const char VPN_STATUS_AUTH_FAILED[] = "AuthFailed";

const char VPN_COMMAND_CONNECT[] = "Connect";
const char VPN_COMMAND_DISCONNECT[] = "Disconnect";
const char VPN_COMMAND_STATUS[] = "Status";

const char VPN_SINGLETON_COMMAND_SHOW[] = "Show";


void TVpnIntegration::setVpnInstalled( bool installed )
{
    QSettings settings( "PopcornTime", "VpnIntegration" );
    settings.setValue( "VpnAppInstalled", installed ? QApplication::applicationFilePath() : "" );
}

bool TVpnIntegration::isVpnInstalled()
{
    QSettings settings( "PopcornTime", "VpnIntegration" );
    return settings.value( "VpnAppInstalled", "" ).toString().size();
}

QString TVpnIntegration::getVpnExecutable()
{
    QSettings settings( "PopcornTime", "VpnIntegration" );
    return settings.value( "VpnAppInstalled", "" ).toString();
}


TVpnIntegrationServer::TVpnIntegrationServer( QObject *parent ) : QObject( parent ), server( new QLocalServer( this ) )
{
    QObject::connect( server, SIGNAL( newConnection() ), this, SLOT( onServerConnect() ) );
    server->setSocketOptions( QLocalServer::WorldAccessOption );
}

TVpnIntegrationServer::~TVpnIntegrationServer() { sendMessage( VPN_STATUS_EXITED ); }


bool TVpnIntegrationServer::listen() { return server->listen( VPN_SOCKET_NAME ); }
bool TVpnIntegrationServer::isConnected() { return socketList.size(); }

void TVpnIntegrationServer::onNotifyConnectionStatus( bool nowConnected )
{
    connected = nowConnected;
    sendStatus();
}

void TVpnIntegrationServer::onAuthFailed() { sendMessage( VPN_STATUS_AUTH_FAILED ); }

void TVpnIntegrationServer::onServerConnect()
{
    qDebug() << this->metaObject()->className() << __FUNCTION__;
    QLocalSocket *clientConnection = server->nextPendingConnection();
    connect( clientConnection, SIGNAL( disconnected() ), clientConnection, SLOT( deleteLater() ) );
    connect( clientConnection, SIGNAL( readyRead() ), this, SLOT( onSocketReadyRead() ) );
    socketList << clientConnection;
    sendStatus( clientConnection );
}

void TVpnIntegrationServer::onSocketReadyRead()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>( sender() );
    while ( socket && socket->bytesAvailable() )
    {
        QByteArray line = socket->readLine();
        qDebug() << this->metaObject()->className() << __FUNCTION__ << line.simplified();
        if ( line.contains( VPN_COMMAND_STATUS ) )
        {
            sendStatus( socket );
            return;
        }

        if ( line.contains( VPN_COMMAND_CONNECT ) ) emit connectCommand();
        else if ( line.contains( VPN_COMMAND_DISCONNECT ) ) emit disconnectCommand();
        else return;
        sendMessage( VPN_STATUS_ACKNOWLEDGED );
    }
}

void TVpnIntegrationServer::sendStatus( QLocalSocket *socket ) { sendMessage( socket, connected ? VPN_STATUS_CONNECTED : VPN_STATUS_DISCONNECTED ); }
void TVpnIntegrationServer::sendStatus() { sendMessage( connected ? VPN_STATUS_CONNECTED : VPN_STATUS_DISCONNECTED ); }
void TVpnIntegrationServer::sendMessage( QLocalSocket *socket, const QByteArray& data )
{
    if ( !socket ) return;
    qDebug() << this->metaObject()->className() << __FUNCTION__ << data;
    socket->write( QByteArray( data + "\n" ) );
    socket->flush();
}

void TVpnIntegrationServer::sendMessage( const QByteArray& data )
{
    QMutableListIterator < QPointer < QLocalSocket >> i( socketList );
    while ( i.hasNext() )
    {
        QLocalSocket *socket = i.next();
        if ( !socket ) i.remove();
        else if ( !socket->isOpen() ) socket->deleteLater();
        else sendMessage( socket, data );
    }
}

