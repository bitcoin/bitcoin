// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/clientmodel.h>

#include <qt/bantablemodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/peertablemodel.h>
#include <qt/peertablesortproxy.h>

#include <evo/deterministicmns.h>

#include <clientversion.h>
#include <governance/object.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <net.h>
#include <netbase.h>
#include <util/system.h>
#include <util/threadnames.h>
#include <util/time.h>
#include <validation.h>

#include <stdint.h>

#include <QDebug>
#include <QMetaObject>
#include <QThread>
#include <QTimer>

static int64_t nLastHeaderTipUpdateNotification = 0;
static int64_t nLastBlockTipUpdateNotification = 0;

ClientModel::ClientModel(interfaces::Node& node, OptionsModel *_optionsModel, QObject *parent) :
    QObject(parent),
    m_node(node),
    optionsModel(_optionsModel),
    peerTableModel(nullptr),
    banTableModel(nullptr),
    m_thread(new QThread(this))
{
    cachedBestHeaderHeight = -1;
    cachedBestHeaderTime = -1;

    peerTableModel = new PeerTableModel(m_node, this);
    m_peer_table_sort_proxy = new PeerTableSortProxy(this);
    m_peer_table_sort_proxy->setSourceModel(peerTableModel);

    banTableModel = new BanTableModel(m_node, this);
    mnListCached = std::make_unique<CDeterministicMNList>();

    QTimer* timer = new QTimer;
    timer->setInterval(MODEL_UPDATE_DELAY);
    connect(timer, &QTimer::timeout, [this] {
        // no locking required at this point
        // the following calls will acquire the required lock
        Q_EMIT mempoolSizeChanged(m_node.getMempoolSize(), m_node.getMempoolDynamicUsage(), m_node.getMempoolMaxUsage());
        Q_EMIT islockCountChanged(m_node.llmq().getInstantSentLockCount());
    });
    connect(m_thread, &QThread::finished, timer, &QObject::deleteLater);
    connect(m_thread, &QThread::started, [timer] { timer->start(); });
    // move timer to thread so that polling doesn't disturb main event loop
    timer->moveToThread(m_thread);
    m_thread->start();
    QTimer::singleShot(0, timer, []() {
        util::ThreadRename("qt-clientmodl");
    });

    subscribeToCoreSignals();
}

void ClientModel::stop()
{
    unsubscribeFromCoreSignals();

    m_thread->quit();
    m_thread->wait();
}

ClientModel::~ClientModel()
{
    stop();
}

int ClientModel::getNumConnections(unsigned int flags) const
{
    ConnectionDirection connections = ConnectionDirection::None;

    if(flags == CONNECTIONS_IN)
        connections = ConnectionDirection::In;
    else if (flags == CONNECTIONS_OUT)
        connections = ConnectionDirection::Out;
    else if (flags == CONNECTIONS_ALL)
        connections = ConnectionDirection::Both;

    return m_node.getNodeCount(connections);
}

void ClientModel::setMasternodeList(const CDeterministicMNList& mnList, const CBlockIndex* tip)
{
    LOCK(cs_mnlist);
    if (mnListCached->GetBlockHash() == mnList.GetBlockHash()) {
        return;
    }
    mnListCached = std::make_unique<CDeterministicMNList>(mnList);
    mnListTip = tip;
    Q_EMIT masternodeListChanged();
}

std::pair<CDeterministicMNList, const CBlockIndex*> ClientModel::getMasternodeList() const
{
    LOCK(cs_mnlist);
    return {*mnListCached, mnListTip};
}

void ClientModel::refreshMasternodeList()
{
    auto [mnList, tip] = m_node.evo().getListAtChainTip();

    LOCK(cs_mnlist);
    setMasternodeList(mnList, tip);
}

