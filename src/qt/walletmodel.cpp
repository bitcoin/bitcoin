// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletmodel.h"

#include "addresstablemodel.h"
#include "guiconstants.h"
#include "recentrequeststablemodel.h"
#include "transactiontablemodel.h"

#include "base58.h"
#include "db.h"
#include "keystore.h"
#include "main.h"
#include "sync.h"
#include "ui_interface.h"
#include "wallet.h"
#include "walletdb.h" // for BackupWallet
#include "txdb.h"
#include "bitcreditunits.h"

#include <stdint.h>

#include <QDebug>
#include <QSet>
#include <QTimer>

Bitcredit_WalletModel::Bitcredit_WalletModel(Credits_CWallet *wallet, Credits_CWallet *deposit_wallet, Bitcoin_CWallet *bitcoin_wallet, OptionsModel *optionsModel, Credits_CWallet *keyholder_wallet, bool isForDepositWallet, QObject *parent) :
    QObject(parent), wallet(wallet), deposit_wallet(deposit_wallet), bitcoin_wallet(bitcoin_wallet), optionsModel(optionsModel), addressTableModel(0),
    transactionTableModel(0),
    recentRequestsTableModel(0),
    cachedBalance(0), cachedUnconfirmedBalance(0), cachedImmatureBalance(0), cachedPreparedDepositBalance(0), cachedInDepositBalance(0),
    cachedNumTransactions(0),
    cachedEncryptionStatus(Unencrypted),
    cachedNumBlocks(0)
{
    addressTableModel = new Bitcredit_AddressTableModel(wallet, this);
    transactionTableModel = new Bitcredit_TransactionTableModel(wallet, keyholder_wallet, isForDepositWallet, this);
    recentRequestsTableModel = new Bitcredit_RecentRequestsTableModel(wallet, this);

    // This timer will be fired repeatedly to update the balance
    pollTimer = new QTimer(this);
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollBalanceChanged()));
    pollTimer->start(MODEL_UPDATE_DELAY);

    subscribeToCoreSignals();
}

Bitcredit_WalletModel::~Bitcredit_WalletModel()
{
    unsubscribeFromCoreSignals();
}

qint64 Bitcredit_WalletModel::getBalance(map<uint256, set<int> >& mapFilterTxInPoints, const CCoinControl *coinControl) const
{
	wallet->MarkDirty();

    if (coinControl)
    {
        qint64 nBalance = 0;
        std::vector<Credits_COutput> vCoins;
        wallet->AvailableCoins(vCoins, mapFilterTxInPoints, true, coinControl);
        BOOST_FOREACH(const Credits_COutput& out, vCoins)
            nBalance += out.tx->vout[out.i].nValue;

        return nBalance;
    }

    return wallet->GetBalance(mapFilterTxInPoints);
}

qint64 Bitcredit_WalletModel::getUnconfirmedBalance(map<uint256, set<int> >& mapFilterTxInPoints) const
{
	wallet->MarkDirty();
    return wallet->GetUnconfirmedBalance(mapFilterTxInPoints);
}

