#include "ResizeDragEventHandler.h"

#include "defaults.h"
#include "FramelessMainWindow.h"
#include "DropHandler.h"
#include <QEvent.h>
#include <QApplication>
#include <QDebug>
#ifdef _DEBUGGING
    #include "VideoPlayer.h"
    #include "VideoControl.h"
    #include "commontypes.h"
    #include "hostApp.h"
    #include <QDialog>
    #include <QVBoxLayout>
    #include <QLineEdit>
    #include <QDialogButtonBox>
    #include <QMessageBox>
    #include "TorrentWrapper.h"
#endif

ResizeDragEventHandler::ResizeDragEventHandler( FramelessMainWindow& window, QObject& filteredWidget ) : QObject( &window ),
   m_ResizeState( IdleResize ), mw( &window )
{
    filteredWidget.installEventFilter( this );
    qApp->installEventFilter( this );
}

ResizeDragEventHandler::~ResizeDragEventHandler()
{
//  mw->removeEventFilter( this );
}

bool ResizeDragEventHandler::eventFilter( QObject *obj, QEvent *event )
{
    (void) obj;
    if ( event->type() == QEvent::ApplicationStateChange && !mw->isPlayerShown() )
    {
        Qt::ApplicationState state = static_cast<QApplicationStateChangeEvent *>( event )->applicationState();
        switch ( state )
        {
        case Qt::ApplicationInactive:
        case Qt::ApplicationHidden:
        case Qt::ApplicationSuspended:
            emit applicationStateInactive( false );
            break;
        case Qt::ApplicationActive:
            emit applicationStateInactive( true );
        default:
            break;
        }
        return false;
    }
    if ( event->type() == QEvent::WindowStateChange && !mw->isPlayerShown() )
    {
        Qt::WindowStates state = mw->windowState();
        switch ( state )
        {
        case Qt::WindowMinimized:
            emit applicationStateInactive( false );
            break;
        case Qt::WindowActive:
        case Qt::WindowMaximized:
        case Qt::WindowFullScreen:
        case Qt::WindowNoState:
            emit applicationStateInactive( true );
            break;

        }
        return false;
    }
    if ( event->type() == QEvent::MouseButtonDblClick )
    {
        if ( obj->inherits( "QWidgetWindow" ) ) return QObject::eventFilter( obj, event ); // Skip QWidgetWindow to show real QWidget subclass
        QMouseEvent *e = static_cast<QMouseEvent *>( event );
        const QRect r = mw->geometry();
        const bool headerArea = inDragZones( r, e->globalPos() );
        const bool playerClick = mw->isPlayerShown() && QByteArray( obj->metaObject()->className() ) != "VideoWidget";
        if ( e->button() != Qt::LeftButton || ( !headerArea && !playerClick ) ) return false;
        mw->toggleMaximize();
        e->accept();
        return true;
    }
    if ( event->type() == QEvent::MouseButtonPress )
    {
//      qDebug() << "win" << obj->isWindowType() << "wid" << obj-isWidgetType();
//      if ( obj->objectName() != "FramelessMainWindowClassWindow" ) return false;
        QMouseEvent *e = static_cast<QMouseEvent *>( event );
        if ( e->button() != Qt::LeftButton ) return false;
        m_ResizeState = getResizeState( e->globalPos() );
        m_ResizeClickCoords = e->globalPos();
        if ( m_ResizeState == IdleResize ) return false;
        e->accept();
        return true;
    }
    if ( event->type() == QEvent::MouseButtonRelease )
    {
        QMouseEvent *e = static_cast<QMouseEvent *>( event );
        if ( m_ResizeState == IdleResize ) return false;
        m_ResizeState = IdleResize;
        QApplication::restoreOverrideCursor();
        e->accept();
        return true;
    }

    if ( event->type() == QEvent::DragEnter )
    {
        QDragEnterEvent *e = static_cast<QDragEnterEvent *>( event );
        if ( e && !mw->isPlayerShown() && DropHandler::canAccept( e->mimeData() ) )
        {
            event->accept();
            return true;
        }
        event->ignore();
        return false;
    }

    if ( event->type() == QEvent::Drop )
    {
        QDropEvent *e = static_cast<QDropEvent *>( event );
        if ( e && !mw->isPlayerShown() )
        {
            TorrentInfo info = DropHandler::getInfo( e->mimeData() );
            if ( info.isValid() )
            {
                emit torrentDropped( info );
                event->accept();
                return true;
            }
        }
        if ( e ) qDebug() << this->metaObject()->className() << __FUNCTION__;
        event->ignore();
        return false;
    }


    if ( event->type() == QEvent::KeyPress )
    {
        QKeyEvent *e = static_cast<QKeyEvent *>( event );



#ifdef _DEBUGGING
        if ( e->key() == Qt::Key_Asterisk && VideoPlayer::hasDebugFile() )
        {
            if ( mw->isPlayerShown() )
            {
                VideoPlayer::PlayerObject().data()->stop();
                mw->playbackStopped();
            }
            else
            {
                VideoPlayer::PlayerObject().data()->openFile( "" );
                mw->playbackStarted();
                DownloadInfo info;
                info.downloaded = 50000;
                info.total = 100000;
                info.piecesDone = 50;
                info.piecesTotal = 100;
                mw->control()->updateDownloadStatus( info );
            }
        }
            break;
#endif
            if ( !mw->isPlayerShown() ) return false;
            else switch ( e->key() )
                {
                case Qt::Key_Return:
                case Qt::Key_Enter: emit enterPressed(); break;
                case Qt::Key_Left: emit leftPressed(); break;
                case Qt::Key_Right: emit rightPressed(); break;
                case Qt::Key_Up: emit upPressed(); break;
                case Qt::Key_Down: emit downPressed(); break;
                case Qt::Key_Space: emit pausePressed(); break;
                case Qt::Key_Escape: emit escapePressed(); break;
                default: return false;
                }
        event->accept();
        return true;
    }

    if ( event->type() == QEvent::MouseMove )
    {
        QMouseEvent *e = static_cast<QMouseEvent *>( event );
        QPoint eventPoint = e->globalPos();
        QRect geom = mw->geometry();
        QPoint diff = eventPoint - m_ResizeClickCoords;
        emit mouseMoved();

        switch ( m_ResizeState )
        {
        default:
            return false;
        case TopResize:
            geom.setTop( geom.top() + diff.y() );
            break;
        case BottomResize:
            geom.setBottom( geom.bottom() + diff.y() );
            break;
        case LeftResize:
            geom.setLeft( geom.left() + diff.x() );
            break;
        case RightResize:
            geom.setRight( geom.right() + diff.x() );
            break;
        case Drag:
            geom.translate( diff );
            break;
        case IdleResize:
            {
                switch ( getResizeState( e->globalPos() ) )
                {
                case TopResize:
                case BottomResize:
                    QApplication::setOverrideCursor( Qt::SizeVerCursor );
                    return true;
                case LeftResize:
                case RightResize:
                    QApplication::setOverrideCursor( Qt::SizeHorCursor );
                    return true;
                case Drag:
                    QApplication::setOverrideCursor( Qt::SizeAllCursor );
                    return true;
                case IdleResize:
                default:
                    QApplication::restoreOverrideCursor();
                    return false;
                }
            }
            break;
        }

        //do some checks
        if ( geom.size().boundedTo( mw->maximumSize() ).expandedTo( mw->minimumSize() ) == geom.size() )
        {
            mw->setGeometry( geom );
            m_ResizeClickCoords = eventPoint;
        }
        return true;
    }
//  if ( event->type() == QEvent::KeyPress && static_cast<QKeyEvent *>( event )->key() == Qt::Key_Space ) // MouseButtonPress )
//  {
//      event->accept();
//      if ( mw->video() ) mw->stopPlayback();
//      else mw->playMedia( VLC::defaultFile );
//      return true;
//  }



// standard event processing
    return false;
}


