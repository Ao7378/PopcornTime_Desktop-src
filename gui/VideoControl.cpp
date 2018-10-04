#include <QBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QMetaMethod>
#include <QApplication>
#include <QScreen>
#include "VideoPlayer.h"
#include "defaults.h"
#include "SeekSlider.h"
#include "VideoControl.h"
#include "GlyphButton.h"
#include "ChromeCast.h"

#include "SubComboBox.h"
#include "TrackComboBox.h"
#include "Subtitler.h"
#include "Zoom4k.h"

#include <QPainter>
#include <QFontDatabase>
#include <QMenu>
#include <QWidgetAction>
#include <QDoubleSpinBox>
//class VolumeSlider : public VlcWidgetVolumeSlider
//{
//protected:
//    void mousePressEvent( QMouseEvent *event ) { event->accept();  }
//    void mouseReleaseEvent( QMouseEvent *event ) { event->ignore(); }
//};

void VolumeSlider::paintEvent( QPaintEvent *event )
{
    QSlider::paintEvent( event );
    QStyleOptionSlider opt;
    initStyleOption( &opt );
    const QRect gRect = style()->subControlRect( QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this );
    const QRect hRect = style()->subControlRect( QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this );

    const int zoomFactor = Zoom4k::zoomFactorInt();

    QPainter p( this );
    QBrush brush = QBrush( QColor( 47, 111, 214 ) );
    const int left = gRect.left();
    const int top = gRect.top() + gRect.height() / 2 - 3 * zoomFactor;
    const int height = 6 * zoomFactor; //gRect.height();
    const int length = hRect.left() - gRect.left();
    if ( length < 1 ) return;

    p.fillRect( QRect( left, top, length, height ), brush );
}




VideoControl::VideoControl( QWidget *parent )
   : QFrame( parent ), m_statusLabel( new QLabel )

{
    QFont *font = 0;
    const int fontId = QFontDatabase::addApplicationFont( ":/controls.ttf" );
    if ( fontId >= 0 )
    {
        QString family = QFontDatabase::applicationFontFamilies( fontId ).at( 0 );
        font = new QFont( family );

        qreal screenDPI = QApplication::primaryScreen()->physicalDotsPerInch();

#ifdef WINDOWS
        qreal RENDER_DPI = 96;
#else
        qreal RENDER_DPI = 72;
#endif

        int pixelSize = (int) ((qreal) 16 * screenDPI / RENDER_DPI);

        font->setPixelSize( pixelSize * Zoom4k::zoomFactorInt() );
    }

    this->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );

    m_hidableFrame = new QFrame();
    (new QVBoxLayout( this )) ->addWidget( m_hidableFrame );
    this->layout()->setContentsMargins( 0, 0, 0, 0 );
    this->layout()->setSpacing( 0 );

    QVBoxLayout *vl = new QVBoxLayout( m_hidableFrame );
    vl->setContentsMargins( 0, 0, 0, 0 );
    vl->setSpacing( 0 );

    QLabel *byteRateLabel = new QLabel;
    QLabel *elapsedLabel = new QLabel;
    QLabel *fullLabel = new QLabel;

    m_seek = new SeekControl( elapsedLabel, fullLabel, byteRateLabel );
    vl->addWidget( m_seek );
    connect( m_seek, SIGNAL( updateStatusLabel( const QString& ) ), this, SLOT( updateLabel( const QString& ) ) );
    connect( m_seek, SIGNAL( forcePauseonBuffering() ), this, SLOT( onPauseReq() ) );
    connect( m_seek, SIGNAL( timeOnRewind( int ) ), this, SIGNAL( timeOnRewind( int ) ) );

    QHBoxLayout *hl = new QHBoxLayout();
    vl->addLayout( hl );
    hl->setContentsMargins( 10, 0, 10, 0 );
    hl->setSpacing( 15 );

    m_pauseBtn = new GlyphButton( *font, QChar( 0xF04C ), QChar( 0xF04B ) );
    hl->addWidget( m_pauseBtn );

    m_stopBtn = new GlyphButton( *font, QChar( 0xF04D ) );
    hl->addWidget( m_stopBtn );

    hl->addWidget( elapsedLabel );
    hl->addWidget( fullLabel );

    m_muteBtn = new GlyphButton( *font, QChar( 0xF028 ), QChar( 0xF026 ) );
    hl->addWidget( m_muteBtn );

    m_vol = new VolumeSlider();
    m_vol->setMaximum( 200 );
    m_vol->setValue( m_vol->maximum() );
    hl->addWidget( m_vol );
    connect( m_vol, SIGNAL( valueChanged( int ) ), this, SIGNAL( volumeChanged( int ) ) );

    hl->addWidget( m_statusLabel );

    hl->addSpacerItem( new QSpacerItem( 20, 0, QSizePolicy::Expanding, QSizePolicy::Expanding ));

    QLabel *lbl = new QLabel( "Downloading" );
    hl->addWidget( lbl );

    hl->addWidget( byteRateLabel );

    hl->addSpacerItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding ));


    m_subsBtn = new GlyphButton( *font, QChar( 0xF075 ), QChar( 0xF075 ) );
