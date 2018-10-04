#include "SubtitleFetcher.h"
#include "config.h"
#include <QTimer>
#include <QRegularExpression>

#ifdef _DEBUGGING
    #define SUBS_LOAD_DEBUG    qDebug() << this->metaObject()->className() << __FUNCTION__  << ( m_fetchAll ? "fetchAll" : "" )
#endif

#ifndef SUBS_LOAD_DEBUG
    #define SUBS_LOAD_DEBUG    if (0) qDebug()
#endif

SubtitleFetcher::SubtitleFetcher( QSharedPointer<SubtitleLanguage> subLang, QObject *parent, bool fetchAll )
   : QObject( parent ), language( subLang ), m_fetchAll( fetchAll )
{
    auto lang = language.toStrongRef();
    if ( !lang || lang->subs.isEmpty() )
    {
        emit finished();
        deleteLater();
        SUBS_LOAD_DEBUG << "error";
        return;
    }

    currSub = lang->subs.first();
    request();
    SUBS_LOAD_DEBUG << *lang.data() << fetchAll;
}

void SubtitleFetcher::onFinished()
{
    auto lang = language.toStrongRef();
    if ( !lang )
    {
        deleteLater();
        return;
    }
    if ( m_reply->error() )
    {
        SUBS_LOAD_DEBUG << lang->language() << m_reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt()
           << "retries left" << retriesLeft << "error: " << m_reply->errorString();

        if ( hasRetries() )
        {
            QTimer::singleShot( SUBTITLE_DOWNLOAD_RETRY_DELAY_S * 1000, this, SLOT( request() ) );
            return;
        }
        SUBS_LOAD_DEBUG << "error" << *currSub << m_reply->errorString();
        lang->subs.removeOne( currSub );
    }
    else
    {
        updateFilename( m_reply->rawHeader( "Content-Disposition" ) );
        if ( currSub->setZipData( m_reply->readAll() ) )
        {
            SUBS_LOAD_DEBUG << *lang << *currSub << "size" << currSub->zipData().size() << currSub->canDisplay();
            emit subtitleLoaded( *currSub );

            if ( !currSub->canDisplay() ) lang->subs.removeOne( currSub );
        }
        else lang->subs.removeOne( currSub );
    }

    currSub = getNextSub( lang );
    request();
}

void SubtitleFetcher::request()
{
    auto lang = language.toStrongRef();
    if ( !lang )
    {
        deleteLater();
        return;
    }
    if ( !currSub )
    {
        emit finished();
        deleteLater();
        SUBS_LOAD_DEBUG << "finished" << *lang;
        return;
    }

    retriesLeft--;
    if ( retriesLeft != SUBTITLE_DOWNLOAD_RETRIES ) SUBS_LOAD_DEBUG << lang->language() << currSub->url() << "retries left" << retriesLeft;
    if ( !m_reply.isNull() ) m_reply->deleteLater();
    QNetworkRequest req = AppConfig::getHttpHeaders();
    req.setUrl( currSub->url() );
    m_reply = QPointer<QNetworkReply>( m_man->get( req ) );
    connect( m_reply, SIGNAL( finished() ), this, SLOT( onFinished() ) );
}

QSharedPointer<SubtitleItem> SubtitleFetcher::getNextSub( QSharedPointer<SubtitleLanguage>& language )
{
    retriesLeft = SUBTITLE_DOWNLOAD_RETRIES + 1;
    if ( !language ) return QSharedPointer<SubtitleItem>();
    if ( m_fetchAll || language->subs.isEmpty() || !language->subs.first()->canDisplay() )
    {
        foreach( QSharedPointer<SubtitleItem> sub, language->subs )
        {
            if ( !sub->canDisplay() )
            {
                SUBS_LOAD_DEBUG << *sub;
                return sub;
            }
        }
    }
    return QSharedPointer<SubtitleItem>();
}

void SubtitleFetcher::updateFilename( const QByteArray& contentDisposition )
{
    static const QRegularExpression ContentDispositionMatcher( "filename[^;=\\n]*=((['\"]).*?\\2|[^;\\n]*)" );
    ///@TODO handle UTF-8 (A great mess!) http://stackoverflow.com/questions/7967079/special-characters-in-content-disposition-filename/7969807#7969807
    if ( !contentDisposition.size() ) return;
    const QRegularExpressionMatch matcher = ContentDispositionMatcher.match( contentDisposition );
    if ( matcher.hasMatch() ) currSub->setOriginalName( matcher.captured( 1 ).toLatin1() );
}


