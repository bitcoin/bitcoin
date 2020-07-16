// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletmodel.h>

#include <qt/addresstablemodel.h>
#include <consensus/validation.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/paymentserver.h>
#include <qt/recentrequeststablemodel.h>
#include <qt/transactiontablemodel.h>

#include <base58.h>
#include <chain.h>
#include <keystore.h>
#include <validation.h>
#include <net.h> // for g_connman
#include <sync.h>
#include <txmempool.h>
#include <ui_interface.h>
#include <util.h> // for GetBoolArg
#include <wallet/coincontrol.h>
#include <wallet/walletdb.h> // for BackupWallet

#include <spork.h>
#include <privatesend/privatesend-client.h>
#include <llmq/quorums_instantsend.h>

#include <stdint.h>

#include <QDebug>
#include <QSet>
#include <QTimer>


WalletModel::WalletModel(const PlatformStyle *platformStyle, CWallet *_wallet, OptionsModel *_optionsModel, QObject *parent) :
    QObject(parent), wallet(_wallet), optionsModel(_optionsModel), addressTableModel(0),
    transactionTableModel(0),
    recentRequestsTableModel(0),
    cachedBalance(0), cachedUnconfirmedBalance(0), cachedImmatureBalance(0),
    cachedWatchOnlyBalance{0}, cachedWatchUnconfBalance{0}, cachedWatchImmatureBalance{0},
    cachedAnonymizedBalance(0),
    cachedEncryptionStatus(Unencrypted),
    cachedNumBlocks(0),
    cachedNumISLocks(0),
    cachedPrivateSendRounds(0)
{
    fHaveWatchOnly = wallet->HaveWatchOnly();
    fForceCheckBalanceChanged = false;

    addressTableModel = new AddressTableModel(wallet, this);
    transactionTableModel = new TransactionTableModel(platformStyle, wallet, this);
    recentRequestsTableModel = new RecentRequestsTableModel(wallet, this);

    // This timer will be fired repeatedly to update the balance
    pollTimer = new QTimer(this);
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollBalanceChanged()));
    pollTimer->start(MODEL_UPDATE_DELAY);

    subscribeToCoreSignals();
}

WalletModel::~WalletModel()
{
    unsubscribeFromCoreSignals();
}

CAmount WalletModel::getBalance(const CCoinControl *coinControl) const
{
    if (coinControl)
    {
        return wallet->GetAvailableBalance(coinControl);
    }

    return wallet->GetBalance();
}


CAmount WalletModel::getAnonymizableBalance(bool fSkipDenominated, bool fSkipUnconfirmed) const
{
    return wallet->GetAnonymizableBalance(fSkipDenominated, fSkipUnconfirmed);
}

CAmount WalletModel::getAnonymizedBalance() const
{
    return wallet->GetAnonymizedBalance();
}

CAmount WalletModel::getDenominatedBalance(bool unconfirmed) const
{
    return wallet->GetDenominatedBalance(unconfirmed);
}

CAmount WalletModel::getNormalizedAnonymizedBalance() const
{
    return wallet->GetNormalizedAnonymizedBalance();
}

CAmount WalletModel::getAverageAnonymizedRounds() const
{
    return wallet->GetAverageAnonymizedRounds();
}

CAmount WalletModel::getUnconfirmedBalance() const
{
    return wallet->GetUnconfirmedBalance();
}

CAmount WalletModel::getImmatureBalance() const
{
    return wallet->GetImmatureBalance();
}

bool WalletModel::haveWatchOnly() const
{
    return fHaveWatchOnly;
}

CAmount WalletModel::getWatchBalance() const
{
    return wallet->GetWatchOnlyBalance();
}

CAmount WalletModel::getWatchUnconfirmedBalance() const
{
    return wallet->GetUnconfirmedWatchOnlyBalance();
}

CAmount WalletModel::getWatchImmatureBalance() const
{
    return wallet->GetImmatureWatchOnlyBalance();
}

void WalletModel::updateStatus()
{
    EncryptionStatus newEncryptionStatus = getEncryptionStatus();

    if(cachedEncryptionStatus != newEncryptionStatus) {
        Q_EMIT encryptionStatusChanged();
    }
}