//  m_subsBtn = new GlyphButton( "balloon", "balloon-over" );
    hl->addWidget( m_subsBtn );
    connect( m_subsBtn, SIGNAL( clicked() ), this, SLOT( showSubsMenu() ) );

    m_tracks = new TTrackComboBox;
    m_subs = new SubComboBox;
    m_subsVars = new VariantComboBox;
    m_subtitler = new Subtitler( parent, *m_subs, *m_subsVars );
    connect( m_subtitler, SIGNAL( haveSubtitleJson( QJsonObject ) ), m_rating, SLOT( setSubtitleJson( QJsonObject ) ) );


    m_subsDelay = new QDoubleSpinBox();
    m_subsDelay->setDecimals( 1 );
    m_subsDelay->setRange( -60 * 60, 60 * 60 );
    m_subsDelay->setSingleStep( 0.5 );
    m_subsDelay->setSuffix( " s" );
    m_subsDelay->setStyleSheet( "background-color:black; color:#787878; padding: 2px 20px 2px 1px;" );

    connect( m_subsDelay,
            static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            [=]( double d )
    {
        if ( !subtitleDelayTimer.isActive() )
        {
            subtitleDelayTimer.start();
            emit subtitleDelay( d * 1000000 );
            qDebug() << "subtitleDelayTimer start";
        }
        else qDebug() << "subtitleDelayTimer active";

    });
    subtitleDelayTimer.setSingleShot( true );
    subtitleDelayTimer.setInterval( 3000 );
    connect( &subtitleDelayTimer, &QTimer::timeout, [=]() {emit subtitleDelay( m_subsDelay->value() * 1000000 ); qDebug() << "subtitleDelayTimer timeout";} );



    auto subsMenu = new QMenu( m_subsBtn );

    auto subsAction = new QWidgetAction( subsMenu );
    subsAction->setDefaultWidget( m_subs );
    subsMenu->addAction( subsAction );
    connect( m_subs, SIGNAL( activated( const QString& ) ), subsMenu, SLOT( close() ) );

    auto subsVarAction = new QWidgetAction( subsMenu );
    subsVarAction->setObjectName( "subsVarAction" );
    subsVarAction->setDefaultWidget( m_subsVars );
    subsMenu->addAction( subsVarAction );
    connect( m_subsVars, SIGNAL( activated( const QString& ) ), subsMenu, SLOT( close() ) );

    auto tracksAction = new QWidgetAction( subsMenu );
    tracksAction->setDefaultWidget( m_tracks );
    subsMenu->addAction( tracksAction );
    connect( m_tracks, SIGNAL( activated( const QString& ) ), subsMenu, SLOT( close() ) );
    auto subsDelayAction = new QWidgetAction( subsMenu );

    auto delayFrame = new QFrame;
    auto delayLabel = new QLabel( delayFrame );
    delayLabel->setText( "Subtitle delay" );
    delayLabel->setStyleSheet( "QLabel {padding-bottom: 0px;}" );

    auto delayLayout = new QHBoxLayout( delayFrame );
    delayLayout->setContentsMargins( 2, 0, 3, 0 );
    delayLayout->setSpacing( 5 );
    delayLayout->addWidget( delayLabel );
    delayLayout->addWidget( m_subsDelay );

    subsDelayAction->setDefaultWidget( delayFrame );
    subsMenu->addAction( subsDelayAction );

