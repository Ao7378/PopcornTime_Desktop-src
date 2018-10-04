#ifdef USE_QCEF
    #include "cefclient/qcefwebview.h"
    #undef max // Avoid Windows.h bug
#elif defined USE_WEBENGINE
    #include "WebEngineWebView.h"
#else
    #include "GraphicsBrowser.h"
    #include <QGraphicsWebView>
    #include <QWebFrame>
    #include <QWebPage>
#endif

#include "FramelessMainWindow.h"
#include "defaults.h"
#include "hostApp.h"
#include "VideoControl.h"
#include "ControlRevealer.h"
#include "Zoom4k.h"
#include "widgets.h"
#include "DropHandler.h"

#include <QApplication>
#include <QDir>
#include <QShortcut>
#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QPainter>
#include <QDesktopWidget>
#include "UrlSelector.h"


#include "VideoPlayer.h"
#include "TorrentWrapper.h"
#ifndef NO_TORRENT_IO_WRAPPER
    #include "TorrentIODevice.h"
    #include "GlyphButton.h"
#endif

FramelessMainWindow::FramelessMainWindow( QWidget *parent ) :
   QMainWindow( parent ),
#if defined USE_QCEF
   m_browser( new QCefWebView( this ) ),
   m_hostApp( new HostApp( m_browser, this )) ,
#elif defined USE_WEBENGINE
   m_browser( new WebEngineView( this )) ,
m_hostApp( new HostApp( m_browser->page(), this )) ,
#else
   m_scene( new QGraphicsScene( this ) ),
   m_browser( new GraphicsBrowser( m_scene, *this )) ,
m_hostApp( new HostApp( m_browser->webView()->page()->mainFrame(), this ) ),
#endif
#if defined USE_QCEF || defined USE_WEBENGINE
m_resizeTop( new TransparentWidget( this )) ,
m_resizeLeft( new TransparentWidget( this ) ),
m_resizeRight( new TransparentWidget( this ) ),
m_resizeBottom( new TransparentWidget( this )) ,
m_dragZone1( new TransparentWidget( this )) ,
m_dragZone2( new TransparentWidget( this )) ,
#endif
m_mouseFilter( new ResizeDragEventHandler( *this, *qApp )) ,
m_video( new VideoWidget( this, ":/popcorntime_icon" )) ,
m_intro( new IntroWidget( this )) ,
m_control( new VideoControl( this )) ,
m_closeControl( new QFrame( this )) ,
m_cCast( new ChromeCast( this, m_control->castButton() ) ),
m_revealer( new ControlRevealer( this )) ,
{
    this->setWindowTitle( "Popcorn Time" );
    this->setStyleSheet( "QMainWindow{background-color:black; color:#999999;}" );
    VideoPlayer::PlayerObject()->setVideoWidget( m_video->getVOut() );
    VideoPlayer::PlayerObject()->setTorrentWrapper( m_hostApp->torrentEngine() );
    m_video->hide();
    m_control->hide();

    m_browser->setObjectName( "MainBrowser" );

//  m_browser->webView()->page()->settings()->setAttribute( QWebSettings::JavascriptCanOpenWindows, true );

    m_intro->show();
    this->setCentralWidget( m_intro );


    m_control->setVolume( m_hostApp->getVolume() );
    m_control->setCast( m_cCast );

    m_closeControl->hide();

    QLabel *m_movieName = new QLabel;
    m_movieName->setText( "Player" );

    QHBoxLayout *hl = new QHBoxLayout( m_closeControl );
    hl->setContentsMargins( QMargins() );
    hl->addSpacerItem( new QSpacerItem( 10, 0, QSizePolicy::Minimum, QSizePolicy::Minimum ));
    hl->addWidget( m_movieName );
    hl->addSpacerItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum ));

    m_closeButton = new QPushButton();
    m_closeButton->setCursor( Qt::PointingHandCursor );
//  btn->setFlat( true );
    hl->addWidget( m_closeButton );


    new QShortcut( QKeySequence( "Ctrl+M" ), this, SLOT( minimize() ) );

