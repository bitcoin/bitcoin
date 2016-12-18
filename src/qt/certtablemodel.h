#ifndef CERTTABLEMODEL_H
#define CERTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class CertTablePriv;
class CWallet;
class WalletModel;
enum CertType {AllCert=0,MyCert};

typedef enum CertType CertModelType;

class CertTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit CertTableModel(CWallet *wallet, WalletModel *parent = 0, CertModelType = AllCert);
    ~CertTableModel();

    enum ColumnIndex {
        Name = 0,   /**< cert name */
        Title = 1,  /**< Cert value */
		Data = 2,
		PubData = 3,
		Category = 4,
		ExpiresOn = 5,
		Expired = 6,
		Alias = 7,
		SafeSearch = 8,
		NUMBER_OF_COLUMNS
    };

    enum RoleIndex {
        TypeRole = Qt::UserRole, /**< Type of cert (#Send or #Receive) */
		NameRole,
		PrivateRole,
		ExpiredRole,
		AliasRole,
		SafeSearchRole,
		CategoryRole,
    };

    /** Return status of edit/insert operation */
    enum EditStatus {
        OK,                     /**< Everything ok */
        NO_CHANGES,             /**< No changes were made during edit operation */
        INVALID_CERT,        /**< Unparseable cert */
        DUPLICATE_CERT,      /**< Cert already in cert book */
        WALLET_UNLOCK_FAILURE  /**< Wallet could not be unlocked */
    };

    static const QString Cert;      /**< Specifies certificate  */

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

    /* Add an cert to the model.
       Returns the added cert on success, and an empty string otherwise.
     */
    QString addRow(const QString &type, const QString &cert, const QString &value, const QString &data, const QString &pubdata, const QString &category, const QString &expires_on, const QString &expired,const QString &alias, const QString &safesearch);


    /* Look up row index of an cert in the model.
       Return -1 if not found.
     */
    int lookupCert(const QString &cert) const;
	void clear();
	void refreshCertTable();
    EditStatus getEditStatus() const { return editStatus; }
	EditStatus editStatus;
private:
    WalletModel *walletModel;
    CWallet *wallet;
    CertTablePriv *priv;
    QStringList columns;
	CertModelType modelType;
    /** Notify listeners that data changed. */
    void emitDataChanged(int index);

public Q_SLOTS:
    /* Update cert list from core.
     */
    void updateEntry(const QString &cert, const QString &value, const QString &data, const QString &pubdata,const QString &category, const QString &expires_on, const QString &expired, const QString &alias, const QString &safesearch, CertModelType type, int status);

    friend class CertTablePriv;
};

#endif // CERTTABLEMODEL_H
