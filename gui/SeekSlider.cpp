#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>

#include <QDebug>
#include <QStyleOption>
#include <QPainter>


#include "SeekSlider.h"
#include "defaults.h"
#include "Zoom4k.h"



SeekSlider::SeekSlider( Qt::Orientation orientation, QWidget *parent )
   : QSlider( orientation, parent ), fullSize( 0 ), currentSize( 0 ), playablePart( 0 )
{ }


void SeekSlider::updateCurrentSize( quint64 size )
{
    currentSize = size; // >> 8;
    if ( currentSize > fullSize ) currentSize = fullSize;
//  qDebug() << "updCurDl" << currentSize << "of" << fullSize;
}

void SeekSlider::updateFullSize( quint64 size )
{
    fullSize = size; // >> 8;
//  qDebug() << "updCurFull" << fullSize << "(" << currentSize << ")";
}

void SeekSlider::paintEvent( QPaintEvent *event )
{
    QSlider::paintEvent( event );
    QStyleOptionSlider opt;
    initStyleOption( &opt );
    const QRect gRect = style()->subControlRect( QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this );
    const QRect hRect = style()->subControlRect( QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this );

    int length = 1;
    const int zoomFactor = Zoom4k::zoomFactorInt();

    if ( currentSize == fullSize ) length = gRect.width();
    else if ( fullSize ) length = gRect.width() * currentSize / fullSize;
    if ( length < 1 ) length = 1;
    else if ( length > gRect.width() - 2 ) length = gRect.width() - 2;
    const int top = gRect.top() + gRect.height() / 2 - 3 * zoomFactor;
    const int height = gRect.height() - 4 * zoomFactor;

    QPainter p( this );
    QBrush brush = QBrush( QColor( 47, 111, 214 ) );

    int start = gRect.left();
    int end = start + length;
    if ( hRect.left() < end )
    {
        end = hRect.left();
        p.fillRect( QRect( start, top, end - start, height ), brush );
        length -= end - start - 2 + hRect.width();
        start = hRect.right() + 1;
    }
    end = start + length;
    p.fillRect( QRect( start, top, end - start, height ), brush );
}

bool SeekSlider::canStillPlay( int time )
{
    if ( arbitrarySeek ) return true;
//  qDebug() << "size" << getCurrentSize() << getFullSize() << "time" << value() << maximum();
//  if ( getFullSize() && maximum() ) qDebug() << "ratios" << QString::number( float(getCurrentSize()) / getFullSize(), 'f', 6 ) << QString::number( float(value()) / maximum(), 'f', 6 );
    if ( !getFullSize() || !maximum() || fullSize == currentSize ) return true; //|| time < DONT_PAUSE_IF_JUST_STARTED_TIME_MS
    const double playRatio = float( time ) / float( maximum() );
    return playRatio < playablePart;
}


SeekControl::SeekControl( QWidget *parent )
   : QWidget( parent )
{
    initWidgetSeek();
}

SeekControl::SeekControl( QLabel *elapsed, QLabel *full, QLabel *byteRate )
{
    initWidgetSeek();
    setLabels( elapsed, full, byteRate );
}

void SeekControl::initWidgetSeek()
{
    _lock = false;

    _seek = new SeekSlider( Qt::Horizontal, this );
    _seek->setMaximum( 1 );
    _seek->setAttribute( Qt::WA_TransparentForMouseEvents );

    QHBoxLayout *layout = new QHBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );
    layout->addWidget( _seek );
    updateZoom( 1 );
}

void SeekControl::updateZoom( double zoomFactor )
{
    _seek->setMinimumHeight( 12 * zoomFactor );
    _seek->setStyleSheet( QString(
                             "QSlider { margin: 0px; }"
                             "QSlider::groove:horizontal {"
                             " border: 0px;"
                             " height: %1px; /* the groove expands to the size of the slider by default. by giving it a height, it has a fixed size */"
                             " background: #999999;"
                             " margin: %2px 0; }"
                             "QSlider::handle:horizontal {"
                             " background: white;"
                             " border: 0px solid #5c5c5c;"
                             " width: %3px;"
                             " margin: -%2px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */"
                             " border-radius: %4px;}" ).arg( 6 * zoomFactor ).arg( 2 * zoomFactor ).arg( 10 * zoomFactor ).arg( 5 * zoomFactor ) );

}

void SeekControl::mouseMoveEvent( QMouseEvent *event )
{
    event->ignore();

    if ( !_lock ) return;

    updateEvent( event->pos() );
}

void SeekControl::mousePressEvent( QMouseEvent *event )
{
    event->ignore();

    lock();
}

void SeekControl::mouseReleaseEvent( QMouseEvent *event )
{
    event->ignore();

    updateEvent( event->pos() );

    QTimer::singleShot( 500, [=]() 
                                   {
                                       unlock(); 
                                   } );
}

//void SeekControl::wheelEvent( QWheelEvent *event )
//{
//    event->ignore();
//
//    if ( !_vlcMediaPlayer ) return;
//
//    if ( event->delta() > 0 ) _vlcMediaPlayer->setTime( _vlcMediaPlayer->time() + _vlcMediaPlayer->length() * 0.01 );
//    else _vlcMediaPlayer->setTime( _vlcMediaPlayer->time() - _vlcMediaPlayer->length() * 0.01 );
//}

