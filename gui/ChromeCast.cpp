#include "ChromeCast.h"
#include "NjsProcess.h"
#include "GlyphButton.h"

#include <QDebug>

#include <QPushButton>
#include <QMovie>

#include <QFrame>
#include <QBoxLayout>
#include <QLabel>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QWidget>

#include <QNetworkProxy>
#include <QThread>
#include <QEventLoop>

const char CHROMECAST_SCRIPT_DISCOVERY[] = "device-discovery.js";
const char CHROMECAST_SCRIPT_SERVER[] = "server.js";
const char CHROMECAST_DEVICE_TEXT_LOCAL[] = "Popcorn Time";

//const int CHROMECAST_DISCOVERY_RESTARTS = 3;

const int CHROMECAST_REFRESH_TIMER_INTERVAL = 500;
const int CHROMECAST_REFRESH_INTERVAL_CC = 1000;
const int CHROMECAST_REFRESH_INTERVAL_OTHER = 30000;





CastDevice::CastDevice() : type( CAST_DEVICETYPE_INVALID ) { }

CastDevice::CastDevice( const QJsonObject& jObj ) : type( CAST_DEVICETYPE_INVALID )
{
    QString typeStr = jObj.value( "deviceType" ).toString();

    if ( typeStr == "googlecast" ) type = CAST_DEVICETYPE_CHROMECAST;
    else if ( typeStr == "airplay" ) type = CAST_DEVICETYPE_AIRPLAY;
    else if ( typeStr == "dlna" ) type = CAST_DEVICETYPE_DLNA;
    else return;

    name = jObj.value( "name" ).toString().toLatin1();
    ip = jObj.value( "ip" ).toArray().at( 0 ).toString().toLatin1();
    url = jObj.value( "url" ).toString().toLatin1();
}







ChromeCast::ChromeCast( QWidget *parent, GlyphButton *castBtn ) :
   QObject( parent ),
   netMan( parent ),
   discoveryProc( new NjsProcess( this ) ),
// discoveryRestartCounter( CHROMECAST_DISCOVERY_RESTARTS ),
   castProc( new NjsProcess( this ) ),
   popup( new Frame( parent ) ), //Qt::Dialog |
   devicesView( new ListWidget ),
   m_castBtn( castBtn ),
   btnMovie( new QMovie( ":/chromecast_search" ) ),
   isCasting( false ),
   m_state( Idle ),
   fontSize( 0 ),
   startTime( 0 )
{
    if ( castBtn ) setButton( castBtn );

    netMan.setProxy( QNetworkProxy( QNetworkProxy::NoProxy ) );

    volumeTimer.setSingleShot( true );
    connect( &volumeTimer, SIGNAL( timeout() ), this, SLOT( onVolumeTimer() ) );

    discoveryRestartTimer.setSingleShot( true );
    discoveryRestartTimer.setInterval( 45000 );
    connect( &discoveryRestartTimer, SIGNAL( timeout() ), discoveryProc, SLOT( kill() ) );

    connect( discoveryProc, SIGNAL( finished( int, QProcess::ExitStatus ) ), this, SLOT( discoveryStopped( int, QProcess::ExitStatus ) ) );
    connect( discoveryProc, SIGNAL( haveToken( QByteArray, QByteArray ) ), this, SLOT( parseNjsData( QByteArray, QByteArray ) ) );
    discoveryProc->setArguments( QStringList( CHROMECAST_SCRIPT_DISCOVERY ) );
    discoveryProc->start();
    discoveryRestartTimer.start();

    connect( castProc, SIGNAL( finished( int, QProcess::ExitStatus ) ), this, SLOT( serverStopped( int, QProcess::ExitStatus ) ) );
    connect( castProc, SIGNAL( haveToken( QByteArray, QByteArray ) ), this, SLOT( parseNjsData( QByteArray, QByteArray ) ) );
    castProc->setArguments( QStringList( CHROMECAST_SCRIPT_SERVER ) );

    QVBoxLayout *vl = new QVBoxLayout( popup );
    QLabel *lbl = new QLabel( "Play on:" );
    vl->addWidget( lbl, 0 );
    lbl->setStyleSheet( "" );

    vl->addWidget( devicesView );
//  vl->setSizeConstraint( QLayout::SetFixedSize );
    devicesView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    devicesView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    devicesView->addItem( CHROMECAST_DEVICE_TEXT_LOCAL );
    devicesView->setSelectionMode( QAbstractItemView::SingleSelection );
    devicesView->item( 0 )->setSelected( true );
    devicesView->setStyleSheet( "QListView { border:0px; font:10pt; } " );
    connect( devicesView, SIGNAL(itemClicked(QListWidgetItem*)),
             this, SLOT(onListItemClicked(QListWidgetItem*)) );

    popup->setStyleSheet( "QFrame {background-color:black; color:#cacaca; border:1px solid #666666; padding:0px; margin:0px; font:10pt;} "
                          "QLabel {font:13pt; border:0px} " );

    connect( btnMovie, SIGNAL( frameChanged( int ) ), this, SLOT( frameChange( int ) ) );
    if ( btnMovie->loopCount() != -1 ) connect( btnMovie, SIGNAL( finished() ), btnMovie, SLOT( start() ) );
    btnMovie->setCacheMode( QMovie::CacheAll );
}

