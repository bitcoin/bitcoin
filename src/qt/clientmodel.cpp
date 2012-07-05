#include "clientmodel.h"
#include "guiconstants.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "transactiontablemodel.h"

#include "main.h"
#include "init.h" // for pwalletMain
#include "ui_interface.h"

#include <QDateTime>
#include <QTimer>

static const int64 nClientStartupTime = GetTime();

ClientModel::ClientModel(OptionsModel *optionsModel, QObject *parent) :
    QObject(parent), optionsModel(optionsModel),
    cachedNumBlocks(0), cachedNumBlocksOfPeers(0), cachedHashrate(0), pollTimer(0)
{
    numBlocksAtStartup = -1;

    // Read our specific settings from the wallet db
    /*
    CWalletDB walletdb(optionsModel->getWallet()->strWalletFile);
    walletdb.ReadSetting("miningDebug", miningDebug);
    walletdb.ReadSetting("miningScanTime", miningScanTime);
    std::string str;
    walletdb.ReadSetting("miningServer", str);
    miningServer = QString::fromStdString(str);
    walletdb.ReadSetting("miningPort", str);
    miningPort = QString::fromStdString(str);
    walletdb.ReadSetting("miningUsername", str);
    miningUsername = QString::fromStdString(str);
    walletdb.ReadSetting("miningPassword", str);
    miningPassword = QString::fromStdString(str);
    */
//    if (fGenerateBitcoins)
//    {
        miningType = SoloMining;
        miningStarted = true;
//    }
//    else
//    {
//        miningType = PoolMining;
//        walletdb.ReadSetting("miningStarted", miningStarted);
//    }
//    miningThreads = nLimitProcessors;

    pollTimer = new QTimer(this);
    pollTimer->setInterval(MODEL_UPDATE_DELAY);
    pollTimer->start();
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(updateTimer()));

    subscribeToCoreSignals();
}

ClientModel::~ClientModel()
{
    unsubscribeFromCoreSignals();
}

int ClientModel::getNumConnections() const
{
    return vNodes.size();
}

int ClientModel::getNumBlocks() const
{
    return nBestHeight;
}

int ClientModel::getNumBlocksAtStartup()
{
    if (numBlocksAtStartup == -1) numBlocksAtStartup = getNumBlocks();
    return numBlocksAtStartup;
}

ClientModel::MiningType ClientModel::getMiningType() const
{
    return miningType;
}

int ClientModel::getMiningThreads() const
{
    return miningThreads;
}

bool ClientModel::getMiningStarted() const
{
    return miningStarted;
}

bool ClientModel::getMiningDebug() const
{
    return miningDebug;
}

void ClientModel::setMiningDebug(bool debug)
{
    miningDebug = debug;
//    WriteSetting("miningDebug", miningDebug);
}

int ClientModel::getMiningScanTime() const
{
    return miningScanTime;
}

void ClientModel::setMiningScanTime(int scantime)
{
    miningScanTime = scantime;
//    WriteSetting("miningScanTime", miningScanTime);
}

QString ClientModel::getMiningServer() const
{
    return miningServer;
}

void ClientModel::setMiningServer(QString server)
{
    miningServer = server;
//    WriteSetting("miningServer", miningServer.toStdString());
}

QString ClientModel::getMiningPort() const
{
    return miningPort;
}

void ClientModel::setMiningPort(QString port)
{
    miningPort = port;
//    WriteSetting("miningPort", miningPort.toStdString());
}

QString ClientModel::getMiningUsername() const
{
    return miningUsername;
}

void ClientModel::setMiningUsername(QString username)
{
    miningUsername = username;
//    WriteSetting("miningUsername", miningUsername.toStdString());
}

QString ClientModel::getMiningPassword() const
{
    return miningPassword;
}

void ClientModel::setMiningPassword(QString password)
{
    miningPassword = password;
//    WriteSetting("miningPassword", miningPassword.toStdString());
}

int ClientModel::getHashrate() const
{
    if (GetTimeMillis() - nHPSTimerStart > 8000)
        return (boost::int64_t)0;
    return (boost::int64_t)dHashesPerSec;
}