int ClientModel::getHeaderTipHeight() const
{
    if (cachedBestHeaderHeight == -1) {
        // make sure we initially populate the cache via a cs_main lock
        // otherwise we need to wait for a tip update
        int height;
        int64_t blockTime;
        if (m_node.getHeaderTip(height, blockTime)) {
            cachedBestHeaderHeight = height;
            cachedBestHeaderTime = blockTime;
        }
    }
    return cachedBestHeaderHeight;
}

int64_t ClientModel::getHeaderTipTime() const
{
    if (cachedBestHeaderTime == -1) {
        int height;
        int64_t blockTime;
        if (m_node.getHeaderTip(height, blockTime)) {
            cachedBestHeaderHeight = height;
            cachedBestHeaderTime = blockTime;
        }
    }
    return cachedBestHeaderTime;
}

void ClientModel::getAllGovernanceObjects(std::vector<CGovernanceObject> &obj)
{
    m_node.gov().getAllNewerThan(obj, 0);
}

std::map<CNetAddr, LocalServiceInfo> ClientModel::getNetLocalAddresses() const
{
    return m_node.getNetLocalAddresses();
}

int ClientModel::getNumBlocks() const
{
    if (m_cached_num_blocks == -1) {
        m_cached_num_blocks = m_node.getNumBlocks();
    }
    return m_cached_num_blocks;
}

uint256 ClientModel::getBestBlockHash()
{
    uint256 tip{WITH_LOCK(m_cached_tip_mutex, return m_cached_tip_blocks)};

    if (!tip.IsNull()) {
        return tip;
    }

    // Lock order must be: first `cs_main`, then `m_cached_tip_mutex`.
    // The following will lock `cs_main` (and release it), so we must not
    // own `m_cached_tip_mutex` here.
    tip = m_node.getBestBlockHash();

    LOCK(m_cached_tip_mutex);
    // We checked that `m_cached_tip_blocks` is not null above, but then we
    // released the mutex `m_cached_tip_mutex`, so it could have changed in the
    // meantime. Thus, check again.
    if (m_cached_tip_blocks.IsNull()) {
        m_cached_tip_blocks = tip;
    }
    return m_cached_tip_blocks;
}

BlockSource ClientModel::getBlockSource() const
{
    if (m_node.isLoadingBlocks()) return BlockSource::DISK;
    if (getNumConnections() > 0) return BlockSource::NETWORK;
    return BlockSource::NONE;
}

QString ClientModel::getStatusBarWarnings() const
{
    return QString::fromStdString(m_node.getWarnings().translated);
}

OptionsModel *ClientModel::getOptionsModel()
{
    return optionsModel;
}

PeerTableModel *ClientModel::getPeerTableModel()
{
    return peerTableModel;
}

PeerTableSortProxy* ClientModel::peerTableSortProxy()
{
    return m_peer_table_sort_proxy;
}

BanTableModel *ClientModel::getBanTableModel()
{
    return banTableModel;
}

QString ClientModel::formatFullVersion() const
{
    return QString::fromStdString(FormatFullVersion());
}

QString ClientModel::formatSubVersion() const
{
    return QString::fromStdString(strSubVersion);
}

bool ClientModel::isReleaseVersion() const
{
    return CLIENT_VERSION_IS_RELEASE;
}

QString ClientModel::formatClientStartupTime() const
{
    return QDateTime::fromSecsSinceEpoch(GetStartupTime()).toString();
}

QString ClientModel::dataDir() const
{
    return GUIUtil::PathToQString(gArgs.GetDataDirNet());
}

QString ClientModel::blocksDir() const
{
    return GUIUtil::PathToQString(gArgs.GetBlocksDirPath());
}

