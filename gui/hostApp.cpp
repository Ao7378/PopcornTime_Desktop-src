#if !defined( USE_QCEF ) && !defined( USE_WEBENGINE )
    #include <QGraphicsWebView>
    #include <QWebFrame>
    #include <QWebSettings>
#elif USE_WEBENGINE
    #include "WebEngineWebView.h"
#endif
#include <QWidget>
#include <QFileDialog>
#include <QApplication>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkProxy>
#include <QVariantList>
#include <QMetaMethod>
#include <QTimer>

#include "hostApp.h"
#include "defaults.h"
#include "vlc.h"
#include "TorrentWrapper.h"
#include "vpnIntegration.h"
#include "Zoom4k.h"

HostApp::HostApp( QObject *jsObject, QWidget *parent ) : QObject( parent ), torrent( new TorrentWrapper( parent ) ), jsObj( jsObject )
{
    torrent->moveToThread( &torrentThread );
    torrentThread.start();
    vpn = new TVpnIntegrationClient( this );
    QObject::connect( vpn, SIGNAL( vpnConnected() ), this, SLOT( vpnConnected() ) );
    QObject::connect( vpn, SIGNAL( vpnDisconnected() ), this, SLOT( vpnDisconnected() ) );
    QObject::connect( vpn, SIGNAL( appDisconnected() ), this, SLOT( vpnDisconnected() ) );
    QObject::connect( vpn, SIGNAL( authFailed( bool ) ), this, SLOT( vpnAuthFailed( bool ) ) );

    vpn->makeConnection();


#ifdef USE_QCEF
    QObject::connect( jsObj, SIGNAL( loadStarted() ), this, SLOT( attachObject() ) );
    QObject::connect( jsObj, SIGNAL( jsMessage( const QString&, const QVariantList& ) ),
                      this, SLOT( onJsMessage( const QString&, const QVariantList& ) ) );
    QObject::connect( torrent, SIGNAL( leechingStarted( int ) ), this, SLOT( setTorrentIndex( int ) ) );


#elif USE_WEBENGINE
    QObject::connect( jsObj, SIGNAL( loadFinished( bool ) ), this, SLOT( attachObject() ) );
    QObject::connect( torrent.data(), SIGNAL( leechingStarted( int ) ), this, SLOT( setTorrentIndex( int ) ) );
#else // QWebKit
    QObject::connect( jsObj, SIGNAL( javaScriptWindowObjectCleared() ), this, SLOT( attachObject() ) );

    QWebSettings::globalSettings()->setLocalStoragePath( QDir::toNativeSeparators( QDir::tempPath() + QDir::separator() + "PopcornTime" ) );
    QWebSettings::globalSettings()->setAttribute( QWebSettings::LocalStorageEnabled, true );
    QWebSettings::globalSettings()->setAttribute( QWebSettings::JavascriptCanOpenWindows, true );
    QWebSettings::globalSettings()->setAttribute( QWebSettings::JavascriptCanCloseWindows, true );
#ifdef _DEBUGGING
    QWebSettings::globalSettings()->setAttribute( QWebSettings::DeveloperExtrasEnabled, true );
#endif
    attachObject();
    downloadDir = AppConfig::value( SETTINGS_KEY_DOWNLOAD_DIR ).toString();
    cleanOnExit = AppConfig::value( SETTINGS_KEY_CLEAN_ON_EXIT, CLEAN_ON_EXIT_DEFAULT ).toBool();
    fontSize = AppConfig::value( SETTINGS_KEY_FONT_SIZE, SETTINGS_DEFAULT_FONT_SIZE ).toInt();
    if ( fontSize < -2 || fontSize > 2 ) fontSize = 0;
    setUserFontSize( QString::number( fontSize ) );
#endif


//  downloadDir = AppConfig::value( SETTINGS_KEY_DOWNLOAD_DIR ).toString();
//  cleanOnExit = AppConfig::value( SETTINGS_KEY_CLEAN_ON_EXIT, CLEAN_ON_EXIT_DEFAULT ).toBool();
//  fontSize = AppConfig::value( SETTINGS_KEY_FONT_SIZE, SETTINGS_DEFAULT_FONT_SIZE ).toInt();
//  if ( fontSize < -2 || fontSize > 2 ) fontSize = 0;
//  setUserFontSize( fontSize );
//  torrent->init( downloadDir, cleanOnExit );

#ifdef USE_SOCKS5
    QObject::connect( settings, SIGNAL( newProxySettings( ProxySettings ) ), this, SLOT( setProxy( ProxySettings ) ) );
#endif

    QObject::connect( torrent.data(), SIGNAL( torrentDataAvailable( QString ) ), this, SLOT( hideLoading() ) );
    QObject::connect( torrent.data(), SIGNAL( torrentProgressAvailable( int, int, int, int, QString ) ),
                      this, SLOT( updateTorrentProgress( int, int, int, int, QString ) ) );
    if ( Requester::getInstance() )
    {
        QObject::connect( this, SIGNAL( subThumbRequest( QString ) ),
                          Requester::getInstance(), SLOT( getSubtitleThumb( QString ) ), Qt::QueuedConnection );
        QObject::connect( Requester::getInstance(), SIGNAL( haveSubtitleThumb( QString, QString ) ),
                          this, SLOT( onSubThumbReply( QString, QString ) ), Qt::QueuedConnection );
    }

//#ifdef _DEBUGGING
//    TVpnIntegration::setCredentials( "testUser", "testPass" );
//#endif

    vpnCheckTimer.start( 5000, this );
}