void WalletModel::pollBalanceChanged()
{
    // Get required locks upfront. This avoids the GUI from getting stuck on
    // periodical polls if the core is holding the locks for a longer time -
    // for example, during a wallet rescan.
    TRY_LOCK(cs_main, lockMain);
    if(!lockMain)
        return;
    TRY_LOCK(wallet->cs_wallet, lockWallet);
    if(!lockWallet)
        return;

    if(fForceCheckBalanceChanged || chainActive.Height() != cachedNumBlocks || privateSendClientOptions.nPrivateSendRounds != cachedPrivateSendRounds)
    {
        fForceCheckBalanceChanged = false;

        // Balance and number of transactions might have changed
        cachedNumBlocks = chainActive.Height();
        cachedPrivateSendRounds = privateSendClientOptions.nPrivateSendRounds;

        checkBalanceChanged();
        if(transactionTableModel)
            transactionTableModel->updateConfirmations();
    }
}

void WalletModel::checkBalanceChanged()
{
    CAmount newBalance = getBalance();
    CAmount newUnconfirmedBalance = getUnconfirmedBalance();
    CAmount newImmatureBalance = getImmatureBalance();
    CAmount newAnonymizedBalance = getAnonymizedBalance();
    CAmount newWatchOnlyBalance = 0;
    CAmount newWatchUnconfBalance = 0;
    CAmount newWatchImmatureBalance = 0;
    if (haveWatchOnly())
    {
        newWatchOnlyBalance = getWatchBalance();
        newWatchUnconfBalance = getWatchUnconfirmedBalance();
        newWatchImmatureBalance = getWatchImmatureBalance();
    }

    if(cachedBalance != newBalance || cachedUnconfirmedBalance != newUnconfirmedBalance || cachedImmatureBalance != newImmatureBalance ||
        cachedAnonymizedBalance != newAnonymizedBalance ||
        cachedWatchOnlyBalance != newWatchOnlyBalance || cachedWatchUnconfBalance != newWatchUnconfBalance || cachedWatchImmatureBalance != newWatchImmatureBalance)
    {
        cachedBalance = newBalance;
        cachedUnconfirmedBalance = newUnconfirmedBalance;
        cachedImmatureBalance = newImmatureBalance;
        cachedAnonymizedBalance = newAnonymizedBalance;
        cachedWatchOnlyBalance = newWatchOnlyBalance;
        cachedWatchUnconfBalance = newWatchUnconfBalance;
        cachedWatchImmatureBalance = newWatchImmatureBalance;
        Q_EMIT balanceChanged(newBalance, newUnconfirmedBalance, newImmatureBalance, newAnonymizedBalance,
                            newWatchOnlyBalance, newWatchUnconfBalance, newWatchImmatureBalance);
    }
}

void WalletModel::updateTransaction()
{
    // Balance and number of transactions might have changed
    fForceCheckBalanceChanged = true;
}

void WalletModel::updateNumISLocks()
{
    cachedNumISLocks++;
}

void WalletModel::updateChainLockHeight(int chainLockHeight)
{
    if (transactionTableModel)
        transactionTableModel->updateChainLockHeight(chainLockHeight);
    // Number and status of confirmations might have changed (WalletModel::pollBalanceChanged handles this as well)
    fForceCheckBalanceChanged = true;
}

int WalletModel::getNumISLocks() const
{
    return cachedNumISLocks;
}

int WalletModel::getRealOutpointPrivateSendRounds(const COutPoint& outpoint) const
{
    return wallet->GetRealOutpointPrivateSendRounds(outpoint);
}

void WalletModel::updateAddressBook(const QString &address, const QString &label,
        bool isMine, const QString &purpose, int status)
{
    if(addressTableModel)
        addressTableModel->updateEntry(address, label, isMine, purpose, status);
}

void WalletModel::updateWatchOnlyFlag(bool fHaveWatchonly)
{
    fHaveWatchOnly = fHaveWatchonly;
    Q_EMIT notifyWatchonlyChanged(fHaveWatchonly);
}

bool WalletModel::validateAddress(const QString &address)
{
    return IsValidDestinationString(address.toStdString());
}