void ChromeCast::frameChange( int frame )
{
    (void) frame;
//  static QPixmap pixmap;
//  qDebug() << "frame" << frame << btnMovie->isValid() << ( btnMovie->currentPixmap().toImage() == pixmap.toImage());
//  pixmap = btnMovie->currentPixmap();
    if ( m_castBtn ) m_castBtn->setIcon( QIcon( btnMovie->currentPixmap() ) );
}


void ChromeCast::discoveryStopped( int exitCode, QProcess::ExitStatus exitStatus )
{
    (void) exitCode; (void) exitStatus;
    qDebug() << "discovery finished, exit code" << exitCode << ( exitStatus == QProcess::NormalExit ? "" : "crashed" );
    if ( exitStatus != QProcess::NormalExit )
    {
        QProcess *proc = qobject_cast<QProcess *>( sender() );
        if ( proc ) CC_DEBUG << "CC discovery STDERR" << proc->readAllStandardError();
    }
    startDiscovery( exitStatus == QProcess::NormalExit );
}

void ChromeCast::serverStopped( int exitCode, QProcess::ExitStatus exitStatus )
{
    (void) exitCode; (void) exitStatus;
    qDebug() << "server finished, exit code" << exitCode << ( exitStatus == QProcess::NormalExit ? "" : "crashed" );
    QProcess *proc = qobject_cast<QProcess *>( sender() );
    handleCastingSwitch( false );
}


void ChromeCast::setButton( GlyphButton *btn )
{
    m_castBtn = btn;
    QObject::connect( m_castBtn, SIGNAL( clicked() ), this, SLOT( buttonClicked() ) );
    m_castBtn->setCheckable( false );
}


void ChromeCast::buttonClicked()
{
    if ( m_castBtn->isCheckable() )
    {
        stop();
        return;
    }
    devicesView->setFixedSize( devicesView->sizeHintForColumn( 0 ) + 2 * devicesView->frameWidth(), devicesView->sizeHintForRow( 0 ) * devicesView->count() + 2 * devicesView->frameWidth() );

    popup->adjustSize();

    QWidget *src = qobject_cast<QWidget *>( sender() );
    if ( src )
    {
        const QPoint srcCenter = src->parentWidget()->mapToGlobal( src->geometry().center() );
        const QSize pSize = popup->size();
        const QPoint newPos = QPoint( srcCenter.x() - pSize.width(), srcCenter.y() - pSize.height() );
        popup->move( newPos );
    }

    popup->show();
}

