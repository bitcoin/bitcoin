#ifndef OFFERWHITELISTTABLEMODEL_H
#define OFFERWHITELISTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class OfferWhitelistTablePriv;
class CWallet;
class WalletModel;


class OfferWhitelistTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit OfferWhitelistTableModel(WalletModel *parent = 0);
    ~OfferWhitelistTableModel();

    enum ColumnIndex {
		Alias = 0,
		Discount = 1,
		Expires = 2,
		NUMBER_OF_COLUMNS
    };

    enum RoleIndex {
        AliasRole = Qt::UserRole
    };
    /** Return status of edit/insert operation */
    enum EditStatus {
        OK,                     /**< Everything ok */
        NO_CHANGES,             /**< No changes were made during edit operation */
        INVALID_ENTRY,        /**< Unparseable entry */
        DUPLICATE_ENTRY,      /**< entry already in book */
        WALLET_UNLOCK_FAILURE  /**< Wallet could not be unlocked */
    };


    /** @name Methods overridden from QAbstractTableModel
        @{*/
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    /*@}*/

    /* Add an offer to the model.
       Returns the added offer on success, and an empty string otherwise.
     */
    QString addRow(const QString &alias, const QString &expires,const QString &discount);

    /* Look up row index of a alias in the model.
       Return -1 if not found.
     */
    int lookupEntry(const QString &alias) const;
	void clear();
    EditStatus getEditStatus() const { return editStatus; }
	EditStatus editStatus;
private:
    WalletModel *walletModel;
    OfferWhitelistTablePriv *priv;
    QStringList columns;

    /** Notify listeners that data changed. */
    void emitDataChanged(int index);

public Q_SLOTS:
    /* Update offer list from core.
     */
    void updateEntry(const QString &alias, const QString &expires,const QString &discount, int status);

    friend class OfferWhitelistTablePriv;
};

#endif // OFFERWHITELISTTABLEMODEL_H
