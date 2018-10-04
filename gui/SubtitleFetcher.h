#ifndef __SUBTITLEFETCHER_H_INCL__
    #define __SUBTITLEFETCHER_H_INCL__

    #include <QNetworkAccessManager>
    #include <QPointer>
    #include <QSharedPointer>
    #include <QNetworkReply>
    #include "commontypes.h"
    #include "Subtitler.h"

const int SUBTITLE_DOWNLOAD_RETRIES = 2;
const int SUBTITLE_DOWNLOAD_RETRY_DELAY_S = 10;


class SubtitleFetcher : public QObject
{
    Q_OBJECT

public:
    SubtitleFetcher( QSharedPointer<SubtitleLanguage> subLang, QObject *parent, bool fetchAll = false );

    private slots:
    void onFinished();
    void request();

    signals:
    void finished();
    void subtitleLoaded( const SubtitleItem& item );

protected:
    bool hasRetries() { return retriesLeft > 0; }
    QSharedPointer<SubtitleItem> getNextSub( QSharedPointer<SubtitleLanguage>& language );
    void updateFilename( const QByteArray& contentDisposition );

    QPointer<QNetworkReply> m_reply;
    QSharedPointer<QNetworkAccessManager> m_man = QSharedPointer<QNetworkAccessManager>( new QNetworkAccessManager);
    QWeakPointer<SubtitleLanguage> language;
    QSharedPointer<SubtitleItem> currSub;
    int retriesLeft = SUBTITLE_DOWNLOAD_RETRIES + 1;
    bool m_fetchAll;
};

#endif // __SUBTITLEFETCHER_H_INCL__
