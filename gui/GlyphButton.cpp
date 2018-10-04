#include "GlyphButton.h"

GlyphButton::GlyphButton( QString thePix, QString theHoverPix, QString thePressed, QString thePressedHover )
   : pix( thePix ), hoverPix( theHoverPix ), pPix( thePressed ), pHoverPix( thePressedHover )
{
    setStyleSheet( "QPushButton:flat { border: none; }" );
    setCheckable( thePressed.size() && thePressedHover.size() );
    setFlat( true );
    setIcon( QIcon( ":/" + pix ) );
    QPixmap p = QPixmap( ":/" + pix );
    size = p.size();
    setIconSize( size );
    resize( size );
    connect( this, SIGNAL( toggled( bool ) ), this, SLOT( changedState( bool ) ) );
}

GlyphButton::GlyphButton( QFont& font, QString symbol, QString symbolPressed )
   : sym( symbol ), pSym( symbolPressed )
{
    setStyleSheet( "QPushButton:flat { border: none;color:#999999; } "
                   "QPushButton:flat:hover { border: none;color:white; } "
                   "QPushButton::menu-indicator { image: ""; }" );

    setCheckable( symbolPressed.size() );
    setFlat( true );
    setFont( font );
    setText( sym );

    size = QSize( font.pixelSize(), font.pixelSize() );
    resize( size );
    connect( this, SIGNAL( toggled( bool ) ), this, SLOT( changedState( bool ) ) );
}


void GlyphButton::enterEvent( QEvent *event )
{
    QPushButton::enterEvent( event );
    if ( pix.size() ) changedState( isChecked() );
}

void GlyphButton::leaveEvent( QEvent *event )
{
    QPushButton::leaveEvent( event );
    leave();
}

void GlyphButton::changedState( bool checked )
{
    if ( pix.size() )
    {
        QPixmap p;
        if ( isCheckable() && checked && pHoverPix.size() ) p = QPixmap( ":/" + pHoverPix );
        else p = QPixmap( ":/" + hoverPix );
        setIcon( p.scaled( size * zoom ) );
        setIconSize( size * zoom );
    }
    else if ( sym.size() )
    {
        if ( isCheckable() && checked && pSym.size() ) setText( pSym );
        else setText( sym );
    }
}

void GlyphButton::updateZoom( double theZoom )
{
    if ( !pix.size() ) return; 
    zoom = theZoom;
    resize( size * zoom );
    leave();
}

void GlyphButton::leave()
{
    if ( !pix.size() ) return;
    QPixmap p;
    if ( isChecked() && pPix.size() ) p = QPixmap( ":/" + pPix );
    else p = QPixmap( ":/" + pix );
    setIcon( p.scaled( size * zoom ) );
    setIconSize( size * zoom );
}
