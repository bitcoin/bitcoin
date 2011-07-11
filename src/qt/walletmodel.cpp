#include "walletmodel.h"
#include "guiconstants.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "transactiontablemodel.h"

#include "headers.h"

#include <QTimer>

WalletModel::WalletModel(CWallet *wallet, QObject *parent) :
    QObject(parent), wallet(wallet), optionsModel(0), addressTableModel(0),
    transactionTableModel(0)
{
    // Until signal notifications is built into the bitcoin core,
    //  simply update everything after polling using a timer.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(MODEL_UPDATE_DELAY);

    optionsModel = new OptionsModel(wallet, this);
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
    CRITICAL_BLOCK(wallet->cs_mapWallet)
    {
        numTransactions = wallet->mapWallet.size();
    }
    return numTransactions;
}

void WalletModel::update()
{
    // Plainly emit all signals for now. To be more efficient this should check
    //   whether the values actually changed first, although it'd be even better if these
    //   were events coming in from the bitcoin core.
    emit balanceChanged(getBalance());
    emit numTransactionsChanged(getNumTransactions());

    addressTableModel->update();
}

WalletModel::StatusCode WalletModel::sendCoins(const QString &payTo, qint64 payAmount, const QString &addToAddressBookAs)
{
    uint160 hash160 = 0;
    bool valid = false;

    if(!AddressToHash160(payTo.toUtf8().constData(), hash160))
    {
        return InvalidAddress;
    }

    if(payAmount <= 0)
    {
        return InvalidAmount;
    }

    if(payAmount > getBalance())
    {
        return AmountExceedsBalance;
    }

    if((payAmount + nTransactionFee) > getBalance())
    {
        return AmountWithFeeExceedsBalance;
    }

    CRITICAL_BLOCK(cs_main)
    {
        // Send to bitcoin address
        CWalletTx wtx;
        CScript scriptPubKey;
        scriptPubKey << OP_DUP << OP_HASH160 << hash160 << OP_EQUALVERIFY << OP_CHECKSIG;

        std::string strError = wallet->SendMoney(scriptPubKey, payAmount, wtx, true);
        if (strError == "")
        {
            // OK
        }
        else if (strError == "ABORTED")
        {
            return Aborted;
        }
        else
        {
            emit error(tr("Sending..."), QString::fromStdString(strError));
            return MiscError;
        }
    }

    // Add addresses that we've sent to to the address book
    std::string strAddress = payTo.toStdString();
    CRITICAL_BLOCK(wallet->cs_mapAddressBook)
    {
        if (!wallet->mapAddressBook.count(strAddress))
            wallet->SetAddressBookName(strAddress, addToAddressBookAs.toStdString());
    }

    return OK;
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