//  subsMenu->setStyleSheet( "QMenu {background-color:black; color:#cacaca; border:1px solid #666666; font:10pt;} " );//padding:0px; margin:0px;
//  subsBtn->setStyleSheet( "padding-bottom: 4px;" );



#ifndef NO_ASPECT_RATIO
    m_setsBtn = new GlyphButton( *font, QChar( 0xF013 ), QChar( 0xF013 ) );
//  m_setsBtn = new GlyphButton( "settings", "settings-over" );
    hl->addWidget( m_setsBtn );


    auto menu = new QMenu( m_setsBtn );
    connect( m_setsBtn, SIGNAL( clicked() ), this, SLOT( showSubsMenu() ) );
//  setsBtn->setMenu( menu );
    m_aspectAction = menu->addAction( QString( "Aspect Ratio: " ) + "default" );
//  menu->setStyleSheet( "QMenu {background-color:black; color:#cacaca; border:1px solid #666666; padding:0px; margin:0px; font:10pt;} " );
//  setsBtn->setStyleSheet( "padding-bottom: 4px; QPushButton::menu-indicator { subcontrol-origin: padding; margin-right: -60px}" );
#endif



    m_castBtn = new GlyphButton( "chromecast_off", "chromecast_off_over", "chromecast_on", "chromecast_on_over" ); //*font, QChar( 0xE604 ) );
//  m_castBtn->setStyleSheet( "padding-bottom: 4px;" );
    m_castBtn->setIconSize( QSize( 16, 16 ) );
    hl->addWidget( m_castBtn );
    m_fullscrBtn = new GlyphButton( "full-scr", "full-scr-over", "exit-full-scr", "exit-full-scr-over" );
    m_fullscrBtn = new GlyphButton( *font, QChar( 0xE601 ), QChar( 0xE602 ) );
    m_fullscrBtn->setStyleSheet( "padding-bottom: 10px;" );
    hl->addWidget( m_fullscrBtn );

    reconnectMediaPlayer();

    clearSubtitles();

//  m_hidableFrame->setStyleSheet( ".QFrame{ background-color:red; color:gray; border-color:blue; }" );
//  m_seek->setStyleSheet( "QWidget{ background-color:yellow; border-color:white; }" );
//  m_vol->setStyleSheet( " QSlider*{ background-color:blue; } " );

    updateZoom();

}


VideoControl::~VideoControl()
{
//  qDebug() << "VideoControl destructor";
}

void VideoControl::reconnectMediaPlayer()
{
    localPlayer = VideoPlayer::PlayerObject().data();
    qDebug() << this->metaObject()->className() << __FUNCTION__ << localPlayer;
    if ( !localPlayer ) return;
    currentPlayer = localPlayer;
//
    connect( localPlayer, SIGNAL( end() ), m_seek, SLOT( end() ) );
//
}

