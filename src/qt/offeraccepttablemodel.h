#ifndef OFFERACCEPTTABLEMODEL_H
#define OFFERACCEPTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
class OfferAcceptTablePriv;
class CWallet;
class WalletModel;
enum OfferAcceptType {Accept=0,MyAccept};

typedef enum OfferAcceptType OfferAcceptModelType;


class OfferAcceptTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit OfferAcceptTableModel(CWallet *wallet, WalletModel *parent = 0, OfferAcceptModelType = MyAccept);
    ~OfferAcceptTableModel();

    enum ColumnIndex {
        Name = 0,   /**< offer name */
        GUID = 1,  /**< Offer value */
		Title = 2,  /**< Offer value */
		Height = 3,
		Price = 4,
		Currency = 5,
		Qty = 6,
		Total = 7,
		Alias = 8,
		Buyer = 9,
		Status = 10,
		NUMBER_OF_COLUMNS
    };

    enum RoleIndex {
        TypeRole = Qt::UserRole, /**< Type of offer (#Send or #Receive) */
        /** Offer name */
        NameRole,
        /** Offer accept guid */
        GUIDRole,
		AliasRole,
		BuyerRole
    };

    /** Return status of edit/insert operation */
    enum EditStatus {
        OK,                     /**< Everything ok */
        NO_CHANGES,             /**< No changes were made during edit operation */
        INVALID_OFFER,        /**< Unparseable offer */
        DUPLICATE_OFFER,      /**< Offer already in offer book */
        WALLET_UNLOCK_FAILURE  /**< Wallet could not be unlocked */
    };

    static const QString Offer;      /**< Specifies Offer's  */

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
    QString addRow(const QString &type, const QString &offer, const QString &value, const QString &title, const QString &height,const QString &price, const QString &currency, const QString &qty, const QString &total, const QString &alias, const QString &status, const QString &buyer);

    /* Look up row index of an offer in the model.
       Return -1 if not found.
     */
    int lookupOffer(const QString &offer) const;
	void clear();
	void refreshOfferTable();
    EditStatus getEditStatus() const { return editStatus; }
	EditStatus editStatus;
private:
    WalletModel *walletModel;
    CWallet *wallet;
    OfferAcceptTablePriv *priv;
    QStringList columns;
	OfferAcceptModelType modelType;
    /** Notify listeners that data changed. */
    void emitDataChanged(int index);

public Q_SLOTS:
    /* Update offer list from core.
     */
    void updateEntry(const QString &offer, const QString &value, const QString &title, const QString &height,const QString &price, const QString &currency, const QString &qty, const QString &total,  const QString &alias, const QString &status, const QString &buyer, OfferAcceptModelType type, int statusi);

    friend class OfferAcceptTablePriv;
};

#endif // OFFERACCEPTTABLEMODEL_H
