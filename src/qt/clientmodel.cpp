// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "clientmodel.h"

#include "guiconstants.h"
#include "peertablemodel.h"

#include "alert.h"
#include "chainparams.h"
#include "checkpoints.h"
#include "bitcoin_checkpoints.h"
#include "main.h"
#include "net.h"
#include "ui_interface.h"

#include <stdint.h>

#include <QDateTime>
#include <QDebug>
#include <QTimer>

static const int64_t nClientStartupTime = GetTime();

ClientModel::ClientModel(OptionsModel *optionsModel, QObject *parent, CNetParams * netParamsIn, MainState& mainStateIn, CChain& chainActiveIn) :
    QObject(parent),
    optionsModel(optionsModel),
    peerTableModel(0),
    cachedNumBlocks(0),
    cachedReindexing(0), cachedImporting(0),
    numBlocksAtStartup(-1), pollTimer(0),
    mainState(mainStateIn),
    chainActive(chainActiveIn)
{
	netParams = netParamsIn;
    peerTableModel = new PeerTableModel(this);
    pollTimer = new QTimer(this);
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(updateTimer()));
    pollTimer->start(MODEL_UPDATE_DELAY);

    nPrevNodeCount = 0;

    isForBitcredit = netParams->MessageStart() == Credits_NetParams()->MessageStart();

    subscribeToCoreSignals();
}

ClientModel::~ClientModel()
{
    unsubscribeFromCoreSignals();
}

int ClientModel::getNumConnections(unsigned int flags) const
{
    LOCK(netParams->cs_vNodes);
    if (flags == CONNECTIONS_ALL) // Shortcut if we want total
        return netParams->vNodes.size();

    int nNum = 0;
    BOOST_FOREACH(CNode* pnode, netParams->vNodes)
    if (flags & (pnode->fInbound ? CONNECTIONS_IN : CONNECTIONS_OUT))
        nNum++;

    return nNum;
}

int ClientModel::getNumBlocks() const
{
    LOCK(mainState.cs_main);
    return chainActive.Height();
}

int ClientModel::getNumBlocksAtStartup()
{
    if (numBlocksAtStartup == -1) numBlocksAtStartup = getNumBlocks();
    return numBlocksAtStartup;
}

quint64 ClientModel::getTotalBytesRecv() const
{
    if(isForBitcredit) {
        return CNode::bitcredit_GetTotalBytesRecv();
    } else {
        return CNode::bitcoin_GetTotalBytesRecv();
    }
}

quint64 ClientModel::getTotalBytesSent() const
{
    if(isForBitcredit) {
        return CNode::bitcredit_GetTotalBytesSent();
    } else {
        return CNode::bitcoin_GetTotalBytesSent();
    }
}

QDateTime ClientModel::getLastBlockDate() const
{
    LOCK(mainState.cs_main);
    if (chainActive.Tip()) {
        return QDateTime::fromTime_t(chainActive.Tip()->GetBlockTime());
    } else {
        if(isForBitcredit) {
            return QDateTime::fromTime_t(Credits_Params().GenesisBlock().nTime); // Genesis block's time of current network
        } else {
            return QDateTime::fromTime_t(Bitcoin_Params().GenesisBlock().nTime); // Genesis block's time of current network
        }
    }

}

double ClientModel::getVerificationProgress() const
{
    LOCK(mainState.cs_main);
    if(isForBitcredit) {
        return Checkpoints::Credits_GuessVerificationProgress((Credits_CBlockIndex*)chainActive.Tip());
    } else {
        return Checkpoints::Bitcoin_GuessVerificationProgress((Bitcoin_CBlockIndex*)chainActive.Tip());
    }

}

void ClientModel::updateTimer()
{
    // Get required lock upfront. This avoids the GUI from getting stuck on
    // periodical polls if the core is holding the locks for a longer time -
    // for example, during a wallet rescan.
    TRY_LOCK(mainState.cs_main, lockMain);
    if(!lockMain)
        return;
    // Some quantities (such as number of blocks) change so fast that we don't want to be notified for each change.
    // Periodically check and update with a timer.
    int newNumBlocks = getNumBlocks();

    // check for changed number of blocks we have, number of blocks peers claim to have, reindexing state and importing state
    if (cachedNumBlocks != newNumBlocks ||
        cachedReindexing != mainState.fReindex || cachedImporting != mainState.fImporting)
    {
        cachedNumBlocks = newNumBlocks;
        cachedReindexing = mainState.fReindex;
        cachedImporting = mainState.fImporting;

        emit numBlocksChanged(newNumBlocks);
    }

    emit bytesChanged(getTotalBytesRecv(), getTotalBytesSent());

    LOCK(netParams->cs_vNodes);
    if(netParams->vNodes.size() != nPrevNodeCount) {
        nPrevNodeCount = netParams->vNodes.size();
        emit numConnectionsChanged(nPrevNodeCount);
    }
}

void ClientModel::updateAlert(const QString &hash, int status)
{
    // Show error message notification for new alert
    if(status == CT_NEW)
    {
        uint256 hash_256;
        hash_256.SetHex(hash.toStdString());
        CAlert alert = CAlert::getAlertByHash(hash_256);
        if(!alert.IsNull())
        {
            emit message(tr("Network Alert"), QString::fromStdString(alert.strStatusBar), CClientUIInterface::ICON_ERROR);
        }
    }

    emit alertsChanged(getStatusBarWarnings());
}

QString ClientModel::getNetworkName() const
{
    QString netname(QString::fromStdString(netParams->Params().DataDir()));
    if(netname.isEmpty())
        netname = "main";
    return netname;
}

enum BlockSource ClientModel::getBlockSource() const
{
    if (mainState.fReindex)
        return BLOCK_SOURCE_REINDEX;
    else if (mainState.fImporting)
        return BLOCK_SOURCE_DISK;
    else if (getNumConnections() > 0)
        return BLOCK_SOURCE_NETWORK;

    return BLOCK_SOURCE_NONE;
}

QString ClientModel::getStatusBarWarnings() const
{
    return QString::fromStdString(Bitcredit_GetWarnings("statusbar"));
}

OptionsModel *ClientModel::getOptionsModel()
{
    return optionsModel;
}

PeerTableModel *ClientModel::getPeerTableModel()
{
    return peerTableModel;
}

QString ClientModel::formatFullVersion() const
{
    return QString::fromStdString(FormatFullVersion());
}

QString ClientModel::bitcoin_formatVersion() const
{
    return QString::fromStdString(FormatVersion(Bitcoin_Params().ClientVersion()));
}

QString ClientModel::formatBuildDate() const
{
    return QString::fromStdString(CLIENT_DATE);
}

bool ClientModel::isReleaseVersion() const
{
    return CLIENT_VERSION_IS_RELEASE;
}

QString ClientModel::credits_clientName() const
{
    return QString::fromStdString(BITCREDIT_CLIENT_NAME);
}
QString ClientModel::bitcoin_clientName() const
{
    return QString::fromStdString(BITCOIN_CLIENT_NAME);
}

QString ClientModel::formatClientStartupTime() const
{
    return QDateTime::fromTime_t(nClientStartupTime).toString();
}

// Handlers for core signals
static void ShowProgress(ClientModel *clientmodel, const std::string &title, int nProgress)
{
    // emits signal "showProgress"
    QMetaObject::invokeMethod(clientmodel, "showProgress", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(title)),
                              Q_ARG(int, nProgress));
}

void ClientModel::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
}

void ClientModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
}
