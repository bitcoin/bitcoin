// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletmodel.h>

#include <qt/addresstablemodel.h>
#include <consensus/validation.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/paymentserver.h>
#include <qt/recentrequeststablemodel.h>
#include <qt/sendcoinsdialog.h>
#include <qt/transactiontablemodel.h>

#include <base58.h>
#include <chain.h>
#include <consensus/tx_verify.h>
#include <keystore.h>
#include <validation.h>
#include <net.h> // for g_connman
#include <policy/fees.h>
#include <policy/rbf.h>
#include <sync.h>
#include <ui_interface.h>
#include <util.h> // for GetBoolArg
#include <wallet/coincontrol.h>
#include <wallet/feebumper.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h> // for BackupWallet

#include <stdint.h>

#include <QDebug>
#include <QMessageBox>
#include <QSet>
#include <QTimer>


WalletModel::WalletModel(const PlatformStyle *platformStyle, CWallet *_wallet, OptionsModel *_optionsModel, QObject *parent) :
    QObject(parent), wallet(_wallet), optionsModel(_optionsModel), addressTableModel(0),
    transactionTableModel(0),
    recentRequestsTableModel(0),
    cachedBalance(0), cachedUnconfirmedBalance(0), cachedImmatureBalance(0), cachedLoanBalance(0), cachedBorrowBalance(0), cachedLockedBalance(0),
    cachedEncryptionStatus(Unencrypted),
    cachedNumBlocks(0)
{
    fHaveWatchOnly = wallet->HaveWatchOnly();
    fForceCheckBalanceChanged = false;

    addressTableModel = new AddressTableModel(platformStyle, wallet, this);
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

CAmount WalletModel::getUnconfirmedBalance() const
{
    return wallet->GetUnconfirmedBalance();
}

CAmount WalletModel::getImmatureBalance() const
{
    return wallet->GetImmatureBalance();
}

CAmount WalletModel::getLoanBalance() const
{
    return wallet->GetLoanBalance();
}

CAmount WalletModel::getBorrowBalance() const
{
    return wallet->GetBorrowBalance();
}

CAmount WalletModel::getLockedBalance() const
{
    return wallet->GetLockedBalance();
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

CAmount WalletModel::getWatchLoanBalance() const
{
    return wallet->GetLoanWatchOnlyBalance();
}

CAmount WalletModel::getWatchBorrowBalance() const
{
    return wallet->GetBorrowWatchOnlyBalance();
}

CAmount WalletModel::getWatchLockedBalance() const
{
    return wallet->GetLockedWatchOnlyBalance();
}

void WalletModel::updateStatus()
{
    EncryptionStatus newEncryptionStatus = getEncryptionStatus();

    if(cachedEncryptionStatus != newEncryptionStatus)
        Q_EMIT encryptionStatusChanged(newEncryptionStatus);
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

    if(fForceCheckBalanceChanged || chainActive.Height() != cachedNumBlocks)
    {
        fForceCheckBalanceChanged = false;

        // Balance and number of transactions might have changed
        cachedNumBlocks = chainActive.Height();

        checkBalanceChanged();
        if(transactionTableModel)
            transactionTableModel->updateConfirmations();
        if(addressTableModel)
            addressTableModel->updateBalance();
    }
}

void WalletModel::walletPrimaryAddressChanged(CWallet * wallet)
{
    if (addressTableModel && wallet == addressTableModel->getWallet())
        addressTableModel->reload();
}

void WalletModel::checkBalanceChanged()
{
    CAmount newBalance = getBalance();
    CAmount newUnconfirmedBalance = getUnconfirmedBalance();
    CAmount newImmatureBalance = getImmatureBalance();
    CAmount newPledgeCreditBalance = getLoanBalance();
    CAmount newPledgeDebitBalance = getBorrowBalance();
    CAmount newLockedBalance = getLockedBalance();
    CAmount newWatchBalance = 0;
    CAmount newWatchUnconfBalance = 0;
    CAmount newWatchImmatureBalance = 0;
    CAmount newWatchPledgeCreditBalance = 0;
    CAmount newWatchPledgeDebitBalance = 0;
    CAmount newWatchLockedBalance = 0;
    if (haveWatchOnly())
    {
        newWatchBalance = getWatchBalance();
        newWatchUnconfBalance = getWatchUnconfirmedBalance();
        newWatchImmatureBalance = getWatchImmatureBalance();
        newWatchPledgeCreditBalance = getWatchLoanBalance();
        newWatchPledgeDebitBalance = getWatchBorrowBalance();
        newWatchLockedBalance = getWatchLockedBalance();
    }

    if (cachedBalance != newBalance || cachedUnconfirmedBalance != newUnconfirmedBalance || cachedImmatureBalance != newImmatureBalance ||
        cachedLoanBalance != newPledgeCreditBalance || cachedBorrowBalance != newPledgeDebitBalance || cachedLockedBalance != newLockedBalance ||
        cachedWatchBalance != newWatchBalance || cachedWatchUnconfBalance != newWatchUnconfBalance || cachedWatchImmatureBalance != newWatchImmatureBalance ||
        cachedWatchLoanBalance != newWatchPledgeCreditBalance || cachedWatchBorrowBalance != newWatchPledgeDebitBalance ||
        cachedWatchLockedBalance != newWatchLockedBalance)
    {
        cachedBalance = newBalance;
        cachedUnconfirmedBalance = newUnconfirmedBalance;
        cachedImmatureBalance = newImmatureBalance;
        cachedLoanBalance = newPledgeCreditBalance;
        cachedBorrowBalance = newPledgeDebitBalance;
        cachedLockedBalance = newLockedBalance;
        cachedWatchBalance = newWatchBalance;
        cachedWatchUnconfBalance = newWatchUnconfBalance;
        cachedWatchImmatureBalance = newWatchImmatureBalance;
        cachedWatchLoanBalance = newWatchPledgeCreditBalance;
        cachedWatchBorrowBalance = newWatchPledgeDebitBalance;
        cachedWatchLockedBalance = newWatchLockedBalance;
        Q_EMIT balanceChanged(newBalance, newUnconfirmedBalance, newImmatureBalance,
                              newPledgeCreditBalance, newPledgeDebitBalance, newLockedBalance,
                              newWatchBalance, newWatchUnconfBalance, newWatchImmatureBalance,
                              newWatchPledgeCreditBalance, newWatchPledgeDebitBalance, newWatchLockedBalance);
    }
}

void WalletModel::updateTransaction()
{
    // Balance and number of transactions might have changed
    fForceCheckBalanceChanged = true;
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

WalletModel::SendCoinsReturn WalletModel::prepareTransaction(WalletModelTransaction &transaction, const CCoinControl& coinControl, PayOperateMethod payOperateMethod)
{
    CAmount total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;

    const int nSpendHeight = GetSpendHeight(*pcoinsTip);

    // Pre-check input data for validity
    if (payOperateMethod == PayOperateMethod::Pay) {
        if (recipients.empty())
            return OK;

        // BUG Define out of if scope will crash on mingw64/windows. May be Qt bug
        QSet<QString> setAddress; // Used to detect duplicates
        int nAddresses = 0;
        for (const SendCoinsRecipient &rcp : recipients) {
            if (rcp.fSubtractFeeFromAmount)
                fSubtractFeeFromAmount = true;

            if (rcp.paymentRequest.IsInitialized()) {
                // PaymentRequest...
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
            } else {
                // User-entered bitcoin address / amount:
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
        if (setAddress.size() != nAddresses)
            return DuplicateAddress;
        if (transaction.getTransaction()->mapValue.count("tx_text")) {
            std::string text = transaction.getTransaction()->mapValue["tx_text"];
            if (text.size() > PROTOCOL_TEXT_MAXSIZE)
                text = text.substr(0, PROTOCOL_TEXT_MAXSIZE);
            CScript script = GetTextScript(text);
            assert(!script.empty());
            vecSend.push_back({script, 0, false});
            nChangePosRet = GetRandInt(vecSend.size());
        }
    }
    else if (payOperateMethod == PayOperateMethod::LoanTo) {
        if (recipients.size() != 1 || recipients[0].paymentRequest.IsInitialized() ||
                coinControl.coinPickPolicy != CoinPickPolicy::IncludeIfSet ||
                coinControl.destChange != coinControl.destPick ||
                !IsValidDestination(coinControl.destPick))
            return InvalidAddress;

        LOCK2(cs_main, wallet->cs_wallet);
        if (nSpendHeight < Params().GetConsensus().BHDIP006Height)
            return InactivedBHDIP006;

        const SendCoinsRecipient &rcp = recipients[0];
        if (!validateAddress(rcp.address))
            return InvalidAddress;

        fSubtractFeeFromAmount = rcp.fSubtractFeeFromAmount;

        vecSend.push_back({GetScriptForDestination(coinControl.destPick), rcp.amount, rcp.fSubtractFeeFromAmount});
        vecSend.push_back({GetRentalScriptForDestination(DecodeDestination(rcp.address.toStdString())), 0, false});
        nChangePosRet = 1;

        total += rcp.amount;
    }
    else if (payOperateMethod == PayOperateMethod::BindPlotter) {
        if (recipients.size() != 1 || recipients[0].paymentRequest.IsInitialized() ||
                coinControl.coinPickPolicy != CoinPickPolicy::IncludeIfSet ||
                coinControl.destChange != coinControl.destPick ||
                !IsValidDestination(coinControl.destPick) ||
                recipients[0].plotterPassphrase.isEmpty())
            return InvalidAddress;

        LOCK2(cs_main, wallet->cs_wallet);
        if (nSpendHeight < Params().GetConsensus().BHDIP006Height)
            return InactivedBHDIP006;
        if (nSpendHeight >= Params().GetConsensus().BHDIP006CheckRelayHeight &&
                (coinControl.m_fee_mode != FeeEstimateMode::FIXED || coinControl.fixedFee < PROTOCOL_BINDPLOTTER_MINFEE))
            return InvalidBindPlotterAmount;

        const SendCoinsRecipient &rcp = recipients[0];
        if (!validateAddress(rcp.address) || coinControl.destPick != DecodeDestination(rcp.address.toStdString()))
            return InvalidAddress;

        fSubtractFeeFromAmount = rcp.fSubtractFeeFromAmount;

        vecSend.push_back({GetScriptForDestination(coinControl.destPick), rcp.amount, rcp.fSubtractFeeFromAmount});
        if (rcp.plotterPassphrase.size() == PROTOCOL_BINDPLOTTER_SCRIPTSIZE * 2 && IsHex(rcp.plotterPassphrase.toStdString())) {
            std::vector<unsigned char> bindData(ParseHex(rcp.plotterPassphrase.toStdString()));
            vecSend.push_back({CScript(bindData.cbegin(), bindData.cend()), 0, false});
        } else {
            vecSend.push_back({GetBindPlotterScriptForDestination(coinControl.destPick, rcp.plotterPassphrase.toStdString(), nSpendHeight - 1 + rcp.plotterDataValidHeight), 0, false});
        }
        nChangePosRet = 1;

        total += rcp.amount;
    }
    else {
        assert(false);
        return InvalidAddress;
    }

    CAmount nBalance = getBalance(&coinControl);

    if (total > nBalance)
        return AmountExceedsBalance;

    {
        LOCK2(cs_main, wallet->cs_wallet);

        transaction.newPossibleKeyChange(wallet);

        CAmount nFeeRequired = 0;
        int nTxVersion = (payOperateMethod != PayOperateMethod::Pay ? CTransaction::UNIFORM_VERSION : 0);
        std::string strFailReason;

        CWalletTx *newTx = transaction.getTransaction();
        CReserveKey *keyChange = transaction.getPossibleKeyChange();
        bool fCreated = wallet->CreateTransaction(vecSend, *newTx, *keyChange, nFeeRequired, nChangePosRet, strFailReason, coinControl, true, nTxVersion);
        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && fCreated)
            transaction.reassignAmounts(nChangePosRet);

        if (!fCreated) {
            if (!fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            Q_EMIT message(tr("Send Coins"), QString::fromStdString(strFailReason), CClientUIInterface::MSG_ERROR);
            return TransactionCreationFailed;
        }

        // Check bind plotter amount
        if (payOperateMethod == PayOperateMethod::BindPlotter && newTx->tx->vout[0].nValue != PROTOCOL_BINDPLOTTER_LOCKAMOUNT)
            return InvalidBindPlotterAmount;
        // Check pledge amount
        if (payOperateMethod == PayOperateMethod::LoanTo && newTx->tx->vout[0].nValue < PROTOCOL_RENTAL_AMOUNT_MIN)
            return SmallLoanAmountExcludeFee;

        // reject absurdly high fee. (This can never happen because the
        // wallet caps the fee at maxTxFee. This merely serves as a
        // belt-and-suspenders check)
        if (nFeeRequired > maxTxFee && (payOperateMethod != PayOperateMethod::BindPlotter ||
                                        coinControl.m_fee_mode != FeeEstimateMode::FIXED ||
                                        nFeeRequired > coinControl.fixedFee))
            return AbsurdFee;
    }

    return SendCoinsReturn(OK);
}

WalletModel::SendCoinsReturn WalletModel::sendCoins(WalletModelTransaction &transaction, PayOperateMethod payOperateMethod)
{
    QByteArray transaction_array; /* store serialized transaction */

    {
        LOCK2(cs_main, wallet->cs_wallet);
        CWalletTx *newTx = transaction.getTransaction();

        for (const SendCoinsRecipient &rcp : transaction.getRecipients())
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
            else if (!rcp.message.isEmpty()) // Message from normal btchd:URI (btchd:123...?message=example)
                newTx->vOrderForm.push_back(make_pair("Message", rcp.message.toStdString()));
        }

        // Check tx
        const int nSpendHeight = GetSpendHeight(*pcoinsTip);
        if (payOperateMethod == PayOperateMethod::LoanTo) {
            CDatacarrierPayloadRef payload = ExtractTransactionDatacarrier(*newTx->tx, nSpendHeight);
            assert(payload && payload->type == DATACARRIER_TYPE_RENTAL);
        } else if (payOperateMethod == PayOperateMethod::BindPlotter) {
            CDatacarrierPayloadRef payload = ExtractTransactionDatacarrier(*newTx->tx, nSpendHeight);
            assert(payload && payload->type == DATACARRIER_TYPE_BINDPLOTTER);
        }

        CReserveKey *keyChange = transaction.getPossibleKeyChange();
        CValidationState state;
        if(!wallet->CommitTransaction(*newTx, *keyChange, g_connman.get(), state))
            return SendCoinsReturn(TransactionCommitFailed, QString::fromStdString(state.GetRejectReason()));

        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << *newTx->tx;
        transaction_array.append(&(ssTx[0]), ssTx.size());
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
    else if(wallet->IsLocked())
    {
        return Locked;
    }
    else
    {
        return Unlocked;
    }
}

CWallet * WalletModel::getWallet()
{
    return wallet;
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

bool WalletModel::setWalletLocked(bool locked, const SecureString &passPhrase)
{
    if(locked)
    {
        // Lock
        return wallet->Lock();
    }
    else
    {
        // Unlock
        return wallet->Unlock(passPhrase);
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
    wallet->ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
    wallet->NotifyWatchonlyChanged.connect(boost::bind(NotifyWatchonlyChanged, this, _1));
}

void WalletModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    wallet->NotifyStatusChanged.disconnect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.disconnect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.disconnect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
    wallet->NotifyWatchonlyChanged.disconnect(boost::bind(NotifyWatchonlyChanged, this, _1));
}

// WalletModel::UnlockContext implementation
WalletModel::UnlockContext WalletModel::requestUnlock()
{
    bool was_locked = getEncryptionStatus() == Locked;
    if(was_locked)
    {
        // Request UI to unlock wallet
        Q_EMIT requireUnlock();
    }
    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    bool valid = getEncryptionStatus() != Locked;

    return UnlockContext(this, valid, was_locked);
}

WalletModel::UnlockContext::UnlockContext(WalletModel *_wallet, bool _valid, bool _relock):
        wallet(_wallet),
        valid(_valid),
        relock(_relock)
{
}

WalletModel::UnlockContext::~UnlockContext()
{
    if(valid && relock)
    {
        wallet->setWalletLocked(true);
    }
}

void WalletModel::UnlockContext::CopyFrom(const UnlockContext& rhs)
{
    // Transfer context; old object no longer relocks wallet
    *this = rhs;
    rhs.relock = false;
}

bool WalletModel::getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    return wallet->GetPubKey(address, vchPubKeyOut);
}

bool WalletModel::IsSpendable(const CTxDestination& dest) const
{
    return IsMine(*wallet, dest) & ISMINE_SPENDABLE;
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
        COutput out(&it->second, outpoint.n, nDepth, true /* spendable */, true /* solvable */, true /* safe */, (outpoint.n == 0 && it->second.mapValue.count("lock")) /* lock */);
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

bool WalletModel::transactionCanBeBumped(uint256 hash) const
{
    return feebumper::TransactionCanBeBumped(wallet, hash);
}

bool WalletModel::bumpFee(uint256 hash)
{
    CCoinControl coin_control;
    coin_control.signalRbf = true;
    std::vector<std::string> errors;
    CAmount old_fee;
    CAmount new_fee;
    CMutableTransaction mtx;
    if (feebumper::CreateTransaction(wallet, hash, coin_control, 0 /* totalFee */, errors, old_fee, new_fee, mtx) != feebumper::Result::OK) {
        QMessageBox::critical(0, tr("Fee bump error"), tr("Increasing transaction fee failed") + "<br />(" +
            (errors.size() ? QString::fromStdString(errors[0]) : "") +")");
        return false;
    }

    // allow a user based fee verification
    QString questionString = tr("Do you want to increase the fee?");
    questionString.append("<br />");
    questionString.append("<table style=\"text-align: left;\">");
    questionString.append("<tr><td>");
    questionString.append(tr("Current fee:"));
    questionString.append("</td><td>");
    questionString.append(BitcoinUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), old_fee));
    questionString.append("</td></tr><tr><td>");
    questionString.append(tr("Increase:"));
    questionString.append("</td><td>");
    questionString.append(BitcoinUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), new_fee - old_fee));
    questionString.append("</td></tr><tr><td>");
    questionString.append(tr("New fee:"));
    questionString.append("</td><td>");
    questionString.append(BitcoinUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), new_fee));
    questionString.append("</td></tr></table>");
    SendConfirmationDialog confirmationDialog(tr("Confirm fee bump"), questionString);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();

    // cancel sign&broadcast if users doesn't want to bump the fee
    if (retval != QMessageBox::Yes) {
        return false;
    }

    WalletModel::UnlockContext ctx(requestUnlock());
    if(!ctx.isValid())
    {
        return false;
    }

    // sign bumped transaction
    if (!feebumper::SignTransaction(wallet, mtx)) {
        QMessageBox::critical(0, tr("Fee bump error"), tr("Can't sign transaction."));
        return false;
    }
    // commit the bumped transaction
    uint256 txid;
    if (feebumper::CommitTransaction(wallet, hash, std::move(mtx), errors, txid) != feebumper::Result::OK) {
        QMessageBox::critical(0, tr("Fee bump error"), tr("Could not commit transaction") + "<br />(" +
            QString::fromStdString(errors[0])+")");
         return false;
    }
    return true;
}

