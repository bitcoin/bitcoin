#include "clientmodel.h"
#include "main.h"
#include "guiconstants.h"
#include "optionsmodel.h"

#include <QTimer>

ClientModel::ClientModel(QObject *parent) :
    QObject(parent), options_model(0)
{
    /* Until we build signal notifications into the bitcoin core,
       simply update everything using a timer.
    */
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(MODEL_UPDATE_DELAY);

    options_model = new OptionsModel(this);
}

qint64 ClientModel::getBalance()
{
    return GetBalance();
}

QString ClientModel::getAddress()
{
    std::vector<unsigned char> vchPubKey;
    if (CWalletDB("r").ReadDefaultKey(vchPubKey))
    {
        return QString::fromStdString(PubKeyToAddress(vchPubKey));
    } else {
        return QString();
    }
}

int ClientModel::getNumConnections()
{
    return vNodes.size();
}

int ClientModel::getNumBlocks()
{
    return nBestHeight;
}

int ClientModel::getNumTransactions()
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
    emit balanceChanged(getBalance());
    emit addressChanged(getAddress());
    emit numConnectionsChanged(getNumConnections());
    emit numBlocksChanged(getNumBlocks());
    emit numTransactionsChanged(getNumTransactions());
}

ClientModel::StatusCode ClientModel::sendCoins(const QString &payTo, qint64 payAmount)
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
            return OK;
        else if (strError == "ABORTED")
            return Aborted;
        else
        {
            emit error(tr("Sending..."), QString::fromStdString(strError));
            return MiscError;
        }
    }

    return OK;
}

OptionsModel *ClientModel::getOptionsModel()
{
    return options_model;
}