qint64 Bitcredit_WalletModel::getImmatureBalance(map<uint256, set<int> >& mapFilterTxInPoints) const
{
	wallet->MarkDirty();
    return wallet->GetImmatureBalance(mapFilterTxInPoints);
}
qint64 Bitcredit_WalletModel::getPreparedDepositBalance() const
{
	//NOTE! deposit_wallet referenced here
    return deposit_wallet->GetPreparedDepositBalance();
}
qint64 Bitcredit_WalletModel::getInDepositBalance() const
{
    return wallet->GetInDepositBalance();
}
int Bitcredit_WalletModel::getBlockHeight() const
{
    return credits_chainActive.Height();
}
qint64 Bitcredit_WalletModel::getTotalMonetaryBase() const
{
    uint64_t nTotalMonetaryBase = 0;
    if(credits_chainActive.Tip()) {
    	const Credits_CBlockIndex* ptip = (Credits_CBlockIndex*)credits_chainActive.Tip();
    	nTotalMonetaryBase = ptip->nTotalMonetaryBase;
    }
    return nTotalMonetaryBase;
}
qint64 Bitcredit_WalletModel::getTotalDepositBase() const
{
    uint64_t nTotalDepositBase = 0;
    if(credits_chainActive.Tip()) {
    	const Credits_CBlockIndex* ptip = (Credits_CBlockIndex*)credits_chainActive.Tip();
    	nTotalDepositBase = ptip->nTotalDepositBase;
    }
    return nTotalDepositBase;
}
int Bitcredit_WalletModel::getNextSubsidyUpdateHeight() const
{
	uint64_t monetaryBase = getTotalMonetaryBase();
	uint64_t maxBlockSubsidy = Bitcredit_GetMaxBlockSubsidy(monetaryBase);
	uint64_t nextMaxBlockSubsidy = maxBlockSubsidy;
	//50 000 just to prevent eternal loop
	unsigned int i = 1;
	for (; i < 50000; i++) {
		monetaryBase += Bitcredit_GetMaxBlockSubsidy(monetaryBase);
		nextMaxBlockSubsidy = Bitcredit_GetMaxBlockSubsidy(monetaryBase);
		if(maxBlockSubsidy != nextMaxBlockSubsidy) {
			break;
		}
	}
	return getBlockHeight() + i;
}
qint64 Bitcredit_WalletModel::getRequiredDepositLevel(unsigned int forwardBlocks) const
{
	uint64_t monetaryBase = getTotalMonetaryBase();
	uint64_t forwardSubsidies = 0;
	uint64_t depositBase = getTotalDepositBase();
	uint64_t reqDepositLevel = Bitcredit_GetRequiredDeposit(depositBase);
	for (unsigned int i = 0; i < forwardBlocks; ++i) {
		forwardSubsidies += Bitcredit_GetMaxBlockSubsidy(monetaryBase + forwardSubsidies);
		reqDepositLevel = Bitcredit_GetRequiredDeposit(depositBase + forwardSubsidies);
	}
	return reqDepositLevel;
}
qint64 Bitcredit_WalletModel::getMaxBlockSubsidy(unsigned int forwardBlocks) const
{
	uint64_t monetaryBase = getTotalMonetaryBase();
	uint64_t maxBlockSubsidy = Bitcredit_GetMaxBlockSubsidy(monetaryBase);
	for (unsigned int i = 0; i < forwardBlocks; ++i) {
		monetaryBase += Bitcredit_GetMaxBlockSubsidy(monetaryBase);
		maxBlockSubsidy = Bitcredit_GetMaxBlockSubsidy(monetaryBase);
	}
	return maxBlockSubsidy;
}

int Bitcredit_WalletModel::getNumTransactions() const
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

void Bitcredit_WalletModel::updateStatus()
{
    EncryptionStatus newEncryptionStatus = getEncryptionStatus();

    if(cachedEncryptionStatus != newEncryptionStatus)
        emit encryptionStatusChanged(newEncryptionStatus);
}

void Bitcredit_WalletModel::pollBalanceChanged()
{
    // Get required locks upfront. This avoids the GUI from getting stuck on
    // periodical polls if the core is holding the locks for a longer time -
    // for example, during a wallet rescan.
    TRY_LOCK(credits_mainState.cs_main, lockMain);
    if(!lockMain)
        return;
    TRY_LOCK(wallet->cs_wallet, lockWallet);
    if(!lockWallet)
        return;

    if(credits_chainActive.Height() != cachedNumBlocks)
    {
        // Balance and number of transactions might have changed
        cachedNumBlocks = credits_chainActive.Height();

        checkBalanceChanged();
        if(transactionTableModel)
            transactionTableModel->updateConfirmations();

        checkMinerStatisticsChanged();
    }
}

void Bitcredit_WalletModel::checkBalanceChanged()
{
    //Find all prepared deposit transactions
    map<uint256, set<int> > mapPreparedDepositTxInPoints;
    deposit_wallet->PreparedDepositTxInPoints(mapPreparedDepositTxInPoints);

    qint64 newBalance = getBalance(mapPreparedDepositTxInPoints);
    qint64 newUnconfirmedBalance = getUnconfirmedBalance(mapPreparedDepositTxInPoints);
    qint64 newImmatureBalance = getImmatureBalance(mapPreparedDepositTxInPoints);
    qint64 newPreparedDepositBalance = getPreparedDepositBalance();
    qint64 newInDepositBalance = getInDepositBalance();

    if(cachedBalance != newBalance || cachedUnconfirmedBalance != newUnconfirmedBalance || cachedImmatureBalance != newImmatureBalance || cachedPreparedDepositBalance != newPreparedDepositBalance || cachedInDepositBalance != newInDepositBalance)
    {
        cachedBalance = newBalance;
        cachedUnconfirmedBalance = newUnconfirmedBalance;
        cachedImmatureBalance = newImmatureBalance;
        cachedPreparedDepositBalance = newPreparedDepositBalance;
        cachedInDepositBalance = newInDepositBalance;
        emit balanceChanged(newBalance, newUnconfirmedBalance, newImmatureBalance, newPreparedDepositBalance, newInDepositBalance);
    }
}