void ChromeCast::stop()
{
    if ( isCasting )
    {
        QTimer timer;
        timer.setSingleShot( true );
        QEventLoop loop;
        QObject::connect( &timer, SIGNAL( timeout() ), &loop, SLOT( quit() ) );
        QNetworkReply *reply = issueCommand( "command/stop" );
        if ( reply )
        {
            QObject::connect( reply, SIGNAL( finished() ), &loop, SLOT( quit() ) );
            timer.start( 500 );
            qDebug() << "stopping";
            loop.exec();
            qDebug() << "stopped, reply finished" << reply->isFinished() << "timer active" << timer.isActive();
        }
    }
    castProc->kill();
    startTime = 0;
}

void ChromeCast::pause() { if ( state() != Paused ) issueCommand( "command/pause" ); }
void ChromeCast::setTime( int time )
{
    if ( state() == Opening || state() == Idle ) startTime = time;
    else issueCommand( "command/seek", "seek=" + QByteArray::number( double( time ) / 1000 ) );
}
void ChromeCast::setStartTime( int time ) { startTime = time; CC_DEBUG << "CC start time =" << time; }

void ChromeCast::setMute( bool muted ) { issueCommand( "command/mute", muted ? "muted=1" : "muted=0" ); }
void ChromeCast::onVolumeTimer() { issueCommand( "command/volume", QByteArray( "volume=" ) + QByteArray::number( double( lastVolume ) / 200 ) ); }

void ChromeCast::setPause( bool setPause )
{
    if ( setPause && state() == Paused ) return;
    if ( !setPause && isPlaying() ) return;
    issueCommand( setPause ? "command/pause" : "command/play" );
}

void ChromeCast::setVolume( int volume )
{
    static QTime blockUntil = QTime::currentTime();
    if ( lastVolume == volume ) return;
    lastVolume = volume;
    if ( QTime::currentTime() < blockUntil ) volumeTimer.start( 250 );
    else
    {
        issueCommand( "command/volume", QByteArray( "volume=" ) + QByteArray::number( double( volume ) / 200 ) );
        blockUntil = QTime::currentTime().addMSecs( 500 );
    }
}

void ChromeCast::setSubtitleFileV( QString fileName )
{
    subtitleFile = fileName;
    static QStringList supportedList = getSupportedSubsList();
    foreach( QString ext, supportedList )
    {
        if ( fileName.toLower().endsWith( ext ) )
        {
            issueCommand( "command/subtitles", QString( "subtitles=%1" ).arg( subtitleFile ).toLocal8Bit() );
            return;
        }
    }
    issueCommand( "command/subtitles", "subtitles=" );
}

void ChromeCast::setFontSize( int fontSize )
{
    if ( fontSize > 2 ) fontSize = 2;
    if ( fontSize < -2 ) fontSize = -2;
    this->fontSize = fontSize;
    issueCommand( "command/fontSize", QByteArray( "fontSize=" ) + QByteArray::number( fontSize ) );
}
void ChromeCast::parseDiscovery( QByteArray data )
{
    QJsonParseError err;
    QJsonObject jObj = QJsonDocument::fromJson( data, &err ).object();
    if ( jObj.isEmpty() )
    {
        qDebug() << "JSON error" << err.errorString() << data;
        return;
    }

    addDevice( CastDevice( jObj ) );
}

bool ChromeCast::addDevice( const CastDevice& device )
{
    if ( !device.isValid() || devs.contains( device ) ) return false;
    devs += device;
    devicesView->addItem( device.name );
    devicesView->adjustSize();
    popup->adjustSize();
    qDebug() << "discovered" << item.type << item.name << item.ip << item.url;
    return true;
}

