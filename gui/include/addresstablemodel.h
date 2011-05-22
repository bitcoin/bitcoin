#ifndef ADDRESSTABLEMODEL_H
#define ADDRESSTABLEMODEL_H

#include <QAbstractTableModel>

class AddressTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AddressTableModel(QObject *parent = 0);

    enum {
        Label = 0,   /* User specified label */
        Address = 1  /* Bitcoin address */
    } ColumnIndex;

    enum {
        TypeRole = Qt::UserRole
    } RoleIndex;

    static const QString Send; /* Send addres */
    static const QString Receive; /* Receive address */

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
signals:

public slots:

};

#endif // ADDRESSTABLEMODEL_H
