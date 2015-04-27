// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoin_walletmodel.h"

#include "bitcoin_addresstablemodel.h"
#include "guiconstants.h"

#include "base58.h"
#include "bitcoin_db.h"
#include "keystore.h"
#include "bitcoin_main.h"
#include "sync.h"
#include "ui_interface.h"
#include "wallet.h"
#include "bitcoin_wallet.h"
#include "bitcoin_walletdb.h" // for BackupWallet

#include <stdint.h>

#include <QDebug>
#include <QSet>
#include <QTimer>

Bitcoin_WalletModel::Bitcoin_WalletModel(Bitcoin_CWallet *wallet, Bitcredit_CWallet *bitcredit_wallet, OptionsModel *optionsModel, QObject *parent) :
    QObject(parent), wallet(wallet), bitcredit_wallet(bitcredit_wallet), optionsModel(optionsModel), addressTableModel(0),
    cachedBalance(0), cachedUnconfirmedBalance(0), cachedImmatureBalance(0),
    cachedNumTransactions(0),
    cachedEncryptionStatus(Unencrypted),
    cachedNumBlocks(0)
{
    addressTableModel = new Bitcoin_AddressTableModel(wallet, this);

    // This timer will be fired repeatedly to update the balance
    pollTimer = new QTimer(this);
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollBalanceChanged()));
    pollTimer->start(MODEL_UPDATE_DELAY);

    subscribeToCoreSignals();
}

Bitcoin_WalletModel::~Bitcoin_WalletModel()
{
    unsubscribeFromCoreSignals();
}

qint64 Bitcoin_WalletModel::getBalance(Bitcoin_CClaimCoinsViewCache* claim_view, map<uint256, set<int> >& mapFilterTxInPoints, const CCoinControl *coinControl) const
{
    wallet->MarkDirty();

    if (coinControl)
    {
        qint64 nBalance = 0;
        std::vector<Bitcoin_COutput> vCoins;
        wallet->AvailableCoins(vCoins, claim_view, mapFilterTxInPoints, true, coinControl);
        BOOST_FOREACH(const Bitcoin_COutput& out, vCoins)
            nBalance += out.nValue;

        return nBalance;
    }

    return wallet->GetBalance(claim_view, mapFilterTxInPoints);
}

qint64 Bitcoin_WalletModel::getUnconfirmedBalance(Bitcoin_CClaimCoinsViewCache* claim_view, map<uint256, set<int> >& mapFilterTxInPoints) const
{
	wallet->MarkDirty();
    return wallet->GetUnconfirmedBalance(claim_view, mapFilterTxInPoints);
}

qint64 Bitcoin_WalletModel::getImmatureBalance(Bitcoin_CClaimCoinsViewCache* claim_view, map<uint256, set<int> >& mapFilterTxInPoints) const
{
	wallet->MarkDirty();
    return wallet->GetImmatureBalance(claim_view, mapFilterTxInPoints);
}

int Bitcoin_WalletModel::getNumTransactions() const
{
    int numTransactions = 0;
    {
        LOCK(wallet->cs_wallet);
        // the size of mapWallet contains the number of unique transaction IDs
        // (e.g. payments to yourself generate 2 transactions, but both share the same transaction ID)
        numTransactions = wallet->mapWallet.size();
    }
    return numTransactions;
}

void Bitcoin_WalletModel::updateStatus()
{
    EncryptionStatus newEncryptionStatus = getEncryptionStatus();

    if(cachedEncryptionStatus != newEncryptionStatus)
        emit encryptionStatusChanged(newEncryptionStatus);
}

void Bitcoin_WalletModel::pollBalanceChanged()
{
    // Get required locks upfront. This avoids the GUI from getting stuck on
    // periodical polls if the core is holding the locks for a longer time -
    // for example, during a wallet rescan.
    TRY_LOCK(bitcoin_mainState.cs_main, lockMain);
    if(!lockMain)
        return;
    TRY_LOCK(wallet->cs_wallet, lockWallet);
    if(!lockWallet)
        return;

    if(bitcoin_chainActive.Height() != cachedNumBlocks)
    {
        // Balance and number of transactions might have changed
        cachedNumBlocks = bitcoin_chainActive.Height();

        checkBalanceChanged();
    }
}

