#include "cefclient/cefclient.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>

#include <QDebug>

extern CefRefPtr<ClientHandler> g_handler;

void CefQuitUntilAllBrowserClosed()
{
//qDebug() << __FUNCTION__ << __LINE__;
    if ( ClientHandler::m_BrowserCount > 0 && g_handler.get() )
    {
        g_handler->CloseAllBrowsers( false );
    }
//qDebug() << __FUNCTION__ << __LINE__;
    CefQuit();
}

QString AppGetWorkingDirectory()
{
    QString path = QStandardPaths::writableLocation( QStandardPaths::DataLocation );
    QDir dir( path );
    if ( !dir.mkpath( "." ) ) path = qApp->applicationDirPath();
    return path;
}

void NotifyAllBrowserClosed()
{
//qDebug() << __FUNCTION__;
// Notify all browser windows have closed.
}
