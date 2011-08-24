#ifndef WALLETMODEL_H
#define WALLETMODEL_H

#include <QObject>
#include <string>

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

    enum EncryptionStatus
    {
        Unencrypted,  // !wallet->IsCrypted()
        Locked,       // wallet->IsCrypted() && wallet->IsLocked()
        Unlocked      // wallet->IsCrypted() && !wallet->IsLocked()
    };

    OptionsModel *getOptionsModel();
    AddressTableModel *getAddressTableModel();
    TransactionTableModel *getTransactionTableModel();

    qint64 getBalance() const;
    qint64 getUnconfirmedBalance() const;
    int getNumTransactions() const;
    EncryptionStatus getEncryptionStatus() const;

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

    // Wallet encryption
    bool setWalletEncrypted(bool encrypted, const std::string &passphrase);
    // Passphrase only needed when unlocking
    bool setWalletLocked(bool locked, const std::string &passPhrase=std::string());
    bool changePassphrase(const std::string &oldPass, const std::string &newPass);

    // RAI object for unlocking wallet, returned by requestUnlock()
    class UnlockContext
    {
    public:
        UnlockContext(WalletModel *wallet, bool valid, bool relock);
        ~UnlockContext();

        bool isValid() const { return valid; }

        UnlockContext(const UnlockContext& obj)
        { CopyFrom(obj); }
    private:
        UnlockContext& operator=(const UnlockContext& rhs)
        { CopyFrom(rhs); return *this; }

    private:
        WalletModel *wallet;
        bool valid;
        mutable bool relock; // mutable, as it can be set to false by copying

        void CopyFrom(const UnlockContext& rhs);
    };

    UnlockContext requestUnlock();

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
    EncryptionStatus cachedEncryptionStatus;

signals:
    void balanceChanged(qint64 balance, qint64 unconfirmedBalance);
    void numTransactionsChanged(int count);
    void encryptionStatusChanged(int status);
    void requireUnlock();

    // Asynchronous error notification
    void error(const QString &title, const QString &message);

public slots:

private slots:
    void update();
};


#endif // WALLETMODEL_H
