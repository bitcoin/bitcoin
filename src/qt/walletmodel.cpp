#include "walletmodel.h"
#include "guiconstants.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "transactiontablemodel.h"

#include "headers.h"

#include <QTimer>
#include <QSet>

WalletModel::WalletModel(CWallet *wallet, OptionsModel *optionsModel, QObject *parent) :
    QObject(parent), wallet(wallet), optionsModel(optionsModel), addressTableModel(0),
    transactionTableModel(0),
    cachedBalance(0), cachedUnconfirmedBalance(0), cachedNumTransactions(0),
    cachedEncryptionStatus(Unencrypted)
{
    // Until signal notifications is built into the bitcoin core,
    //  simply update everything after polling using a timer.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(MODEL_UPDATE_DELAY);

    addressTableModel = new AddressTableModel(wallet, this);
    transactionTableModel = new TransactionTableModel(wallet, this);
}

qint64 WalletModel::getBalance() const
{
    return wallet->GetBalance();
}

qint64 WalletModel::getUnconfirmedBalance() const
{
    return wallet->GetUnconfirmedBalance();
}

int WalletModel::getNumTransactions() const
{
    int numTransactions = 0;
    CRITICAL_BLOCK(wallet->cs_wallet)
    {
        numTransactions = wallet->mapWallet.size();
    }
    return numTransactions;
}

void WalletModel::update()
{
    qint64 newBalance = getBalance();
    qint64 newUnconfirmedBalance = getUnconfirmedBalance();
    int newNumTransactions = getNumTransactions();
    EncryptionStatus newEncryptionStatus = getEncryptionStatus();

    if(cachedBalance != newBalance || cachedUnconfirmedBalance != newUnconfirmedBalance)
        emit balanceChanged(newBalance, newUnconfirmedBalance);

    if(cachedNumTransactions != newNumTransactions)
        emit numTransactionsChanged(newNumTransactions);

    if(cachedEncryptionStatus != newEncryptionStatus)
        emit encryptionStatusChanged(newEncryptionStatus);

    cachedBalance = newBalance;
    cachedUnconfirmedBalance = newUnconfirmedBalance;
    cachedNumTransactions = newNumTransactions;

    addressTableModel->update();
}

bool WalletModel::validateAddress(const QString &address)
{
    CBitcoinAddress addressParsed(address.toStdString());
    return addressParsed.IsValid();
}

WalletModel::SendCoinsReturn WalletModel::sendCoins(const QList<SendCoinsRecipient> &recipients)
{
    qint64 total = 0;
    QSet<QString> setAddress;
    QString hex;

    if(recipients.empty())
    {
        return OK;
    }

    // Pre-check input data for validity
    foreach(const SendCoinsRecipient &rcp, recipients)
    {
        if(!validateAddress(rcp.address))
        {
            return InvalidAddress;
        }
        setAddress.insert(rcp.address);

        if(rcp.amount <= 0)
        {
            return InvalidAmount;
        }
        total += rcp.amount;
    }

    if(recipients.size() > setAddress.size())
    {
        return DuplicateAddress;
    }

    if(total > getBalance())
    {
        return AmountExceedsBalance;
    }

    if((total + nTransactionFee) > getBalance())
    {
        return SendCoinsReturn(AmountWithFeeExceedsBalance, nTransactionFee);
    }

    CRITICAL_BLOCK(cs_main)
    CRITICAL_BLOCK(wallet->cs_wallet)
    {
        // Sendmany
        std::vector<std::pair<CScript, int64> > vecSend;
        foreach(const SendCoinsRecipient &rcp, recipients)
        {
            CScript scriptPubKey;
            scriptPubKey.SetBitcoinAddress(rcp.address.toStdString());
            vecSend.push_back(make_pair(scriptPubKey, rcp.amount));
        }

        CWalletTx wtx;
        CReserveKey keyChange(wallet);
        int64 nFeeRequired = 0;
        bool fCreated = wallet->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired);

        if(!fCreated)
        {
            if((total + nFeeRequired) > wallet->GetBalance())
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance, nFeeRequired);
            }
            return TransactionCreationFailed;
        }
        if(!ThreadSafeAskFee(nFeeRequired, tr("Sending...").toStdString(), NULL))
        {
            return Aborted;
        }
        if(!wallet->CommitTransaction(wtx, keyChange))
        {
            return TransactionCommitFailed;
        }
        hex = QString::fromStdString(wtx.GetHash().GetHex());
    }

    // Add addresses that we've sent to to the address book
    foreach(const SendCoinsRecipient &rcp, recipients)
    {
        std::string strAddress = rcp.address.toStdString();
        CRITICAL_BLOCK(wallet->cs_wallet)
        {
            if (!wallet->mapAddressBook.count(strAddress))
                wallet->SetAddressBookName(strAddress, rcp.label.toStdString());
        }
    }

    // Update our model of the address table
    addressTableModel->updateList();

    return SendCoinsReturn(OK, 0, hex);
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

bool WalletModel::setWalletEncrypted(bool encrypted, const std::string &passphrase)
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

bool WalletModel::setWalletLocked(bool locked, const std::string &passPhrase)
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

bool WalletModel::changePassphrase(const std::string &oldPass, const std::string &newPass)
{
    bool retval;
    CRITICAL_BLOCK(wallet->cs_wallet)
    {
        wallet->Lock(); // Make sure wallet is locked before attempting pass change
        retval = wallet->ChangeWalletPassphrase(oldPass, newPass);
    }
    return retval;
}

// WalletModel::UnlockContext implementation
WalletModel::UnlockContext WalletModel::requestUnlock()
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

WalletModel::UnlockContext::UnlockContext(WalletModel *wallet, bool valid, bool relock):
        wallet(wallet),
        valid(valid),
        relock(relock)
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