ResizeDragEventHandler::ResizeState ResizeDragEventHandler::getResizeState( const QPoint& pos ) const
{
    const QRect r = mw->geometry();

    if ( mw->isMaximized() || mw->isFullScreen() ) return IdleResize;
    if ( QRect( r.topLeft(), QPoint( r.right(), r.top() + RESIZE_ZONE ) ).contains( pos ) ) return TopResize;
    if ( QRect( QPoint( r.left(), r.bottom() - RESIZE_ZONE ), r.bottomRight() ).contains( pos ) ) return BottomResize;
    if ( QRect( QPoint( r.right() - RESIZE_ZONE, r.top() ), r.bottomRight() ).contains( pos ) ) return RightResize;
    if ( QRect( r.topLeft(), QPoint( r.left() + RESIZE_ZONE, r.bottom() ) ).contains( pos ) ) return LeftResize;

    if ( inDragZones( r, pos ) ) return Drag;
    return IdleResize;
}

bool ResizeDragEventHandler::inDragZones( const QRect& windowRect, const QPoint& point ) const
{
#if defined USE_QCEF || defined USE_WEBENGINE
    return QRect( QPoint( windowRect.left() + DRAG_ZONE_LEFT_MARGIN,
                          windowRect.top() ),
                  QPoint( windowRect.right() - DRAG_ZONE1_RIGHT_MARGIN,
                          windowRect.top() + DRAG_ZONE1_HEIGHT )
                 ).contains( point )
           ||
           QRect( QPoint( windowRect.left() + DRAG_ZONE_LEFT_MARGIN,
                          windowRect.top() ),
                  QPoint( windowRect.right() - DRAG_ZONE2_RIGHT_MARGIN,
                          windowRect.top() + DRAG_ZONE1_HEIGHT + DRAG_ZONE2_HEIGHT )
                 ).contains( point );
#else
    return QRect( windowRect.topLeft(), QPoint( windowRect.right() - DRAG_ZONE_RIGHT_MARGIN, windowRect.top() + DRAG_ZONE_HEIGHT ) ).contains( point );
#endif
}
