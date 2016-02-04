#ifndef TRANSACTIONTABLEMODEL_H
#define TRANSACTIONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class CWallet;
class TransactionTablePriv;
class TransactionRecord;
class WalletModel;

/** UI model for the transaction table of a wallet.
 */
class TransactionTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TransactionTableModel(CWallet* wallet, WalletModel *parent = 0);
    ~TransactionTableModel();

    enum ColumnIndex {
        Status = 0,
        Date = 1,
        Type = 2,
        ToAddress = 3,
        Narration = 4,
        Amount = 5
    };

    /** Roles to get specific information from a transaction row.
        These are independent of column.
    */
    enum RoleIndex {
        /** Type of transaction */
        TypeRole = Qt::UserRole,
        /** Date and time this transaction was created */
        DateRole,
        /** Long description (HTML format) */
        LongDescriptionRole,
        /** Address of transaction */
        AddressRole,
        /** Label of address related to transaction */
        LabelRole,
        /** Net amount of transaction */
        AmountRole,
        /** Unique identifier */
        TxIDRole,
        /** Is transaction confirmed? */
        ConfirmedRole,
        /** Formatted amount, without brackets when unconfirmed */
        FormattedAmountRole,
        /** Transaction status (TransactionRecord::Status) */
        StatusRole,
        /** Amount of Confirmations */
        ConfirmationsRole
    };

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;

    int lookupTransaction(const QString &txid) const;
private:
    CWallet* wallet;
    WalletModel *walletModel;
    QStringList columns;
    TransactionTablePriv *priv;

    QString lookupAddress(const std::string &address, bool tooltip) const;
    QVariant addressColor(const TransactionRecord *wtx) const;
    QString formatTxStatus(const TransactionRecord *wtx) const;
    QString formatTxDate(const TransactionRecord *wtx) const;
    QString formatTxToAddress(const TransactionRecord *wtx, bool tooltip) const;
    QString formatTxAmount(const TransactionRecord *wtx, bool showUnconfirmed=true) const;
    QString formatNarration(const TransactionRecord *wtx) const;
    QString formatTooltip(const TransactionRecord *rec) const;
    QString txStatusDecoration(const TransactionRecord *wtx) const;
    QVariant txAddressDecoration(const TransactionRecord *wtx) const;

public slots:
    void updateTransaction(const QString &hash, int status);
    void updateConfirmations();
    void updateDisplayUnit();

    friend class TransactionTablePriv;
};

#endif

