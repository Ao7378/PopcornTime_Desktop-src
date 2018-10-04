#include "Subtitler.h"
#include "SubtitleFetcher.h"
#include "SubComboBox.h"
#include "defaults.h"
#include <QJsonDocument>
#include <QTimer>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QWidgetAction>
#include <QMetaMethod>



Subtitler::Subtitler( QObject *parent, SubComboBox& languages, VariantComboBox& variants )
   : QObject( parent ), m_languageBox( &languages ), m_variantBox( &variants )
{
    connect( m_languageBox.data(), SIGNAL( activated( const QString& ) ), this, SLOT( languageSelected( const QString& ) ) );
    connect( m_variantBox.data(), SIGNAL( activated( const QString& ) ), this, SLOT( variantSelected( const QString& ) ) );
    variantsSetEnabled( false );
}

void Subtitler::startSubtitles( TorrentInfo info )
{
    clearSubtitles( info );

    QJsonArray langArr = info.getJsonValue( "subtitles2" ).toArray();;
    while ( langArr.size() )
    {
        const QJsonArray item = langArr.takeAt( 0 ).toArray();
        if ( item.isEmpty() || item.size() < 3 ) continue;

        const QByteArray abbr = item.at( 0 ).toString().toLatin1();
        const QString lang = item.at( 1 ).toString();
        QJsonArray subs = item.at( 2 ).toArray();
        if ( lang.isEmpty() || subs.isEmpty() ) continue;

        QSharedPointer<SubtitleLanguage> language( new SubtitleLanguage( lang, abbr ) );

        while ( subs.size() )
        {
            const QJsonArray sub = subs.takeAt( 0 ).toArray();
            const QUrl qurl( sub.at( 0 ).toString() );
            if ( !qurl.isValid() ) continue;
            language->subs += QSharedPointer<SubtitleItem>( new SubtitleItem( info, lang, abbr, qurl ) );
        }
        if ( language->subs.size() ) languages += language;
    }

    foreach( QSharedPointer<SubtitleLanguage> lang, languages )
    {
        auto fetcher = new SubtitleFetcher( lang, this );
        connect( fetcher, SIGNAL( finished() ), this, SLOT( updateLanguages() ) );
    }
}

void Subtitler::clearSubtitles( TorrentInfo info )
{
    currentTorrent = info;
    subtitlesLocale = info.getJsonValue( "subtitles_locale" ).toString().toLatin1();
    languages.clear();
    currentLanguage.clear();
    currentSub.clear();
    customSub.clear();
    if ( m_languageBox ) m_languageBox->clear();
    if ( m_variantBox ) m_variantBox->clear();
    updateLanguages();
}

void Subtitler::filterSubtitles( const QStringList& filterString )
{
//    currentFilter = filterString;
//    QString currentText = this->currentText();
//    this->clear();
//    this->addItem( SUBTITLES_OFF, SUBTITLES_OFF );
//    const SubtitleItem *currentItem = 0;
//    bool hasInserts = false;
//    for ( int i = 0; i < subs.size(); ++i ) if ( tryInsertItem( subs.at( i ) ) )
//        {
//            hasInserts = true;
//            if ( subs.at( i ).language() == currentText ) currentItem = &subs.at( i );
//        }
//    const int index = this->findText( currentText );
//    if ( index < 0 || !currentItem )
//    {
//        emit haveSubtitleFile( "" );
//    }
//    else
//    {
//        this->setCurrentIndex( index );
//        useSubtitle( *currentItem );
//    }
//    if ( !hasInserts )
//    {
//        clearSubtitles();
//    }
//    else
//    {
//        this->setFocusPolicy( Qt::StrongFocus );
//        this->setEnabled( true );
//    }
}



void Subtitler::provide()
{
    QTimer::singleShot( 2000, this, SLOT( provideTimer() ) );
}

void Subtitler::provideTimer()
{
    if ( !currentSub ) return;

    if ( !subsTempFile.isNull() ) emit haveSubtitleFile( subsTempFile->fileName() );
    if ( currentSub ) emit haveSubtitleJson( currentSub->getSubtitleData() );

//  subtitleSelected( this->currentText() );
}


void Subtitler::languageSelected( const QString& text )
{
    if ( text == SUBTITLES_CUSTOM ) customSubtitle();
    else if ( text != SUBTITLES_OFF )
    {
        foreach( QSharedPointer<SubtitleLanguage> lang, languages )
        {
            if ( lang->language() != text ) continue;
            useLanguage( lang );
            return;
        }
    }
    else disableSubtitle();
}

void Subtitler::variantSelected( const QString& text )
{
    static const QRegularExpression matcher( "\\b([\\d]+)\\b" );
    QRegularExpressionMatch match;
    if ( ( match = matcher.match( text ) ).hasMatch() )
    {
        bool ok;
        int index = match.captured( 1 ).toInt( &ok ) - 1;
        if ( !ok || index < 0 || index >= currentLanguage->subs.count() ) return;
        useSubtitle( currentLanguage->subs.at( index ) );
    }
}