void Bitcoin_WalletModel::checkBalanceChanged()
{
    //Find all claimed  transactions
    map<uint256, set<int> > mapClaimTxInPoints;
    bitcredit_wallet->ClaimTxInPoints(mapClaimTxInPoints);

    qint64 newBalance = getBalance(bitcoin_pclaimCoinsTip, mapClaimTxInPoints);
    qint64 newUnconfirmedBalance = getUnconfirmedBalance(bitcoin_pclaimCoinsTip, mapClaimTxInPoints);
    qint64 newImmatureBalance = getImmatureBalance(bitcoin_pclaimCoinsTip, mapClaimTxInPoints);

    if(cachedBalance != newBalance || cachedUnconfirmedBalance != newUnconfirmedBalance || cachedImmatureBalance != newImmatureBalance)
    {
        cachedBalance = newBalance;
        cachedUnconfirmedBalance = newUnconfirmedBalance;
        cachedImmatureBalance = newImmatureBalance;
        emit balanceChanged(newBalance, newUnconfirmedBalance, newImmatureBalance);
    }
}

void Bitcoin_WalletModel::updateTransaction(const QString &hash, int status)
{
    // Balance and number of transactions might have changed
    checkBalanceChanged();

    int newNumTransactions = getNumTransactions();
    if(cachedNumTransactions != newNumTransactions)
    {
        cachedNumTransactions = newNumTransactions;
        emit numTransactionsChanged(newNumTransactions);
    }
}

void Bitcoin_WalletModel::updateAddressBook(const QString &address, const QString &label,
        bool isMine, const QString &purpose, int status)
{
    if(addressTableModel)
        addressTableModel->updateEntry(address, label, isMine, purpose, status);
}

bool Bitcoin_WalletModel::validateAddress(const QString &address)
{
    CBitcoinAddress addressParsed(address.toStdString());
    return addressParsed.IsValid();
}

