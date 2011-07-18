#include "clientmodel.h"
#include "guiconstants.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "transactiontablemodel.h"

#include "headers.h"

#include <QTimer>
#include <QDateTime>

ClientModel::ClientModel(CWallet *wallet, QObject *parent) :
    QObject(parent), wallet(wallet), optionsModel(0),
    cachedNumConnections(0), cachedNumBlocks(0)
{
    // Until signal notifications is built into the bitcoin core,
    //  simply update everything after polling using a timer.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(MODEL_UPDATE_DELAY);

    optionsModel = new OptionsModel(wallet, this);
}

int ClientModel::getNumConnections() const
{
    return vNodes.size();
}

int ClientModel::getNumBlocks() const
{
    return nBestHeight;
}

QDateTime ClientModel::getLastBlockDate() const
{
    return QDateTime::fromTime_t(pindexBest->GetBlockTime());
}

void ClientModel::update()
{
    int newNumConnections = getNumConnections();
    int newNumBlocks = getNumBlocks();

    if(cachedNumConnections != newNumConnections)
        emit numConnectionsChanged(newNumConnections);
    if(cachedNumBlocks != newNumBlocks)
        emit numBlocksChanged(newNumBlocks);

    cachedNumConnections = newNumConnections;
    cachedNumBlocks = newNumBlocks;
}

bool ClientModel::isTestNet() const
{
    return fTestNet;
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

QString ClientModel::formatFullVersion() const
{
    return QString::fromStdString(FormatFullVersion());
}
