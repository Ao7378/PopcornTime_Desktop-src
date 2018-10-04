#ifndef __TORRENTIODEVICE_H_INCL__
    #define __TORRENTIODEVICE_H_INCL__

    #include <QIODevice>
    #include <QFile>

class TorrentWrapper;

class TorrentIODevice : public QIODevice
{
    Q_OBJECT;

public:
    TorrentIODevice( QObject *parent = 0 );
    ~TorrentIODevice();

    void setFileName( const QString& fileName );
    QString getFileName();
    void bindTorrentWrapper( TorrentWrapper *torrent );

    bool open();
    virtual bool open( OpenMode mode );
    virtual void close();


    virtual bool isSequential() const;
    bool isWritable() const;

    virtual qint64 pos() const;
    virtual qint64 size() const;
    virtual bool seek( qint64 pos );

    virtual qint64 readData( char *data, qint64 maxSize );
    virtual qint64 writeData( const char *data, qint64 maxSize );

    private slots:
    void abortWait();

    signals:
    // Info signals
    void bufferUnderrun();
    void buffered();

    // TorrentWrapper interaction signals
    void checkDownloaded( bool& result, quint64 start, quint64 end, int index = -1 );
    void seekRequest( quint64 start );

private:
    QFile file;
    QMutex mutex;
    volatile bool abort = false;
};