//Bitcoin_WalletModel::SendCoinsReturn Bitcoin_WalletModel::prepareTransaction(Bitcoin_WalletModelTransaction &transaction, const CCoinControl *coinControl)
//{
//    qint64 total = 0;
//    QList<Bitcoin_SendCoinsRecipient> recipients = transaction.getRecipients();
//    std::vector<std::pair<CScript, int64_t> > vecSend;
//
//    if(recipients.empty())
//    {
//        return OK;
//    }
//
//    QSet<QString> setAddress; // Used to detect duplicates
//    int nAddresses = 0;
//
//    // Pre-check input data for validity
//    foreach(const Bitcoin_SendCoinsRecipient &rcp, recipients)
//    {
//        if (rcp.paymentRequest.IsInitialized())
//        {   // PaymentRequest...
//            int64_t subtotal = 0;
//            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
//            for (int i = 0; i < details.outputs_size(); i++)
//            {
//                const payments::Output& out = details.outputs(i);
//                if (out.amount() <= 0) continue;
//                subtotal += out.amount();
//                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
//                CScript scriptPubKey(scriptStr, scriptStr+out.script().size());
//                vecSend.push_back(std::pair<CScript, int64_t>(scriptPubKey, out.amount()));
//            }
//            if (subtotal <= 0)
//            {
//                return InvalidAmount;
//            }
//            total += subtotal;
//        }
//        else
//        {   // User-entered bitcoin address / amount:
//            if(!validateAddress(rcp.address))
//            {
//                return InvalidAddress;
//            }
//            if(rcp.amount <= 0)
//            {
//                return InvalidAmount;
//            }
//            setAddress.insert(rcp.address);
//            ++nAddresses;
//
//            CScript scriptPubKey;
//            scriptPubKey.SetDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
//            vecSend.push_back(std::pair<CScript, int64_t>(scriptPubKey, rcp.amount));
//
//            total += rcp.amount;
//        }
//    }
//    if(setAddress.size() != nAddresses)
//    {
//        return DuplicateAddress;
//    }
//
//    qint64 nBalance = getBalance(coinControl);
//
//    if(total > nBalance)
//    {
//        return AmountExceedsBalance;
//    }
//
//    if((total + bitcoin_nTransactionFee) > nBalance)
//    {
//        transaction.setTransactionFee(bitcoin_nTransactionFee);
//        return SendCoinsReturn(AmountWithFeeExceedsBalance);
//    }
//
//    {
//        LOCK2(bitcoin_mainState.cs_main, wallet->cs_wallet);
//
//        transaction.newPossibleKeyChange(wallet);
//        int64_t nFeeRequired = 0;
//        std::string strFailReason;
//
//        Bitcoin_CWalletTx *newTx = transaction.getTransaction();
//        Bitcoin_CReserveKey *keyChange = transaction.getPossibleKeyChange();
//        bool fCreated = wallet->CreateTransaction(vecSend, *newTx, *keyChange, nFeeRequired, strFailReason, coinControl);
//        transaction.setTransactionFee(nFeeRequired);
//
//        if(!fCreated)
//        {
//            if((total + nFeeRequired) > nBalance)
//            {
//                return SendCoinsReturn(AmountWithFeeExceedsBalance);
//            }
//            emit message(tr("Send Coins"), QString::fromStdString(strFailReason),
//                         CClientUIInterface::MSG_ERROR);
//            return TransactionCreationFailed;
//        }
//    }
//
//    return SendCoinsReturn(OK);
//}
//
//Bitcoin_WalletModel::SendCoinsReturn Bitcoin_WalletModel::sendCoins(Bitcoin_WalletModelTransaction &transaction)
//{
//    QByteArray transaction_array; /* store serialized transaction */
//
//    {
//        LOCK2(bitcoin_mainState.cs_main, wallet->cs_wallet);
//        Bitcoin_CWalletTx *newTx = transaction.getTransaction();
//
//        // Store PaymentRequests in wtx.vOrderForm in wallet.
//        foreach(const Bitcoin_SendCoinsRecipient &rcp, transaction.getRecipients())
//        {
//            if (rcp.paymentRequest.IsInitialized())
//            {
//                std::string key("PaymentRequest");
//                std::string value;
//                rcp.paymentRequest.SerializeToString(&value);
//                newTx->vOrderForm.push_back(make_pair(key, value));
//            }
//            else if (!rcp.message.isEmpty()) // Message from normal bitcredit:URI (bitcredit:123...?message=example)
//                newTx->vOrderForm.push_back(make_pair("Message", rcp.message.toStdString()));
//        }
//
//        Bitcoin_CReserveKey *keyChange = transaction.getPossibleKeyChange();
//        if(!wallet->CommitTransaction(*newTx, *keyChange))
//            return TransactionCommitFailed;
//
//        Bitcoin_CTransaction* t = (Bitcoin_CTransaction*)newTx;
//        CDataStream ssTx(SER_NETWORK, BITCOIN_PROTOCOL_VERSION);
//        ssTx << *t;
//        transaction_array.append(&(ssTx[0]), ssTx.size());
//    }
//
//    // Add addresses / update labels that we've sent to to the address book,
//    // and emit coinsSent signal for each recipient
//    foreach(const Bitcoin_SendCoinsRecipient &rcp, transaction.getRecipients())
//    {
//        // Don't touch the address book when we have a payment request
//        if (!rcp.paymentRequest.IsInitialized())
//        {
//            std::string strAddress = rcp.address.toStdString();
//            CTxDestination dest = CBitcoinAddress(strAddress).Get();
//            std::string strLabel = rcp.label.toStdString();
//            {
//                LOCK(wallet->cs_wallet);
//
//                std::map<CTxDestination, CAddressBookData>::iterator mi = wallet->mapAddressBook.find(dest);
//
//                // Check if we have a new address or an updated label
//                if (mi == wallet->mapAddressBook.end())
//                {
//                    wallet->SetAddressBook(dest, strLabel, "send");
//                }
//                else if (mi->second.name != strLabel)
//                {
//                    wallet->SetAddressBook(dest, strLabel, ""); // "" means don't change purpose
//                }
//            }
//        }
//        emit coinsSent(wallet, rcp, transaction_array);
//    }
//
//    return SendCoinsReturn(OK);
//}

