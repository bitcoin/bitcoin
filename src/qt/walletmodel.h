#ifndef WALLETMODEL_H
#define WALLETMODEL_H

#include <QObject>

class OptionsModel;
class AddressTableModel;
class TransactionTableModel;
class CWallet;

// Interface to a Bitcoin wallet
class WalletModel : public QObject
{
    Q_OBJECT
public:
    explicit WalletModel(CWallet *wallet, QObject *parent = 0);

    enum StatusCode
    {
        OK,
        InvalidAmount,
        InvalidAddress,
        AmountExceedsBalance,
        AmountWithFeeExceedsBalance,
        Aborted,
        MiscError
    };

    OptionsModel *getOptionsModel();
    AddressTableModel *getAddressTableModel();
    TransactionTableModel *getTransactionTableModel();

    qint64 getBalance() const;
    int getNumTransactions() const;

    /* Send coins */
    StatusCode sendCoins(const QString &payTo, qint64 payAmount, const QString &addToAddressBookAs=QString());
private:
    CWallet *wallet;

    // Wallet has an options model for wallet-specific options
    // (transaction fee, for example)
    OptionsModel *optionsModel;

    AddressTableModel *addressTableModel;
    TransactionTableModel *transactionTableModel;

signals:
    void balanceChanged(qint64 balance);
    void numTransactionsChanged(int count);

    // Asynchronous error notification
    void error(const QString &title, const QString &message);

public slots:

private slots:
    void update();
};


#endif // WALLETMODEL_H