WalletModel::SendCoinsReturn WalletModel::prepareTransaction(WalletModelTransaction &transaction, const CCoinControl& coinControl)
{
    CAmount total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;

    if(recipients.empty())
    {
        return OK;
    }

    // This should never really happen, yet another safety check, just in case.
    if(wallet->IsLocked()) {
        return TransactionCreationFailed;
    }

    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    // Pre-check input data for validity
    for (const SendCoinsRecipient &rcp : recipients)
    {
        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;

        if (rcp.paymentRequest.IsInitialized())
        {   // PaymentRequest...
            CAmount subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int i = 0; i < details.outputs_size(); i++)
            {
                const payments::Output& out = details.outputs(i);
                if (out.amount() <= 0) continue;
                subtotal += out.amount();
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr+out.script().size());
                CAmount nAmount = out.amount();
                CRecipient recipient = {scriptPubKey, nAmount, rcp.fSubtractFeeFromAmount};
                vecSend.push_back(recipient);
            }
            if (subtotal <= 0)
            {
                return InvalidAmount;
            }
            total += subtotal;
        }
        else
        {   // User-entered dash address / amount:
            if(!validateAddress(rcp.address))
            {
                return InvalidAddress;
            }
            if(rcp.amount <= 0)
            {
                return InvalidAmount;
            }
            setAddress.insert(rcp.address);
            ++nAddresses;

            CScript scriptPubKey = GetScriptForDestination(DecodeDestination(rcp.address.toStdString()));
            CRecipient recipient = {scriptPubKey, rcp.amount, rcp.fSubtractFeeFromAmount};
            vecSend.push_back(recipient);

            total += rcp.amount;
        }
    }
    if(setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }

    CAmount nBalance = getBalance(&coinControl);

    if(total > nBalance)
    {
        return AmountExceedsBalance;
    }

    CAmount nFeeRequired = 0;
    CAmount nValueOut = 0;
    size_t nVinSize = 0;
    bool fCreated;
    std::string strFailReason;
    {
        LOCK2(cs_main, mempool.cs);
        LOCK(wallet->cs_wallet);

        transaction.newPossibleKeyChange(wallet);

        int nChangePosRet = -1;

        CWalletTx* newTx = transaction.getTransaction();
        CReserveKey *keyChange = transaction.getPossibleKeyChange();

        fCreated = wallet->CreateTransaction(vecSend, *newTx, *keyChange, nFeeRequired, nChangePosRet, strFailReason, coinControl);
        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && fCreated)
            transaction.reassignAmounts();

        nValueOut = newTx->tx->GetValueOut();
        nVinSize = newTx->tx->vin.size();
    }

    if(!fCreated)
    {
        if(!fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
        {
            return SendCoinsReturn(AmountWithFeeExceedsBalance);
        }
        Q_EMIT message(tr("Send Coins"), QString::fromStdString(strFailReason),
                     CClientUIInterface::MSG_ERROR);
        return TransactionCreationFailed;
    }

    // reject absurdly high fee. (This can never happen because the
    // wallet caps the fee at maxTxFee. This merely serves as a
    // belt-and-suspenders check)
    if (nFeeRequired > maxTxFee)
        return AbsurdFee;

    return SendCoinsReturn(OK);
}

WalletModel::SendCoinsReturn WalletModel::sendCoins(WalletModelTransaction &transaction)
{
    QByteArray transaction_array; /* store serialized transaction */

    {
        LOCK2(cs_main, mempool.cs);
        LOCK(wallet->cs_wallet);

        CWalletTx *newTx = transaction.getTransaction();
        QList<SendCoinsRecipient> recipients = transaction.getRecipients();

        for (const SendCoinsRecipient &rcp : recipients)
        {
            if (rcp.paymentRequest.IsInitialized())
            {
                // Make sure any payment requests involved are still valid.
                if (PaymentServer::verifyExpired(rcp.paymentRequest.getDetails())) {
                    return PaymentRequestExpired;
                }

                // Store PaymentRequests in wtx.vOrderForm in wallet.
                std::string key("PaymentRequest");
                std::string value;
                rcp.paymentRequest.SerializeToString(&value);
                newTx->vOrderForm.push_back(make_pair(key, value));
            }
            else if (!rcp.message.isEmpty()) // Message from normal dash:URI (dash:XyZ...?message=example)
            {
                newTx->vOrderForm.push_back(make_pair("Message", rcp.message.toStdString()));
            }
        }

        CReserveKey *keyChange = transaction.getPossibleKeyChange();
        CValidationState state;
        if(!wallet->CommitTransaction(*newTx, *keyChange, g_connman.get(), state))
            return SendCoinsReturn(TransactionCommitFailed, QString::fromStdString(state.GetRejectReason()));

        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << *newTx->tx;
        transaction_array.append(ssTx.data(), ssTx.size());
    }

    // Add addresses / update labels that we've sent to the address book,
    // and emit coinsSent signal for each recipient
    for (const SendCoinsRecipient &rcp : transaction.getRecipients())
    {
        // Don't touch the address book when we have a payment request
        if (!rcp.paymentRequest.IsInitialized())
        {
            std::string strAddress = rcp.address.toStdString();
            CTxDestination dest = DecodeDestination(strAddress);
            std::string strLabel = rcp.label.toStdString();
            {
                LOCK(wallet->cs_wallet);

                std::map<CTxDestination, CAddressBookData>::iterator mi = wallet->mapAddressBook.find(dest);

                // Check if we have a new address or an updated label
                if (mi == wallet->mapAddressBook.end())
                {
                    wallet->SetAddressBook(dest, strLabel, "send");
                }
                else if (mi->second.name != strLabel)
                {
                    wallet->SetAddressBook(dest, strLabel, ""); // "" means don't change purpose
                }
            }
        }
        Q_EMIT coinsSent(wallet, rcp, transaction_array);
    }
    checkBalanceChanged(); // update balance immediately, otherwise there could be a short noticeable delay until pollBalanceChanged hits

    return SendCoinsReturn(OK);
}

