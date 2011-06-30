#include "clientmodel.h"
#include "guiconstants.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "transactiontablemodel.h"

#include "headers.h"

#include <QTimer>

ClientModel::ClientModel(CWallet *wallet, QObject *parent) :
    QObject(parent), wallet(wallet), optionsModel(0)
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

void ClientModel::update()
{
    // Plainly emit all signals for now. To be more efficient this should check
    //   whether the values actually changed first, although it'd be even better if these
    //   were events coming in from the bitcoin core.
    emit numConnectionsChanged(getNumConnections());
    emit numBlocksChanged(getNumBlocks());
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

