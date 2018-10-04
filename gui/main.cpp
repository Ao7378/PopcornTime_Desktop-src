#include <QtGlobal>
#include <QApplication>

#include "FramelessMainWindow.h"
#include "defaults.h"
#include "commontypes.h"
#include "TorHelpers.h"
#include "Zoom4k.h"

#include <QPushButton>
#include <QMessageBox>
#include <QThread>
#include <QDir>
#include <QStyleFactory>
#include <QDesktopWidget>
#include <QStandardPaths>
#include "QtAVWidgets/global.h"


#ifdef USE_QCEF
    #include "cefclient/init_wrapper.h"
    #include <QMessageBox>
#else
    #include <QtWebEngine>
#endif

#ifdef Q_OS_WIN
    #include <Windows.h>
    #include <QtDebug>
    #include <QtGlobal>

void MyMessageOutput( QtMsgType Type, const QMessageLogContext& Context, const QString& Message )
{
    (void) Type;
    (void) Context;
    const char symbols[] = { 'D', 'W', 'C', 'F' };
#ifdef QT_DEBUG
    QString output = QString( "[%1] %2\r\n" ).arg( symbols[Type] ).arg( Message );
#else
    QString output = QString( "[%1] %2" ).arg( symbols[Type] ).arg( Message );
#endif
    OutputDebugString( reinterpret_cast<const wchar_t *>( output.utf16() ) );
}
#endif

#ifdef Q_OS_MAC
    #include <syslog.h>
    #include <QtDebug>
void MyMessageOutput( QtMsgType Type, const QMessageLogContext& Context, const QString& Message )
{
    (void) Type;
    (void) Context;
    const char symbols[] = { 'D', 'W', 'C', 'F' };
    QString output = QString( "[%1] %2" ).arg( symbols[Type] ).arg( Message );
    syslog( LOG_NOTICE, "%s", output.toLocal8Bit().constData() );
}
#endif


int main( int argc, char **argv )
{
    QApplication app( argc, argv );

#ifdef APP_VERSION
    const char *str = APP_VERSION;
    QCoreApplication::setApplicationVersion( str );
#else
#warning no APP_VERSION string!
    QCoreApplication::setApplicationVersion( "0.0.0" );
#endif
    QCoreApplication::setOrganizationName( "PopcornTime" );
    QCoreApplication::setApplicationName( "PopcornTime" );

    qsrand( QTime::currentTime().msec() );
#ifdef USE_QCEF
    CefInitWrapper w( argc, argv );
    if ( !w.wasCefInstance() ) return w.resultCode();
#endif

    QApplication::setStyle( QStyleFactory::create( "Fusion" ) );
    QtAV::Widgets::registerRenderers();
    registerMetaTypes();

    Requester req; //= new Requester;

#ifdef USE_WEBENGINE
    QtWebEngine::initialize();
#endif

    VLC vlc;
    Requester req; //= new Requester;

    FramelessMainWindow win;
    if ( Zoom4k::zoomFactorPercent() == 100 ) win.resize( DEFAULT_WIDTH, DEFAULT_HEIGHT );
    else win.resize( QApplication::desktop()->availableGeometry( &win ).width() * 0.8, QApplication::desktop()->availableGeometry( &win ).height() * 0.8 );
    win.move( QApplication::desktop()->screen()->rect().center() - QPoint( win.size().width() / 2, win.size().height() / 2 ) );
    win.show();

    const int result = app.exec();

    return result;
}

