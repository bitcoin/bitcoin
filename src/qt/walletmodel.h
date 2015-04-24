// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WALLETMODEL_H
#define WALLETMODEL_H

#include "paymentrequestplus.h"
#include "walletmodeltransaction.h"
#include "wallet.h"
#include "bitcoin_walletmodel.h"
#include "optionsmodel.h"

#include "allocators.h" /* for SecureString */

#include <map>
#include <vector>

#include <QObject>

class Bitcredit_AddressTableModel;
class OptionsModel;
class Bitcredit_RecentRequestsTableModel;
class Bitcredit_TransactionTableModel;
class Bitcredit_WalletModelTransaction;
class Bitcredit_CReserveKey;

class CCoinControl;
class CKeyID;
class COutPoint;
class Bitcredit_COutput;
class CPubKey;
class Bitcredit_CWallet;
class uint256;

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

class Bitcredit_SendCoinsRecipient
{
public:
    explicit Bitcredit_SendCoinsRecipient() : amount(0), nVersion(Bitcredit_SendCoinsRecipient::CURRENT_VERSION) { }
    explicit Bitcredit_SendCoinsRecipient(const QString &addr, const QString &label, quint64 amount, const QString &message):
        address(addr), label(label), amount(amount), message(message), nVersion(Bitcredit_SendCoinsRecipient::CURRENT_VERSION) {}

    // If from an insecure payment request, this is used for storing
    // the addresses, e.g. address-A<br />address-B<br />address-C.
    // Info: As we don't need to process addresses in here when using
    // payment requests, we can abuse it for displaying an address list.
    // Todo: This is a hack, should be replaced with a cleaner solution!
    QString address;
    QString label;
    qint64 amount;
    // If from a payment request, this is used for storing the memo
    QString message;

    // If from a payment request, paymentRequest.IsInitialized() will be true
    PaymentRequestPlus paymentRequest;
    // Empty if no authentication or invalid signature/cert/etc.
    QString authenticatedMerchant;

    static const int CURRENT_VERSION = 1;
    int nVersion;

    IMPLEMENT_SERIALIZE
    (
        Bitcredit_SendCoinsRecipient* pthis = const_cast<Bitcredit_SendCoinsRecipient*>(this);

        std::string sAddress = pthis->address.toStdString();
        std::string sLabel = pthis->label.toStdString();
        std::string sMessage = pthis->message.toStdString();
        std::string sPaymentRequest;
        if (!fRead && pthis->paymentRequest.IsInitialized())
            pthis->paymentRequest.SerializeToString(&sPaymentRequest);
        std::string sAuthenticatedMerchant = pthis->authenticatedMerchant.toStdString();

        READWRITE(pthis->nVersion);
        nVersion = pthis->nVersion;
        READWRITE(sAddress);
        READWRITE(sLabel);
        READWRITE(amount);
        READWRITE(sMessage);
        READWRITE(sPaymentRequest);
        READWRITE(sAuthenticatedMerchant);

        if (fRead)
        {
            pthis->address = QString::fromStdString(sAddress);
            pthis->label = QString::fromStdString(sLabel);
            pthis->message = QString::fromStdString(sMessage);
            if (!sPaymentRequest.empty())
                pthis->paymentRequest.parse(QByteArray::fromRawData(sPaymentRequest.data(), sPaymentRequest.size()));
            pthis->authenticatedMerchant = QString::fromStdString(sAuthenticatedMerchant);
        }
    )
};

/** Interface to Credits wallet from Qt view code. */
class Bitcredit_WalletModel : public QObject
{
    Q_OBJECT

public:
    explicit Bitcredit_WalletModel(Bitcredit_CWallet *wallet, Bitcredit_CWallet *deposit_wallet, Bitcoin_CWallet *bitcoin_wallet, OptionsModel *optionsModel, Bitcredit_CWallet *keyholder_wallet, bool isForDepositWallet, QObject *parent = 0);
    ~Bitcredit_WalletModel();

    enum StatusCode // Returned by sendCoins
    {
        OK,
        InvalidAmount,
        InvalidAddress,
        AmountExceedsBalance,
        AmountWithFeeExceedsBalance,
        DuplicateAddress,
        TransactionCreationFailed, // Error returned when wallet is still locked
        TransactionCommitFailed,
        TransactionDoubleSpending
    };