void Bitcredit_WalletModel::checkMinerStatisticsChanged()
{
	const qint64 newReqDeposit1 = getRequiredDepositLevel(1);
	const qint64 newReqDeposit2 = getRequiredDepositLevel(2);
	const qint64 newReqDeposit3 = getRequiredDepositLevel(101);
	const qint64 newReqDeposit4 = getRequiredDepositLevel(1001);
	const qint64 newReqDeposit5 = getRequiredDepositLevel(10001);

	const qint64 newMaxBlockSubsidy1 = getMaxBlockSubsidy(1);
	const qint64 newMaxBlockSubsidy2 = getMaxBlockSubsidy(2);
	const qint64 newMaxBlockSubsidy3 = getMaxBlockSubsidy(101);
	const qint64 newMaxBlockSubsidy4 = getMaxBlockSubsidy(1001);
	const qint64 newMaxBlockSubsidy5 = getMaxBlockSubsidy(10001);

	const int newBlockHeight = getBlockHeight();
	const qint64 newTotalMonetaryBase = getTotalMonetaryBase();
	const qint64 newTotalDepositBase = getTotalDepositBase();
	const qint64 newNextSubsidyUpdateHeight = getNextSubsidyUpdateHeight();

    if(cachedReqDeposit1 != newReqDeposit1 ||
    	cachedReqDeposit2 != newReqDeposit2 ||
    	cachedReqDeposit3 != newReqDeposit3 ||
    	cachedReqDeposit4 != newReqDeposit4 ||
    	cachedReqDeposit5 != newReqDeposit5 ||
    	cachedMaxBlockSubsidy1 != newMaxBlockSubsidy1 ||
    	cachedMaxBlockSubsidy2 != newMaxBlockSubsidy2 ||
    	cachedMaxBlockSubsidy3 != newMaxBlockSubsidy3 ||
    	cachedMaxBlockSubsidy4 != newMaxBlockSubsidy4 ||
    	cachedMaxBlockSubsidy5 != newMaxBlockSubsidy5 ||
    	cachedBlockHeight != newBlockHeight ||
    	cachedTotalMonetaryBase != newTotalMonetaryBase ||
    	cachedTotalDepositBase != newTotalDepositBase ||
    	cachedNextSubsidyUpdateHeight != newNextSubsidyUpdateHeight)
    {
    	cachedReqDeposit1 = newReqDeposit1;
    	cachedReqDeposit2 = newReqDeposit2;
    	cachedReqDeposit3 = newReqDeposit3;
    	cachedReqDeposit4 = newReqDeposit4;
    	cachedReqDeposit5 = newReqDeposit5;
    	cachedMaxBlockSubsidy1 = newMaxBlockSubsidy1;
    	cachedMaxBlockSubsidy2 = newMaxBlockSubsidy2;
    	cachedMaxBlockSubsidy3 = newMaxBlockSubsidy3;
    	cachedMaxBlockSubsidy4 = newMaxBlockSubsidy4;
    	cachedMaxBlockSubsidy5 = newMaxBlockSubsidy5;
    	cachedBlockHeight = newBlockHeight;
    	cachedTotalMonetaryBase = newTotalMonetaryBase;
    	cachedTotalDepositBase = newTotalDepositBase;
    	cachedNextSubsidyUpdateHeight = newNextSubsidyUpdateHeight;
        emit minerStatisticsChanged(newReqDeposit1,
        													newReqDeposit2,
        													newReqDeposit3,
        													newReqDeposit4,
        													newReqDeposit5,
        													newMaxBlockSubsidy1,
        													newMaxBlockSubsidy2,
        													newMaxBlockSubsidy3,
        													newMaxBlockSubsidy4,
        													newMaxBlockSubsidy5,
        													newBlockHeight,
        													newTotalMonetaryBase,
        													newTotalDepositBase,
        													newNextSubsidyUpdateHeight);
    }
}