OptionsModel *Bitcoin_WalletModel::getOptionsModel()
{
    return optionsModel;
}

Bitcoin_AddressTableModel *Bitcoin_WalletModel::getAddressTableModel()
{
    return addressTableModel;
}

Bitcoin_WalletModel::EncryptionStatus Bitcoin_WalletModel::getEncryptionStatus() const
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

bool Bitcoin_WalletModel::setWalletEncrypted(bool encrypted, const SecureString &passphrase)
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

bool Bitcoin_WalletModel::setWalletLocked(bool locked, const SecureString &passPhrase)
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

bool Bitcoin_WalletModel::changePassphrase(const SecureString &oldPass, const SecureString &newPass)
{
    bool retval;
    {
        LOCK(wallet->cs_wallet);
        wallet->Lock(); // Make sure wallet is locked before attempting pass change
        retval = wallet->ChangeWalletPassphrase(oldPass, newPass);
    }
    return retval;
}

bool Bitcoin_WalletModel::backupWallet(const QString &filename)
{
    return Bitcoin_BackupWallet(*wallet, filename.toLocal8Bit().data());
}

// Handlers for core signals
static void NotifyKeyStoreStatusChanged(Bitcoin_WalletModel *walletmodel, CCryptoKeyStore *wallet)
{
    qDebug() << "NotifyKeyStoreStatusChanged";
    QMetaObject::invokeMethod(walletmodel, "updateStatus", Qt::QueuedConnection);
}

static void NotifyAddressBookChanged(Bitcoin_WalletModel *walletmodel, Bitcoin_CWallet *wallet,
        const CTxDestination &address, const std::string &label, bool isMine,
        const std::string &purpose, ChangeType status)
{
    QString strAddress = QString::fromStdString(CBitcoinAddress(address).ToString());
    QString strLabel = QString::fromStdString(label);
    QString strPurpose = QString::fromStdString(purpose);

    qDebug() << "NotifyAddressBookChanged : " + strAddress + " " + strLabel + " isMine=" + QString::number(isMine) + " purpose=" + strPurpose + " status=" + QString::number(status);
    QMetaObject::invokeMethod(walletmodel, "updateAddressBook", Qt::QueuedConnection,
                              Q_ARG(QString, strAddress),
                              Q_ARG(QString, strLabel),
                              Q_ARG(bool, isMine),
                              Q_ARG(QString, strPurpose),
                              Q_ARG(int, status));
}

// queue notifications to show a non freezing progress dialog e.g. for rescan
static bool fQueueNotifications = false;
static std::vector<std::pair<uint256, ChangeType> > vQueueNotifications;
static void NotifyTransactionChanged(Bitcoin_WalletModel *walletmodel, Bitcoin_CWallet *wallet, const uint256 &hash, ChangeType status)
{
    if (fQueueNotifications)
    {
        vQueueNotifications.push_back(make_pair(hash, status));
        return;
    }

    QString strHash = QString::fromStdString(hash.GetHex());

    qDebug() << "NotifyTransactionChanged : " + strHash + " status= " + QString::number(status);
    QMetaObject::invokeMethod(walletmodel, "updateTransaction", Qt::QueuedConnection,
                              Q_ARG(QString, strHash),
                              Q_ARG(int, status));
}

static void ShowProgress(Bitcoin_WalletModel *walletmodel, const std::string &title, int nProgress)
{
    // emits signal "showProgress"
    QMetaObject::invokeMethod(walletmodel, "showProgress", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(title)),
                              Q_ARG(int, nProgress));

    if (nProgress == 0)
        fQueueNotifications = true;

    if (nProgress == 100)
    {
        fQueueNotifications = false;
        BOOST_FOREACH(const PAIRTYPE(uint256, ChangeType)& notification, vQueueNotifications)
            NotifyTransactionChanged(walletmodel, NULL, notification.first, notification.second);
        std::vector<std::pair<uint256, ChangeType> >().swap(vQueueNotifications); // clear
    }
}

