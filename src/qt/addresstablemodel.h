#ifndef ADDRESSTABLEMODEL_H
#define ADDRESSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class AddressTablePriv;
class CWallet;
class WalletModel;

class AddressTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AddressTableModel(CWallet *wallet, WalletModel *parent = 0);
    ~AddressTableModel();

    enum ColumnIndex {
        Label = 0,   /* User specified label */
        Address = 1  /* Bitcoin address */
    };

    enum RoleIndex {
        TypeRole = Qt::UserRole
    };

    // Return status of last edit/insert operation
    enum EditStatus {
        OK,
        INVALID_ADDRESS,
        DUPLICATE_ADDRESS,
        WALLET_UNLOCK_FAILURE,
        KEY_GENERATION_FAILURE
    };

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
    Qt::ItemFlags flags(const QModelIndex & index) const;

    /* Add an address to the model.
       Returns the added address on success, and an empty string otherwise.
     */
    QString addRow(const QString &type, const QString &label, const QString &address);

    /* Update address list from core. Invalidates any indices.
     */
    void updateList();

    /* Look up label for address in address book, if not found return empty string.
     */
    QString labelForAddress(const QString &address) const;

    /* Look up row index of an address in the model.
       Return -1 if not found.
     */
    int lookupAddress(const QString &address) const;

    EditStatus getEditStatus() const { return editStatus; }

private:
    WalletModel *walletModel;
    CWallet *wallet;
    AddressTablePriv *priv;
    QStringList columns;
    EditStatus editStatus;

signals:
    void defaultAddressChanged(const QString &address);

public slots:
    void update();
};

#endif // ADDRESSTABLEMODEL_H
