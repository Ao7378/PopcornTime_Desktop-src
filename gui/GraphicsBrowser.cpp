#include "GraphicsBrowser.h"
#include <QMainWindow>
#include <QApplication>


//GraphicsBrowser::GraphicsBrowser( QWidget *parent ) : QGraphicsView( parent ) { init(); }
GraphicsBrowser::GraphicsBrowser( QGraphicsScene *scene_, QMainWindow& parent )
   : QGraphicsView( scene_, &parent ),
     m_webView( new QGraphicsWebView() )
//{ init(); }
//void GraphicsBrowser::init()
{
    setFrameShape( QFrame::NoFrame );
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
//  setAttribute( Qt::WA_TransparentForMouseEvents );

#ifndef _DEBUGGING
    setContextMenuPolicy( Qt::CustomContextMenu ); // disable right click
#endif
    if ( scene() )
    {
        scene()->addItem( m_webView );
    }

}

