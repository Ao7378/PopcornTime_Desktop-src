#include "WebEngineWebView.h"
#include "defaults.h"

#include <QtGui/QClipboard>
#include <QtNetwork/QAuthenticator>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtGui/QMouseEvent>

#include <QtCore/QDebug>
#include <QtCore/QBuffer>

#include <QApplication>
#include <QMainWindow>
#include <QPointer>
#include <QDesktopServices>

#include <QWebEnginePage>
#include <QWebEngineView>
#include <QtWebSockets/QWebSocketServer>
#include <QWebChannel>
#include "websocketclientwrapper.h"
#include "websockettransport.h"
#include <QWebEngineSettings>


QPointer<QObject> WebEngineView::hostApp;


WebEnginePage::WebEnginePage( QObject *parent )
   : QWebEnginePage( parent )
   , m_keyboardModifiers( Qt::NoModifier )
   , m_pressedButtons( Qt::NoButton )
{
#if defined(QWEBENGINEPAGE_SETNETWORKACCESSMANAGER)
    setNetworkAccessManager( BrowserApplication::networkAccessManager() );
#endif
}

WebEngineView::WebEngineView( QWidget *parent, bool jsCanOpenWindows )
   : QWebEngineView( parent )
   , m_page( new WebEnginePage( this ) )
{
    setPage( m_page );
    connect( page(), SIGNAL( loadingUrl( QUrl ) ), this, SIGNAL( urlChanged( QUrl ) ) );
}

void WebEngineView::contextMenuEvent( QContextMenuEvent *event ) { event->ignore(); }

void WebEngineView::executeJs( QString script )
{
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << script.mid( 0, 50 );
    m_page->runJavaScript( script );
}


void WebEngineView::mousePressEvent( QMouseEvent *event )
{
    m_page->m_pressedButtons = event->buttons();
    m_page->m_keyboardModifiers = event->modifiers();
    QWebEngineView::mousePressEvent( event );
}

void WebEngineView::mouseReleaseEvent( QMouseEvent *event )
{
    QWebEngineView::mouseReleaseEvent( event );
    if ( !event->isAccepted() && ( m_page->m_pressedButtons & Qt::MidButton ) )
    {
        QUrl url( QApplication::clipboard()->text( QClipboard::Selection ) );
        if ( !url.isEmpty() && url.isValid() && !url.scheme().isEmpty() )
        {
            setUrl( url );
        }
    }
}



bool WebEnginePage::certificateError( const QWebEngineCertificateError& certificateError )
{
    qDebug() << this->metaObject()->className() << __FUNCTION__ << certificateError.errorDescription();
    return true;
}