void Subtitler::updateLanguages()
{
    QList<QString> languageTexts;
    QSharedPointer<SubtitleLanguage> localeLang;
    QMutableListIterator<QSharedPointer<SubtitleLanguage>> langIt( languages );
    while ( langIt.hasNext() )
    {
        QSharedPointer<SubtitleLanguage> lang = langIt.next();
        if ( lang->subs.isEmpty() ) langIt.remove();
        else if ( lang->subs.first()->matchesTorrent( currentTorrent ) )
        {
            languageTexts += lang->language();
            if ( currentLanguage.isNull() && currentSub.isNull() && subtitlesLocale.size() && lang->abbr() == subtitlesLocale )
            {
                localeLang = lang.toWeakRef();
            }
        }
    }
    if ( m_languageBox ) m_languageBox->updateItemList( languageTexts );

    if ( currentLanguage ) updateVariants();
    else
    {
        if ( !localeLang.isNull() && localeLang->subs.size() && localeLang->subs.first()->matchesTorrent( currentTorrent ) )
        {
            m_languageBox->select( localeLang->language() );
            languageSelected( localeLang->language() );
        }
    }
}

void Subtitler::updateVariants()
{
    if ( !currentLanguage )
    {
        variantsSetVisible( false );
        return;
    }

    SubtitleFetcher *fetcher = qobject_cast<SubtitleFetcher *>( sender() );

    QList<QString> variantTexts;
    QMutableListIterator<QSharedPointer<SubtitleItem>> itemIt( currentLanguage->subs );

    while ( itemIt.hasNext() )
    {
        auto item = itemIt.next();
        if ( item->canDisplay() )
        {
            variantTexts += currentLanguage->language() + " Variant " + QString::number( currentLanguage->subs.indexOf( item ) + 1 );
        }
    }
    if ( m_variantBox ) m_variantBox->updateItemList( variantTexts );
    variantsSetEnabled( variantTexts.size() > 1 );
}


void Subtitler::customSubtitle()
{
    if ( !sender() && customSub )
    {
        useSubtitle( customSub );
        return;
    }

    QString extFilter;
    foreach( QString ext, currentFilter ) extFilter += "*." + ext + " ";
    extFilter.chop( 1 );

    const QString fileName = QFileDialog::getOpenFileName( qobject_cast<QWidget *>( parent() ), "Open Subtitles", "", "Subtitle Files (" + extFilter + ")" );

    QFileInfo info( fileName );
    QFile file( fileName );
    if ( !fileName.size() || !info.exists() || info.size() == 0 || !file.open( QFile::QIODevice::ReadOnly ) )
    {
        disableSubtitle();
        return;
    }

    customSub.reset( new SubtitleItem( currentTorrent ) );
    const QByteArray data = file.readAll();
    customSub->setData( data, info.fileName().toLatin1() );

    useSubtitle( customSub );
    variantsSetEnabled( false ); 

    return;
}

void Subtitler::useLanguage( QSharedPointer<SubtitleLanguage> lang )
{

    if ( currentLanguage == lang ) return;
    currentLanguage = lang;
    updateVariants();

    const int subsCount = lang->subs.count();
    if ( subsCount < 1 ) return;


    useSubtitle( lang->subs.first() );

    if ( subsCount > 1 )
    {
//      m_variantBox->clear();
//      m_variantBox->addItem( QString( "Variant " ) + QString::number( 1 ) );
//      variantsSetVisible( true );

        auto fetcher = new SubtitleFetcher( lang, this, true );

        if ( m_variantBox ) m_variantBox->updateItemList( QList<QString>() << "Updating variant list..." );
        connect( fetcher, SIGNAL( finished() ), this, SLOT( updateVariants() ) );
    }
}

void Subtitler::useSubtitle( QSharedPointer<SubtitleItem> item )
{
    currentSub = item;
    if ( item.isNull() ) return;
    subsTempFile.reset( new QFile( QDir::tempPath() + QDir::separator() + item->langAbbr() + item->fileName().toLower(), this ) );
    subsTempFile->open( QIODevice::WriteOnly );
    subsTempFile->write( sDecoder.decodeToUtf8( *item ) );
    subsTempFile->close();


    emit haveSubtitleFile( subsTempFile->fileName() );
    emit haveSubtitleJson( item->getSubtitleData() );
}

void Subtitler::disableSubtitle()
{
    const int index = m_languageBox->findText( SUBTITLES_OFF );
    if ( index >= 0 ) m_languageBox->setCurrentIndex( index );
    emit haveSubtitleFile( "" );
    currentLanguage.clear();
    updateVariants();
}

bool Subtitler::canUseItem( const SubtitleItem& item ) const
{
    foreach( QString ext, currentFilter )
    {
        if ( item.fileName().toLower().endsWith( ext ) ) return true;
    }
    return false;
}

void Subtitler::variantsSetVisible( bool visible )
{
    m_variantBox->setEnabled( visible );
    if ( !visible ) m_variantBox->clear();
}