#if defined USE_QCEF || defined USE_WEBENGINE
    QObject::connect( m_browser, SIGNAL( loadFinished( bool ) ),
#else
    QObject::connect (m_browser->webView () , SIGNAL (loadFinished (bool) ) ,
#endif
                     this, SLOT( removeIntro() ) );

#ifndef NO_TORRENT_IO_WRAPPER
    QObject::connect( VideoPlayer::PlayerObject()->getTorrentDevice(), SIGNAL( bufferUnderrun() ), m_video, SLOT( onBufferingStart() ) );
    QObject::connect( VideoPlayer::PlayerObject()->getTorrentDevice(), SIGNAL( buffered() ), m_video, SLOT( onBufferingEnd() ) );
#endif

    QObject::connect( m_hostApp->torrentEngine(), SIGNAL( torrentDataAvailable( QString ) ), VideoPlayer::PlayerObject().data(), SLOT( openFile( QString ) ) );
    QObject::connect( m_hostApp->torrentEngine(), SIGNAL( torrentDataAvailable( QString ) ), this, SLOT( playbackStarted() ) );
    QObject::connect( m_hostApp->torrentEngine(), SIGNAL( torrentDataAvailable( QString ) ), m_cCast, SLOT( setMediaFile( QString ) ) );
    QObject::connect( m_hostApp->torrentEngine(), SIGNAL( startSubs( TorrentInfo ) ), m_control, SLOT( startSubtitles( TorrentInfo ) ) );
    QObject::connect( m_hostApp->torrentEngine(), SIGNAL( leechingStarted( int ) ), m_control, SLOT( clearSubtitles() ) );
    QObject::connect( m_hostApp->torrentEngine(), SIGNAL( downloadInfoChanged( DownloadInfo ) ), m_control, SLOT( updateDownloadStatus( DownloadInfo ) ) );
    QObject::connect( m_hostApp->torrentEngine(), SIGNAL( newMovieName( QString ) ), m_movieName, SLOT( setText( QString ) ) );

    QObject::connect( m_hostApp, SIGNAL( commandedClose() ), this, SLOT( close() ) );
    QObject::connect( m_hostApp, SIGNAL( commandedMinimize() ), this, SLOT( minimize() ) );
    QObject::connect( m_hostApp, SIGNAL( toggledMaximize() ), this, SLOT( toggleMaximize() ) );
    QObject::connect( m_hostApp, &HostApp::initCompleted, this, &FramelessMainWindow::checkCommandLine );
    QObject::connect( this, &FramelessMainWindow::commandLineTorrent, m_hostApp, &HostApp::showLoading );
    QObject::connect( this, &FramelessMainWindow::commandLineTorrent, m_hostApp->torrentEngine(), &TorrentWrapper::get );

    QObject::connect( m_hostApp, SIGNAL( fontSizeChanged( int ) ), m_cCast, SLOT( setFontSize( int ) ) );
    QObject::connect( m_hostApp, SIGNAL( fontSizeChanged( int ) ), VideoPlayer::PlayerObject().data(), SLOT( setFontSize( int ) ) );


    QObject::connect( m_control, SIGNAL( volumeChanged( int ) ), m_hostApp, SLOT( setVolume( int ) ) );

    QObject::connect( m_mouseFilter, SIGNAL( applicationStateInactive( bool ) ), m_hostApp, SLOT( setAnimations( bool ) ) );
    QObject::connect( m_mouseFilter, &ResizeDragEventHandler::torrentDropped, m_hostApp, &HostApp::showLoading );
    QObject::connect( m_mouseFilter, &ResizeDragEventHandler::torrentDropped, m_hostApp->torrentEngine(), &TorrentWrapper::get );

    QObject::connect( m_control, SIGNAL( newImage( QString ) ), m_video, SLOT( setImage( QString ) ) );

    QObject::connect( m_control->fullscreenButton(), SIGNAL( clicked() ), this, SLOT( toggleMaximize() ) );
    QObject::connect( m_mouseFilter, SIGNAL( enterPressed() ), this, SLOT( toggleMaximize() ) );
    QObject::connect( m_mouseFilter, SIGNAL( escapePressed() ), this, SLOT( escapePressed() ) );
    QObject::connect( m_mouseFilter, SIGNAL( upPressed() ), m_control, SLOT( volumeUp() ) );
    QObject::connect( m_mouseFilter, SIGNAL( downPressed() ), m_control, SLOT( volumeDown() ) );
    QObject::connect( m_mouseFilter, SIGNAL( pausePressed() ), m_control, SLOT( pauseToggleReq() ) );
    QObject::connect( m_video, SIGNAL( clicked() ), m_control, SLOT( pauseToggleReq() ) );

    QObject::connect( m_control, SIGNAL( timeOnRewind( int ) ), m_cCast, SLOT( setStartTime( int ) ) );
    QObject::connect( m_cCast, SIGNAL( castingSwitched( bool ) ), m_control, SLOT( castSwitch( bool ) ) );

    QObject::connect( m_revealer, SIGNAL( revealed() ), m_control, SLOT( uiShow() ) );
    QObject::connect( m_revealer, SIGNAL( concealed() ), m_control, SLOT( uiHide() ) );
    QObject::connect( m_revealer, SIGNAL( revealed() ), this, SLOT( uiShow() ) );
    QObject::connect( m_revealer, SIGNAL( concealed() ), this, SLOT( uiHide() ) );

    connect( m_control, SIGNAL( switchedtoCast() ), m_revealer, SLOT( startPermanentReveal() ) );
    connect( m_control, SIGNAL( switchedtoVLC() ), m_revealer, SLOT( stopPermanentReveal() ) );
    connect( m_control, SIGNAL( mouseEnter() ), m_revealer, SLOT( startPermanentReveal() ) );
    connect( m_control, SIGNAL( mouseLeave() ), m_revealer, SLOT( stopPermanentReveal() ) );

    connect( m_closeButton, SIGNAL( clicked() ), this, SLOT( stopPlayback() ) );
    connect( m_hostApp, SIGNAL( commandedClosePlayer() ), this, SLOT( stopPlayback() ) );
    connect( this, SIGNAL( stoppingPlayback() ), m_revealer, SLOT( disableReveal() ) );
    connect( this, SIGNAL( stoppingPlayback() ), m_cCast, SLOT( stop() ) );
    connect( this, SIGNAL( stoppingPlayback() ), VideoPlayer::PlayerObject().data(), SLOT( closeFile() ) );
    connect( this, SIGNAL( stoppingPlayback() ), this, SLOT( playbackStopped() ) );
    connect( this, SIGNAL( stoppingPlayback() ), m_hostApp, SLOT( cancelTorrent() ) );

    connect( VideoPlayer::PlayerObject().data(), SIGNAL( playing() ), this, SLOT( updateVideoSize() ) );
    connect( VideoPlayer::PlayerObject().data(), SIGNAL( paused() ), m_revealer, SLOT( temporaryReveal() ) );
    this->setWindowFlags( Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint );
#ifdef Q_OS_MAC
    fixNativeWindow( this );
#endif
    this->setMinimumSize( 800, 330 );
    this->setMouseTracking( true );
    qApp->installEventFilter( this );

    reconnectMediaPlayer();

    QObject::connect( VLC::VLCObject(), SIGNAL( mediaPlayerReplaced() ),
                      this, SLOT( reconnectMediaPlayer() ) );
      loadUrl( "STARTUP_URL" );
#ifdef USE_QCEF
    QObject::connect( m_browser, SIGNAL( haveWindow( QWindow * ) ),
                      this, SLOT( onCefWindowCreated( QWindow * ) ) );
#elif defined USE_WEBENGINE
    WebEngineView::hostApp = m_hostApp;
#endif
}