void SeekControl::end()
{
    QTime time = QTime( 0, 0, 0, 0 );
    QString display = "mm:ss";

    if ( !_labelElapsed.isNull() ) _labelElapsed->setText( time.toString( display ) );
    if ( !_labelFull.isNull() ) _labelFull->setText( time.toString( display ) );
    _seek->setMaximum( 1 );
    _seek->setValue( 0 );
}

void SeekControl::updateEvent( const QPoint& pos )
{
    if ( pos.x() < _seek->pos().x() || pos.x() > _seek->pos().x() + _seek->width() ) return;

    int clickPos = pos.x() - _seek->pos().x();
    if ( !_seek->canArbitrarySeek() && clickPos * _seek->getFullSize() >= _seek->getCurrentSize() * _seek->width() )
    {
        qDebug() << this->metaObject()->className() << "can't seek here";
        return;
    }
    float op = _seek->maximum() / _seek->width();
    float newValue = clickPos * op;

    emit sought( newValue );
    _seek->setValue( newValue );
}

void SeekControl::updateCurrentTime( int time )
{
    if ( _lock ) return;

    if ( !_seek->canStillPlay( time ) )
    {
        emit forcePauseonBuffering();
        emit updateStatusLabel( TK_NET_BUFFERING );
//      qDebug() << "paused due to ratio";
    }
//  else
//      if ( time < _seek->value() )
//  {
//      qDebug() << this->metaObject()->className() << __FUNCTION__ << _seek->value() >> " -> " << time;
//      emit timeOnRewind( _seek->value() );
//  }


    QTime currentTime = QTime( 0, 0, 0, 0 ).addMSecs( time );

    QString display = "mm:ss";
    if ( currentTime.hour() > 0 ) display = "hh:mm:ss";

    if ( !_labelElapsed.isNull() ) _labelElapsed->setText( currentTime.toString( display ) );
    _seek->setValue( time );
}


void SeekControl::updateFullTime( int time )
{
    if ( _lock )
    {
        qDebug() << __FUNCTION__ << "lock error";
        return;
    }

    QTime fullTime = QTime( 0, 0, 0, 0 ).addMSecs( time );

    QString display = "mm:ss";
    if ( fullTime.hour() > 0 ) display = "hh:mm:ss";

    if ( !_labelFull.isNull() ) _labelFull->setText( fullTime.toString( display ) );
    qDebug() << __FUNCTION__ << time << fullTime.toString( display );

    _seek->setMaximum( time ? time : 1 );
}

void SeekControl::setLabels( QLabel *elapsed, QLabel *full, QLabel *byteRate )
{
    _labelElapsed = elapsed;
    _labelFull = full;
    _labelByteRate = byteRate;

    if ( !_labelElapsed.isNull() )
    {
        _labelElapsed->setText( "00:00" );
        _labelElapsed->setStyleSheet( "QLabel { color : white;}" );
    }
    if ( !_labelFull.isNull() )
    {
        _labelFull->setText( "00:00" );
        _labelFull->setStyleSheet( "QLabel { color : #999999;}" );
    }

}

void SeekControl::downloadInfoChange( DownloadInfo info )
{
    unsigned int piecesDone = 0;
    double ratio = 0;
    unsigned int piecesTotal = info.piecesTotal - info.piecesStartupAtEnd;

    if ( info.piecesDone == info.piecesTotal ) piecesDone = piecesTotal = info.piecesTotal;
    else piecesDone = info.piecesDone - info.piecesStartup;
    if ( piecesDone < 1 ) piecesDone = 1;

    if ( info.piecesDone == info.piecesTotal ) ratio = 1;
    else ratio = float( piecesDone ) / float( piecesTotal );
//  qDebug() << __FUNCTION__ << "pieces" << piecesDone << piecesTotal << ratio << "real" << info.piecesDone << info.piecesTotal;

    _seek->updateFullSize( piecesTotal );
    _seek->updateCurrentSize( piecesDone );
    _seek->updatePlayablePart( ratio );

    if ( !_seek->canStillPlay( _seek->value() ) ) emit updateStatusLabel( TK_NET_BUFFERING );
    else emit needTextUpdate();

    if ( !_labelByteRate.isNull() )
    {
        _labelByteRate->setText( "-" );
        _labelByteRate->setStyleSheet( "color:#999999;" );
        if ( info.total && info.downloaded )
        {
//          qDebug() << "size" << info.total << "time" << _seek->maximum() << "rate" << info.total / _seek->maximum() << "dlrate" << info.downloadRateBs;
            if ( _seek->maximum() > 1 ) kByteRateMedia = info.total / _seek->maximum();
            const int kByteRateDownload = info.downloadRateBs / 1000;

            _labelByteRate->setText( QString( " %1 Kb/s" ).arg( kByteRateDownload ) );
//          byteRateLabel->setText( QString( " %1/%2" ).arg( kByteRateDownload ).arg( kByteRateMedia ) );
            if ( kByteRateDownload < kByteRateMedia ) _labelByteRate->setStyleSheet( "color:red;" );
            else _labelByteRate->setStyleSheet( "color:green;" );
        }
    }
}

