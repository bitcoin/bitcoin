// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TRANSACTIONTABLEMODEL_H
#define TRANSACTIONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class Bitcredit_TransactionRecord;
class Bitcredit_TransactionTablePriv;
class Bitcredit_WalletModel;

class Bitcredit_CWallet;

/** UI model for the transaction table of a wallet.
 */
class Bitcredit_TransactionTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit Bitcredit_TransactionTableModel(Bitcredit_CWallet* bitcredit_wallet, Bitcredit_CWallet *keyholder_wallet, bool isForDepositWallet, Bitcredit_WalletModel *parent = 0);
    ~Bitcredit_TransactionTableModel();

    enum ColumnIndex {
        Status = 0,
        Date = 1,
        Type = 2,
        ToAddress = 3,
        Amount = 4,
        DepositAmount = 4
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
        /** Formatted amount, without brackets when unconfirmed */
		FormattedDepositAmountRole,
        /** Transaction status (Bitcredit_TransactionRecord::Status) */
        StatusRole
    };

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;

private:
    Bitcredit_CWallet* wallet;
    Bitcredit_CWallet* keyholder_wallet;
    Bitcredit_WalletModel *walletModel;
    QStringList columns;
    Bitcredit_TransactionTablePriv *priv;
    bool isForDepositWallet;

    QString lookupAddress(const std::string &address, bool tooltip) const;
    QVariant addressColor(const Bitcredit_TransactionRecord *wtx) const;
    QString formatTxStatus(const Bitcredit_TransactionRecord *wtx) const;
    QString formatTxDate(const Bitcredit_TransactionRecord *wtx) const;
    QString formatTxType(const Bitcredit_TransactionRecord *wtx) const;
    QString formatTxToAddress(const Bitcredit_TransactionRecord *wtx, bool tooltip) const;
    QString formatTxAmount(const Bitcredit_TransactionRecord *wtx, bool showUnconfirmed=true) const;
    QString formatTxAmountWithDeposit(const Bitcredit_TransactionRecord *wtx, bool showUnconfirmed=true) const;
    QString formatTxDepositAmount(const Bitcredit_TransactionRecord *wtx) const;
    QString formatTooltip(const Bitcredit_TransactionRecord *rec) const;
    QVariant txStatusDecoration(const Bitcredit_TransactionRecord *wtx) const;
    QVariant txAddressDecoration(const Bitcredit_TransactionRecord *wtx) const;

public slots:
    void updateTransaction(const QString &hash, int status);
    void updateConfirmations();
    void updateDisplayUnit();

    friend class Bitcredit_TransactionTablePriv;
};

#endif // TRANSACTIONTABLEMODEL_H
