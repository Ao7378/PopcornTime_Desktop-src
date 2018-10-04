#ifndef __DROPHANDLER_H_INCL__
    #define __DROPHANDLER_H_INCL__

    #include "commontypes.h"

class QMimeData;

class DropHandler
{
public:
    static bool canAccept( const QMimeData *mime );
    static TorrentInfo getInfo( const QMimeData *mime );
    static TorrentInfo getInfo( const QString & url );
};

#endif // __DROPHANDLER_H_INCL__