OptionsModel *WalletModel::getOptionsModel()
{
    return optionsModel;
}

AddressTableModel *WalletModel::getAddressTableModel()
{
    return addressTableModel;
}

TransactionTableModel *WalletModel::getTransactionTableModel()
{
    return transactionTableModel;
}

RecentRequestsTableModel *WalletModel::getRecentRequestsTableModel()
{
    return recentRequestsTableModel;
}

WalletModel::EncryptionStatus WalletModel::getEncryptionStatus() const
{
    if(!wallet->IsCrypted())
    {
        return Unencrypted;
    }
    else if(wallet->IsLocked(true))
    {
        return Locked;
    }
    else if (wallet->IsLocked())
    {
        return UnlockedForMixingOnly;
    }
    else
    {
        return Unlocked;
    }
}

bool WalletModel::setWalletEncrypted(bool encrypted, const SecureString &passphrase)
{
    if(encrypted)
    {
        // Encrypt
        return wallet->EncryptWallet(passphrase);
    }
    else
    {
        // Decrypt -- TODO; not supported yet
        return false;
    }
}

bool WalletModel::setWalletLocked(bool locked, const SecureString &passPhrase, bool fMixing)
{
    if(locked)
    {
        // Lock
        return wallet->Lock(fMixing);
    }
    else
    {
        // Unlock
        return wallet->Unlock(passPhrase, fMixing);
    }
}

bool WalletModel::changePassphrase(const SecureString &oldPass, const SecureString &newPass)
{
    bool retval;
    {
        LOCK(wallet->cs_wallet);
        wallet->Lock(); // Make sure wallet is locked before attempting pass change
        retval = wallet->ChangeWalletPassphrase(oldPass, newPass);
    }
    return retval;
}

bool WalletModel::backupWallet(const QString &filename)
{
    return wallet->BackupWallet(filename.toLocal8Bit().data());
}

bool WalletModel::autoBackupWallet(QString& strBackupWarningRet, QString& strBackupErrorRet)
{
    std::string strBackupWarning;
    std::string strBackupError;
    bool result = wallet->AutoBackupWallet("", strBackupWarning, strBackupError);
    strBackupWarningRet = QString::fromStdString(strBackupWarning);
    strBackupErrorRet = QString::fromStdString(strBackupError);
    return result;
}

int64_t WalletModel::getKeysLeftSinceAutoBackup() const
{
    return wallet->nKeysLeftSinceAutoBackup;
}

// Handlers for core signals
static void NotifyKeyStoreStatusChanged(WalletModel *walletmodel, CCryptoKeyStore *wallet)
{
    qDebug() << "NotifyKeyStoreStatusChanged";
    QMetaObject::invokeMethod(walletmodel, "updateStatus", Qt::QueuedConnection);
}

static void NotifyAddressBookChanged(WalletModel *walletmodel, CWallet *wallet,
        const CTxDestination &address, const std::string &label, bool isMine,
        const std::string &purpose, ChangeType status)
{
    QString strAddress = QString::fromStdString(EncodeDestination(address));
    QString strLabel = QString::fromStdString(label);
    QString strPurpose = QString::fromStdString(purpose);

    qDebug() << "NotifyAddressBookChanged: " + strAddress + " " + strLabel + " isMine=" + QString::number(isMine) + " purpose=" + strPurpose + " status=" + QString::number(status);
    QMetaObject::invokeMethod(walletmodel, "updateAddressBook", Qt::QueuedConnection,
                              Q_ARG(QString, strAddress),
                              Q_ARG(QString, strLabel),
                              Q_ARG(bool, isMine),
                              Q_ARG(QString, strPurpose),
                              Q_ARG(int, status));
}