void Bitcoin_WalletModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    wallet->NotifyStatusChanged.connect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.connect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.connect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
}

void Bitcoin_WalletModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    wallet->NotifyStatusChanged.disconnect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.disconnect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.disconnect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
}

// Bitcoin_WalletModel::UnlockContext implementation
Bitcoin_WalletModel::UnlockContext Bitcoin_WalletModel::requestUnlock()
{
    bool was_locked = getEncryptionStatus() == Locked;
    if(was_locked)
    {
        // Request UI to unlock wallet
        emit requireUnlock();
    }
    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    bool valid = getEncryptionStatus() != Locked;

    return UnlockContext(this, valid, was_locked);
}

Bitcoin_WalletModel::UnlockContext::UnlockContext(Bitcoin_WalletModel *wallet, bool valid, bool relock):
        wallet(wallet),
        valid(valid),
        relock(relock)
{
}

Bitcoin_WalletModel::UnlockContext::~UnlockContext()
{
    if(valid && relock)
    {
        wallet->setWalletLocked(true);
    }
}

void Bitcoin_WalletModel::UnlockContext::CopyFrom(const UnlockContext& rhs)
{
    // Transfer context; old object no longer relocks wallet
    *this = rhs;
    rhs.relock = false;
}

bool Bitcoin_WalletModel::getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    return wallet->GetPubKey(address, vchPubKeyOut);
}

// returns a list of COutputs from COutPoints
void Bitcoin_WalletModel::getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<Bitcoin_COutput>& vOutputs, Bitcoin_CClaimCoinsViewCache *claim_view)
{
    LOCK2(bitcoin_mainState.cs_main, wallet->cs_wallet);
    BOOST_FOREACH(const COutPoint& outpoint, vOutpoints)
    {
    	const uint256 &hash = outpoint.hash;
    	const unsigned int &n = outpoint.n;

        if (!wallet->mapWallet.count(hash)) continue;
        int nDepth = wallet->mapWallet[hash].GetDepthInMainChain();
        if (nDepth < 0) continue;

        if(claim_view == NULL) {
			Bitcoin_COutput out(&wallet->mapWallet[hash], n, nDepth, wallet->mapWallet[hash].vout[n].nValue);
			vOutputs.push_back(out);
        } else {
            if(claim_view->HaveCoins(hash)) {
    			const Bitcoin_CClaimCoins & claimCoins = claim_view->GetCoins(hash);
    			if(claimCoins.HasClaimable(n)) {
    				Bitcoin_COutput out(&wallet->mapWallet[hash], n, nDepth, claimCoins.vout[n].nValueClaimable);
    				vOutputs.push_back(out);
    			}
            }
        }
    }
}

bool Bitcoin_WalletModel::isSpent(const COutPoint& outpoint, Bitcoin_CClaimCoinsViewCache *claim_view) const
{
    LOCK2(bitcoin_mainState.cs_main, wallet->cs_wallet);
	const int nClaimBestBlockDepth = wallet->GetBestBlockClaimDepth(claim_view);
    return wallet->IsSpent(outpoint.hash, outpoint.n, nClaimBestBlockDepth);
}