HostApp::~HostApp()
{
    torrent->moveToThread( QThread::currentThread() );
    torrentThread.quit();
    torrentThread.wait( 1000 );
}

TorrentWrapper* HostApp::torrentEngine() { return torrent.data(); }


void HostApp::attachObject()
{
#ifdef USE_QCEF
    QFile dic( ":/JShostapp" );
    if ( dic.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        executeJs( dic.readAll() );
        dic.close();
    }
#elif USE_WEBENGINE
    QWebEnginePage *page = qobject_cast<WebEnginePage *>( sender() );
    if ( page )
    {
        WebEngineView *view = qobject_cast<WebEngineView *>( page->view() );
        if ( view ) view->attachJsObject( "hostApp", ":/JShostapp", this );
    }
#else //QWebKit
    QWebFrame *frame = qobject_cast<QWebFrame *>( jsObj );
    if ( frame ) frame->addToJavaScriptWindowObject( QString( "hostApp" ), this );
    else qDebug() << "addToJavaScriptWindowObject error";
#endif
    QTimer::singleShot( 1000, this, SLOT( setConfig() ) );
    QTimer::singleShot( 1500, this, SIGNAL( initCompleted() ) );
}


// JS control slots



void HostApp::setVolume( int volume ) { AppConfig::setValue( SETTINGS_KEY_VOLUME, volume ); }
int HostApp::getVolume() { return AppConfig::value( SETTINGS_KEY_VOLUME, 100 ).toInt(); }
void HostApp::vpnConnected() { executeJs( "app.config.hostApp.isVpnConnected=true;app.vpn.connected();" ); }
void HostApp::vpnDisconnected() { executeJs( "app.config.hostApp.isVpnConnected=false;app.vpn.disconnected();" ); }
void HostApp::hideLoading()
{
    executeJs( "ui.loading_wrapper.hide(true);" );
    qDebug() << __FUNCTION__;
}

void HostApp::showLoading()
{
    executeJs( "app.state='movie';ui.loading_wrapper.show();" );
//    qDebug() << __FUNCTION__;
}

void HostApp::updateTorrentProgress( int progressPeers, int dlSpeed, int seeds, int peers, QString msg )
{
    QString js = QString( "app.torrent.updateInfo(%1,%2,%3,%4,\"%5\")" ).arg( progressPeers ).arg( dlSpeed ).arg( seeds ).arg( peers ).arg( msg );
//  qDebug() << "updateProgress " << js;
    executeJs( js );
}
void HostApp::setTorrentIndex( int index ) { executeJs( QString( "app.torrent.current_torrent_id=%1;" ).arg( index ) ); }
void HostApp::setAnimations( bool anim )
{
//  qDebug() << "ui.disableAnimations " << anim << QThread::currentThread();
    executeJs( QString( "ui.disableAnimations( %1 );" ).arg( anim ? 0 : 1 ) );
}

