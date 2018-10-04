#ifndef __GRAPHICSBROWSER_H_INCL__
    #define __GRAPHICSBROWSER_H_INCL__

    #include "QGraphicsView.h"
    #include "FramelessMainWindow.h"

    #include <QGraphicsWebView>

class GraphicsBrowser : public QGraphicsView
{
    Q_OBJECT

public:
    GraphicsBrowser( QGraphicsScene *scene_, QMainWindow& parent );

    QGraphicsWebView* webView( void ) { return m_webView; }

protected:
    friend class FramelessMainWindow;
    void resizeEvent( QResizeEvent *evt )
    {
        QGraphicsView::resizeEvent( evt );

        QRect r = contentsRect();

        setSceneRect( 0, 0, r.width(), r.height() );
        m_webView->resize( r.width(), r.height() );
    }

private:
    QGraphicsWebView *m_webView;
};

#endif // __GRAPHICSBROWSER_H_INCL__
