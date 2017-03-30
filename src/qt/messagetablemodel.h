#ifndef MESSAGETABLEMODEL_H
#define MESSAGETABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class MessageTablePriv;
class CWallet;
class WalletModel;
enum MessageType {InMessage=0,OutMessage};

typedef enum MessageType MessageModelType;
/**
   Qt model of the message book in the core. This allows views to access and modify the message book.
 */
class MessageTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit MessageTableModel(CWallet *wallet, WalletModel *parent = 0, MessageModelType = InMessage);
    ~MessageTableModel();

    enum ColumnIndex {
        GUID = 0,   
		Time = 1,
        From = 2,  
		To = 3,  
		Subject = 4,
		Message = 5,
		NUMBER_OF_COLUMNS
    };

    enum RoleIndex {
        TypeRole = Qt::UserRole, /**< Type of message (#Send or #Receive) */
		ToRole
    };

    /** Return status of edit/insert operation */
    enum EditStatus {
        OK,                     /**< Everything ok */
        NO_CHANGES,             /**< No changes were made during edit operation */
        INVALID_MESSAGE,        /**< Unparseable message */
        DUPLICATE_MESSAGE,      /**< Message already in message book */
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

    /* Look up row index of an message in the model.
       Return -1 if not found.
     */
    int lookupMessage(const QString &guid) const;
	void clear();
	void refreshMessageTable();
    EditStatus getEditStatus() const { return editStatus; }
	EditStatus editStatus;
private:
    WalletModel *walletModel;
    CWallet *wallet;
    MessageTablePriv *priv;
    QStringList columns;
	MessageModelType modelType;	
    /** Notify listeners that data changed. */
    void emitDataChanged(int index);

public Q_SLOTS:
    friend class MessageTablePriv;
};

#endif // MESSAGETABLEMODEL_H
