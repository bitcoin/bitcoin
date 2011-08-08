#ifndef WALLETMODEL_H
#define WALLETMODEL_H

#include <QObject>

class OptionsModel;
class AddressTableModel;
class TransactionTableModel;
class CWallet;

struct SendCoinsRecipient
{
    QString address;
    QString label;
    qint64 amount;
};

// Interface to Bitcoin wallet from Qt view code
class WalletModel : public QObject
{
    Q_OBJECT
public:
    explicit WalletModel(CWallet *wallet, OptionsModel *optionsModel, QObject *parent = 0);

    enum StatusCode
    {
        OK,
        InvalidAmount,
        InvalidAddress,
        AmountExceedsBalance,
        AmountWithFeeExceedsBalance,
        DuplicateAddress,
        TransactionCreationFailed,
        TransactionCommitFailed,
        Aborted,
        MiscError
    };

    OptionsModel *getOptionsModel();
    AddressTableModel *getAddressTableModel();
    TransactionTableModel *getTransactionTableModel();

    qint64 getBalance() const;
    qint64 getUnconfirmedBalance() const;
    int getNumTransactions() const;

    // Check address for validity
    bool validateAddress(const QString &address);

    // Return status record for SendCoins, contains error id + information
    struct SendCoinsReturn
    {
        SendCoinsReturn(StatusCode status,
                         qint64 fee=0,
                         QString hex=QString()):
            status(status), fee(fee), hex(hex) {}
        StatusCode status;
        qint64 fee; // is used in case status is "AmountWithFeeExceedsBalance"
        QString hex; // is filled with the transaction hash if status is "OK"
    };

    // Send coins to a list of recipients
    SendCoinsReturn sendCoins(const QList<SendCoinsRecipient> &recipients);
private:
    CWallet *wallet;

    // Wallet has an options model for wallet-specific options
    // (transaction fee, for example)
    OptionsModel *optionsModel;

    AddressTableModel *addressTableModel;
    TransactionTableModel *transactionTableModel;

    qint64 cachedBalance;
    qint64 cachedUnconfirmedBalance;
    qint64 cachedNumTransactions;

signals:
    void balanceChanged(qint64 balance, qint64 unconfirmedBalance);
    void numTransactionsChanged(int count);

    // Asynchronous error notification
    void error(const QString &title, const QString &message);

public slots:

private slots:
    void update();
};


#endif // WALLETMODEL_H
