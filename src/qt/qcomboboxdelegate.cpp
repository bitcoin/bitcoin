#include "qcomboboxdelegate.h"

ComboBoxDelegate::ComboBoxDelegate(QObject *parent)
:QItemDelegate(parent)
{
}
void ComboBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator"))
    {
        painter->setPen(Qt::gray);
        painter->drawLine(option.rect.left(), option.rect.center().y(), option.rect.right(), option.rect.center().y());
    }
    else if(index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("parent"))
    {
        QStyleOptionViewItem parentOption = option;
        parentOption.state |= QStyle::State_Enabled;
        QItemDelegate::paint( painter, parentOption, index );
    }
    else if ( index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String( "child" ) ) {
        QStyleOptionViewItem childOption = option;
        int indent = option.fontMetrics.width( QString( 4, QChar( ' ' ) ) );
        childOption.rect.adjust( indent, 0, 0, 0 );
        childOption.textElideMode = Qt::ElideNone;
        QItemDelegate::paint( painter, childOption, index );
    }
    else
    {
        QItemDelegate::paint(painter, option, index);
    }
}

QSize ComboBoxDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString type = index.data(Qt::AccessibleDescriptionRole).toString();
    if(type == QLatin1String("separator"))
        return QSize(0, 10);
    return QItemDelegate::sizeHint( option, index );
}