void FramelessMainWindow::onCefWindowCreated( QWindow *cefWindow )
{
    qDebug()<< this->metaObject()->className()<< __FUNCTION__ << QThread::currentThread()<< cefWindow;
    if ( !cefWindow ) return;
}
FramelessMainWindow::~FramelessMainWindow()
{
    if ( VLC::_player ) VLC::_player->closeFile();
#ifdef Q_OS_WIN
    QProcess::execute( "taskkill /im node.exe /f" );
#else
    QProcess::execute( "pkill node" );
#endif
}

bool FramelessMainWindow::isPlayerShown() { return m_video->isVisible(); }

void FramelessMainWindow::loadUrl( QString url )
{
//  qDebug() << __FUNCTION__ << url;
#ifdef USE_QCEF
    m_browser->load( url );
#elif defined USE_WEBENGINE
    m_browser->load( url );
#else
    m_browser->webView()->load( url );
#endif
}





void FramelessMainWindow::checkCommandLine()
{
    if ( isPlayerShown() ) return;

    QStringList args = qApp->arguments();
    TorrentInfo info;

#ifndef _DEBUGGING
    int index = args.indexOf( "--open" );
    if ( index >= 0 && args.size() > index + 1 ) info = DropHandler::getInfo( args.at( index + 1 ) );
#endif
    if ( !info.isValid() ) if ( args.size() > 1 ) info = DropHandler::getInfo( args.at( 1 ) );
    if ( info.isValid() ) emit commandLineTorrent( info );
}