    enum EncryptionStatus
    {
        Unencrypted,  // !wallet->IsCrypted()
        Locked,       // wallet->IsCrypted() && wallet->IsLocked()
        Unlocked      // wallet->IsCrypted() && !wallet->IsLocked()
    };

    OptionsModel *getOptionsModel();
    Bitcredit_AddressTableModel *getAddressTableModel();
    Bitcredit_TransactionTableModel *getTransactionTableModel();
    Bitcredit_RecentRequestsTableModel *getRecentRequestsTableModel();

    qint64 getBalance(map<uint256, set<int> >& mapFilterTxInPoints, const CCoinControl *coinControl = NULL) const;
    qint64 getUnconfirmedBalance(map<uint256, set<int> >& mapFilterTxInPoints) const;
    qint64 getImmatureBalance(map<uint256, set<int> >& mapFilterTxInPoints) const;
    qint64 getPreparedDepositBalance() const;
    qint64 getInDepositBalance() const;
    int getBlockHeight() const;
    qint64 getTotalMonetaryBase() const;
    qint64 getTotalDepositBase() const;
    int getNextSubsidyUpdateHeight() const;
    qint64 getRequiredDepositLevel(unsigned int forwardBlocks) const;
    qint64 getMaxBlockSubsidy(unsigned int forwardBlocks) const;
    int getNumTransactions() const;
    EncryptionStatus getEncryptionStatus() const;

    // Check address for validity
    bool validateAddress(const QString &address);

    // Return status record for SendCoins, contains error id + information
    struct SendCoinsReturn
    {
        SendCoinsReturn(StatusCode status = OK):
            status(status) {}
        StatusCode status;
    };

    // prepare transaction for getting txfee before sending coins
    SendCoinsReturn prepareTransaction(Bitcredit_WalletModel *deposit_model, Bitcredit_WalletModelTransaction &transaction, const CCoinControl *coinControl = NULL);

    // prepare deposit transaction for getting txfee before sending coins
    SendCoinsReturn prepareDepositTransaction(Bitcredit_WalletModel *deposit_model, Bitcredit_WalletModelTransaction &transaction, const Bitcredit_COutput& coin, Bitcredit_CCoinsViewCache &bitcredit_view, Bitcoin_CClaimCoinsViewCache &claim_view);

    // prepare claim transaction for getting txfee before sending coins
    SendCoinsReturn prepareClaimTransaction(Bitcoin_WalletModel *bitcoin_model, Bitcoin_CClaimCoinsViewCache &claim_view, Bitcredit_WalletModelTransaction &transaction, const CCoinControl *coinControl = NULL);

    // Store deposit coins for later use
    SendCoinsReturn storeDepositTransaction(Bitcredit_WalletModel *bitcredit_model, Bitcredit_WalletModelTransaction &transaction);

    // Send coins to a list of recipients
    SendCoinsReturn sendCoins(Bitcredit_WalletModelTransaction &transaction);

    // Wallet encryption
    bool setWalletEncrypted(bool encrypted, const SecureString &passphrase);
    // Passphrase only needed when unlocking
    bool setWalletLocked(bool locked, const SecureString &passPhrase=SecureString());
    bool changePassphrase(const SecureString &oldPass, const SecureString &newPass);
    // Wallet backup
    bool backupWallet(const QString &filename);

    // RAI object for unlocking wallet, returned by requestUnlock()
    class UnlockContext
    {
    public:
        UnlockContext(Bitcredit_WalletModel *wallet, bool valid, bool relock);
        ~UnlockContext();

        bool isValid() const { return valid; }

        // Copy operator and constructor transfer the context
        UnlockContext(const UnlockContext& obj) { CopyFrom(obj); }
        UnlockContext& operator=(const UnlockContext& rhs) { CopyFrom(rhs); return *this; }
    private:
        Bitcredit_WalletModel *wallet;
        bool valid;
        mutable bool relock; // mutable, as it can be set to false by copying

        void CopyFrom(const UnlockContext& rhs);
    };

