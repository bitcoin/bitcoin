// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletmodel.h>

#include <qt/addresstablemodel.h>
#include <qt/guiconstants.h>
#include <qt/optionsmodel.h>
#include <qt/paymentserver.h>
#include <qt/recentrequeststablemodel.h>
#include <qt/sendcoinsdialog.h>
#include <qt/transactiontablemodel.h>

#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <ui_interface.h>
#include <util.h> // for GetBoolArg
#include <wallet/coincontrol.h>
#include <wallet/wallet.h>
#include <wallet/hdwallet.h>
#include <rpc/rpcutil.h>
#include <util.h>
#include <univalue.h>


#include <stdint.h>

#include <QDebug>
#include <QSet>
#include <QTimer>
#include <QApplication>

WalletModel::WalletModel(std::unique_ptr<interfaces::Wallet> wallet, interfaces::Node& node, const PlatformStyle *platformStyle, OptionsModel *_optionsModel, QObject *parent) :
    QObject(parent), m_wallet(std::move(wallet)), m_node(node), optionsModel(_optionsModel), addressTableModel(0),
    transactionTableModel(0),
    recentRequestsTableModel(0),
    cachedEncryptionStatus(Unencrypted),
    cachedNumBlocks(0)
{
    fHaveWatchOnly = m_wallet->haveWatchOnly();
    fForceCheckBalanceChanged = false;

    addressTableModel = new AddressTableModel(this);
    transactionTableModel = new TransactionTableModel(platformStyle, this);
    recentRequestsTableModel = new RecentRequestsTableModel(this);

    // This timer will be fired repeatedly to update the balance
    pollTimer = new QTimer(this);
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollBalanceChanged()));
    pollTimer->start(MODEL_UPDATE_DELAY);

    connect(getOptionsModel(), SIGNAL(setReserveBalance(CAmount)), this, SLOT(setReserveBalance(CAmount)));
    connect(this, SIGNAL(notifyReservedBalanceChanged(CAmount)), getOptionsModel(), SLOT(updateReservedBalance(CAmount)));

    subscribeToCoreSignals();
}

WalletModel::~WalletModel()
{
    unsubscribeFromCoreSignals();
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
    if (fForceCheckBalanceChanged || m_node.getNumBlocks() != cachedNumBlocks) {
        // Try to get balances and return early if locks can't be acquired. This
        // avoids the GUI from getting stuck on periodical polls if the core is
        // holding the locks for a longer time - for example, during a wallet
        // rescan.
        interfaces::WalletBalances new_balances;
        int numBlocks = -1;
        if (!m_wallet->tryGetBalances(new_balances, numBlocks)) {
            return;
        }

        fForceCheckBalanceChanged = false;

        // Balance and number of transactions might have changed
        cachedNumBlocks = m_node.getNumBlocks();

        checkBalanceChanged(new_balances);
        if(transactionTableModel)
            transactionTableModel->updateConfirmations();
    }
}

void WalletModel::setReserveBalance(CAmount nReserveBalanceNew)
{
    m_wallet->setReserveBalance(nReserveBalanceNew);
};

void WalletModel::updateReservedBalanceChanged(CAmount nValue)
{
    Q_EMIT notifyReservedBalanceChanged(nValue);
};

void WalletModel::startRescan()
{
    std::thread t(CallRPCVoid, "rescanblockchain 0", m_wallet->getWalletName());
    t.detach();
};