void FramelessMainWindow::stopPlayback()
{
    emit stoppingPlayback();
}



void FramelessMainWindow::playbackStarted()
{
    if ( isPlayerShown() ) return;
    m_hostApp->setAnimations( false );
    connect( m_mouseFilter, SIGNAL( mouseMoved() ), m_revealer, SLOT( temporaryReveal() ) );
//  qDebug() << "playbackStarted 1";
    updateVideoSize();
    m_video->show();
    if ( isMaximized() ) showFullScreen();
    m_control->show();
    m_closeControl->show();
    m_revealer->enableReveal();
    updateVideoSize();
}

void FramelessMainWindow::playbackStopped()
{
    if ( !isPlayerShown() ) return;
    m_revealer->stopPermanentReveal();
    m_control->castSwitch( false );
    m_cCast->stop();
    m_hostApp->setAnimations( true );
    disconnect( m_mouseFilter, SIGNAL( mouseMoved() ), m_revealer, SLOT( temporaryReveal() ) );
    m_video->hide();
    qApp->restoreOverrideCursor();
    m_control->hide();
    m_closeControl->hide();
    if ( isFullScreen() ) showMaximized();
    if ( isPlayerShown() ) m_video->hide();
}

void FramelessMainWindow::toggleMaximize()
{
    const bool willGoNormal = isMaximized() || isFullScreen();
    m_control->fullscreenButton()->setChecked( !willGoNormal );
    if ( willGoNormal )
    {
        showNormal();
#if defined USE_QCEF || defined USE_WEBENGINE
        m_resizeTop->show();
        m_resizeLeft->show();
        m_resizeRight->show();
        m_resizeBottom->show();
#endif
    }
    else
    {
#if defined USE_QCEF || defined USE_WEBENGINE
        m_resizeTop->hide();
        m_resizeLeft->hide();
        m_resizeRight->hide();
        m_resizeBottom->hide();
#endif
        isPlayerShown() ? showFullScreen() : showMaximized();
    }
}

void FramelessMainWindow::minimize()
{
#ifdef Q_OS_MAC
    miniaturize( this );
#else
    this->showMinimized();
#endif
}

void FramelessMainWindow::uiShow()
{
    setCursor( Qt::ArrowCursor );
    qApp->restoreOverrideCursor();
    if ( !isPlayerShown() ) return;

    m_closeControl->show();
#ifdef Q_OS_MAC
    updateVideoSize();
#endif
}

void FramelessMainWindow::uiHide()
{
    m_closeControl->hide();
#ifdef Q_OS_MAC
    updateVideoSize();
#endif
    qApp->setOverrideCursor( QCursor( Qt::BlankCursor ) );
}