void VideoControl::castSwitch( bool casting )
{
    QObject *player = cast;
    QObject *idlePlayer = localPlayer;
    if ( !casting || !player )
    {
        idlePlayer = cast;
        player = localPlayer;
    }
    currentPlayer = player;
    qDebug() << this->metaObject()->className() << __FUNCTION__ << casting << sender();

    while ( connList.size() ) QObject::disconnect( connList.takeFirst() );

    connList += connect( player, SIGNAL( lengthChanged( int ) ), m_seek, SLOT( updateFullTime( int ) ) );
    connList += connect( player, SIGNAL( lengthChanged( int ) ), m_rating, SLOT( updateFullTime( int ) ) );
    connList += connect( player, SIGNAL( timeChanged( int ) ), m_seek, SLOT( updateCurrentTime( int ) ) );
    connList += connect( player, SIGNAL( timeChanged( int ) ), idlePlayer, SLOT( setTime( int ) ) );
//  connList += connect( player, SIGNAL( stopped() ), m_seek, SLOT( rewind() ) );
    connList += connect( player, SIGNAL( stateText( const QString& ) ), this, SLOT( updateLabel( const QString& ) ) );
    connList += connect( m_seek, SIGNAL( sought( int ) ), player, SLOT( setTime( int ) ) );
    connList += connect( m_seek, SIGNAL( needTextUpdate() ), player, SLOT( onStateChange() ) );

    connList += connect( m_pauseBtn, SIGNAL( clicked( bool ) ), player, SLOT( setPause( bool ) ) );
    connList += QObject::connect( player, SIGNAL( playing() ), m_pauseBtn, SLOT( uncheck() ) );
    connList += QObject::connect( player, SIGNAL( stopped() ), m_pauseBtn, SLOT( check() ) );
    connList += connect( m_stopBtn, SIGNAL( clicked() ), player, SLOT( stop() ) );

    connList += connect( m_muteBtn, SIGNAL( toggled( bool ) ), player, SLOT( setMute( bool ) ) );
    connList += connect( m_vol, SIGNAL( valueChanged( int ) ), player, SLOT( setVolume( int ) ) );
    connList += connect( player, SIGNAL( stateChanged() ), this, SLOT( provideControlSettings() ) );
    connList += connect( player, SIGNAL( stateImage( QString ) ), this, SIGNAL( newImage( QString ) ) );
    connect( localPlayer, SIGNAL( playing() ), m_subtitler, SLOT( provide() ) );

    connList += connect( m_subtitler, SIGNAL( haveSubtitleFile( QString ) ), player, SLOT( setSubtitleFileV( QString ) ) );
    connList += connect( this, SIGNAL( subtitleDelay( quint64 ) ), player, SLOT( setSubtitleDelay( quint64 ) ) );
    connList += connect( m_tracks, SIGNAL( newTrackIndex( int ) ), player, SLOT( setAudioTrack( int ) ) );
    connList += connect( player, SIGNAL( newAudioTrackList( TIndexedString, int ) ), m_tracks, SLOT( setAudioTrackList( TIndexedString, int ) ) );

    provideControlSettings();

    m_seek->setArbitrarySeek( player == localPlayer );

    if ( player == localPlayer )
    {
        //      qDebug() << "Switching to VLC";
#ifndef NO_ASPECT_RATIO
        connList += connect( m_aspectAction, SIGNAL( triggered( bool ) ), player, SLOT( nextAspectRatio() ) );
        connList += connect( player, SIGNAL( newAspectRatio( QString ) ), this, SLOT( updateAspectRatio( QString ) ) );
#endif
        QMetaObject::invokeMethod( player, "provideAspectRatio" );

        m_subtitler->filterSubtitles( VideoPlayer::getSupportedSubsList() );
        emit newImage( "" );
        emit switchedtoVLC();
        QTimer::singleShot( 500, [=]() {qDebug() << this->metaObject()->className() << __FUNCTION__ << "unpause"; QMetaObject::invokeMethod( localPlayer, "setPause", Q_ARG( bool, false ) );} );
    }
    else
    {
//      qDebug() << "Switching to Cast";
        m_subtitler->filterSubtitles( ChromeCast::getSupportedSubsList() );
        QMetaObject::invokeMethod( localPlayer, "setPause", Q_ARG( bool, true ) );
        emit switchedtoCast();
    }
    m_pauseBtn->provide();
}

