#ifndef __SUBTITLER_H_INCL__
    #define __SUBTITLER_H_INCL__

    #include <QComboBox>
    #include <QPointer>
    #include <QList>
    #include "commontypes.h"
    #include "SubtitleDecoder.h"

class QFile;
class SubComboBox;
class VariantComboBox;


struct SubtitleLanguage
{
    SubtitleLanguage( QString language, QByteArray abbr ) : m_language( language ), m_abbr( abbr ) { }

    QString language() { return m_language; }
    QByteArray abbr() { return m_abbr; }
    operator QString() { return "SubtitleLanguage " + m_language + ", abbr " + m_abbr + ", count " + QString::number( subs.count() ); }

    QString m_language;
    QByteArray m_abbr;
    QList<QSharedPointer<SubtitleItem>> subs;
};

class Subtitler : public QObject
{
    Q_OBJECT

public:
    Subtitler( QObject *parent, SubComboBox& languages, VariantComboBox& variants );

    public slots:
    void startSubtitles( TorrentInfo info );
    void clearSubtitles( TorrentInfo newInfo = TorrentInfo() );
    void filterSubtitles( const QStringList& filterString );
    void provide();

    private slots:
    void updateLanguages();
    void updateVariants();
    void provideTimer();
    void languageSelected( const QString& text );
    void variantSelected( const QString& text );

    signals:
    void haveSubtitleFile( QString fileName );
    void haveSubtitleJson( QJsonObject );

protected:
    void customSubtitle();
    void useSubtitle( QSharedPointer<SubtitleItem> item );
    void disableSubtitle();
    void useLanguage( QSharedPointer<SubtitleLanguage> lang );
    bool canUseItem( const SubtitleItem& item ) const;
    void variantsSetEnabled( bool visible );

    QPointer<SubComboBox> m_languageBox;
    QPointer<VariantComboBox> m_variantBox;
    TorrentInfo currentTorrent;
    QByteArray subtitlesLocale;
    QList<QSharedPointer<SubtitleLanguage>> languages;
    QSharedPointer<SubtitleItem> currentSub;
    QSharedPointer<SubtitleLanguage> currentLanguage;
    QSharedPointer<SubtitleItem> customSub;
    QSharedPointer<QFile> subsTempFile;
    SubtitleDecoder sDecoder;
    QStringList currentFilter;
};


#endif // __SUBTITLER_H_INCL__