void HostApp::onSubThumbReply( QString url, QString data )
{
    QString js = QString( "utils.url_response[\"%1\"](%2)" ).arg( url, data );
    qDebug() << "updateSubtitles " << js.size() << js.left( 250 );
    executeJs( js );
}

#include <QMessageBox>

// JS callback slots


void HostApp::url_request( QString url )
{
    emit subThumbRequest( url );
}

void HostApp::onJsMessage( const QString& name, const QVariantList& args )
{
    qDebug() << __FILE__ << __FUNCTION__ << name.toLocal8Bit() << args;
    QString argsString;
    if ( args.size() ) argsString = args.first().toString();
    int methodIndex;

    methodIndex = slotIndex( name + "(QString)" );
    if ( methodIndex >= 0 )
    {
        qDebug() << "JS name" << name.toLocal8Bit() << "method" << methodIndex << this->metaObject()->method( methodIndex ).name() << args;
        QMetaObject::invokeMethod( this, qPrintable( name ), Q_ARG( QString, argsString ) );
        return;
    }

    methodIndex = slotIndex( name + "()" );
    if ( methodIndex < 0 ) methodIndex = slotIndex( "QString " + name );
    if ( methodIndex >= 0 )
    {
        qDebug() << "JS name" << name.toLocal8Bit() << "method(QString)" << methodIndex << this->metaObject()->method( methodIndex ).name() << args;
        QMetaObject::invokeMethod( this, qPrintable( name ) );
        return;
    }
    qDebug() << "JS bind method not found" << name.toLocal8Bit() << "method" << methodIndex << this->metaObject()->method( methodIndex ).name() << args;
}

//void HostApp::BindingTest( QString str )
//{
//    qDebug() << __FUNCTION__ << str;
//    executeJs( "alert( '" + str + "' )" );
//}


void HostApp::sendWinAction( QString str )
{
    if ( str == "max" ) emit toggledMaximize();
    else if ( str == "min" ) emit commandedMinimize();
    else if ( str == "close" ) emit commandedClose();
}

void HostApp::getTorrent( QJsonValue jVal )
{
    QJsonObject jObj = jVal.toObject();
    if ( jObj.isEmpty() )
    {
        qDebug() << __FUNCTION__ << "JSON error" << jObj;
        return;
    }
//  qDebug() << __FUNCTION__ << "JSON" << jObj;
//
//  QString title = jObj.value( "title" ).toString();
//  QByteArray locale = jObj.value( "subtitles_locale" ).toString().toLatin1();
//  QJsonObject tor = jObj.value( "torrent" ).toObject();
//  QString url = tor.value( "url" ).toString();
//  QString magnet = tor.value( "magnet" ).toString();
//  QString file = tor.value( "file" ).toString();
//  QByteArray quality = tor.value( "quality" ).toString().toLatin1();
//  QString subsJson = QJsonDocument( jObj.value( "subtitles" ).toArray() ).toJson();

    QMetaObject::invokeMethod( torrent.data(), "get", Q_ARG( TorrentInfo, TorrentInfo( jObj ) ) );

    }

void HostApp::getTorrent( QString session )
{
    QJsonParseError err;
    getTorrent( QJsonDocument::fromJson( session.toUtf8(), &err ).object() );
}


QString HostApp::getTorrent( QString url, QString fileName, QString subtitles_url )
{
    QString url1;
    QString magnet1;
    if ( url.startsWith( "magnet" ) ) magnet1 = url;
    else url1 = url;

    torrent->get( TorrentInfo( url1, magnet1, fileName, subtitles_url ) );
    return "1";
}

void HostApp::cancelTorrent( QString ATorrentId )
{
//  qDebug() << "cancel Torrent HostApp" << ATorrentId;
    bool ok;
    int index = ATorrentId.toInt( &ok );
    if ( ok ) cancelTorrent( index );
}

