#ifndef __WIDGETS_H_INCL__
    #define __WIDGETS_H_INCL__

    #include <QWidget>
    #include <QPointer>
    #include "VideoPlayer.h"
    #include "QtAVWidgets/OpenGLWidgetRenderer.h"

class IntroWidget : public QWidget
{
public:
    IntroWidget( QWidget *parent = 0 ) : QWidget( parent ) { }

protected:
    virtual void paintEvent( QPaintEvent *event );
};

class TransparentWidget : public QWidget
{
public:
    TransparentWidget( QWidget *parent ) : QWidget( parent )
    {
        setWindowFlags( Qt::FramelessWindowHint | Qt::Tool ); //Qt::WindowStaysOnTopHint |
        setWindowOpacity( .01 );
        show();
//        raise(); //Crashes here sometimes ///@TODO investigate
    }

protected:
    virtual void closeEvent( QCloseEvent * ) { parentWidget()->close(); }
};

class QLabel;
class QMovie;

class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    VideoWidget( QWidget *parent = 0, QString resource = "" );
    QtAV::VideoRenderer* getVOut();

    public slots:
    void setImage( QString resourcePng );
//  void setPixmap( QPixmap pixmap );
    void onBufferingStart();
    void onBufferingEnd();

    private slots:
    void bufferingFrameChanged( int );

    signals:
    void clicked();

protected:
    virtual void showEvent( QShowEvent *event );
    virtual void mouseReleaseEvent( QMouseEvent *e );
    QPointer<QWidget> _icon;
    QPointer<QLabel> _label;
    QMovie *movie;
    QPointer<QtAV::OpenGLWidgetRenderer> _video;
};





#endif // __WIDGETS_H_INCL__
