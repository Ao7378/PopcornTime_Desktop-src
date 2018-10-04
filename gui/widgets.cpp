#include <QHBoxLayout>
#include <QLabel>
#include <QMovie>
#include <QDebug>
#include <QMouseEvent>
#include "widgets.h"

VideoWidget::VideoWidget( QWidget *parent, QString resource ) :
   QWidget( parent ), _icon( 0 ), _video( new QtAV::OpenGLWidgetRenderer() ), movie( new QMovie( ":/buffering" ) )
{
//  qDebug() << this->metaObject()->className() << __FUNCTION__;
    if ( movie->loopCount() != -1 ) connect( movie, SIGNAL( finished() ), movie, SLOT( start() ) );
    movie->setCacheMode( QMovie::CacheAll );
    movie->setScaledSize( QSize( 40, 40 ) );
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << movie << movie->currentPixmap() << movie->currentPixmap().isNull() << movie->currentPixmap().size() << movie->loopCount();


    setLayout( new QHBoxLayout( this ) );
    this->layout()->setContentsMargins( 0, 0, 0, 0 );
    this->layout()->setSpacing( 0 );
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << 2;
//  setLayout( new QHBoxLayout( this ) );
    if ( _video->widget() ) layout()->addWidget( _video->widget() );
//  else qDebug() << this->metaObject()->className() << __FUNCTION__ << "_video->widget() error";
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << layout()->count();
    setStyleSheet( "background-color:black;" );
    setImage( resource );
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << "end";
}

QtAV::VideoRenderer* VideoWidget::getVOut() { return _video; }

void VideoWidget::setImage( QString resourcePng )
{
    static QString styleSheet = "background-image: url(%1); "
       "background-position: center center; "
       "background-repeat: no-repeat;";
    if ( resourcePng.size() )
    {
        onBufferingEnd();
        if ( !_icon ) _icon = new QWidget( this );
        _icon->move( 0, 0 );
        _icon->resize( this->size() );
//      qDebug() << "new icon" << styleSheet.arg( resourcePng );
        _icon->setStyleSheet( styleSheet.arg( resourcePng ) );
        _icon->show();
    }
    else if ( _icon ) delete _icon;
}

void VideoWidget::onBufferingStart()
{
    qDebug() << this->metaObject()->className() << __FUNCTION__ << _icon << _label << movie->currentPixmap().size();
    if ( _icon ) delete _icon;
    if ( !_label ) _label = new QLabel( this );
    _label->setMovie( movie );
    _label->setAttribute( Qt::WA_TranslucentBackground, true );
    movie->start();
    _label->resize( movie->currentPixmap().size() );
    _label->move( ( width() - movie->currentPixmap().width() ) / 2, ( height() - movie->currentPixmap().height() ) / 2 );
    _label->show();
}

void VideoWidget::onBufferingEnd()
{
    qDebug() << this->metaObject()->className() << __FUNCTION__;
    movie->stop();
    if ( _label ) delete _label;
}


//void VideoWidget::setPixmap( QPixmap pixmap )
//{
//    if ( !pixmap.isNull() )
//    {
//        if ( _icon ) delete _icon;
//        if ( !_label ) _label = new QLabel( this );
//        _label->resize( pixmap.size() );
//        _label->move( ( width() - pixmap.width() ) / 2, ( height() - pixmap.height() ) / 2 );
//        qDebug() << "new icon" << pixmap;
//        _label->setPixmap( pixmap );
//        _label->show();
//    }
//    else if ( _label ) delete _label;
//
//}

void VideoWidget::bufferingFrameChanged( int frame )
{
    (void) frame;
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << "frame" << frame << movie->isValid();
//  qDebug() << this->metaObject()->className() << __FUNCTION__ << movie << movie->currentPixmap() << movie->currentPixmap().isNull() << movie->currentPixmap().size();
    _label->setPixmap( movie->currentPixmap() );
}


void VideoWidget::mouseReleaseEvent( QMouseEvent *event )
{
    emit clicked();
    event->ignore();
}

void VideoWidget::showEvent( QShowEvent * )
{
    if ( !isVisible() )
    {
//      qDebug() << this->metaObject()->className() << __FUNCTION__ << "on invisible";
        onBufferingEnd();
    }
}
