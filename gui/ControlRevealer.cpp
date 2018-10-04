#include "ControlRevealer.h"
#include "defaults.h"
#include <QTimer>
#include <QDebug>

ControlRevealer::ControlRevealer( QObject *parent ) : QObject( parent ),
   m_enabled( false ), m_permanent( 0 ), timer( new QTimer( this ) )
{
    timer->setSingleShot( true );
    timer->setInterval( CONTROL_HIDE_INTERVAL_MOUSE_AWAY_MS );
    connect( timer, SIGNAL( timeout() ), this, SLOT( onTimeout() ) );
}

void ControlRevealer::enableReveal()
{
//  qDebug() << "enableReveal";
    m_enabled = true;
    m_permanent = 0;
    temporaryReveal();
}

void ControlRevealer::disableReveal()
{
//  qDebug() << "disableReveal";
    m_enabled = false;
    conceal();
}

void ControlRevealer::temporaryReveal()
{
    if ( m_permanent || !m_enabled ) return;
    if ( timer->isActive() )
    {
//      qDebug() << "temporaryReveal with active timer";
        timer->start();
        return;
    }
//  else qDebug() << "temporaryReveal";

    timer->start();
    reveal();
}

void ControlRevealer::startPermanentReveal()
{
    timer->stop();
//  qDebug() << "startPermanentReveal" << m_permanent;
    ++m_permanent;
    if ( m_enabled ) reveal();
}

void ControlRevealer::stopPermanentReveal()
{
//  qDebug() << "stopPermanentReveal" << m_permanent;
    if ( m_permanent ) --m_permanent;
    if ( !m_permanent && m_enabled ) timer->start();
}

void ControlRevealer::onTimeout()
{
    timer->stop();
//  qDebug() << "concealOnTimeout";
    conceal();
}

void ControlRevealer::reveal()
{
//  qDebug() << "revealing";
    emit revealed();
//  qDebug() << "revealed";
}

void ControlRevealer::conceal()
{
//  qDebug() << "concealing";
    timer->stop();
    emit concealed();
//  qDebug() << "concealed";
}