void Bitcredit_WalletModel::updateTransaction(const QString &hash, int status)
{
    if(transactionTableModel)
        transactionTableModel->updateTransaction(hash, status);

    // Balance and number of transactions might have changed
    checkBalanceChanged();

    int newNumTransactions = getNumTransactions();
    if(cachedNumTransactions != newNumTransactions)
    {
        cachedNumTransactions = newNumTransactions;
        emit numTransactionsChanged(newNumTransactions);
    }
}

void Bitcredit_WalletModel::updateAddressBook(const QString &address, const QString &label,
        bool isMine, const QString &purpose, int status)
{
    if(addressTableModel)
        addressTableModel->updateEntry(address, label, isMine, purpose, status);
}

bool Bitcredit_WalletModel::validateAddress(const QString &address)
{
    CBitcoinAddress addressParsed(address.toStdString());
    return addressParsed.IsValid();
}

Bitcredit_WalletModel::SendCoinsReturn Bitcredit_WalletModel::prepareTransaction(Bitcredit_WalletModel *deposit_model, Bitcredit_WalletModelTransaction &transaction, const CCoinControl *coinControl)
{
    qint64 total = 0;
    QList<Bitcredit_SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<std::pair<CScript, int64_t> > vecSend;

    if(recipients.empty())
    {
        return OK;
    }

    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    // Pre-check input data for validity
    foreach(const Bitcredit_SendCoinsRecipient &rcp, recipients)
    {
        if (rcp.paymentRequest.IsInitialized())
        {   // PaymentRequest...
            int64_t subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int i = 0; i < details.outputs_size(); i++)
            {
                const payments::Output& out = details.outputs(i);
                if (out.amount() <= 0) continue;
                subtotal += out.amount();
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr+out.script().size());
                vecSend.push_back(std::pair<CScript, int64_t>(scriptPubKey, out.amount()));
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

            CScript scriptPubKey;
            scriptPubKey.SetDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
            vecSend.push_back(std::pair<CScript, int64_t>(scriptPubKey, rcp.amount));

            total += rcp.amount;
        }
    }
    if(setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }

    //Find all prepared deposit transactions
    map<uint256, set<int> > mapPreparedDepositTxInPoints;
    deposit_model->wallet->PreparedDepositTxInPoints(mapPreparedDepositTxInPoints);

    qint64 nBalance = getBalance(mapPreparedDepositTxInPoints, coinControl);

    if(total > nBalance)
    {
        return AmountExceedsBalance;
    }

    if((total + credits_nTransactionFee) > nBalance)
    {
        transaction.setTransactionFee(credits_nTransactionFee);
        return SendCoinsReturn(AmountWithFeeExceedsBalance);
    }

    {
        LOCK2(credits_mainState.cs_main, wallet->cs_wallet);

        transaction.newPossibleKeyChange(wallet);
        int64_t nFeeRequired = 0;
        std::string strFailReason;

        Credits_CWalletTx *newTx = transaction.getTransaction();
        Credits_CReserveKey *keyChange = transaction.getPossibleKeyChange();
        bool fCreated = wallet->CreateTransaction(deposit_model->wallet, vecSend, *newTx, *keyChange, nFeeRequired, strFailReason, coinControl);
        transaction.setTransactionFee(nFeeRequired);

        if(!fCreated)
        {
            if((total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }
            emit message(tr("Send Coins"), QString::fromStdString(strFailReason),
                         CClientUIInterface::MSG_ERROR);
            return TransactionCreationFailed;
        }
    }

    return SendCoinsReturn(OK);
}

