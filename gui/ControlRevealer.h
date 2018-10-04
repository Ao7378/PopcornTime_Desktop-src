#ifndef __CONTROLREVEALER_H_INCL__
    #define __CONTROLREVEALER_H_INCL__

    #include <QObject>

class QTimer;

class ControlRevealer : public QObject
{
    Q_OBJECT

public:
    ControlRevealer( QObject *parent );

    public slots:
    void enableReveal();
    void disableReveal();

    void temporaryReveal();
    void startPermanentReveal();
    void stopPermanentReveal();

    private slots:
    void onTimeout();

    signals:
    void revealed();
    void concealed();

private:
    void reveal();
    void conceal();
    bool isTemporaryRevealed(); 
    bool m_enabled;
    unsigned int m_permanent;
    QTimer *timer;
};



#endif // __CONTROLREVEALER_H_INCL__
