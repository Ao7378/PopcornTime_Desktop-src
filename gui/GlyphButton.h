#ifndef __GLYPHBUTTON_H_INCL__
    #define __GLYPHBUTTON_H_INCL__

    #include <QPushButton>
    #include <QPixmap>
    #include <QSize>

class GlyphButton : public QPushButton
{
    Q_OBJECT

public:
    GlyphButton( QString thePix, QString theHoverPix, QString thePressed = "", QString thePressedHover = "" );
    GlyphButton( QFont& font, QString symbol, QString symbolPressed = "" );

    public slots:
    void changedState( bool checked );
    void check() { setChecked( true ); }
    void uncheck() { setChecked( false ); }
    void provide() { emit toggled( isChecked() ); }
    void updateZoom( double newZoom );

protected:
    void enterEvent( QEvent *event );
    void leaveEvent( QEvent *event );
    void leave(); 

    QString pix;
    QString hoverPix;
    QString pPix;
    QString pHoverPix;
    QString sym;
    QString pSym;
    QSize size;
    double zoom = 1;
};

#endif // __GLYPHBUTTON_H_INCL__