Bitcredit_WalletModel::SendCoinsReturn Bitcredit_WalletModel::prepareDepositTransaction(Bitcredit_WalletModel *deposit_model, Bitcredit_WalletModelTransaction &transaction, const Credits_COutput& coin, Credits_CCoinsViewCache &credits_view)
{
    QList<Bitcredit_SendCoinsRecipient> recipients = transaction.getRecipients();
    if(recipients.size() != 1) {
		LogPrintf("TOO MANY RECIPIENT ADRESSES\n");
		return InvalidAddress;
    }

    Bitcredit_SendCoinsRecipient& rcp = recipients.back();
    // Pre-check input data for validity
	{
		//Reserve new addresses for each output
		LOCK2(credits_mainState.cs_main, wallet->cs_wallet);

		transaction.newKeyRecipient(wallet);
		CPubKey pubKey;
		transaction.getKeyRecipients().back()->GetReservedKey(pubKey);
		rcp.address = QString::fromStdString(CBitcoinAddress(pubKey.GetID()).ToString());
	}

	// User-entered bitcoin address / amount:
	if(!validateAddress(rcp.address))
	{
		LogPrintf("INVALID RECIPIENT ADRESS: %s\n", rcp.address.toStdString().c_str());
		return InvalidAddress;
	}
	if(rcp.amount <= 0)
	{
		return InvalidAmount;
	}

	CScript scriptPubKey;
	scriptPubKey.SetDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
    std::pair<CScript, int64_t> send(scriptPubKey, rcp.amount);

    qint64 nBalance = coin.tx->vout[coin.i].nValue;
    if(rcp.amount != nBalance)
    {
        return AmountExceedsBalance;
    }

    {
        LOCK2(credits_mainState.cs_main, wallet->cs_wallet);

        transaction.newKeyDepositSignature(deposit_wallet);
        //int64_t nFeeRequired = 0;
        std::string strFailReason;

        Credits_CWalletTx *newTx = transaction.getTransaction();
        Credits_CReserveKey *keyDepositSignature = transaction.getKeyDepositSignature();
        bool fCreated = wallet->CreateDepositTransaction(deposit_model->wallet, send, *newTx, *keyDepositSignature, rcp.amount, strFailReason, coin, credits_view);
        //transaction.setTransactionFee(nFeeRequired);

        //Basic checks to make sure nothing has gone wrong in deposit creation
        assert(transaction.getTransactionFee() == 0);

        if(!fCreated)
        {
            if(rcp.amount > nBalance)
            {
                return SendCoinsReturn(AmountExceedsBalance);
            }
            emit message(tr("Claim bitcoins"), QString::fromStdString(strFailReason),
                         CClientUIInterface::MSG_ERROR);
            return TransactionCreationFailed;
        }
    }

    return SendCoinsReturn(OK);
}

