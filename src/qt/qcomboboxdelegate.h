#ifndef COMBOBOXDELEGATE_H
#define COMBOBOXDELEGATE_H

#include <QItemDelegate>
#include <QPainter>
QT_BEGIN_NAMESPACE
QT_END_NAMESPACE
class ComboBoxDelegate : public QItemDelegate
{
Q_OBJECT

public:
    explicit ComboBoxDelegate(QObject *parent = 0);
	virtual ~ComboBoxDelegate() {};
protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // COMBOBOXDELEGATE_H