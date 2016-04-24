#ifndef ESCROWTABLEMODEL_H
#define ESCROWTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class EscrowTablePriv;
class CWallet;
class WalletModel;
enum EscrowType {AllEscrow=0,MyEscrow};

typedef enum EscrowType EscrowModelType;
/**
   Qt model of the escrow book in the core. This allows views to access and modify the escrow book.
 */
class EscrowTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit EscrowTableModel(CWallet *wallet, WalletModel *parent = 0, EscrowModelType = MyEscrow);
    ~EscrowTableModel();

    enum ColumnIndex {
        Escrow = 0,  
		Time = 1, 
		Seller = 2,
        Arbiter = 3,  
		Buyer = 4,
		Offer = 5,
		Offer = 6,
		OfferAccept = 7,
		Total = 8,
		Status = 9

    };

    enum RoleIndex {
        TypeRole = Qt::UserRole,
		EscrowRole,
		BuyerRole,
		SellerRole,
		ArbiterRole,
		StatusRole
    };

    /** Return status of edit/insert operation */
    enum EditStatus {
        OK,                     /**< Everything ok */
        NO_CHANGES,             /**< No changes were made during edit operation */
        INVALID_ESCROW,        /**< Unparseable escrow */
        DUPLICATE_ESCROW,      /**< Escrow already in escrow book */
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

    /* Add an escrow to the model.
       Returns the added escrow on success, and an empty string otherwise.
     */
    QString addRow(const QString &escrow, const QString &time, const QString &seller, const QString &arbiter, const QString &offer,const QString &offertitle, const QString &offeraccept, const QString &total, const QString &status, const QString &buyer);

    /* Look up row index of an escrow in the model.
       Return -1 if not found.
     */
    int lookupEscrow(const QString &escrow) const;
	void clear();
	void refreshEscrowTable();
    EditStatus getEditStatus() const { return editStatus; }
	EditStatus editStatus;
private:
    WalletModel *walletModel;
    CWallet *wallet;
    EscrowTablePriv *priv;
    QStringList columns;
    
	EscrowModelType modelType;
    /** Notify listeners that data changed. */
    void emitDataChanged(int index);

public Q_SLOTS:
    /* Update escrow list from core.
     */
    void updateEntry(const QString &escrow, const QString &time, const QString &seller, const QString &arbiter, const QString &offer, const QString &offertitle, const QString &offeraccept, const QString &total, const QString &status, const QString &buyer, EscrowModelType type, int statusi);

    friend class EscrowTablePriv;
};

#endif // ESCROWTABLEMODEL_H