void WalletModel::checkBalanceChanged(const interfaces::WalletBalances& new_balances)
{
    if(new_balances.balanceChanged(m_cached_balances)) {
        bool fHaveWatchOnlyCheck = new_balances.watch_only_balance || new_balances.unconfirmed_watch_only_balance || new_balances.balanceWatchStaked;
        if (fHaveWatchOnlyCheck != fHaveWatchOnly)
            updateWatchOnlyFlag(fHaveWatchOnlyCheck);
        m_cached_balances = new_balances;
        Q_EMIT balanceChanged(new_balances);
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

WalletModel::SendCoinsReturn WalletModel::prepareTransaction(WalletModelTransaction &transaction, const CCoinControl& coinControl)
{
    CAmount total = 0;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;

    if(recipients.empty())
    {
        return OK;
    }

    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    // Pre-check input data for validity
    for (const SendCoinsRecipient &rcp : recipients)
    {

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
        {   // User-entered bitcoin address / amount:
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

    /*
    CAmount nBalance = m_wallet->getAvailableBalance(coinControl);

    if(total > nBalance)
    {
        return AmountExceedsBalance;
    }


    {
        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        std::string strFailReason;

        auto& newTx = transaction.getWtx();
        newTx = m_wallet->createTransaction(vecSend, coinControl, true / sign /, nChangePosRet, nFeeRequired, strFailReason);
        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && newTx)
            transaction.reassignAmounts(nChangePosRet);

        if(!newTx)
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
        if (nFeeRequired > m_node.getMaxTxFee())
            return AbsurdFee;
    }
    */
    return SendCoinsReturn(OK);
}

WalletModel::SendCoinsReturn WalletModel::sendCoins(WalletModelTransaction &transaction)
{
    QByteArray transaction_array; /* store serialized transaction */

    {
        std::vector<std::pair<std::string, std::string>> vOrderForm;
        for (const SendCoinsRecipient &rcp : transaction.getRecipients())
        {
            if (rcp.paymentRequest.IsInitialized())
            {
                // Make sure any payment requests involved are still valid.
                if (PaymentServer::verifyExpired(rcp.paymentRequest.getDetails())) {
                    return PaymentRequestExpired;
                }

                // Store PaymentRequests in wtx.vOrderForm in wallet.
                std::string value;
                rcp.paymentRequest.SerializeToString(&value);
                vOrderForm.emplace_back("PaymentRequest", std::move(value));
            }
            else if (!rcp.message.isEmpty()) // Message from normal bitcoin:URI (bitcoin:123...?message=example)
                vOrderForm.emplace_back("Message", rcp.message.toStdString());
        }

        auto& newTx = transaction.getWtx();
        std::string rejectReason;
        if (!newTx->commit({} /* mapValue */, std::move(vOrderForm), {} /* fromAccount */, rejectReason))
            return SendCoinsReturn(TransactionCommitFailed, QString::fromStdString(rejectReason));

        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << newTx->get();
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
                // Check if we have a new address or an updated label
                std::string name;
                if (!m_wallet->getAddress(
                     dest, &name, /* is_mine= */ nullptr, /* purpose= */ nullptr))
                {
                    m_wallet->setAddressBook(dest, strLabel, "send");
                }
                else if (name != strLabel)
                {
                    m_wallet->setAddressBook(dest, strLabel, ""); // "" means don't change purpose
                }
            }
        }
        Q_EMIT coinsSent(this, rcp, transaction_array);
    }

    checkBalanceChanged(m_wallet->getBalances()); // update balance immediately, otherwise there could be a short noticeable delay until pollBalanceChanged hits

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
    if(!m_wallet->isCrypted())
    {
        return Unencrypted;
    }
    else if(m_wallet->isLocked())
    {
        return Locked;
    }
    else
    {
        if (m_wallet->isUnlockForStakingOnlySet())
            return UnlockedForStaking;

        return Unlocked;
    }
}

bool WalletModel::setWalletEncrypted(bool encrypted, const SecureString &passphrase)
{
    if(encrypted)
    {
        // Encrypt
        return m_wallet->encryptWallet(passphrase);
    }
    else
    {
        // Decrypt -- TODO; not supported yet
        return false;
    }
}

bool WalletModel::setWalletLocked(bool locked, const SecureString &passPhrase, bool stakingOnly)
{
    if(locked)
    {
        // Lock
        return m_wallet->lock();
    }
    else
    {
        // Unlock
        return m_wallet->unlock(passPhrase, stakingOnly);
    }
}

bool WalletModel::setUnlockedForStaking()
{
    if (!m_wallet->setUnlockedForStaking())
        return false;
    updateStatus();
    return true;
}

bool WalletModel::changePassphrase(const SecureString &oldPass, const SecureString &newPass)
{
    m_wallet->lock(); // Make sure wallet is locked before attempting pass change
    return m_wallet->changeWalletPassphrase(oldPass, newPass);
}

// Handlers for core signals
static void NotifyUnload(WalletModel* walletModel)
{
    qDebug() << "NotifyUnload";
    QMetaObject::invokeMethod(walletModel, "unload", Qt::QueuedConnection);
}

static void NotifyKeyStoreStatusChanged(WalletModel *walletmodel)
{
    qDebug() << "NotifyKeyStoreStatusChanged";
    QMetaObject::invokeMethod(walletmodel, "updateStatus", Qt::QueuedConnection);
}

static void NotifyAddressBookChanged(WalletModel *walletmodel,
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

static void NotifyTransactionChanged(WalletModel *walletmodel, const uint256 &hash, ChangeType status)
{
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

static void NotifyReservedBalanceChanged(WalletModel *walletmodel, CAmount nValue)
{
    QMetaObject::invokeMethod(walletmodel, "updateReservedBalanceChanged", Qt::QueuedConnection,
                              Q_ARG(CAmount, nValue));
}

void WalletModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    m_handler_unload = m_wallet->handleUnload(boost::bind(&NotifyUnload, this));
    m_handler_status_changed = m_wallet->handleStatusChanged(boost::bind(&NotifyKeyStoreStatusChanged, this));
    m_handler_address_book_changed = m_wallet->handleAddressBookChanged(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5));
    m_handler_transaction_changed = m_wallet->handleTransactionChanged(boost::bind(NotifyTransactionChanged, this, _1, _2));
    m_handler_show_progress = m_wallet->handleShowProgress(boost::bind(ShowProgress, this, _1, _2));
    m_handler_watch_only_changed = m_wallet->handleWatchOnlyChanged(boost::bind(NotifyWatchonlyChanged, this, _1));

    if (m_wallet->IsParticlWallet()) {
        m_handler_reserved_balance_changed = m_wallet->handleReservedBalanceChanged(boost::bind(NotifyReservedBalanceChanged, this, _1));
    }
}

void WalletModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    m_handler_unload->disconnect();
    m_handler_status_changed->disconnect();
    m_handler_address_book_changed->disconnect();
    m_handler_transaction_changed->disconnect();
    m_handler_show_progress->disconnect();
    m_handler_watch_only_changed->disconnect();

    if (m_wallet->IsParticlWallet()) {
        m_handler_reserved_balance_changed->disconnect();
    }
}

// WalletModel::UnlockContext implementation
WalletModel::UnlockContext WalletModel::requestUnlock()
{
    bool was_locked = getEncryptionStatus() == Locked || getEncryptionStatus() == UnlockedForStaking;
    bool was_unlocked_for_staking = getEncryptionStatus() == UnlockedForStaking;
    if(was_locked)
    {
        // Request UI to unlock wallet
        Q_EMIT requireUnlock();
    }
    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    //bool valid = getEncryptionStatus() != Locked;
    EncryptionStatus newStatus = getEncryptionStatus();
    bool valid = newStatus == Unlocked || newStatus == Unencrypted;

    return UnlockContext(this, valid, was_locked, was_unlocked_for_staking);
}

WalletModel::UnlockContext::UnlockContext(WalletModel *_wallet, bool _valid, bool _relock, bool _was_unlocked_for_staking):
        wallet(_wallet),
        valid(_valid),
        relock(_relock),
        was_unlocked_for_staking(_was_unlocked_for_staking)
{
}

WalletModel::UnlockContext::~UnlockContext()
{
    if(valid && relock)
    {
        if (was_unlocked_for_staking)
            wallet->setUnlockedForStaking();
        else
            wallet->setWalletLocked(true);
    }
}

void WalletModel::UnlockContext::CopyFrom(const UnlockContext& rhs)
{
    // Transfer context; old object no longer relocks wallet
    *this = rhs;
    rhs.relock = false;
}

bool WalletModel::tryCallRpc(const QString &sCommand, UniValue &rv, bool returnError) const
{
    try {
        rv = CallRPC(sCommand.toStdString(), m_wallet->getWalletName());
    } catch (UniValue& objError)
    {
        if (returnError)
        {
            rv = objError;
            return false;
        };
        try { // Nice formatting for standard-format error
            int code = find_value(objError, "code").get_int();
            std::string message = find_value(objError, "message").get_str();
            warningBox(tr("Wallet Model"), QString::fromStdString(message) + " (code " + QString::number(code) + ")");
            return false;
        } catch (const std::runtime_error&) // raised when converting to invalid type, i.e. missing code or message
        {   // Show raw JSON object
            warningBox(tr("Wallet Model"), QString::fromStdString(objError.write()));
            return false;
        };
    } catch (const std::exception& e)
    {
        if (returnError)
        {
            rv = UniValue(UniValue::VOBJ);
            rv.pushKV("Error", e.what());
        } else
        {
            warningBox(tr("Wallet Model"), QString::fromStdString(e.what()));
        };
        return false;
    };

    return true;
};

void WalletModel::warningBox(QString heading, QString msg) const
{
    qWarning() << msg;
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    msgParams.second = CClientUIInterface::MSG_WARNING;
    msgParams.first = msg;

    Q_EMIT message(heading, msgParams.first, msgParams.second);
};

void WalletModel::loadReceiveRequests(std::vector<std::string>& vReceiveRequests)
{
    vReceiveRequests = m_wallet->getDestValues("rr"); // receive request
}

bool WalletModel::saveReceiveRequest(const std::string &sAddress, const int64_t nId, const std::string &sRequest)
{
    CTxDestination dest = DecodeDestination(sAddress);

    std::stringstream ss;
    ss << nId;
    std::string key = "rr" + ss.str(); // "rr" prefix = "receive request" in destdata

    if (sRequest.empty())
        return m_wallet->eraseDestData(dest, key);
    else
        return m_wallet->addDestData(dest, key, sRequest);
}

bool WalletModel::bumpFee(uint256 hash)
{
    CCoinControl coin_control;
    coin_control.m_signal_bip125_rbf = true;
    std::vector<std::string> errors;
    CAmount old_fee;
    CAmount new_fee;
    CMutableTransaction mtx;
    if (!m_wallet->createBumpTransaction(hash, coin_control, 0 /* totalFee */, errors, old_fee, new_fee, mtx)) {
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
    QMessageBox::StandardButton retval = static_cast<QMessageBox::StandardButton>(confirmationDialog.result());

    // cancel sign&broadcast if user doesn't want to bump the fee
    if (retval != QMessageBox::Yes) {
        return false;
    }

    WalletModel::UnlockContext ctx(requestUnlock());
    if(!ctx.isValid())
    {
        return false;
    }

    // sign bumped transaction
    if (!m_wallet->signBumpTransaction(mtx)) {
        QMessageBox::critical(0, tr("Fee bump error"), tr("Can't sign transaction."));
        return false;
    }
    // commit the bumped transaction
    uint256 txid;
    if(!m_wallet->commitBumpTransaction(hash, std::move(mtx), errors, txid)) {
        QMessageBox::critical(0, tr("Fee bump error"), tr("Could not commit transaction") + "<br />(" +
            QString::fromStdString(errors[0])+")");
         return false;
    }
    return true;
}

bool WalletModel::isWalletEnabled()
{
   return !gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET);
}

void WalletModel::lockWallet()
{
    m_wallet->lockWallet();
};


QString WalletModel::getWalletName() const
{
    return QString::fromStdString(m_wallet->getWalletName());
}

bool WalletModel::isMultiwallet()
{
    return m_node.getWallets().size() > 1;
}