Bitcredit_WalletModel::SendCoinsReturn Bitcredit_WalletModel::prepareClaimTransaction(Bitcoin_WalletModel *bitcoin_model, Credits_CCoinsViewCache *claim_view, Bitcredit_WalletModelTransaction &transaction, const CCoinControl *coinControl)
{
    qint64 total = 0;
    QList<Bitcredit_SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<std::pair<CScript, int64_t> > vecSend;

    if(recipients.empty())
    {
        return OK;
    }

    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    //Fetch new coin addresses for each output
    // Pre-check input data for validity
    BOOST_FOREACH(Bitcredit_SendCoinsRecipient &rcp, recipients)
    {
        {
        	//Reserve new addresses for each output
            LOCK2(credits_mainState.cs_main, wallet->cs_wallet);

            transaction.newKeyRecipient(wallet);
            CPubKey pubKey;
            transaction.getKeyRecipients().back()->GetReservedKey(pubKey);
            rcp.address = QString::fromStdString(CBitcoinAddress(pubKey.GetID()).ToString());
        }

		if(!validateAddress(rcp.address))
		{
			LogPrintf("INVALID RECIPIENT ADRESS: %s\n", rcp.address.toStdString().c_str());
			return InvalidAddress;
		}
		if(rcp.amount <= 0)
		{
			return InvalidAmount;
		}
		setAddress.insert(rcp.address);
		++nAddresses;

		CScript scriptPubKey;
		scriptPubKey.SetDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
		vecSend.push_back(std::pair<CScript, int64_t>(scriptPubKey, rcp.amount));

		total += rcp.amount;
    }
    if(setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }

    //Find all claimed  transactions
    map<uint256, set<int> > mapClaimTxInPoints;
    wallet->ClaimTxInPoints(mapClaimTxInPoints);

    qint64 nBalance = bitcoin_model->getBalance(claim_view, mapClaimTxInPoints, coinControl);

    if(total > nBalance)
    {
        return AmountExceedsBalance;
    }

    if((total + credits_nTransactionFee) > nBalance)
    {
        transaction.setTransactionFee(credits_nTransactionFee);
        return SendCoinsReturn(AmountWithFeeExceedsBalance);
    }

    {
        LOCK2(credits_mainState.cs_main, wallet->cs_wallet);

        transaction.newPossibleKeyChange(wallet);
        int64_t nFeeRequired = 0;
        std::string strFailReason;

        Credits_CWalletTx *newTx = transaction.getTransaction();
        Credits_CReserveKey *keyChange = transaction.getPossibleKeyChange();
        bool fCreated = wallet->CreateClaimTransaction(bitcoin_model->wallet, claim_view, vecSend, *newTx, *keyChange, nFeeRequired, strFailReason, coinControl);
        transaction.setTransactionFee(nFeeRequired);

        if(!fCreated)
        {

            if((total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }
            emit message(tr("Claim Coins"), QString::fromStdString(strFailReason),
                         CClientUIInterface::MSG_ERROR);
            return TransactionCreationFailed;
        }
    }

    return SendCoinsReturn(OK);
}

Bitcredit_WalletModel::SendCoinsReturn Bitcredit_WalletModel::sendCoins(Bitcredit_WalletModelTransaction &transaction)
{
    QByteArray transaction_array; /* store serialized transaction */

    {
        LOCK2(credits_mainState.cs_main, wallet->cs_wallet);
        Credits_CWalletTx *newTx = transaction.getTransaction();

        // Store PaymentRequests in wtx.vOrderForm in wallet.
        foreach(const Bitcredit_SendCoinsRecipient &rcp, transaction.getRecipients())
        {
            if (rcp.paymentRequest.IsInitialized())
            {
                std::string key("PaymentRequest");
                std::string value;
                rcp.paymentRequest.SerializeToString(&value);
                newTx->vOrderForm.push_back(make_pair(key, value));
            }
            else if (!rcp.message.isEmpty()) // Message from normal bitcredit:URI (bitcredit:123...?message=example)
                newTx->vOrderForm.push_back(make_pair("Message", rcp.message.toStdString()));
        }

        Credits_CReserveKey *keyChange = transaction.getPossibleKeyChange();
        if(!wallet->CommitTransaction(bitcoin_wallet, *newTx, *keyChange, transaction.getKeyRecipients()))
            return TransactionCommitFailed;

        Credits_CTransaction* t = (Credits_CTransaction*)newTx;
        CDataStream ssTx(SER_NETWORK, CREDITS_PROTOCOL_VERSION);
        ssTx << *t;
        transaction_array.append(&(ssTx[0]), ssTx.size());
    }

    // Add addresses / update labels that we've sent to to the address book,
    // and emit coinsSent signal for each recipient
    foreach(const Bitcredit_SendCoinsRecipient &rcp, transaction.getRecipients())
    {
        // Don't touch the address book when we have a payment request
        if (!rcp.paymentRequest.IsInitialized())
        {
            std::string strAddress = rcp.address.toStdString();
            CTxDestination dest = CBitcoinAddress(strAddress).Get();
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
        emit coinsSent(wallet, rcp, transaction_array);
    }

    return SendCoinsReturn(OK);
}

Bitcredit_WalletModel::SendCoinsReturn Bitcredit_WalletModel::storeDepositTransaction(Bitcredit_WalletModel *bitcredit_model, Bitcredit_WalletModelTransaction &transaction)
{
//    QByteArray transaction_array; /* store serialized transaction */

    {
        LOCK2(credits_mainState.cs_main, wallet->cs_wallet);

        //Find all existing prepared deposits
        map<uint256, set<int> > mapDepositTxIns;
        wallet->PreparedDepositTxInPoints(mapDepositTxIns);

        Credits_CWalletTx *newTx = transaction.getTransaction();
        if(IsAnyTxInInFilterPoints(*newTx, mapDepositTxIns)) {
            return TransactionDoubleSpending;
        }

//        // Store PaymentRequests in wtx.vOrderForm in wallet.
//        foreach(const Bitcredit_SendCoinsRecipient &rcp, transaction.getRecipients())
//        {
//            if (!rcp.message.isEmpty()) // Message from normal bitcredit:URI (bitcredit:123...?message=example)
//                newTx->vOrderForm.push_back(make_pair("Message", rcp.message.toStdString()));
//        }

        Credits_CReserveKey *keyDepositSignature = transaction.getKeyDepositSignature();
        if(!wallet->CommitDepositTransaction(bitcredit_model->wallet, *newTx, *keyDepositSignature, transaction.getKeyRecipients()))
            return TransactionCommitFailed;

        LogPrintf("Wrote deposit tx to wallet \n%s\n", newTx->ToString());
    }

    // Add addresses / update labels that we've sent to to the address book,
    // and emit coinsSent signal for each recipient
//    foreach(const Bitcredit_SendCoinsRecipient &rcp, transaction.getRecipients())
//    {
//		std::string strAddress = rcp.address.toStdString();
//		CTxDestination dest = CBitcoinAddress(strAddress).Get();
//		std::string strLabel = rcp.label.toStdString();
//		{
//			LOCK(wallet->cs_wallet);
//
//			std::map<CTxDestination, CAddressBookData>::iterator mi = wallet->mapAddressBook.find(dest);
//
//			// Check if we have a new address or an updated label
//			if (mi == wallet->mapAddressBook.end())
//			{
//				wallet->SetAddressBook(dest, strLabel, "send");
//			}
//			else if (mi->second.name != strLabel)
//			{
//				wallet->SetAddressBook(dest, strLabel, ""); // "" means don't change purpose
//			}
//		}
//        emit coinsSent(wallet, rcp, transaction_array);
//    }

    return SendCoinsReturn(OK);
}

OptionsModel *Bitcredit_WalletModel::getOptionsModel()
{
    return optionsModel;
}

Bitcredit_AddressTableModel *Bitcredit_WalletModel::getAddressTableModel()
{
    return addressTableModel;
}

Bitcredit_TransactionTableModel *Bitcredit_WalletModel::getTransactionTableModel()
{
    return transactionTableModel;
}

Bitcredit_RecentRequestsTableModel *Bitcredit_WalletModel::getRecentRequestsTableModel()
{
    return recentRequestsTableModel;
}

Bitcredit_WalletModel::EncryptionStatus Bitcredit_WalletModel::getEncryptionStatus() const
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

bool Bitcredit_WalletModel::setWalletEncrypted(bool encrypted, const SecureString &passphrase)
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

bool Bitcredit_WalletModel::setWalletLocked(bool locked, const SecureString &passPhrase)
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

bool Bitcredit_WalletModel::changePassphrase(const SecureString &oldPass, const SecureString &newPass)
{
    bool retval;
    {
        LOCK(wallet->cs_wallet);
        wallet->Lock(); // Make sure wallet is locked before attempting pass change
        retval = wallet->ChangeWalletPassphrase(oldPass, newPass);
    }
    return retval;
}

bool Bitcredit_WalletModel::backupWallet(const QString &filename)
{
    return Bitcredit_BackupWallet(*wallet, filename.toLocal8Bit().data());
}

// Handlers for core signals
static void NotifyKeyStoreStatusChanged(Bitcredit_WalletModel *walletmodel, CCryptoKeyStore *wallet)
{
    qDebug() << "NotifyKeyStoreStatusChanged";
    QMetaObject::invokeMethod(walletmodel, "updateStatus", Qt::QueuedConnection);
}

static void NotifyAddressBookChanged(Bitcredit_WalletModel *walletmodel, Credits_CWallet *wallet,
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
static void NotifyTransactionChanged(Bitcredit_WalletModel *walletmodel, Credits_CWallet *wallet, const uint256 &hash, ChangeType status)
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

static void ShowProgress(Bitcredit_WalletModel *walletmodel, const std::string &title, int nProgress)
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

void Bitcredit_WalletModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    wallet->NotifyStatusChanged.connect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.connect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.connect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
}

void Bitcredit_WalletModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    wallet->NotifyStatusChanged.disconnect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.disconnect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.disconnect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
}

// Bitcredit_WalletModel::UnlockContext implementation
Bitcredit_WalletModel::UnlockContext Bitcredit_WalletModel::requestUnlock()
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

Bitcredit_WalletModel::UnlockContext::UnlockContext(Bitcredit_WalletModel *wallet, bool valid, bool relock):
        wallet(wallet),
        valid(valid),
        relock(relock)
{
}

Bitcredit_WalletModel::UnlockContext::~UnlockContext()
{
    if(valid && relock)
    {
        wallet->setWalletLocked(true);
    }
}

void Bitcredit_WalletModel::UnlockContext::CopyFrom(const UnlockContext& rhs)
{
    // Transfer context; old object no longer relocks wallet
    *this = rhs;
    rhs.relock = false;
}

bool Bitcredit_WalletModel::getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    return wallet->GetPubKey(address, vchPubKeyOut);
}