void ChromeCast::parseMStat( QByteArray data )
{
    QJsonParseError err;
    QJsonObject jObj = QJsonDocument::fromJson( data, &err ).object();
    if ( jObj.isEmpty() )
    {
        qDebug() << "JSON error" << err.errorString() << data;
        return;
    }

    QString theState = jObj.value( "playerState" ).toString( "INVALID_VALUE" );
    if ( theState == "BUFFERING" )
    {
        theState = "Buffering";
        if ( m_state == Opening )
        {
            const int time = jObj.value( "media" ).toObject().value( "duration" ).toDouble( -1 ) * 1000;
            if ( time >= 0 ) emit lengthChanged( time );
        }
        else setState( Buffering );
    }
    else if ( theState == "PLAYING" )
    {
        theState = "Playing";
        setState( Playing );
    }
    else if ( theState == "PAUSED" )
    {
        theState = "Paused";
        setState( Paused );
    }
    else if ( theState == "INVALID_VALUE" ) theState = "";
    if ( theState.size() )
    {
        theState.prepend( QString( CAST_DEVICETYPE_USER_STRING[currentDevice.type] ) + " " );
        emit stateText( theState );
    }

    switch ( currentDevice.type )
    {
    case CAST_DEVICETYPE_AIRPLAY: emit stateImage( ":/airplay_icon" ); break;
    case CAST_DEVICETYPE_CHROMECAST: emit stateImage( ":/chromecast_icon" ); break;
    case CAST_DEVICETYPE_DLNA: emit stateImage( ":/dlna_icon" ); break;
    default: emit stateImage( "" ); break;
    }

    const int time = jObj.value( "currentTime" ).toDouble( -1 ) * 1000;

    if ( time >= 0 )
    {
        emit timeChanged( time );
        currentTime = time;
    }

}

void ChromeCast::startDiscovery( bool crashed )
{
    qDebug() << "startDiscovery" << crashed;
    if ( discoveryProc->state() != QProcess::NotRunning ) return;
    if ( crashed )
    {
        QTimer::singleShot( 1000, discoveryProc, SLOT( start() ) );
        discoveryRestartTimer.start();
        return;
//      if ( discoveryRestartCounter <= 0 ) return;
//      --discoveryRestartCounter;
    }
//  else discoveryRestartCounter = CHROMECAST_DISCOVERY_RESTARTS;


//  devs.clear();
//  while ( devicesView->count() > 1 ) devicesView->takeItem( devicesView->count() - 1 );
    discoveryProc->start();
    discoveryRestartTimer.start();
}

void ChromeCast::handleCastingSwitch( bool switchedToEnable )
{
    if ( switchedToEnable == isCasting ) return;
    if ( switchedToEnable )
    {
        if ( state() == Idle ) setState( Opening );
    }
    else
    {
        if ( state() != Idle ) setState( Idle );
    }
    emit castingSwitched( isCasting = switchedToEnable );
    qDebug() << "Casting is now" << ( isCasting ? "connected" : "disconnected" );
    emit stateText( "Starting Casting..." );

    totalLength = 1;
    lastVolume = 100;
    isCasting ? stateRefreshtimer.start( CHROMECAST_REFRESH_TIMER_INTERVAL, this ) : stateRefreshtimer.stop();
    btnMovie->stop();
    currentTime = 0;
    m_castBtn->setCheckable( isCasting );
    m_castBtn->setChecked( isCasting );
    m_castBtn->changedState( m_castBtn->isChecked() );
    if ( !switchedToEnable )
    {
        devicesView->item( 0 )->setSelected( true );
        currentDevice = CastDevice();
    }
}

void ChromeCast::timerEvent( QTimerEvent * )
{
    static int counter = 0;
    static const int CC_INTERVAL = CHROMECAST_REFRESH_INTERVAL_CC / CHROMECAST_REFRESH_TIMER_INTERVAL;
    static const int OTHER_INTERVAL = CHROMECAST_REFRESH_INTERVAL_OTHER / CHROMECAST_REFRESH_TIMER_INTERVAL;
    const int period = currentDevice.type == CAST_DEVICETYPE_CHROMECAST ? CC_INTERVAL : OTHER_INTERVAL;
    currentTime += CHROMECAST_REFRESH_TIMER_INTERVAL;
    emit timeChanged( currentTime );
    if ( counter++ >= period )
    {
        issueCommand( "command/mediaState" );
        counter = 0;
    }
}

