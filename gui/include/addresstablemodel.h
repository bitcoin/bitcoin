#ifndef ADDRESSTABLEMODEL_H
#define ADDRESSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class AddressTablePriv;

class AddressTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AddressTableModel(QObject *parent = 0);
    ~AddressTableModel();

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
    QModelIndex index ( int row, int column, const QModelIndex & parent ) const;
private:
    AddressTablePriv *priv;
    QStringList columns;
signals:

public slots:

};

#endif // ADDRESSTABLEMODEL_H