// Litecoin: copied from bitcoinrpc.cpp.
double ClientModel::GetDifficulty() const
{
    // Floating point number that is a multiple of the minimum difficulty,
    // minimum difficulty = 1.0.

    if (pindexBest == NULL)
        return 1.0;
    int nShift = (pindexBest->nBits >> 24) & 0xff;

    double dDiff =
        (double)0x0000ffff / (double)(pindexBest->nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

QDateTime ClientModel::getLastBlockDate() const
{
    return QDateTime::fromTime_t(pindexBest->GetBlockTime());
}

void ClientModel::updateTimer()
{
    // Some quantities (such as number of blocks) change so fast that we don't want to be notified for each change.
    // Periodically check and update with a timer.
    int newNumBlocks = getNumBlocks();
    int newNumBlocksOfPeers = getNumBlocksOfPeers();

    if(cachedNumBlocks != newNumBlocks || cachedNumBlocksOfPeers != newNumBlocksOfPeers)
        emit numBlocksChanged(newNumBlocks, newNumBlocksOfPeers);

    cachedNumBlocks = newNumBlocks;
    cachedNumBlocksOfPeers = newNumBlocksOfPeers;

    // Only need to update if solo mining. When pool mining, stats are pushed.
    if (miningType == SoloMining)
    {
        int newHashrate = getHashrate();
        if (cachedHashrate != newHashrate)
            emit miningChanged(miningStarted, newHashrate);
        cachedHashrate = newHashrate;
    }
}

void ClientModel::updateNumConnections(int numConnections)
{
    emit numConnectionsChanged(numConnections);
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
            emit error(tr("Network Alert"), QString::fromStdString(alert.strStatusBar), false);
        }
    }

    // Emit a numBlocksChanged when the status message changes,
    // so that the view recomputes and updates the status bar.
    emit numBlocksChanged(getNumBlocks(), getNumBlocksOfPeers());
}

bool ClientModel::isTestNet() const
{
    return fTestNet;
}

bool ClientModel::inInitialBlockDownload() const
{
    return IsInitialBlockDownload();
}

int ClientModel::getNumBlocksOfPeers() const
{
    return GetNumBlocksOfPeers();
}

void ClientModel::setMining(MiningType type, bool mining, int threads, int hashrate)
{
    if (type == SoloMining && mining != miningStarted)
    {
        GenerateBitcoins(mining ? 1 : 0, pwalletMain);
    }
    miningType = type;
    miningStarted = mining;
//    WriteSetting("miningStarted", mining);
//    WriteSetting("fLimitProcessors", 1);
//    WriteSetting("nLimitProcessors", threads);
    emit miningChanged(mining, hashrate);
}

QString ClientModel::getStatusBarWarnings() const
{
    return QString::fromStdString(GetWarnings("statusbar"));
}

OptionsModel *ClientModel::getOptionsModel()
{
    return optionsModel;
}

QString ClientModel::formatFullVersion() const
{
    return QString::fromStdString(FormatFullVersion());
}

QString ClientModel::formatBuildDate() const
{
    return QString::fromStdString(CLIENT_DATE);
}

QString ClientModel::clientName() const
{
    return QString::fromStdString(CLIENT_NAME);
}

QString ClientModel::formatClientStartupTime() const
{
    return QDateTime::fromTime_t(nClientStartupTime).toString();
}

// Handlers for core signals
static void NotifyBlocksChanged(ClientModel *clientmodel)
{
    // This notification is too frequent. Don't trigger a signal.
    // Don't remove it, though, as it might be useful later.
}

static void NotifyNumConnectionsChanged(ClientModel *clientmodel, int newNumConnections)
{
    // Too noisy: OutputDebugStringF("NotifyNumConnectionsChanged %i\n", newNumConnections);
    QMetaObject::invokeMethod(clientmodel, "updateNumConnections", Qt::QueuedConnection,
                              Q_ARG(int, newNumConnections));
}

static void NotifyAlertChanged(ClientModel *clientmodel, const uint256 &hash, ChangeType status)
{
    OutputDebugStringF("NotifyAlertChanged %s status=%i\n", hash.GetHex().c_str(), status);
    QMetaObject::invokeMethod(clientmodel, "updateAlert", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(hash.GetHex())),
                              Q_ARG(int, status));
}

void ClientModel::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.NotifyBlocksChanged.connect(boost::bind(NotifyBlocksChanged, this));
    uiInterface.NotifyNumConnectionsChanged.connect(boost::bind(NotifyNumConnectionsChanged, this, _1));
    uiInterface.NotifyAlertChanged.connect(boost::bind(NotifyAlertChanged, this, _1, _2));
}

void ClientModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.NotifyBlocksChanged.disconnect(boost::bind(NotifyBlocksChanged, this));
    uiInterface.NotifyNumConnectionsChanged.disconnect(boost::bind(NotifyNumConnectionsChanged, this, _1));
    uiInterface.NotifyAlertChanged.disconnect(boost::bind(NotifyAlertChanged, this, _1, _2));
}
