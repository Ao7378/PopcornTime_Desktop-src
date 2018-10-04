#ifndef __TRACKCOMBOBOX_H_INCL__
    #define __TRACKCOMBOBOX_H_INCL__

    #include <QComboBox>
    #include <QStringList>
    #include <QSharedPointer>
    #include "commontypes.h"

class TTrackComboBox : public QComboBox
{
    Q_OBJECT

public:
    TTrackComboBox();

    public slots:
    void setAudioTrackList( TIndexedString list, int active );
//  void sort( Qt::SortOrder sortOrder = Qt::AscendingOrder );
    void clearItems();
    void provide();

    private slots:
    void itemSelected( const QString& text );

    signals:
    void newTrackIndex( int );

protected:
    void changeData( const QString& from, const QString& to );
    TIndexedString trackList;
};


#endif // __TRACKCOMBOBOX_H_INCL__
