#ifndef __TORTHREADS_H_INCL__
    #define __TORTHREADS_H_INCL__

    #include <QThread>
    #include "commontypes.h"
    #include <QPointer>
    #include <QSharedPointer>
    #include <QNetworkAccessManager>

class DownloadNotifyThread : public QThread
{
    Q_OBJECT

public:
    DownloadNotifyThread( QObject *parent );
    ~DownloadNotifyThread();

    signals:
    void torrentProgressAvailable( int progressPeers, int dlSpeed, int seeds, int peers, QString msg );
    void torrentDataAvailable( QString url );
    void downloadInfoChanged( DownloadInfo info );
    public slots:
    void startOnTorrent( int torrentId );
    void stopOnTorrent();

protected:
    void run();

private:
    volatile int tId;
};

class TorrentDownloadThread : public QThread
{
    Q_OBJECT

public:
    TorrentDownloadThread( QObject *object, TorrentInfo& tInfo );
    ~TorrentDownloadThread();

    public slots:
    void abort();

    signals:
    void torrentDownloaded( TorrentInfo tInfo );
    void getMetadataError( TorrentInfo tInfo );
    void gotMetadata();

    private slots:
    void onTorrentFileDownloaded( QByteArray torrentFile );

protected:
    int getAddTorrentFile();
    void run();

private:
    TorrentInfo m_info;
    QByteArray data;
};


class QNetworkReply;
class QHttpMultiPart;


class Requester : public QObject
{
    Q_OBJECT

public:
    Requester();
    ~Requester();

    static Requester* getInstance() { return instance_; }

protected:

    public slots:
    void getSubtitleThumb( QString url );
    void getTorrentFile( QString url );
    void getUrl( QString url );
    QNetworkReply* post( const QNetworkRequest& request, const QByteArray& data );
    QNetworkReply* post( const QNetworkRequest& request, QHttpMultiPart *multiPart );

    private slots:
    void onSubtitleThumbReqestFinished();
    void onTorrentFileReqestFinished();
    void sslInit();

    signals:
    void haveSubtitleThumb( QString url, QString data );
    void haveTorrentFile( QByteArray data );

private:
    QNetworkAccessManager *manager_;
    QThread thread_;
    QNetworkReply *torrentReply_;
    static Requester *instance_;

};

#endif // __TORTHREADS_H_INCL__
