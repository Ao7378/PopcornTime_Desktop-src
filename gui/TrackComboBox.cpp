#include <QFileDialog>
#include <QDir>

#include "defaults.h"
#include "TrackComboBox.h"

#include <QStandardItemModel>
#include <QSortFilterProxyModel>




TTrackComboBox::TTrackComboBox()
{
    QStandardItemModel *model = new QStandardItemModel( 0, 1, this );
    setModel( model );

    setStyleSheet( "background-color:black; color:#787878; padding: 2px 20px 2px 1px;" );
    this->setSizeAdjustPolicy( QComboBox::AdjustToContents );
    this->setEditable( false );
    connect( this, SIGNAL( activated( const QString& ) ), this, SLOT( itemSelected( const QString& ) ) );
    clearItems();
}

void TTrackComboBox::setAudioTrackList( TIndexedString list, int active )
{
    clearItems();
    qDebug() << this->metaObject()->className() << __FUNCTION__ << list << active;
    trackList = list;
    this->clear();
    foreach( const QString& key, trackList.keys() )
    {
        const int idx = trackList.value( key );
        this->insertItem( this->count(), key, idx ); //QString::number( idx ) + ". " +
        if ( active == idx ) this->setCurrentIndex( this->count() - 1 );
    }
    if ( list.size() < 2 )
    {
        clearItems();
        return;
    }
    this->setFocusPolicy( Qt::StrongFocus );
    this->setEnabled( true );
//  this->show();
}

void TTrackComboBox::clearItems()
{
//  this->hide();
    setEnabled( false );
    trackList.clear();
    this->clear();
    this->setCurrentIndex( 0 );
//  this->setFocusPolicy( Qt::NoFocus );
//  this->setEnabled( false );
    this->addItem( "No extra audio tracks" );
}

void TTrackComboBox::provide() { itemSelected( this->currentText() ); }

void TTrackComboBox::changeData( const QString& from, const QString& to )
{
    const int index = this->findText( from );
    if ( index < 0 ) return;
    this->setItemData( index, to );
    this->setItemText( index, to );
}


void TTrackComboBox::itemSelected( const QString& text )
{
    qDebug() << this->metaObject()->className() << __FUNCTION__ << text;

    foreach( const QString& key, trackList.keys() )
    {
        if ( !text.contains( key ) ) continue;
        emit newTrackIndex( trackList.value( key ) );
        qDebug() << this->metaObject()->className() << __FUNCTION__ << "selected" << trackList.value( key );
        break;
    }
}