void FramelessMainWindow::escapePressed()
{
    m_revealer->temporaryReveal();
    if ( isFullScreen() ) toggleMaximize();
}
void FramelessMainWindow::fitGeometry()
{
    resize( QApplication::desktop()->availableGeometry( this ).width() * 0.8, QApplication::desktop()->availableGeometry( this ).height() * 0.8 );
    move( QApplication::desktop()->availableGeometry( this ).center() - QPoint( size().width() / 2, size().height() / 2 ) );
}

void FramelessMainWindow::resizeEvent( QResizeEvent *evt )
{
    updateVideoSize();
    updateZoom();
    QMainWindow::resizeEvent( evt );
}

void FramelessMainWindow::moveEvent( QMoveEvent *evt )
{
    updateVideoSize();
    updateZoom();
    QMainWindow::moveEvent( evt );
}

void FramelessMainWindow::checkGeometry()
{
    QRect screen = QApplication::desktop()->availableGeometry( this );
    if ( rect().width() > screen.width() || rect().height() > screen.height() )
    {
        QTimer::singleShot( 0, this, SLOT( fitGeometry() ) );
        return;
    }

    const int zoom = Zoom4k::zoomFactorInt();
    const QPoint dragTopLeft = frameGeometry().topLeft() + QPoint( DRAG_ZONE_LEFT_MARGIN * zoom, 0 );
    const QPoint dragBottomRight = frameGeometry().topLeft() + QPoint( rect().width() - DRAG_ZONE_LEFT_MARGIN * zoom - DRAG_ZONE1_RIGHT_MARGIN * zoom, DRAG_ZONE1_HEIGHT * zoom );

    for ( int i = 0; i < QApplication::desktop()->screenCount(); ++i )
    {
        const QRect screenRect = QApplication::desktop()->availableGeometry( i );
        if ( screenRect.contains( dragTopLeft ) || screenRect.contains( dragBottomRight ) ) return;
    }
    QTimer::singleShot( 0, this, SLOT( fitGeometry() ) );
}


void FramelessMainWindow::updateVideoSize()
{
    if ( !isMaximized() && !isFullScreen() ) checkGeometry();

    const QRect rect = this->rect();

#if defined USE_QCEF || defined USE_WEBENGINE
    const QPoint gPos = pos();
    const int zoom = Zoom4k::zoomFactorInt();

    m_dragZone1->move( QPoint( DRAG_ZONE_LEFT_MARGIN * zoom, 0 ) + gPos );
    m_dragZone1->resize( rect.width() - DRAG_ZONE_LEFT_MARGIN * zoom - DRAG_ZONE1_RIGHT_MARGIN * zoom, DRAG_ZONE1_HEIGHT * zoom );
    m_dragZone2->move( QPoint( DRAG_ZONE_LEFT_MARGIN * zoom, DRAG_ZONE1_HEIGHT * zoom ) + gPos );
    m_dragZone2->resize( rect.width() - DRAG_ZONE_LEFT_MARGIN * zoom - DRAG_ZONE2_RIGHT_MARGIN * zoom, DRAG_ZONE2_HEIGHT * zoom );

    if ( isMaximized() || isFullScreen() )
    {
        m_browser->move( 0, 0 );
        m_browser->resize( rect.size() );
    }
    else
    {
        const QSize vSzie( RESIZE_ZONE * zoom, rect.height() - RESIZE_ZONE * zoom * 2 );
        const QSize hSize( rect.width() - RESIZE_ZONE * zoom * 2, RESIZE_ZONE * zoom );

//      m_browser->move( 0, 0 );
//      m_browser->resize( rect.size() );
//      m_browser->move( 2, 20 );
//      m_browser->resize( rect.width() - 4, rect.height() - 22 );
        m_resizeTop->move( QPoint( RESIZE_ZONE * zoom, 0 ) + gPos );
        m_resizeTop->resize( hSize );
        m_resizeBottom->move( QPoint( RESIZE_ZONE * zoom, rect.height() - RESIZE_ZONE * zoom ) + gPos );
        m_resizeBottom->resize( hSize );
        m_resizeLeft->move( QPoint( 0, RESIZE_ZONE * zoom ) + gPos );
        m_resizeLeft->resize( vSzie );
        m_resizeRight->move( QPoint( rect.width() - RESIZE_ZONE * zoom, RESIZE_ZONE * zoom ) + gPos );
        m_resizeRight->resize( vSzie );
    }

//  m_browser->move( 50, 0 );
//  m_browser->resize( rect.width() - 100, rect.height() - 100 );
#endif

    m_closeControl->move( 0, 0 );
    m_closeControl->resize( rect.width(), 30 * zoomFactor );
    m_control->resize( rect.width(), m_control->sizeHint().height() ); //40 * Zoom4k::zoomFactorInt() ); //rect.height() / 6 ); //70 ); //
    m_control->move( 0, rect.height() - m_control->sizeHint().height() );
#ifdef Q_OS_MAC
    if ( m_control->panelsVisible() )
    {
        m_video->move( 0, m_closeControl->height() );
        m_video->resize( rect.width(), rect.height() - m_control->height() - 30 * zoomFactor );
    }
    else
    {
        m_video->move( 0, 0 );
        m_video->resize( rect.width(), rect.height() );
    }
#else
    m_video->move( 0, 0 );
    m_video->resize( rect.width(), rect.height() );
#endif
}