static void NotifyTransactionChanged(WalletModel *walletmodel, CWallet *wallet, const uint256 &hash, ChangeType status)
{
    Q_UNUSED(wallet);
    Q_UNUSED(hash);
    Q_UNUSED(status);
    QMetaObject::invokeMethod(walletmodel, "updateTransaction", Qt::QueuedConnection);
}

static void NotifyISLockReceived(WalletModel *walletmodel)
{
    QMetaObject::invokeMethod(walletmodel, "updateNumISLocks", Qt::QueuedConnection);
}

static void NotifyChainLockReceived(WalletModel *walletmodel, int chainLockHeight)
{
    QMetaObject::invokeMethod(walletmodel, "updateChainLockHeight", Qt::QueuedConnection,
                              Q_ARG(int, chainLockHeight));
}

static void ShowProgress(WalletModel *walletmodel, const std::string &title, int nProgress)
{
    // emits signal "showProgress"
    QMetaObject::invokeMethod(walletmodel, "showProgress", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(title)),
                              Q_ARG(int, nProgress));
}

static void NotifyWatchonlyChanged(WalletModel *walletmodel, bool fHaveWatchonly)
{
    QMetaObject::invokeMethod(walletmodel, "updateWatchOnlyFlag", Qt::QueuedConnection,
                              Q_ARG(bool, fHaveWatchonly));
}

void WalletModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    wallet->NotifyStatusChanged.connect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.connect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.connect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->NotifyISLockReceived.connect(boost::bind(NotifyISLockReceived, this));
    wallet->NotifyChainLockReceived.connect(boost::bind(NotifyChainLockReceived, this, _1));
    wallet->ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
    wallet->NotifyWatchonlyChanged.connect(boost::bind(NotifyWatchonlyChanged, this, _1));
}

void WalletModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    wallet->NotifyStatusChanged.disconnect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.disconnect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.disconnect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->NotifyISLockReceived.disconnect(boost::bind(NotifyISLockReceived, this));
    wallet->NotifyChainLockReceived.disconnect(boost::bind(NotifyChainLockReceived, this, _1));
    wallet->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
    wallet->NotifyWatchonlyChanged.disconnect(boost::bind(NotifyWatchonlyChanged, this, _1));
}

// WalletModel::UnlockContext implementation
WalletModel::UnlockContext WalletModel::requestUnlock(bool fForMixingOnly)
{
    EncryptionStatus encStatusOld = getEncryptionStatus();

    // Wallet was completely locked
    bool was_locked = (encStatusOld == Locked);
    // Wallet was unlocked for mixing
    bool was_mixing = (encStatusOld == UnlockedForMixingOnly);
    // Wallet was unlocked for mixing and now user requested to fully unlock it
    bool fMixingToFullRequested = !fForMixingOnly && was_mixing;

    if(was_locked || fMixingToFullRequested) {
        // Request UI to unlock wallet
        Q_EMIT requireUnlock(fForMixingOnly);
    }

    EncryptionStatus encStatusNew = getEncryptionStatus();

    // Wallet was locked, user requested to unlock it for mixing and failed to do so
    bool fMixingUnlockFailed = fForMixingOnly && !(encStatusNew == UnlockedForMixingOnly);
    // Wallet was unlocked for mixing, user requested to fully unlock it and failed
    bool fMixingToFullFailed = fMixingToFullRequested && !(encStatusNew == Unlocked);
    // If wallet is still locked, unlock failed or was cancelled, mark context as invalid
    bool fInvalid = (encStatusNew == Locked) || fMixingUnlockFailed || fMixingToFullFailed;
    // Wallet was not locked in any way or user tried to unlock it for mixing only and succeeded, keep it unlocked
    bool fKeepUnlocked = !was_locked || (fForMixingOnly && !fMixingUnlockFailed);

    return UnlockContext(this, !fInvalid, !fKeepUnlocked, was_mixing);
}

WalletModel::UnlockContext::UnlockContext(WalletModel *_wallet, bool _valid, bool _was_locked, bool _was_mixing):
        wallet(_wallet),
        valid(_valid),
        was_locked(_was_locked),
        was_mixing(_was_mixing)
{
}