    UnlockContext requestUnlock();

    bool getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const;
    void getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<Bitcredit_COutput>& vOutputs);
    bool isSpent(const COutPoint& outpoint) const;
    void listCoins(std::map<QString, std::vector<Bitcredit_COutput> >& mapCoins) const;

    bool isLockedCoin(uint256 hash, unsigned int n) const;
    void lockCoin(COutPoint& output);
    void unlockCoin(COutPoint& output);
    void listLockedCoins(std::vector<COutPoint>& vOutpts);

    void loadReceiveRequests(std::vector<std::string>& vReceiveRequests);
    bool saveReceiveRequest(const std::string &sAddress, const int64_t nId, const std::string &sRequest);

    void checkBalanceChanged();
    void checkMinerStatisticsChanged();

    Bitcredit_CWallet *wallet;
    Bitcredit_CWallet *deposit_wallet;
    Bitcoin_CWallet *bitcoin_wallet;

private:
    // Wallet has an options model for wallet-specific options
    // (transaction fee, for example)
    OptionsModel *optionsModel;

    Bitcredit_AddressTableModel *addressTableModel;
    Bitcredit_TransactionTableModel *transactionTableModel;
    Bitcredit_RecentRequestsTableModel *recentRequestsTableModel;

    // Cache some values to be able to detect changes
    qint64 cachedBalance;
    qint64 cachedUnconfirmedBalance;
    qint64 cachedImmatureBalance;
    qint64 cachedPreparedDepositBalance;
    qint64 cachedInDepositBalance;
    qint64 cachedNumTransactions;
    EncryptionStatus cachedEncryptionStatus;
    int cachedNumBlocks;

    qint64 cachedReqDeposit1;
    qint64 cachedReqDeposit2;
    qint64 cachedReqDeposit3;
    qint64 cachedReqDeposit4;
    qint64 cachedReqDeposit5;
    qint64 cachedMaxBlockSubsidy1;
    qint64 cachedMaxBlockSubsidy2;
    qint64 cachedMaxBlockSubsidy3;
    qint64 cachedMaxBlockSubsidy4;
    qint64 cachedMaxBlockSubsidy5;
    int cachedBlockHeight;
    qint64 cachedTotalMonetaryBase;
    qint64 cachedTotalDepositBase;
    int cachedNextSubsidyUpdateHeight;

    QTimer *pollTimer;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

signals:
    // Signal that balance in wallet changed
    void balanceChanged(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance, qint64 preparedDepositBalance, qint64 inDepositBalance);
    void minerStatisticsChanged(qint64 reqDeposit1, qint64 reqDeposit2, qint64 reqDeposit3, qint64 reqDeposit4, qint64 reqDeposit5, qint64 maxSubsidy1, qint64 maxSubsidy2, qint64 maxSubsidy3, qint64 maxSubsidy4, qint64 maxSubsidy5, unsigned int blockHeight, qint64 totalMonetaryBase, qint64 totalDepositBase, int nextSubsidyUpdateHeight);

    // Number of transactions in wallet changed
    void numTransactionsChanged(int count);

    // Encryption status of wallet changed
    void encryptionStatusChanged(int status);

    // Signal emitted when wallet needs to be unlocked
    // It is valid behaviour for listeners to keep the wallet locked after this signal;
    // this means that the unlocking failed or was cancelled.
    void requireUnlock();

    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

    // Coins sent: from wallet, to recipient, in (serialized) transaction:
    void coinsSent(Bitcredit_CWallet* wallet, Bitcredit_SendCoinsRecipient recipient, QByteArray transaction);

    // Show progress dialog e.g. for rescan
    void showProgress(const QString &title, int nProgress);

public slots:
    /* Wallet status might have changed */
    void updateStatus();
    /* New transaction, or transaction changed status */
    void updateTransaction(const QString &hash, int status);
    /* New, updated or removed address book entry */
    void updateAddressBook(const QString &address, const QString &label, bool isMine, const QString &purpose, int status);
    /* Current, immature or unconfirmed balance might have changed - emit 'balanceChanged' if so */
    void pollBalanceChanged();
};

#endif // WALLETMODEL_H