void FramelessMainWindow::updateZoom()
{
    const int newZoomFactor = Zoom4k::zoomFactorInt();
    if ( zoomFactor == newZoomFactor ) return;
    zoomFactor = newZoomFactor;

    m_closeControl->setStyleSheet( QString( "QWidget{ background-color:black; border:none; padding:0; spacing:0; } "
                                            "QPushButton { %3; border-width: 0px; width:%1; height:%1; } "
                                            "QLabel {color:#cacaca; padding:0px; padding-top:2px; margin:0px; font: %2pt;}" )
                                   .arg( 31 * zoomFactor )
                                   .arg( Zoom4k::is4k() ? 15 : 15 * zoomFactor - 4 )
                                   .arg( zoomFactor == 1 ? "background-image: url(':/close-styled')" : "border-image: url(':/close-styled') 0 0 0 0 stretch stretch" ) );
    m_control->updateZoom();

#ifdef USE_WEBENGINE
//  m_browser->setZoomFactor( zoomFactor )
#endif
}


void FramelessMainWindow::removeIntro()
{
    qDebug()<< __FUNCTION__ << this;
    this->setCentralWidget( m_browser );
#if defined USE_QCEF || defined USE_WEBENGINE
    QObject::disconnect( m_browser, SIGNAL( loadFinished( bool ) ),
#else
    QObject::disconnect (m_browser->webView () , SIGNAL (loadFinished (bool) ) ,
#endif
                        this, SLOT( removeIntro() ) );
    delete m_intro;
    m_intro = 0;
}


void IntroWidget::paintEvent( QPaintEvent * )
{
    QPainter painter( this );
    painter.setRenderHints( QPainter::SmoothPixmapTransform );
    static QPixmap _pixmapBg( ":/intro" );

    auto winSize = size();
    auto pixmapRatio = (float) _pixmapBg.width() / _pixmapBg.height();
    auto windowRatio = (float) winSize.width() / winSize.height();

    if ( pixmapRatio > windowRatio )
    {
        auto newWidth = (int) (winSize.height() * pixmapRatio);
        auto offset = (newWidth - winSize.width()) / -2;
        painter.drawPixmap( offset, 0, newWidth, winSize.height(), _pixmapBg );
    }
    else
    {
        auto newHeight = (int) (winSize.width() / pixmapRatio);
        auto offset = (newHeight - winSize.height()) / -2;
        painter.drawPixmap( 0, offset, winSize.width(), newHeight, _pixmapBg );
    }
}

