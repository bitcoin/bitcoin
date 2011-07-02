#ifndef ADDRESSTABLEMODEL_H
#define ADDRESSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class AddressTablePriv;
class CWallet;

class AddressTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AddressTableModel(CWallet *wallet, QObject *parent = 0);
    ~AddressTableModel();

    enum ColumnIndex {
        Label = 0,   /* User specified label */
        Address = 1  /* Bitcoin address */
    };

    enum {
        TypeRole = Qt::UserRole
    } RoleIndex;

    static const QString Send; /* Send addres */
    static const QString Receive; /* Receive address */

    /* Overridden methods from QAbstractTableModel */
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent) const;
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());

    /* Add an address to the model.
       Returns the added address on success, and an empty string otherwise.
     */
    QString addRow(const QString &type, const QString &label, const QString &address);

    /* Update address list from core. Invalidates any indices.
     */
    void updateList();

private:
    CWallet *wallet;
    AddressTablePriv *priv;
    QStringList columns;

signals:
    void defaultAddressChanged(const QString &address);

public slots:
    void update();
};

#endif // ADDRESSTABLEMODEL_H
