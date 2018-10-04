#ifndef __SUBCOMBOBOX_H_INCL__
    #define __SUBCOMBOBOX_H_INCL__

    #include <QComboBox>
    #include <QStringList>
    #include <QSharedPointer>
    #include "commontypes.h"
    #include "SubtitleDecoder.h"

class BaseSubComboBox : public QComboBox
{
    Q_OBJECT

public:
    BaseSubComboBox();
};

class SubComboBox : public BaseSubComboBox
{
    Q_OBJECT

public:
    SubComboBox();

    public slots:
    void sort( Qt::SortOrder sortOrder = Qt::AscendingOrder );
    void clear();
    void updateItemList( QStringList strings );
    void select( QString selected );

protected:
};

class VariantComboBox : public BaseSubComboBox
{
    Q_OBJECT
       
       public slots:
    void clear();
    void updateItemList( QStringList strings );
};

#endif // __SUBCOMBOBOX_H_INCL__

