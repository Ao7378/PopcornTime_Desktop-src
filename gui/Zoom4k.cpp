#include "Zoom4k.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>

int Zoom4k::currentZoomFactorPercent = -1;
int Zoom4k::currentWidth = 0;
bool Zoom4k::m_4k = false;

const int FHD = 1920;
const int UHD = 3840;
const float uhdRatio = 2.36f;
const float hdRatio = 1.0f;

int Zoom4k::zoomFactorPercent()
{
    calculate();
//  qDebug() << __FUNCTION__ << currentZoomFactorPercent << currentUnadjustedZoomFactorPercent;
    return currentZoomFactorPercent;
}

void Zoom4k::calculate()
{
    const QWidget *window = QApplication::activeWindow();
    const int width = ( window ? QApplication::desktop()->availableGeometry( window ) : QApplication::desktop()->availableGeometry() ).width();
    if ( currentWidth == width ) return;
    currentWidth = width;
    m_4k = width >= 3500; 

    int newZoomFactorPercent = 100;
    if ( width >= UHD ) newZoomFactorPercent = 200;
    else if ( width > FHD ) newZoomFactorPercent = ( ( width - FHD ) * ( uhdRatio - hdRatio ) / ( UHD - FHD ) + 1 ) * 100;
    currentZoomFactorPercent = newZoomFactorPercent;
    qDebug() << __FUNCTION__ << width << ( m_4k ? "4k" : "" ) << currentZoomFactorPercent;
}