void VideoControl::provideControlSettings()
{
    m_vol->provide();
//  m_pauseBtn->provide();
    m_muteBtn->provide();
}


QPushButton* VideoControl::fullscreenButton() { return m_fullscrBtn; }
void VideoControl::updateAspectRatio( QString text )
{
#ifndef NO_ASPECT_RATIO
    m_aspectAction->setText( "Aspect Ratio: " + text );
#else
    (void) text;
#endif
};


void VideoControl::showSubsMenu()
{
    qDebug() << this->metaObject()->className() << __FUNCTION__;
    auto btn = qobject_cast<QPushButton *>( sender() );
    if ( !btn ) return;

    QPoint pos = btn->mapToGlobal( QPoint( 0, btn->height() ) );
    QMenu *menu = btn->findChild<QMenu *>();

    if ( !menu ) return;
    menu->popup( pos );
    qDebug() << this->metaObject()->className() << __FUNCTION__ << pos;
}

void VideoControl::volumeUp() { m_vol->triggerAction( QAbstractSlider::SliderPageStepAdd ); };
void VideoControl::volumeDown() { m_vol->triggerAction( QAbstractSlider::SliderPageStepSub ); };
void VideoControl::setVolume( int volume ) { m_vol->setValue( volume ); }
void VideoControl::pauseToggleReq() { m_pauseBtn->click(); };
void VideoControl::pauseToggled( bool paused )
{
    if ( paused == true ) return;
//  qDebug() << "pauseToggled" << paused << currentPlayer;
    if ( currentPlayer )
    {
        qDebug() << "invokeMethod" << currentPlayer << "open";
        QMetaObject::invokeMethod( currentPlayer, "open", Qt::QueuedConnection, Q_ARG( QString, "" ) );
    }
}

void VideoControl::clearSubtitles()
{
    m_subtitler->clearSubtitles();
    m_rating->clear();
    m_subsDelay->setValue( 0 );
}

void VideoControl::startSubtitles( TorrentInfo info )
{
    m_subtitler->startSubtitles( info );
    m_rating->setTorrentJson( info.getTorrentDesc() );
    m_subsDelay->setValue( 0 );
}

void VideoControl::updateDownloadStatus( DownloadInfo info )
{
    m_seek->downloadInfoChange( info );
    m_rating->downloadInfoChange( info );
}

void VideoControl::uiShow()
{
#ifdef Q_OS_MAC
    m_hidableFrame->show();
#else
    show();
#endif
}

void VideoControl::uiHide()
{
#ifdef Q_OS_MAC
    m_hidableFrame->hide();
#else
    hide();
#endif
}

void VideoControl::setCast( QObject *chromeCast )
{
    cast = chromeCast;
    connect( m_seek, SIGNAL( forcePauseonBuffering() ), cast, SLOT( pause() ) );
    castSwitch( false );
}


bool VideoControl::panelsVisible()
{
#ifdef Q_OS_MAC
    return m_hidableFrame->isVisible();
#else
    return isVisible();
#endif
}

