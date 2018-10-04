#ifndef __RESIZEDRAGEVENTHANDLER_H_INCL__
    #define __RESIZEDRAGEVENTHANDLER_H_INCL__

    #include <QObject.h>
    #include <QPoint.h>

class FramelessMainWindow;
class QWidget;
struct TorrentInfo;

class ResizeDragEventHandler : public QObject
{
    Q_OBJECT
    enum ResizeState
    {
        IdleResize, TopResize, BottomResize, LeftResize, RightResize, Drag
    };

public:
    ResizeDragEventHandler( FramelessMainWindow& window, QObject& filteredWidget );
    ~ResizeDragEventHandler();

    signals:
    void enterPressed();
    void upPressed();
    void downPressed();
    void leftPressed();
    void rightPressed();
    void escapePressed();
    void pausePressed();
    void mouseMoved();
    void applicationStateInactive( bool );
    void torrentDropped( TorrentInfo& info );

protected:
    ResizeState getResizeState( const QPoint& pos ) const;
    bool eventFilter( QObject *obj, QEvent *event );
    bool inDragZones( const QRect& windowRect, const QPoint& point ) const;

protected:
    ResizeState m_ResizeState;
    QPoint m_ResizeClickCoords;
    FramelessMainWindow *mw;

};

#endif // __RESIZEDRAGEVENTHANDLER_H_INCL__
