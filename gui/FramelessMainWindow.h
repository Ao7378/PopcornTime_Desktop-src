#ifndef __FRAMELESSMAINWINDOW_H_INCL__
    #define __FRAMELESSMAINWINDOW_H_INCL__

    #include <QMainWindow>
    #include "ResizeDragEventHandler.h"
    #include <QGraphicsScene>
    #include <QGraphicsPixmapItem>
    #include <QSharedPointer>
    #include "VideoPlayer.h"
    #include <QPointer>
    #include "ChromeCast.h"
    #include <QJsonObject>

    #ifdef USE_QCEF
class QCefWebView;
class TransparentWidget;
    #elif defined USE_WEBENGINE
class QWebEngineView;
class TransparentWidget;
    #else
class GraphicsBrowser;
    #endif

    #include <QtAV/VideoOutput.h>


class QGraphicsScene;
class HostApp;
class VideoControl;
class QMainWindow;
class QFrame;
class QPushButton;
class ControlRevealer;
class VideoWidget;
class WebEngineView;

    #ifdef Q_OS_MAC
void fixNativeWindow( QMainWindow *window );
void miniaturize( QMainWindow *window );
    #endif


class EventfulVidgetVideo : public VlcWidgetVideo
{
    Q_OBJECT

public:
    EventfulVidgetVideo( QWidget *parent = 0 );

    public slots:
    void setImage( QString resourcePng );

    signals:
    void clicked();

protected:
    virtual void mouseReleaseEvent( QMouseEvent *e );
    QWidget *_icon;
};

class IntroScene : public QGraphicsScene
{
public:
    IntroScene() : m_intro() { addItem( m_intro ); }
    ~IntroScene() { delete m_intro; }
    QGraphicsPixmapItem *m_intro;
};

class IntroWidget : public QWidget
{
public:
    IntroWidget( QWidget *parent = 0 ) : QWidget( parent ) { }

protected:
    virtual void paintEvent( QPaintEvent *event );
};
class FramelessMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    FramelessMainWindow( QWidget *parent = 0 );
    ~FramelessMainWindow();
    bool isPlayerShown();
    VideoControl* control() { return m_control; }
    HostApp* hostApp() { return m_hostApp; }

    public slots:
    void toggleMaximize();
    void updateVideoSize();
    void updateZoom();
    void loadUrl( QString url );

    void stopPlayback();
    void playbackStarted();
    void playbackStopped();

    private slots:
    void removeIntro();
    void minimize();
    void uiShow();
    void uiHide();
    void escapePressed();
//  void onLoadError( const QString& url, const QString& errorText, int errorCode );
    void onCefWindowCreated( QWindow *cefWindow );
    void fitGeometry();

    signals:
    void commandLineTorrent( TorrentInfo info );
    void stoppingPlayback();

protected:
    void resizeEvent( QResizeEvent *evt );
    void moveEvent( QMoveEvent * );
    void checkGeometry();

#ifdef USE_QCEF
    QCefWebView *m_browser;
#elif defined USE_WEBENGINE
    QWebEngineView *m_browser;
#else
    QGraphicsScene *m_scene;
    GraphicsBrowser *m_browser;
#endif
    HostApp *m_hostApp;

#if defined USE_QCEF || defined USE_WEBENGINE
    TransparentWidget *m_resizeTop;
    TransparentWidget *m_resizeLeft;
    TransparentWidget *m_resizeRight;
    TransparentWidget *m_resizeBottom;
    TransparentWidget *m_dragZone1;
    TransparentWidget *m_dragZone2;
#endif

    ResizeDragEventHandler *m_mouseFilter;
    VideoWidget *m_video;
    QWidget *m_intro;
    VideoControl *m_control;
    QFrame *m_closeControl;
    QPushButton *m_closeButton;
    ChromeCast *m_cCast;
    ControlRevealer *m_revealer;

    int zoomFactor = 0;
};

#endif // __FRAMELESSMAINWINDOW_H_INCL__
