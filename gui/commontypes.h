#ifndef __COMMON_TYPES_H_INCL__
    #define __COMMON_TYPES_H_INCL__

    #include <QByteArray>
    #include <QString>
    #include <QMetaType>
    #include <QNetworkProxy>
    #include <QJsonObject>
    #include <QJsonValue>
    #include <QJsonArray>
    #include <QMap>

typedef QMap<QString, int> TIndexedString;

Q_DECLARE_METATYPE( TIndexedString );

struct TorrentInfo;

class SubtitleItem
{
public:
    SubtitleItem() { }
    SubtitleItem( const TorrentInfo& info, QString theLang = QString(), QByteArray theAbbr = QByteArray(), QUrl theUrl = QString() );
    SubtitleItem( const SubtitleItem& itm ) : m_language( itm.m_language ), m_langAbbr( itm.m_langAbbr ),
       m_zipData( itm.m_zipData ), m_data( itm.m_data ), m_fileName( itm.m_fileName ), m_zipName( itm.m_zipName ),
       m_url( itm.m_url ), m_zipMD5( itm.m_zipMD5 ), m_dataMD5( itm.m_dataMD5 ), m_torrentDesc( itm.m_torrentDesc ) { }

    void setOriginalName( QByteArray name );
    bool setZipData( const QByteArray& data );
    void setData( const QByteArray& data, QByteArray name );


    bool canDisplay() const;
    bool isCustom() const { return m_url.isEmpty() || m_language.isEmpty(); }
    QByteArray getImdb() const { return m_torrentDesc["imdb"].toString().toLatin1(); }
    QByteArray getEpisodeImdb() const { return m_torrentDesc["eimdb"].toString().toLatin1(); }
    QByteArray getTorrentHash() const { return m_torrentDesc["torrentHash"].toString().toLatin1().toLower(); }
    const QByteArray& getMd5Id() const { return dataMD5(); }
    const QJsonObject& getTorrentData() const { return m_torrentDesc; }
    QJsonObject getSubtitleData() const;
    QString zipFileName() const;
    operator QString() const;

    const QByteArray& langAbbr() const { return m_langAbbr; }
    const QString& language() const { return m_language; }
    const QByteArray& zipData() const { return m_zipData; }
    const QByteArray& data() const { return m_data; }
    const QString& fileName() const { return m_fileName; }
    const QUrl& url() const { return m_url; }
    const QByteArray& zipMD5() const { return m_zipMD5; }
    const QByteArray& dataMD5() const { return m_dataMD5; }

private:
    QString m_language;
    QByteArray m_langAbbr;
    QByteArray m_zipData;
    QByteArray m_data;
    QString m_fileName;
    QString m_zipName;
    QUrl m_url;
    QByteArray m_zipMD5;
    QByteArray m_dataMD5;
    QJsonObject m_torrentDesc;
    bool zipRepacked = false;
};

typedef QList<SubtitleItem> TSubsList;

Q_DECLARE_METATYPE( SubtitleItem );



struct dlInfo;

struct DownloadInfo
{
    DownloadInfo();
    DownloadInfo( const DownloadInfo& info );
    DownloadInfo( const dlInfo& info );
    long long downloadRateBs;
    long long downloaded;
    long long total;
    long long piecesDone;
    long long piecesTotal;
    long long piecesStartup;
    long long pieceSize;
    long long seeders;
    long long peers;
    long long piecesStartupAtEnd;
    long long piecesStartupDone;
};

Q_DECLARE_METATYPE( DownloadInfo );



    #ifdef USE_SOCKS5
struct ProxySettings
{
    ProxySettings() : port( 0 ) { }
    bool isValid() { return host.size() && port > 0; }
    QNetworkProxy networkProxy()
    {
        if ( !isValid() ) return QNetworkProxy( QNetworkProxy::NoProxy );
        return QNetworkProxy( QNetworkProxy::Socks5Proxy, host, port, user, pass );
    }

    QByteArray host;
    int port;
    QByteArray user;
    QByteArray pass;
};

typedef QList<ProxySettings> TProxyList;

Q_DECLARE_METATYPE( ProxySettings );
    #endif

struct TorrentInfo
{
    TorrentInfo() : index( -1 ) { }
    TorrentInfo( const QJsonObject& jObj );
    TorrentInfo( QString iurl, QString imagnet, QString ifileName, QString isubtitles_url, QString iTitle = QString(), QByteArray ilocale = "", QByteArray iquality = "" );
    QJsonValue getJsonValue( const QString& key ) const;
    QJsonObject getTorrentDesc() const;
    QByteArray getTorrentHash() const { return torrentHash; }
    QByteArray getImdb() const { return getJsonValue( "id" ).toString().toLatin1(); }
    bool isValid() { return url.size() || magnet.size(); }
    static TorrentInfo fromUrl( QUrl url, QString fileName = "" ); 
    int index;
    quint16 subsCount;
    QString url;
    QString magnet;
    QString fileName;
    QString title;
    QByteArray torrentHash;
    QJsonObject json;

private:
    void updateTorrentHash();
};

Q_DECLARE_METATYPE( TorrentInfo );



void registerMetaTypes();

#endif
