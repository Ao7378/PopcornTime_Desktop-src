#include "NjsProcess.h"
#include <QApplication>
#include <QDir>
#include <QDebug>

const char CHROMECAST_RESPONSE_HEADER[] = "<<";
const char CHROMECAST_RESPONSE_FOOTER[] = ">>";

NjsProcess::NjsProcess( QObject *parent ) :
   QProcess( parent )
{
    connect( this, SIGNAL( readyRead() ), this, SLOT( onReadyRead() ) );
//  this->setProcessChannelMode( QProcess::MergedChannels );
    this->setWorkingDirectory( QDir::toNativeSeparators( qApp->applicationDirPath() + QDir::separator() + "chromecast" ) );
    QString node = QDir::toNativeSeparators( qApp->applicationDirPath() + QDir::separator()
                                             + "chromecast" + QDir::separator()
#ifdef Q_OS_MAC
                                             + "node" );
#else
                                             +"node.exe" );
#endif
    this->setProgram( node );
}

NjsProcess::~NjsProcess()
{
    this->kill();
    this->waitForFinished( 1000 );
}


void NjsProcess::start()
{
    buffer.clear();
    QProcess::start();
}

void NjsProcess::restart()
{
    if ( state() != QProcess::NotRunning )
    {
        qDebug() << "killing" << state();
        this->kill();
        qDebug() << "killed" << state();
        this->waitForFinished( 1000 );
        qDebug() << "waited" << state();
    }
    this->start();
    qDebug() << "started" << state();
}


void NjsProcess::onReadyRead()
{
    QByteArray data = this->readAll();
//  qDebug() << "read!!!!" << data << "!!!!";
    buffer += data;

    while ( buffer.size() )
    {
        const int start = buffer.indexOf( CHROMECAST_RESPONSE_HEADER );
        if ( start < 0 )
        {
            buffer.clear();
            return;
        }
        if ( start > 0 )
        {
            buffer = buffer.mid( start );
        }

        const int end = buffer.indexOf( CHROMECAST_RESPONSE_FOOTER );
        if ( end <= 0 ) return;
//      qDebug() << "data" << buffer;
        QString str = buffer.mid( sizeof( CHROMECAST_RESPONSE_HEADER ) - 1, end - 2 );
        const int index = str.indexOf( ": " );

        if ( index > 0 && index + 2 < str.size() ) emit haveToken( str.left( index ).toLatin1(), str.mid( index + 2 ).toLatin1() );
//      qDebug() << "token" << str.left( index ).toLatin1() << str.mid( index + 2 ).toLatin1();
//      qDebug() << "trash" << buffer.mid( 0, end + sizeof( CHROMECAST_DEVICE_DETECT_FOOTER ) + 1 );
        buffer = buffer.mid( end + sizeof( CHROMECAST_RESPONSE_FOOTER ) + 1 );
    }
}