// returns a list of COutputs from COutPoints
void Bitcredit_WalletModel::getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<Credits_COutput>& vOutputs)
{
    LOCK2(credits_mainState.cs_main, wallet->cs_wallet);
    BOOST_FOREACH(const COutPoint& outpoint, vOutpoints)
    {
        if (!wallet->mapWallet.count(outpoint.hash)) continue;
        int nDepth = wallet->mapWallet[outpoint.hash].GetDepthInMainChain();
        if (nDepth < 0) continue;
        Credits_COutput out(&wallet->mapWallet[outpoint.hash], outpoint.n, nDepth);
        vOutputs.push_back(out);
    }
}

bool Bitcredit_WalletModel::isSpent(const COutPoint& outpoint) const
{
    LOCK2(credits_mainState.cs_main, wallet->cs_wallet);
    return wallet->IsSpent(outpoint.hash, outpoint.n);
}

// AvailableCoins + LockedCoins grouped by wallet address (put change in one group with wallet address)
void Bitcredit_WalletModel::listCoins(std::map<QString, std::vector<Credits_COutput> >& mapCoins) const
{
	wallet->MarkDirty();

    //Find all prepared deposit transactions
    map<uint256, set<int> > mapPreparedDepositTxInPoints;
    deposit_wallet->PreparedDepositTxInPoints(mapPreparedDepositTxInPoints);

    std::vector<Credits_COutput> vCoins;
    wallet->AvailableCoins(vCoins, mapPreparedDepositTxInPoints);

    LOCK2(credits_mainState.cs_main, wallet->cs_wallet); // ListLockedCoins, mapWallet
    std::vector<COutPoint> vLockedCoins;
    wallet->ListLockedCoins(vLockedCoins);

    // add locked coins
    BOOST_FOREACH(const COutPoint& outpoint, vLockedCoins)
    {
        if (!wallet->mapWallet.count(outpoint.hash)) continue;
        int nDepth = wallet->mapWallet[outpoint.hash].GetDepthInMainChain();
        if (nDepth < 0) continue;
        Credits_COutput out(&wallet->mapWallet[outpoint.hash], outpoint.n, nDepth);
        vCoins.push_back(out);
    }

    BOOST_FOREACH(const Credits_COutput& out, vCoins)
    {
        Credits_COutput cout = out;

        while (wallet->IsChange(cout.tx->vout[cout.i]) && cout.tx->vin.size() > 0 && wallet->IsMine(cout.tx->vin[0]))
        {
            if (!wallet->mapWallet.count(cout.tx->vin[0].prevout.hash)) break;
            cout = Credits_COutput(&wallet->mapWallet[cout.tx->vin[0].prevout.hash], cout.tx->vin[0].prevout.n, 0);
        }

        CTxDestination address;
        if(!ExtractDestination(cout.tx->vout[cout.i].scriptPubKey, address)) continue;
        mapCoins[CBitcoinAddress(address).ToString().c_str()].push_back(out);
    }
}