WalletModel::UnlockContext::~UnlockContext()
{
    if(valid && (was_locked || was_mixing))
    {
        wallet->setWalletLocked(true, "", was_mixing);
    }
}

void WalletModel::UnlockContext::CopyFrom(const UnlockContext& rhs)
{
    // Transfer context; old object no longer relocks wallet
    *this = rhs;
    rhs.was_locked = false;
    rhs.was_mixing = false;
}

bool WalletModel::getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    return wallet->GetPubKey(address, vchPubKeyOut);
}

bool WalletModel::IsSpendable(const CTxDestination& dest) const
{
    return IsMine(*wallet, dest) & ISMINE_SPENDABLE;
}

bool WalletModel::IsSpendable(const CScript& script) const
{
    return IsMine(*wallet, script) & ISMINE_SPENDABLE;
}

bool WalletModel::getPrivKey(const CKeyID &address, CKey& vchPrivKeyOut) const
{
    return wallet->GetKey(address, vchPrivKeyOut);
}

// returns a list of COutputs from COutPoints
void WalletModel::getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<COutput>& vOutputs)
{
    LOCK2(cs_main, wallet->cs_wallet);
    for (const COutPoint& outpoint : vOutpoints)
    {
        auto it = wallet->mapWallet.find(outpoint.hash);
        if (it == wallet->mapWallet.end()) continue;
        int nDepth = it->second.GetDepthInMainChain();
        if (nDepth < 0) continue;
        COutput out(&it->second, outpoint.n, nDepth, true /* spendable */, true /* solvable */, true /* safe */);
        vOutputs.push_back(out);
    }
}

bool WalletModel::isSpent(const COutPoint& outpoint) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    return wallet->IsSpent(outpoint.hash, outpoint.n);
}

// AvailableCoins + LockedCoins grouped by wallet address (put change in one group with wallet address)
void WalletModel::listCoins(std::map<QString, std::vector<COutput> >& mapCoins) const
{
    for (auto& group : wallet->ListCoins()) {
        auto& resultGroup = mapCoins[QString::fromStdString(EncodeDestination(group.first))];
        for (auto& coin : group.second) {
            resultGroup.emplace_back(std::move(coin));
        }
    }
}

bool WalletModel::isLockedCoin(uint256 hash, unsigned int n) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    return wallet->IsLockedCoin(hash, n);
}

void WalletModel::lockCoin(COutPoint& output)
{
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->LockCoin(output);
}

void WalletModel::unlockCoin(COutPoint& output)
{
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->UnlockCoin(output);
}

void WalletModel::listLockedCoins(std::vector<COutPoint>& vOutpts)
{
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->ListLockedCoins(vOutpts);
}

void WalletModel::listProTxCoins(std::vector<COutPoint>& vOutpts)
{
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->ListProTxCoins(vOutpts);
}

void WalletModel::loadReceiveRequests(std::vector<std::string>& vReceiveRequests)
{
    vReceiveRequests = wallet->GetDestValues("rr"); // receive request
}

bool WalletModel::saveReceiveRequest(const std::string &sAddress, const int64_t nId, const std::string &sRequest)
{
    CTxDestination dest = DecodeDestination(sAddress);

    std::stringstream ss;
    ss << nId;
    std::string key = "rr" + ss.str(); // "rr" prefix = "receive request" in destdata

    LOCK(wallet->cs_wallet);
    if (sRequest.empty())
        return wallet->EraseDestData(dest, key);
    else
        return wallet->AddDestData(dest, key, sRequest);
}

bool WalletModel::transactionCanBeAbandoned(uint256 hash) const
{
    return wallet->TransactionCanBeAbandoned(hash);
}

bool WalletModel::abandonTransaction(uint256 hash) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    return wallet->AbandonTransaction(hash);
}

bool WalletModel::isWalletEnabled()
{
   return !gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET);
}

bool WalletModel::hdEnabled() const
{
    return wallet->IsHDEnabled();
}

int WalletModel::getDefaultConfirmTarget() const
{
    return nTxConfirmTarget;
}

QString WalletModel::getWalletName() const
{
    LOCK(wallet->cs_wallet);
    QString walletName = QString::fromStdString(wallet->GetName());
    if (walletName.endsWith(".dat")) {
        walletName.truncate(walletName.size() - 4);
    }
    return walletName;
}

bool WalletModel::isMultiwallet()
{
    return gArgs.GetArgs("-wallet").size() > 1;
}