// AvailableCoins + LockedCoins grouped by wallet address (put change in one group with wallet address)
void Bitcoin_WalletModel::listCoins(std::map<QString, std::vector<Bitcoin_COutput> >& mapCoins, Bitcoin_CClaimCoinsViewCache *claim_view) const
{
    wallet->MarkDirty();

    //Find all claimed  transactions
    map<uint256, set<int> > mapClaimTxInPoints;
    bitcredit_wallet->ClaimTxInPoints(mapClaimTxInPoints);

    std::vector<Bitcoin_COutput> vCoins;
    wallet->AvailableCoins(vCoins, claim_view, mapClaimTxInPoints);

    LOCK2(bitcoin_mainState.cs_main, wallet->cs_wallet); // ListLockedCoins, mapWallet
    std::vector<COutPoint> vLockedCoins;
    wallet->ListLockedCoins(vLockedCoins);

    // add locked coins
    BOOST_FOREACH(const COutPoint& outpoint, vLockedCoins)
    {
    	const uint256 &hash = outpoint.hash;
    	const unsigned int &n = outpoint.n;

        if (!wallet->mapWallet.count(hash)) continue;
        int nDepth = wallet->mapWallet[hash].GetDepthInMainChain();
        if (nDepth < 0) continue;

        if(claim_view == NULL) {
			Bitcoin_COutput out(&wallet->mapWallet[hash], n, nDepth, wallet->mapWallet[hash].vout[n].nValue);
			vCoins.push_back(out);
        } else {
			if(claim_view->HaveCoins(hash)) {
				const Bitcoin_CClaimCoins & claimCoins = claim_view->GetCoins(hash);
				if(claimCoins.HasClaimable(n)) {
					Bitcoin_COutput out(&wallet->mapWallet[hash], n, nDepth, claimCoins.vout[n].nValueClaimable);
					vCoins.push_back(out);
				}
			}
        }
    }

    BOOST_FOREACH(const Bitcoin_COutput& out, vCoins)
    {
        Bitcoin_COutput cout = out;

        while (wallet->IsChange(cout.tx->vout[cout.i]) && cout.tx->vin.size() > 0 && wallet->IsMine(cout.tx->vin[0]))
        {
        	const uint256 &hash = cout.tx->vin[0].prevout.hash;
        	const unsigned int &n = cout.tx->vin[0].prevout.n;

            if (!wallet->mapWallet.count(hash)) break;
			cout = Bitcoin_COutput(&wallet->mapWallet[hash], n, 0, 0);
        }

        CTxDestination address;
        if(!ExtractDestination(cout.tx->vout[cout.i].scriptPubKey, address)) continue;
        mapCoins[CBitcoinAddress(address).ToString().c_str()].push_back(out);
    }
}

bool Bitcoin_WalletModel::isLockedCoin(uint256 hash, unsigned int n) const
{
    LOCK2(bitcoin_mainState.cs_main, wallet->cs_wallet);
    return wallet->IsLockedCoin(hash, n);
}

void Bitcoin_WalletModel::lockCoin(COutPoint& output)
{
    LOCK2(bitcoin_mainState.cs_main, wallet->cs_wallet);
    wallet->LockCoin(output);
}

void Bitcoin_WalletModel::unlockCoin(COutPoint& output)
{
    LOCK2(bitcoin_mainState.cs_main, wallet->cs_wallet);
    wallet->UnlockCoin(output);
}

void Bitcoin_WalletModel::listLockedCoins(std::vector<COutPoint>& vOutpts)
{
    LOCK2(bitcoin_mainState.cs_main, wallet->cs_wallet);
    wallet->ListLockedCoins(vOutpts);
}

void Bitcoin_WalletModel::loadReceiveRequests(std::vector<std::string>& vReceiveRequests)
{
    LOCK(wallet->cs_wallet);
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& item, wallet->mapAddressBook)
        BOOST_FOREACH(const PAIRTYPE(std::string, std::string)& item2, item.second.destdata)
            if (item2.first.size() > 2 && item2.first.substr(0,2) == "rr") // receive request
                vReceiveRequests.push_back(item2.second);
}

bool Bitcoin_WalletModel::saveReceiveRequest(const std::string &sAddress, const int64_t nId, const std::string &sRequest)
{
    CTxDestination dest = CBitcoinAddress(sAddress).Get();

    std::stringstream ss;
    ss << nId;
    std::string key = "rr" + ss.str(); // "rr" prefix = "receive request" in destdata

    LOCK(wallet->cs_wallet);
    if (sRequest.empty())
        return wallet->EraseDestData(dest, key);
    else
        return wallet->AddDestData(dest, key, sRequest);
}