bool WalletModel::transactionCanBeUnlock(uint256 hash, DatacarrierType type) const {
    LOCK2(cs_main, wallet->cs_wallet);

    const Coin &coin = pcoinsTip->AccessCoin(COutPoint(hash, 0));
    if (coin.IsSpent() || !coin.extraData || coin.extraData->type != type)
        return false;

    if (!(wallet->IsMine(coin.out) & ISMINE_SPENDABLE))
        return false;

    // Exist in mempool
    const std::string txid = hash.GetHex();
    for (auto it = wallet->mapWallet.begin(); it != wallet->mapWallet.end(); it++) {
        auto itType = it->second.mapValue.find("type");
        auto itTx = it->second.mapValue.find("relevant_txid");
        if (itType != it->second.mapValue.end() && itTx != it->second.mapValue.end() &&
            (itType->second == "unbindplotter" || itType->second == "withdrawpledge") &&
                itTx->second == txid && it->second.InMempool() && !it->second.isAbandoned())
        {
            return false;
        }
    }

    return true;
}

bool WalletModel::unlockTransaction(uint256 hash) {
    LOCK2(cs_main, wallet->cs_wallet);
    const int nSpendHeight = GetSpendHeight(*pcoinsTip);

    // Coin
    const COutPoint coinEntry(hash, 0);
    const Coin &coin = pcoinsTip->AccessCoin(coinEntry);
    if (coin.IsSpent() || !coin.extraData || (coin.extraData->type != DATACARRIER_TYPE_BINDPLOTTER && coin.extraData->type != DATACARRIER_TYPE_RENTAL))
        return false;

    if (coin.extraData->type == DATACARRIER_TYPE_BINDPLOTTER) {
        int activeHeight = Consensus::GetUnbindPlotterLimitHeight(CBindPlotterInfo(coinEntry, coin), *pcoinsTip, Params().GetConsensus());
        if (nSpendHeight < activeHeight) {
            QString information = tr("Unbind plotter active on %1 block height (%2 blocks after, about %3 minute).").
                                    arg(QString::number(activeHeight),
                                        QString::number(activeHeight - nSpendHeight),
                                        QString::number((activeHeight - nSpendHeight) * Consensus::GetTargetSpacing(nSpendHeight, Params().GetConsensus()) / 60));
            QMessageBox msgBox(QMessageBox::Information, tr("Unbind plotter"), information, QMessageBox::Ok);
            msgBox.exec();
            return false;
        }
    }

    CCoinControl coin_control;
    coin_control.signalRbf = true;

    // Create transaction
    CMutableTransaction txNew;
    txNew.nVersion = CTransaction::UNIFORM_VERSION;
    txNew.nLockTime = std::max(nSpendHeight - 1, Params().GetConsensus().BHDIP006Height);
    txNew.vin.push_back(CTxIn(COutPoint(hash, 0), CScript(), coin_control.signalRbf ? MAX_BIP125_RBF_SEQUENCE : (CTxIn::SEQUENCE_FINAL - 1)));
    CTxDestination lockedDest;
    if (!ExtractDestination(coin.out.scriptPubKey, lockedDest))
        return false;
    txNew.vout.push_back(CTxOut(coin.out.nValue, GetScriptForDestination(lockedDest)));
    coin_control.destChange = lockedDest; // Always change to myself. NEVEN HAPPEN
    CAmount nFeeOut = 0;
    int changePosition = -1;
    std::string strFailReason;
    if (!wallet->FundTransaction(txNew, nFeeOut, changePosition, strFailReason, false, {0}, coin_control)) {
        if (coin.extraData->type == DATACARRIER_TYPE_BINDPLOTTER) {
            QMessageBox::critical(0, tr("Unbind plotter error"), QString::fromStdString(strFailReason));
        } else if (coin.extraData->type == DATACARRIER_TYPE_RENTAL) {
            QMessageBox::critical(0, tr("Withdraw rental error"), QString::fromStdString(strFailReason));
        }
        return false;
    }

    // Sign transaction
    WalletModel::UnlockContext ctx(requestUnlock());
    if(!ctx.isValid())
        return false;
    if (!wallet->SignTransaction(txNew)) {
        if (coin.extraData->type == DATACARRIER_TYPE_BINDPLOTTER) {
            QMessageBox::critical(0, tr("Unbind plotter error"), tr("Can't sign transaction."));
        } else if (coin.extraData->type == DATACARRIER_TYPE_RENTAL) {
            QMessageBox::critical(0, tr("Withdraw rental error"), tr("Can't sign transaction."));
        }
        return false;
    }

    // Make sure
    QMessageBox::StandardButton retval = QMessageBox::NoButton;
    if (coin.extraData->type == DATACARRIER_TYPE_BINDPLOTTER) {
        QString questionString = tr("Are you sure you want to unbind plotter?");
        questionString.append("<br />");
        questionString.append("<table style=\"text-align: left;\">");
        questionString.append("<tr><td>").append(tr("Binded address:")).append("</td><td>").append(QString::fromStdString(EncodeDestination(lockedDest)));
        if (wallet->mapAddressBook.count(lockedDest) && !wallet->mapAddressBook[lockedDest].name.empty())
            questionString.append("(").append(GUIUtil::HtmlEscape(wallet->mapAddressBook[lockedDest].name)).append(")");
        questionString.append("</td></tr>");
        questionString.append("<tr><td>").append(tr("Plotter ID:")).append("</td><td>").append(QString::number(BindPlotterPayload::As(coin.extraData)->GetId())).append("</td></tr>");
        questionString.append("<tr><td>").append(tr("Amount:")).append("</td><td>").append(BitcoinUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), coin.out.nValue)).append("</td></tr>");
        questionString.append("<tr style='color:#aa0000;'><td>").append(tr("Transaction fee:")).append("</td><td>").append(BitcoinUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), nFeeOut)).append("</td></tr>");
        questionString.append("</table>");

        SendConfirmationDialog confirmationDialog(tr("Unbind plotter"), questionString);
        confirmationDialog.exec();
        retval = (QMessageBox::StandardButton)confirmationDialog.result();
    } else if (coin.extraData->type == DATACARRIER_TYPE_RENTAL) {
        QString questionString = tr("Are you sure you want to withdraw loan?");
        questionString.append("<br />");
        questionString.append("<table style=\"text-align: left;\">");
        questionString.append("<tr><td>").append(tr("From address:")).append("</td><td>").append(QString::fromStdString(EncodeDestination(lockedDest)));
        if (wallet->mapAddressBook.count(lockedDest) && !wallet->mapAddressBook[lockedDest].name.empty())
            questionString.append("(").append(GUIUtil::HtmlEscape(wallet->mapAddressBook[lockedDest].name)).append(")");
        questionString.append("</td></tr>");
        {
            CTxDestination dest = CScriptID(RentalPayload::As(coin.extraData)->GetBorrowerAccountID());
            questionString.append("<tr><td>").append(tr("To address:")).append("</td><td>").append(QString::fromStdString(EncodeDestination(dest)));
            if (wallet->mapAddressBook.count(dest) && !wallet->mapAddressBook[dest].name.empty())
                questionString.append("(").append(GUIUtil::HtmlEscape(wallet->mapAddressBook[dest].name)).append(")");
            questionString.append("</td></tr>");
        }
        questionString.append("<tr><td>").append(tr("Amount:")).append("</td><td>").append(BitcoinUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), coin.out.nValue)).append("</td></tr>");
        questionString.append("<tr style='color:#aa0000;'><td>").append(tr("Transaction fee:")).append("</td><td>").append(BitcoinUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), nFeeOut)).append("</td></tr>");
        questionString.append("</table>");

        SendConfirmationDialog confirmationDialog(tr("Withdraw rental"), questionString);
        confirmationDialog.exec();
        retval = (QMessageBox::StandardButton)confirmationDialog.result();
    }
    if (retval != QMessageBox::Yes)
        return false;

    // Send transaction
    CWalletTx wtxNew;
    wtxNew.SetTx(MakeTransactionRef(std::move(txNew)));
    CReserveKey reservekey(wallet);
    CValidationState state;
    if (!wallet->CommitTransaction(wtxNew, reservekey, g_connman.get(), state)) {
        if (coin.extraData->type == DATACARRIER_TYPE_BINDPLOTTER) {
            QMessageBox::critical(0, tr("Unbind plotter error"), tr("Could not commit transaction") + ":" + QString::fromStdString(state.GetRejectReason()));
        } else if (coin.extraData->type == DATACARRIER_TYPE_RENTAL) {
            QMessageBox::critical(0, tr("Withdraw rental error"), tr("Could not commit transaction") + ":" + QString::fromStdString(state.GetRejectReason()));
        }
        return false;
    }

    return true;
}

bool WalletModel::isWalletEnabled()
{
    return !gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET);
}

bool WalletModel::hdEnabled() const
{
    return wallet->IsHDEnabled();
}

OutputType WalletModel::getDefaultAddressType() const
{
    return g_address_type;
}

int WalletModel::getDefaultConfirmTarget() const
{
    return nTxConfirmTarget;
}
