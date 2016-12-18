#ifndef MYOFFERWHITELISTTABLEMODEL_H
#define MYOFFERWHITELISTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class MyOfferWhitelistTablePriv;
class CWallet;
class WalletModel;


class MyOfferWhitelistTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit MyOfferWhitelistTableModel(WalletModel *parent = 0);
    ~MyOfferWhitelistTableModel();

    enum ColumnIndex {
		Offer = 0,
		Alias = 1,
		Discount = 2,
		Expires = 3,
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
    QString addRow(const QString &offer, const QString &alias, const QString &expires,const QString &discount);

	void clear();
    EditStatus getEditStatus() const { return editStatus; }
	EditStatus editStatus;
private:
    WalletModel *walletModel;
    MyOfferWhitelistTablePriv *priv;
    QStringList columns;

    /** Notify listeners that data changed. */
    void emitDataChanged(int index);

public Q_SLOTS:
    /* Update offer list from core.
     */
    void updateEntry(const QString &offer, const QString &alias, const QString &expires,const QString &discount, int status);

    friend class MyOfferWhitelistTablePriv;
};

#endif // MYOFFERWHITELISTTABLEMODEL_H
