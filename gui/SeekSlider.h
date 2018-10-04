#ifndef __SEEKSLIDER_H_INCL__
    #define __SEEKSLIDER_H_INCL__

    #include <QtCore/QPoint>
    #include <QtWidgets/QWidget>
    #include <QtWidgets/QSlider>
    #include <QPointer>
    #include "commontypes.h"

class QLabel;
class QTimer;


class SeekSlider : public QSlider
{
    Q_OBJECT

public:
    SeekSlider( Qt::Orientation orientation, QWidget *parent = 0 );
    void updateCurrentSize( quint64 size );
    void updateFullSize( quint64 size );
    void updatePlayablePart( double part ) { playablePart = part; }
    int getCurrentSize() { return currentSize; }
    int getFullSize() { return fullSize; }
    bool canStillPlay( int time );
    void setArbitrarySeek( bool enabled ) { arbitrarySeek = enabled; }
    bool canArbitrarySeek() { return arbitrarySeek; }

protected:
    virtual void paintEvent( QPaintEvent *event );
    int fullSize;
    int currentSize;
    double playablePart;
    bool arbitrarySeek = true;
};

class SeekControl : public QWidget
{
    Q_OBJECT

public:
    explicit SeekControl( QWidget *parent = 0 );
    SeekControl( QLabel *elapsed, QLabel *full, QLabel *byteRate );

    int getCurrentSize() { return _seek->getCurrentSize(); }
    int getFullSize() { return _seek->getFullSize(); }
    void setLabels( QLabel *elapsed, QLabel *full, QLabel *byteRate );
    void setArbitrarySeek( bool enabled ) { _seek->setArbitrarySeek( enabled ); }
    void updateZoom( double zoomFactor );

    public slots:
    void downloadInfoChange( DownloadInfo info );
    void rewind() { updateCurrentTime( 0 ); }

    private slots:
    void end();
    void updateCurrentTime( int time );
    void updateFullTime( int time );

    signals:
    void sought( int time );
    void timeOnRewind( int time );
    void forcePauseonBuffering();
    void needTextUpdate();
    void updateStatusLabel( const QString& text );

protected:
    void mouseMoveEvent( QMouseEvent *event );
    void mousePressEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent *event );
//  void wheelEvent( QWheelEvent *event );

private slots:
    void lock() { _lock = true; }
    void unlock() { _lock = false; }

private:
    void initWidgetSeek();
    void updateEvent( const QPoint& pos );

    volatile bool _lock;

    SeekSlider *_seek;
    QPointer<QLabel> _labelElapsed;
    QPointer<QLabel> _labelFull;
    QPointer<QLabel> _labelByteRate;
    int kByteRateMedia = 0;
};

#endif // __SEEKSLIDER_H_INCL__
