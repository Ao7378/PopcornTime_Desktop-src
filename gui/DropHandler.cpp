#include "DropHandler.h"

#include <QMimeData>
#include <QDebug>
#include <QFileInfo>
#include <QUrl>
#include <QUrlQuery>

bool DropHandler::canAccept( const QMimeData *mime )
{
    qDebug() << __FUNCTION__ << mime->formats();
    if ( mime->hasFormat( "FileNameW" ) )
    {
        const QString fileName = QString::fromUtf16( reinterpret_cast<const ushort *>( mime->data( "FileNameW" ).data() ) );
        qDebug() << __FUNCTION__ << fileName << QFileInfo::exists( fileName );
        return true;
    }
    else if ( mime->hasUrls() )
    {
        foreach( const QUrl& url, mime->urls() )
        {
            if ( url.scheme() == "magnet" || url.scheme() == "http" || url.scheme() == "https" ) //url.fileName().endsWith( ".torrent" ) ||
            {
                qDebug() << __FUNCTION__ << url << url.scheme();
                return true;
            }
        }
    }
    qDebug() << __FUNCTION__ << "ignored";
    return false;
}

TorrentInfo DropHandler::getInfo( const QMimeData *mime )
{
    if ( mime->hasFormat( "FileNameW" ) )
    {
        const QString fileName = QString::fromUtf16( reinterpret_cast<const ushort *>( mime->data( "FileNameW" ).data() ) );
        if ( !QFileInfo::exists( fileName ) ) return TorrentInfo();
        return TorrentInfo( QUrl::fromLocalFile( fileName ).toString(), "", "", "", QFileInfo( fileName ).baseName() );
    }

    if ( !mime->hasUrls() ) return TorrentInfo();

    foreach( const QUrl& url, mime->urls() )
    {
        TorrentInfo info = TorrentInfo::fromUrl( url );
        if ( info.isValid() ) return info;
    }
    return TorrentInfo();
}

TorrentInfo DropHandler::getInfo( const QString& url )
{
//  qDebug() << __FUNCTION__ << url;
//  qDebug() << __FUNCTION__ << QUrl::fromLocalFile( url );
    if ( QFileInfo::exists( url ) ) return TorrentInfo( QUrl::fromLocalFile( url ).toString(), "", "", "", QFileInfo( url ).baseName() );
    QUrl qurl( url );
    if ( qurl.isLocalFile() ) return TorrentInfo( qurl.toString(), "", "", "", QUrlQuery( qurl ).queryItemValue( "dn", QUrl::FullyDecoded ) );

    return TorrentInfo::fromUrl( qurl );
}

