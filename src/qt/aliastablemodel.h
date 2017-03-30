#ifndef ALIASTABLEMODEL_H
#define ALIASTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class AliasTablePriv;
class CWallet;
class WalletModel;
enum AliasType {AllAlias=0,MyAlias};

typedef enum AliasType AliasModelType;
/**
   Qt model of the alias                                                                                                                                                        book in the core. This allows views to access and modify the alias book.
 */
class AliasTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit AliasTableModel(CWallet *wallet, WalletModel *parent = 0, AliasModelType = AllAlias);
    ~AliasTableModel();

    enum ColumnIndex {
        Name = 0,   /**< alias name */
		Multisig = 1,
		ExpiresOn = 2,
		Expired = 3,
		RatingAsBuyer = 4,
		RatingAsSeller = 5,
		RatingAsArbiter = 6,
		SafeSearch = 7,
		Value = 8,  /**< Alias value */
		PrivValue = 9,
		NUMBER_OF_COLUMNS
    };

    enum RoleIndex {
        TypeRole = Qt::UserRole, /**< Type of alias (#Send or #Receive) */
		NameRole,
		MultisigRole,
		ExpiredRole,
		SafeSearchRole,
		BuyerRatingRole,
		SellerRatingRole,
		ArbiterRatingRole
	};

    /** Return status of edit/insert operation */
    enum EditStatus {
        OK,                     /**< Everything ok */
        NO_CHANGES,             /**< No changes were made during edit operation */
        INVALID_ALIAS,        /**< Unparseable alias */
        DUPLICATE_ALIAS,      /**< Alias already in alias book */
        WALLET_UNLOCK_FAILURE  /**< Wallet could not be unlocked */
    };

    static const QString Alias;      /**< Specifies send alias */

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

    /* Add an alias to the model.
       Returns the added alias on success, and an empty string otherwise.
     */
    QString addRow(const QString &type, const QString &alias, const QString &multisig,const QString &value, const QString &privvalue, const QString &expires_on, const QString &expired, const QString &safesearch,const QString &buyer_rating, const QString &seller_rating, const QString &arbiter_rating);

    /* Look up row index of an alias in the model.
       Return -1 if not found.
     */
    int lookupAlias(const QString &alias) const;
	void clear();
	void refreshAliasTable();
    EditStatus getEditStatus() const { return editStatus; }
	EditStatus editStatus;
private:
    WalletModel *walletModel;
    CWallet *wallet;
    AliasTablePriv *priv;
    QStringList columns;
    
	AliasModelType modelType;
    /** Notify listeners that data changed. */
    void emitDataChanged(int index);

public Q_SLOTS:
    /* Update alias list from core.
     */
    void updateEntry(const QString &alias, const QString &multisig,const QString &value, const QString &privvalue, const QString &expires_on, const QString &expired, const QString &safesearch, const QString &buyer_rating, const QString &seller_rating, const QString &arbiter_rating, AliasModelType type, int status);

    friend class AliasTablePriv;
};

#endif // ALIASTABLEMODEL_H
