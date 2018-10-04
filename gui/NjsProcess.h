#ifndef __NJSPROCESS_H_INCL__
    #define __NJSPROCESS_H_INCL__

    #include <QProcess>
    #include <QByteArray>

class NjsProcess : public QProcess
{
    Q_OBJECT

public:
    NjsProcess( QObject *parent );
    ~NjsProcess();

    signals:
    void haveToken( QByteArray key, QByteArray value );

    public slots:
    void start();
    void restart();

    private slots:
    void onReadyRead();

protected:
    QByteArray buffer;

};

#endif // __NJSPROCESS_H_INCL__
