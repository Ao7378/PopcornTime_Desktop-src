#ifndef __TORRENTWRAPPER_H_INCL__
    #define __TORRENTWRAPPER_H_INCL__

    #include <QObject>
    #include "TorHelpers.h"
    #include "commontypes.h"

class QNetworkAccessManager;

class TorrentWrapper : public QObject
{
    Q_OBJECT

public:
    TorrentWrapper( QObject *parent = 0 );
    ~TorrentWrapper();
    bool isDownloaded( quint64 start, quint64 end, int index = -1 );

    public slots:
    void init( QString downloadDir, bool cleanOnExit );
#ifdef USE_SOCKS5
    void setProxy( ProxySettings proxy );
#endif
    void get( TorrentInfo info );
    void cancel( int index );

    void seek( quint64 start );
    void seek( quint64 start, int index );
    void onCheckDownloaded( bool &result, quint64 start, quint64 end, int index = -1 );

    private slots:
    void onMetadataDownloaded( TorrentInfo tInfo );
    void onTorrentMetadataError( TorrentInfo );


    signals:
    void getMetadataError( TorrentInfo );
    void leechingStarted( int index );
    void torrentProgressAvailable( int progressPeers, int dlSpeed, int seeds, int peers, QString msg );
    void torrentDataAvailable( QString url );
    void downloadInfoChanged( DownloadInfo info );
    void newMovieName( QString name );

    void startSubs( TorrentInfo );
    void preloadCancelled( TorrentInfo );
    void torrentLoadCancelled( TorrentInfo );
    void metadataAvailable();

protected:
    DownloadNotifyThread *m_thread;
    QNetworkAccessManager *m_netMan;
    QList<int> activeTorrentIds;
    TorrentInfo activeTorrent;
    bool activeTorrentPreloaded = false;
};

#endif // __TORRENTWRAPPER_H_INCL__
