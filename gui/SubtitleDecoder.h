#ifndef __SUBTITLEDECODER_H_INCL__
    #define __SUBTITLEDECODER_H_INCL__

    #include <QMap>
    #include <QByteArray>
    #include <QPointer>
    #include "commontypes.h"

typedef QList<QByteArray> TEncodingList;

class DictionaryThread;

class SubtitleDecoder
{
public:
    SubtitleDecoder();
    ~SubtitleDecoder();

    QByteArray decodeToUtf8( const SubtitleItem& item );

protected:
    QByteArray decodeByBom( QByteArray data );
    bool iconvRecodeToUtf8( const QByteArray codec, const QByteArray& src, QByteArray& dst );
    int match( const QByteArray langAbbr, QByteArray utf8Data );
    QMap<QByteArray, TEncodingList> languageToEncoding;
    QPointer<DictionaryThread> dt;
};


#endif // __SUBTITLEDECODER_H_INCL__