void HostApp::cancelTorrent( int index )
{
    torrent->cancel( index );
}
void HostApp::cancelTorrent() { cancelTorrent( -1 ); }

void HostApp::setConfig( QString config )
{
    (void) config;
    setConfig();
}

void HostApp::setConfig()
{
#if defined USE_QCEF || defined USE_WEBENGINE
    executeJs( "hostApp.setConfigVars();" );
#endif
    checkVpnInstalled();
}


void HostApp::setConfigVars( QString vars )
{
    qDebug() << __FILE__ << __FUNCTION__ << vars;

    executeJs( "document.body.style.zoom='" + QString::number( int( Zoom4k::zoomFactorPercent() ) ) + "%'" );
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << "document.body.style.zoom='" + QString::number( int( Zoom4k::zoomFactor()* 100 ) ) + "%'";

    QJsonParseError err;
    QJsonObject jObj = QJsonDocument::fromJson( vars.toUtf8(), &err ).object();
    if ( jObj.isEmpty() )
    {
        qDebug() << __FUNCTION__ << "JSON error" << err.errorString() << vars;
        return;
    }

    downloadDir = jObj.value( "tempPath" ).toString();
    if ( downloadDir.isEmpty() )
    {
        downloadDir = AppConfig::value( SETTINGS_KEY_DOWNLOAD_DIR ).toString();
        if ( downloadDir.isEmpty() ) downloadDir = QDir::homePath() + QDir::separator() + "Downloads";
        qDebug() << QString( "app.config.hostApp.tempPath='%1';app.config.updateView();" ).arg( downloadDir );
        executeJs( QString( "app.config.hostApp.tempPath='%1';app.config.updateView();" ).arg( downloadDir ) );
    }

    fontSize = jObj.value( "subsFontSize" ).toInt();
    if ( fontSize < -2 || fontSize > 2 )
    {
        fontSize = 0;
//      qDebug() << "app.config.hostApp.subsFontSize=0;";
        executeJs( "app.config.hostApp.subsFontSize=0;" );
    }
    emit fontSizeChanged( fontSize );

    cleanOnExit = jObj.value( "cleanOnExit" ).toBool( false );

    torrent->init( downloadDir + QDir::separator() + "PopcornTime", cleanOnExit );
}



QString HostApp::getTempPath()
{
    QJsonObject obj;
    obj.insert( "path", QJsonValue( downloadDir ) );
    obj.insert( "cleanOnExit", QJsonValue( cleanOnExit ) );

    QString result = QJsonDocument( obj ).toJson();
    result.replace( "\\", "\\\\" );
    qDebug() << "getTempPath" << result;
    return result;
}

QString HostApp::setTempPath()
{
    return setTempPath( "" );
}

QString HostApp::setTempPath( QString callback )
{
    QString result = QFileDialog::getExistingDirectory( QApplication::activeWindow(),
                                                        "Select download directory",
                                                        downloadDir.size() ? downloadDir : QDir::homePath(),
                                                        QFileDialog::ShowDirsOnly );
    if ( result.isEmpty() ) return "";
    downloadDir = result;
#if defined USE_QCEF || defined USE_WEBENGINE
    executeJs( QString( "app.config.hostApp.tempPath='%1';" ).arg( downloadDir ) );
    if ( callback.size() ) executeJs( callback );
#else
    AppConfig::setValue( SETTINGS_KEY_DOWNLOAD_DIR, downloadDir );
#endif
    torrent->init( downloadDir + QDir::separator() + "PopcornTime", cleanOnExit );
    return result;
}

void HostApp::OpenTempDir()
{
    QString path = QDir::toNativeSeparators( downloadDir );
    QDesktopServices::openUrl( QUrl( "file:///" + path ) );
}

void HostApp::setCleanOnExit( QString isClear )
{
    cleanOnExit = isClear == "true" || isClear == "1";
#if defined USE_QCEF || defined USE_WEBENGINE
    AppConfig::setValue( SETTINGS_KEY_CLEAN_ON_EXIT, cleanOnExit );
//#else
//    cleanOnExit = isClear != "false";
#endif

}