void ClientModel::TipChanged(SynchronizationState sync_state, interfaces::BlockTip tip, double verification_progress, bool header)
{
    if (header) {
        // cache best headers time and height to reduce future cs_main locks
        cachedBestHeaderHeight = tip.block_height;
        cachedBestHeaderTime = tip.block_time;
    } else {
        m_cached_num_blocks = tip.block_height;
        WITH_LOCK(m_cached_tip_mutex, m_cached_tip_blocks = tip.block_hash;);
    }

    // Throttle GUI notifications about (a) blocks during initial sync, and (b) both blocks and headers during reindex.
    const bool throttle = (sync_state != SynchronizationState::POST_INIT && !header) || sync_state == SynchronizationState::INIT_REINDEX;
    const int64_t now = throttle ? TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) : 0;
    int64_t& nLastUpdateNotification = header ? nLastHeaderTipUpdateNotification : nLastBlockTipUpdateNotification;
    if (throttle && now < nLastUpdateNotification + count_milliseconds(MODEL_UPDATE_DELAY)) {
        return;
    }

    Q_EMIT numBlocksChanged(tip.block_height, QDateTime::fromSecsSinceEpoch(tip.block_time), QString::fromStdString(tip.block_hash.ToString()), verification_progress, header, sync_state);
    nLastUpdateNotification = now;
}

void ClientModel::subscribeToCoreSignals()
{
    m_event_handlers.emplace_back(m_node.handleShowProgress(
        [this](const std::string& title, int progress, [[maybe_unused]] bool resume_possible) {
            Q_EMIT showProgress(QString::fromStdString(title), progress);
        }));
    m_event_handlers.emplace_back(m_node.handleNotifyNumConnectionsChanged(
        [this](int new_num_connections) {
            Q_EMIT numConnectionsChanged(new_num_connections);
        }));
    m_event_handlers.emplace_back(m_node.handleNotifyNetworkActiveChanged(
        [this](bool network_active) {
            Q_EMIT networkActiveChanged(network_active);
        }));
    m_event_handlers.emplace_back(m_node.handleNotifyAlertChanged(
        [this]() {
            qDebug() << "ClientModel: NotifyAlertChanged";
            Q_EMIT alertsChanged(getStatusBarWarnings());
        }));
    m_event_handlers.emplace_back(m_node.handleBannedListChanged(
        [this]() {
            qDebug() << "ClienModel: Requesting update for peer banlist";
            QMetaObject::invokeMethod(banTableModel, [this] { banTableModel->refresh(); });
        }));
    m_event_handlers.emplace_back(m_node.handleNotifyBlockTip(
        [this](SynchronizationState sync_state, interfaces::BlockTip tip, double verification_progress) {
            TipChanged(sync_state, tip, verification_progress, /*header=*/false);
        }));
    m_event_handlers.emplace_back(m_node.handleNotifyHeaderTip(
        [this](SynchronizationState sync_state, interfaces::BlockTip tip, double verification_progress) {
            TipChanged(sync_state, tip, verification_progress, /*header=*/true);
        }));
    m_event_handlers.emplace_back(m_node.handleNotifyAdditionalDataSyncProgressChanged(
        [this](double nSyncProgress) {
            Q_EMIT additionalDataSyncProgressChanged(nSyncProgress);
        }));
    m_event_handlers.emplace_back(m_node.handleNotifyChainLock(
        [this](const std::string& best_hash, int best_height) {
            Q_EMIT chainLockChanged(QString::fromStdString(best_hash), best_height);
        }));
    m_event_handlers.emplace_back(m_node.handleNotifyMasternodeListChanged(
        [this](const CDeterministicMNList& newList, const CBlockIndex* pindex) {
            setMasternodeList(newList, pindex);
        }));
}

void ClientModel::unsubscribeFromCoreSignals()
{
    m_event_handlers.clear();
}

bool ClientModel::getProxyInfo(std::string& ip_port) const
{
    Proxy ipv4, ipv6;
    if (m_node.getProxy((Network) 1, ipv4) && m_node.getProxy((Network) 2, ipv6)) {
      ip_port = ipv4.proxy.ToStringAddrPort();
      return true;
    }
    return false;
}