void VideoControl::updateZoom()
{
    const float zoomFactor = Zoom4k::zoomFactorPercent() / 100;
    const bool k4 = Zoom4k::is4k();
    const QString rightButtons = QString( "QPushButton:flat { border: none;color:#787878; } "
                                          "QPushButton:flat:hover { border: none;color:white; } "
                                          "QPushButton{font:%1pt;padding-bottom: 3px;}" ).arg( ( k4 ? 12 : 16 ) * ( 1 + ( zoomFactor - 1 ) / 2 ) );
    m_subsBtn->setStyleSheet( rightButtons );
#ifndef NO_ASPECT_RATIO
    m_setsBtn->setStyleSheet( rightButtons );
#endif
    QFrame::setStyleSheet( QString( "QFrame{background-color:black; color:#787878;}"
                                    "QMenu {background-color:black; color:#787878; border:1px solid #666666; font:%1pt;}"
                                    "QDoubleSpinBox {font:%1pt;}"
                                    "TTrackComboBox {font:%1pt;}"
                                    "QLabel {font:%1pt; padding-bottom: %2px; }"
                                    "SubComboBox {font:%1pt;}"
                                    "QPushButton {padding-bottom: %3px; font:%4pt }" )
                           .arg( ( k4 ? 7 : 10 ) * zoomFactor )
                           .arg( 5 * zoomFactor )
                           .arg( ( k4 ? 1 : 4 ) * zoomFactor )
                           .arg( ( k4 ? 8 : 16 ) * zoomFactor ) );
    m_fullscrBtn->setStyleSheet( QString( "QPushButton:flat { border: none;color:#787878; } "
                                          "QPushButton:flat:hover { border: none;color:white; } "
                                          "QPushButton{font:%1pt;padding-top: %2px;}" ).arg( ( k4 ? 15 : 20 ) * ( 1 + ( zoomFactor - 1 ) / 2 ) ).arg( 5 * zoomFactor ) );

    m_vol->setStyleSheet( QString(
                             "QSlider { margin: 0px; }"
                             "QSlider::groove:horizontal {"
                             " border: 0px;"
                             " height: %1px; /* the groove expands to the size of the slider by default. by giving it a height, it has a fixed size */"
                             " background: #999999;"
                             " margin: %2px 0; }"
                             "QSlider::handle:horizontal {"
                             " background: white;"
                             " border: 0px solid #5c5c5c;"
                             " width: %3px;"
                             " margin: -%2px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */"
                             " border-radius: %4px;}" ).arg( 6 * zoomFactor ).arg( 2 * zoomFactor ).arg( 10 * zoomFactor ).arg( 5 * zoomFactor ) );
    m_vol->setFixedWidth( 100 * zoomFactor );
    m_vol->setMinimumHeight( 12 * zoomFactor );


//  m_pauseBtn->updateZoom( k4 ? 1 : zoomFactor );
//  m_stopBtn->updateZoom( k4 ? 1 : zoomFactor );
//  m_muteBtn->updateZoom( k4 ? 1 : zoomFactor );
//  m_subsBtn->updateZoom( zoomFactor );
//  m_setsBtn->updateZoom( zoomFactor );
    qreal screenDPI = QApplication::primaryScreen()->logicalDotsPerInch();
#ifdef WINDOWS
    qreal RENDER_DPI = 96;
#else
    qreal RENDER_DPI = 72;
#endif

    qreal zoom = (zoomFactor - 1) / screenDPI * RENDER_DPI / 1.3;
    m_castBtn->updateZoom( k4 ? 2 : 1 + zoom );
    m_fullscrBtn->updateZoom( k4 ? 2 : zoomFactor * 1.4 + ( zoomFactor - 1 ) * -0.4 );
    qDebug() << this->metaObject()->className() << __FUNCTION__ << zoomFactor << screenDPI / RENDER_DPI << zoom;
    m_seek->updateZoom( zoomFactor );
}


void VideoControl::updateLabel( const QString& newText )
{
    if ( newText.contains( "Paused", Qt::CaseInsensitive ) && m_statusLabel->text() == TK_NET_BUFFERING ) return;
    m_statusLabel->setText( newText );
}


void VideoControl::enterEvent( QEvent *event )
{
    QFrame::enterEvent( event );
    emit mouseEnter();
//  qDebug() << "enterEvent";
}

void VideoControl::leaveEvent( QEvent *event )
{
    QFrame::leaveEvent( event );
    emit mouseLeave();
//  qDebug() << "leaveEvent";
}