bool Bitcredit_WalletModel::isLockedCoin(uint256 hash, unsigned int n) const
{
    LOCK2(credits_mainState.cs_main, wallet->cs_wallet);
    return wallet->IsLockedCoin(hash, n);
}

void Bitcredit_WalletModel::lockCoin(COutPoint& output)
{
    LOCK2(credits_mainState.cs_main, wallet->cs_wallet);
    wallet->LockCoin(output);
}

void Bitcredit_WalletModel::unlockCoin(COutPoint& output)
{
    LOCK2(credits_mainState.cs_main, wallet->cs_wallet);
    wallet->UnlockCoin(output);
}

void Bitcredit_WalletModel::listLockedCoins(std::vector<COutPoint>& vOutpts)
{
    LOCK2(credits_mainState.cs_main, wallet->cs_wallet);
    wallet->ListLockedCoins(vOutpts);
}

void Bitcredit_WalletModel::loadReceiveRequests(std::vector<std::string>& vReceiveRequests)
{
    LOCK(wallet->cs_wallet);
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& item, wallet->mapAddressBook)
        BOOST_FOREACH(const PAIRTYPE(std::string, std::string)& item2, item.second.destdata)
            if (item2.first.size() > 2 && item2.first.substr(0,2) == "rr") // receive request
                vReceiveRequests.push_back(item2.second);
}

bool Bitcredit_WalletModel::saveReceiveRequest(const std::string &sAddress, const int64_t nId, const std::string &sRequest)
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
