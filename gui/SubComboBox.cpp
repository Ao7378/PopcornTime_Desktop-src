#include "defaults.h"
#include "SubComboBox.h"

#include <QStandardItemModel>
#include <QSortFilterProxyModel>

BaseSubComboBox::BaseSubComboBox() : QComboBox()
{
    setStyleSheet( "background-color:black; color:#787878; padding: 2px 20px 2px 1px;" );
    this->setSizeAdjustPolicy( QComboBox::AdjustToContents );
//  this->setInsertPolicy( QComboBox::InsertAlphabetically );
    this->setEditable( false );
}


SubComboBox::SubComboBox() : BaseSubComboBox()
{
    // Sortable ComboBox
    QStandardItemModel *model = new QStandardItemModel( 0, 1, this );
//  QSortFilterProxyModel *proxy = new QSortFilterProxyModel( this );
// may be enabled for dynamic sorting
//  proxy->setDynamicSortFilter(true);
//  proxy->setSourceModel( model );
    setModel( model );
}

void SubComboBox::sort( Qt::SortOrder sortOrder )
{
    const bool offSelected = this->currentText() == SUBTITLES_OFF;
    const bool customSelected = this->currentText() == SUBTITLES_CUSTOM;
    this->removeItem( this->findText( SUBTITLES_CUSTOM ) );
    this->removeItem( this->findText( SUBTITLES_OFF ) );
    model()->sort( -1 ); // required to reapply sorting
    model()->sort( 0, sortOrder );
    this->insertItem( 0, SUBTITLES_CUSTOM, QString( SUBTITLES_CUSTOM ) );
    this->insertItem( 0, SUBTITLES_OFF, QString( SUBTITLES_OFF ) );
    if ( offSelected ) this->setCurrentIndex( this->findText( SUBTITLES_OFF ) );
    else if ( customSelected ) this->setCurrentIndex( this->findText( SUBTITLES_CUSTOM ) );
}

void SubComboBox::updateItemList( QStringList strings )
{
    const QString selected = currentText();

    clear();
//  if ( findText( SUBTITLES_OFF ) < 0 ) this->addItem( SUBTITLES_OFF, SUBTITLES_OFF );
//  if ( findText( SUBTITLES_CUSTOM ) < 0 ) this->addItem( SUBTITLES_CUSTOM, SUBTITLES_CUSTOM );

    foreach( QString str, strings ) insertItem( 0, str, str );
    this->sort( Qt::AscendingOrder );
    select( selected );
}

void SubComboBox::select( QString selected )
{
    int i = this->findText( selected );
    if ( i >= 0 ) this->setCurrentIndex( i );
    else
    {
        this->setCurrentIndex( this->findText( SUBTITLES_OFF ) );
        emit activated( SUBTITLES_OFF );
    }
}

void SubComboBox::clear()
{
    QComboBox::clear();
    addItem( SUBTITLES_OFF, SUBTITLES_OFF );
    addItem( SUBTITLES_CUSTOM, SUBTITLES_CUSTOM );
    setCurrentIndex( 0 );
    this->setFocusPolicy( Qt::StrongFocus );
//  setFocusPolicy( Qt::NoFocus );
//  setEnabled( false );
}



void VariantComboBox::updateItemList( QStringList strings )
{
    const QString selected = currentText();

    clear();
//  if ( findText( SUBTITLES_OFF ) < 0 ) this->addItem( SUBTITLES_OFF, SUBTITLES_OFF );
//  if ( findText( SUBTITLES_CUSTOM ) < 0 ) this->addItem( SUBTITLES_CUSTOM, SUBTITLES_CUSTOM );

    foreach( QString str, strings ) insertItem( 0, str, str );

    int i = this->findText( selected );
    if ( i >= 0 ) this->setCurrentIndex( i );
    else emit activated( 0 );
}

void VariantComboBox::clear()
{
    QComboBox::clear();
    setCurrentIndex( 0 );
    this->setFocusPolicy( Qt::StrongFocus );
//  setFocusPolicy( Qt::NoFocus );
//  setEnabled( false );
}


