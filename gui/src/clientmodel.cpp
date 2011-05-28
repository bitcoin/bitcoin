#include "clientmodel.h"
#include "main.h"
#include "guiconstants.h"

#include <QTimer>

ClientModel::ClientModel(QObject *parent) :
    QObject(parent)
{
    /* Until we build signal notifications into the bitcoin core,
       simply update everything using a timer.
    */
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(MODEL_UPDATE_DELAY);
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