void ChromeCast::parseNjsData( QByteArray key, QByteArray value )
{
    if ( key == "DEVICE_FOUND" ) parseDiscovery( value );
    else if ( key == "LISTEN_URL" )
    {
        serverUrl = value;
//      netMan.get( QNetworkRequest( serverUrl.toString() + "command/deviceState" ) );
//      qDebug() << serverUrl << "stat req";
        QString req = serverUrl.toString();
        static QString loadCmd = "command/load?deviceType=%1&deviceIP=%2&torrentFile=%3&fontSize=%4";
        req += loadCmd.arg( CAST_DEVICETYPE_STRING[currentDevice.type],
                            currentDevice.ip.toString(),
                            QString( QUrl::toPercentEncoding( mediaFile ) ),
                            QString::number( fontSize ) );

        QNetworkReply *reply = netMan.get( QNetworkRequest( req ) );
        connect( reply, SIGNAL( finished() ), this, SLOT( onReqestFinish() ) );
        qDebug() << req << "stat req";

    }
    else if ( key == "DSTAT" ) handleCastingSwitch( value == "CONNECTED" );
    else if ( key == "MSTAT" ) parseMStat( value );
    else if ( key == "ERR" )
    {
        qDebug() << "parseCast: ERR" << value;
        stop();
        stateText( "Casting error" );
    }
}

void ChromeCast::onListItemClicked( QListWidgetItem *item )
{
    popup->hide();
    if ( item->text() == currentDevice.name ) return;
    stop();
    currentDevice = CastDevice();
    if ( item->text() == CHROMECAST_DEVICE_TEXT_LOCAL ) return;
    foreach( CastDevice dev, devs )
    {
        if ( item->text() != dev.name ) continue;
        currentDevice = dev;
        btnMovie->start();
        castProc->restart();
//      qDebug() << "selected" << dev.name << "at" << dev.ip;
        emit deviceSelected();
        return;
    }
}

QNetworkReply* ChromeCast::issueCommand( QByteArray command, QByteArray args )
{
    if ( m_state != Paused && m_state != Playing ) return 0;

    static QString reqArgs = "?deviceType=%1&deviceIP=%2";
    command += reqArgs.arg( CAST_DEVICETYPE_STRING[currentDevice.type], currentDevice.ip.toString() );
    if ( args.size() && args.left( 1 ) != "&" ) args.prepend( "&" );
//                          currentDevice.ip.toString() );
    command += args;

    QNetworkReply *reply = netMan.get( QNetworkRequest( serverUrl.toString() + command ) );
    if ( !command.contains( "command/mediaState" ) ) CC_DEBUG << "CC command" << reply->request().url();
    connect( reply, SIGNAL( finished() ), this, SLOT( onReqestFinish() ) );
    QThread::msleep( 50 );
    return reply;
}

void ChromeCast::onReqestFinish()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
//  if ( reply ) qDebug() << "reply to" << reply->request().url().path() << ":" << reply->readAll();
    reply->deleteLater();
}

void ChromeCast::setState( State newState )
{
    if ( state() == newState ) return;
    qDebug() << "setState" << newState << "from" << m_state;
    State oldState = state();
    m_state = newState;
    if ( oldState == Opening && ( newState == Paused || newState == Playing ) )
    {
        setSubtitleFileV( subtitleFile );
        if ( startTime ) QTimer::singleShot( 5000, [=]() {CC_DEBUG << "CC first buffering - seeking startTime" << startTime; issueCommand( "command/seek", "seek=" + QByteArray::number( double( startTime ) / 1000 ) ); startTime = 0;} );
    }

    if ( newState == Idle )
    {
        startTime = 0;
        emit stopped();
    }
    else if ( newState == Opening ) emit opening();
    else if ( newState == Playing && oldState != Buffering ) emit playing();
    else if ( newState == Paused ) emit paused();

    emit stateChanged();
}



const QStringList& ChromeCast::getSupportedSubsList()
{
    static const QStringList data = QStringList() << "srt";
    return data;
}

