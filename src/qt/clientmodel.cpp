#include "clientmodel.h"
#include "main.h"
#include "guiconstants.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "transactiontablemodel.h"

#include <QTimer>

ClientModel::ClientModel(QObject *parent) :
    QObject(parent), optionsModel(0), addressTableModel(0),
    transactionTableModel(0)
{
    // Until signal notifications is built into the bitcoin core,
    //  simply update everything after polling using a timer.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(MODEL_UPDATE_DELAY);

    optionsModel = new OptionsModel(this);
    addressTableModel = new AddressTableModel(this);
    transactionTableModel = new TransactionTableModel(this);
}

qint64 ClientModel::getBalance() const
{
    return GetBalance();
}

int ClientModel::getNumConnections() const
{
    return vNodes.size();
}

int ClientModel::getNumBlocks() const
{
    return nBestHeight;
}

int ClientModel::getNumTransactions() const
{
    int numTransactions = 0;
    CRITICAL_BLOCK(cs_mapWallet)
    {
        numTransactions = mapWallet.size();
    }
    return numTransactions;
}

void ClientModel::update()
{
    // Plainly emit all signals for now. To be more efficient this should check
    //   whether the values actually changed first, although it'd be even better if these
    //   were events coming in from the bitcoin core.
    emit balanceChanged(getBalance());
    emit numConnectionsChanged(getNumConnections());
    emit numBlocksChanged(getNumBlocks());
    emit numTransactionsChanged(getNumTransactions());

    addressTableModel->update();
}

ClientModel::StatusCode ClientModel::sendCoins(const QString &payTo, qint64 payAmount, const QString &addToAddressBookAs)
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

        std::string strError = SendMoney(scriptPubKey, payAmount, wtx, true);
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
    CRITICAL_BLOCK(cs_mapAddressBook)
        if (!mapAddressBook.count(strAddress))
            SetAddressBookName(strAddress, addToAddressBookAs.toStdString());

    return OK;
}

bool ClientModel::inInitialBlockDownload() const
{
    return IsInitialBlockDownload();
}

int ClientModel::getTotalBlocksEstimate() const
{
    return GetTotalBlocksEstimate();
}


OptionsModel *ClientModel::getOptionsModel()
{
    return optionsModel;
}

AddressTableModel *ClientModel::getAddressTableModel()
{
    return addressTableModel;
}

TransactionTableModel *ClientModel::getTransactionTableModel()
{
    return transactionTableModel;
}
