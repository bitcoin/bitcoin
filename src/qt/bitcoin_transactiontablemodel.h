// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TRANSACTIONTABLEMODEL_H
#define BITCOIN_TRANSACTIONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class Bitcoin_TransactionRecord;
class Bitcoin_TransactionTablePriv;
class Bitcoin_WalletModel;

class Bitcoin_CWallet;

/** UI model for the transaction table of a wallet.
 */
class Bitcoin_TransactionTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit Bitcoin_TransactionTableModel(Bitcoin_CWallet* wallet, Bitcoin_WalletModel *parent = 0);
    ~Bitcoin_TransactionTableModel();

    enum ColumnIndex {
        Status = 0,
        Date = 1,
        Type = 2,
        ToAddress = 3,
        Amount = 4
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
        /** Transaction hash */
        TxHashRole,
        /** Is transaction confirmed? */
        ConfirmedRole,
        /** Formatted amount, without brackets when unconfirmed */
        FormattedAmountRole,
        /** Transaction status (Bitcoin_TransactionRecord::Status) */
        StatusRole
    };

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;

private:
    Bitcoin_CWallet* wallet;
    Bitcoin_WalletModel *walletModel;
    QStringList columns;
    Bitcoin_TransactionTablePriv *priv;

    QString lookupAddress(const std::string &address, bool tooltip) const;
    QVariant addressColor(const Bitcoin_TransactionRecord *wtx) const;
    QString formatTxStatus(const Bitcoin_TransactionRecord *wtx) const;
    QString formatTxDate(const Bitcoin_TransactionRecord *wtx) const;
    QString formatTxType(const Bitcoin_TransactionRecord *wtx) const;
    QString formatTxToAddress(const Bitcoin_TransactionRecord *wtx, bool tooltip) const;
    QString formatTxAmount(const Bitcoin_TransactionRecord *wtx, bool showUnconfirmed=true) const;
    QString formatTooltip(const Bitcoin_TransactionRecord *rec) const;
    QVariant txStatusDecoration(const Bitcoin_TransactionRecord *wtx) const;
    QVariant txAddressDecoration(const Bitcoin_TransactionRecord *wtx) const;

public slots:
    void updateTransaction(const QString &hash, int status);
    void updateConfirmations();
    void updateDisplayUnit();

    friend class Bitcoin_TransactionTablePriv;
};

#endif // BITCOIN_TRANSACTIONTABLEMODEL_H