void HostApp::setUserFontSize( QString size )
{
    bool ok;
    fontSize = size.toInt( &ok );
    if ( !ok ) fontSize = 0;
    if ( fontSize > 2 ) fontSize = 2;
    if ( fontSize < -2 ) fontSize = -2;
    emit fontSizeChanged( fontSize );
#if defined USE_QCEF || defined USE_WEBENGINE
    AppConfig::setValue( SETTINGS_KEY_FONT_SIZE, fontSize );
#endif
}

void HostApp::openBrowser( QString url )
{
    qDebug() << this->metaObject()->className() << __FUNCTION__ << url;
    QDesktopServices::openUrl( QUrl( url ) );
}




void HostApp::closePlayer() { emit commandedClosePlayer();}
void HostApp::settingsLoad( QString callback )
    QByteArray string = QByteArray::fromBase64( AppConfig::value( "configuration" ).toString().toLatin1() );
    QString js = QString( "%1(\"%2\")" ).arg( callback, QString( string ) );
    qDebug() << __FUNCTION__ << js;
    executeJs( js );
}

void HostApp::settingsSave( QString data )
{
    AppConfig::setValue( "configuration", data );
}

void HostApp::checkVpnInstalled()
{
    const bool vpnInstalled = vpn->isAppInstalled();
    qDebug() << this->metaObject()->className() << __FUNCTION__ << vpnInstalled << "sender" << sender();
    executeJs( QString( "app.config.hostApp.vpnInstalled(%1);" ).arg( vpnInstalled ? 1 : 0 ) );
    vpn->sendStatusCommand();
}

void HostApp::vpnAuthFailed( bool email )
{
    qDebug() << this->metaObject()->className() << __FUNCTION__ << email << "sender" << sender();
    executeJs( QString( "utils.msgbox("
                        "'<b>VPN:</b> Wrong Username or Password.<br><br>"
                        "<u style=\"cursor: pointer;\" onclick=\"ui.vpn_page.createAccountWindow()\">%1 VPN Account</u>');" ).arg( email ? "Renew" : "Create" ) );

}


//void HostApp::installVpn( QString url, QString username, QString password )
//{
//    TVpnIntegration::setCredentials( username.toLatin1(), password.toLatin1() );
//}

#ifdef USE_SOCKS5
bool HostApp::vpn_isConnected() { return proxyActive; }
void HostApp::vpn_connect() { AppConfig::connectProxy(); }
void HostApp::vpn_disconnect() { AppConfig::disconnectProxy(); }
#else
bool HostApp::vpn_isConnected() { vpn->sendStatusCommand(); return false; }
void HostApp::vpn_connect()
        {
    qDebug() << this->metaObject()->className() << __FUNCTION__;
    vpn->sendConnectCommand();
}
void HostApp::vpn_disconnect()
{
    qDebug() << __FILE__ << __FUNCTION__; vpn->sendDisconnectCommand();
        }
#endif




inline int HostApp::slotIndex( QString name ) { return this->metaObject()->indexOfMethod( QMetaObject::normalizedSignature( qPrintable( name ) ) ); }

inline bool HostApp::executeJs( QString script, QObject *javaScriptObject )
{
#if USE_WEBENGINE
    QWebEnginePage *page = qobject_cast<QWebEnginePage *>( javaScriptObject );
    if ( !page ) page = qobject_cast<QWebEnginePage *>( jsObj );
    if ( javaScriptObject ) qDebug() << this->metaObject()->className() << __FUNCTION__ << javaScriptObject << page << script.mid( 0, 50 );
    if ( !page ) return false;
    page->runJavaScript( script );
    return true;
#else
    return QMetaObject::invokeMethod( jsObj, "evaluateJavaScript", Q_ARG( QString, script ) );
#endif
}


void HostApp::timerEvent( QTimerEvent * )
{
    static bool vpnLastInstalled = false;

    bool vpnInstalled = vpn->isAppInstalled();

    if ( vpnLastInstalled != vpnInstalled ) checkVpnInstalled();
    vpnLastInstalled = vpnInstalled;

    vpnCheckTimer.start( 5000, this );
}


