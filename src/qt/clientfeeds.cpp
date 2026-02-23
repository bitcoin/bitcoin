// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/clientfeeds.h>

#include <coins.h>
#include <governance/object.h>
#include <key_io.h>
#include <script/standard.h>
#include <util/threadnames.h>
#include <util/time.h>

#include <qt/clientmodel.h>
#include <qt/masternodemodel.h>
#include <qt/proposalmodel.h>
#include <qt/util.h>

#include <QDebug>
#include <QThread>

namespace {
constexpr auto CHAINLOCK_UPDATE_INTERVAL{3s};
constexpr auto CREDITPOOL_UPDATE_INTERVAL{3s};
constexpr auto INSTANTSEND_UPDATE_INTERVAL{3s};
constexpr auto MASTERNODE_UPDATE_INTERVAL{3s};
constexpr auto PROPOSAL_UPDATE_INTERVAL{10s};
constexpr auto QUORUM_UPDATE_INTERVAL{3s};
} // anonymous namespace

FeedBase::FeedBase(QObject* parent, const FeedBase::Config& config) :
    QObject{parent},
    m_config{config}
{
}

FeedBase::~FeedBase() = default;

void FeedBase::requestForceRefresh()
{
    if (m_timer) {
        m_timer->start(0);
    }
}

void FeedBase::requestRefresh()
{
    if (m_timer && !m_timer->isActive()) {
        m_timer->start(m_syncing.load() ? m_config.m_throttle : m_config.m_baseline);
    }
}

ChainLockFeed::ChainLockFeed(QObject* parent, ClientModel& client_model) :
    Feed<ChainLockData>{parent, {/*m_baseline=*/CHAINLOCK_UPDATE_INTERVAL, /*m_throttle=*/CHAINLOCK_UPDATE_INTERVAL*10}},
    m_client_model{client_model}
{
}

ChainLockFeed::~ChainLockFeed() = default;

void ChainLockFeed::fetch()
{
    if (m_client_model.node().shutdownRequested()) {
        return;
    }

    const auto cl_info = m_client_model.node().llmq().getBestChainLock();
    if (cl_info.m_height == 0) {
        // Chainlock at genesis height not possible, assume something went wrong
        return;
    }

    auto ret = std::make_shared<Data>();
    ret->m_block_time = cl_info.m_block_time;
    ret->m_height = cl_info.m_height;
    ret->m_hash = QString::fromStdString(cl_info.m_hash.ToString());

    setData(std::move(ret));
}

CreditPoolFeed::CreditPoolFeed(QObject* parent, ClientModel& client_model) :
    Feed<CreditPoolData>{parent, {/*m_baseline=*/CREDITPOOL_UPDATE_INTERVAL, /*m_throttle=*/CREDITPOOL_UPDATE_INTERVAL*10}},
    m_client_model{client_model}
{
}

CreditPoolFeed::~CreditPoolFeed() = default;

void CreditPoolFeed::fetch()
{
    if (m_client_model.node().shutdownRequested()) {
        return;
    }

    auto ret = std::make_shared<Data>();
    ret->m_counts = m_client_model.node().llmq().getCreditPoolCounts();
    ret->m_pending_unlocks = m_client_model.node().llmq().getPendingAssetUnlocks();

    setData(std::move(ret));
}

InstantSendFeed::InstantSendFeed(QObject* parent, ClientModel& client_model) :
    Feed<InstantSendData>{parent, {/*m_baseline=*/INSTANTSEND_UPDATE_INTERVAL, /*m_throttle=*/INSTANTSEND_UPDATE_INTERVAL*10}},
    m_client_model{client_model}
{
}

InstantSendFeed::~InstantSendFeed() = default;

void InstantSendFeed::fetch()
{
    if (m_client_model.node().shutdownRequested()) {
        return;
    }

    auto ret = std::make_shared<Data>();
    ret->m_counts = m_client_model.node().llmq().getInstantSendCounts();

    setData(std::move(ret));
}

QuorumFeed::QuorumFeed(QObject* parent, ClientModel& client_model) :
    Feed<QuorumData>{parent, {/*m_baseline=*/QUORUM_UPDATE_INTERVAL, /*m_throttle=*/QUORUM_UPDATE_INTERVAL*10}},
    m_client_model{client_model}
{
}

QuorumFeed::~QuorumFeed() = default;

void QuorumFeed::fetch()
{
    if (m_client_model.node().shutdownRequested()) {
        return;
    }

    auto ret = std::make_shared<Data>();
    ret->m_quorums = m_client_model.node().llmq().getQuorumStats();

    setData(std::move(ret));
}

MasternodeFeed::MasternodeFeed(QObject* parent, ClientModel& client_model) :
    Feed<MasternodeData>{parent, {/*m_baseline=*/MASTERNODE_UPDATE_INTERVAL, /*m_throttle=*/MASTERNODE_UPDATE_INTERVAL*10}},
    m_client_model{client_model}
{
}

MasternodeFeed::~MasternodeFeed() = default;

void MasternodeFeed::fetch()
{
    if (m_client_model.node().shutdownRequested()) {
        return;
    }

    const auto [dmn, pindex] = m_client_model.node().evo().getListAtChainTip();
    if (!dmn || !pindex) {
        return;
    }

    auto projectedPayees = dmn->getProjectedMNPayees(pindex);
    if (projectedPayees.empty() && dmn->getCounts().enabled() > 0) {
        // GetProjectedMNPayees failed to provide results for a list with valid mns.
        // Keep current list and let it try again later.
        return;
    }

    auto ret = std::make_shared<Data>();
    ret->m_list_height = dmn->getHeight();
    ret->m_counts = dmn->getCounts();

    Uint256HashMap<int> nextPayments;
    for (size_t i = 0; i < projectedPayees.size(); i++) {
        const auto& dmn = projectedPayees[i];
        nextPayments.emplace(dmn->getProTxHash(), ret->m_list_height + (int)i + 1);
    }

    dmn->forEachMN(/*only_valid=*/false, [&](const auto& dmn) {
        CTxDestination collateralDest;
        Coin coin;
        QString collateralStr = QObject::tr("UNKNOWN");
        if (m_client_model.node().getUnspentOutput(dmn->getCollateralOutpoint(), coin) &&
            ExtractDestination(coin.out.scriptPubKey, collateralDest)) {
            collateralStr = QString::fromStdString(EncodeDestination(collateralDest));
        }
        int nNextPayment{0};
        if (auto nextPaymentIt = nextPayments.find(dmn->getProTxHash()); nextPaymentIt != nextPayments.end()) {
            nNextPayment = nextPaymentIt->second;
        }
        auto entry = std::make_shared<MasternodeEntry>(dmn, collateralStr, nNextPayment);
        ret->m_by_protx[dmn->getProTxHash()] = entry.get();
        ret->m_by_service[util::make_array(dmn->getNetInfoPrimary().GetKey())] = entry.get();
        ret->m_entries.push_back(std::move(entry));
    });

    ret->m_valid = true;
    setData(std::move(ret));
}

ProposalFeed::ProposalFeed(QObject* parent, ClientModel& client_model, MasternodeFeed& feed_masternode) :
    Feed<ProposalData>{parent, {/*m_baseline=*/PROPOSAL_UPDATE_INTERVAL, /*m_throttle=*/PROPOSAL_UPDATE_INTERVAL*6}},
    m_client_model{client_model},
    m_feed_masternode{feed_masternode}
{
}

ProposalFeed::~ProposalFeed() = default;

void ProposalFeed::fetch()
{
    if (m_client_model.node().shutdownRequested()) {
        return;
    }

    const auto data_mn = m_feed_masternode.data();
    if (!data_mn || !data_mn->m_valid) {
        return;
    }

    auto ret = std::make_shared<Data>();
    // A proposal is considered passing if (YES votes - NO votes) >= (Total Weight of Masternodes / 10),
    // count total valid (ENABLED) masternodes to determine passing threshold.
    const auto nMinQuorum = static_cast<size_t>(Params().GetConsensus().nGovernanceMinQuorum);
    ret->m_abs_vote_req = static_cast<int>(std::max(nMinQuorum, data_mn->m_counts.m_valid_weighted / 10));
    ret->m_gov_info = m_client_model.node().gov().getGovernanceInfo();
    std::vector<CGovernanceObject> govObjList;
    m_client_model.getAllGovernanceObjects(govObjList);
    for (const auto& govObj : govObjList) {
        if (govObj.GetObjectType() != GovernanceObject::PROPOSAL) {
            continue; // Skip triggers.
        }
        ret->m_proposals.emplace_back(std::make_shared<Proposal>(m_client_model, govObj, ret->m_gov_info, ret->m_gov_info.requiredConfs,
                                                                 /*is_broadcast=*/true));
    }

    auto fundable{m_client_model.node().gov().getFundableProposalHashes()};
    ret->m_fundable_hashes = std::move(fundable.hashes);

    setData(std::move(ret));
}

ClientFeeds::ClientFeeds(QObject* parent) :
    QObject{parent},
    m_thread{new QThread(this)}
{
}

ClientFeeds::~ClientFeeds()
{
    stop();
}

void ClientFeeds::registerFeed(FeedBase* raw)
{
    auto* timer = new QTimer(this);
    timer->setSingleShot(true);
    raw->m_timer = timer;

    connect(timer, &QTimer::timeout, this, [this, raw] {
        if (raw->m_in_progress.exchange(true)) {
            raw->m_retry_pending.store(true);
            return;
        }
        QMetaObject::invokeMethod(m_worker, [this, raw] {
            try {
                raw->fetch();
            } catch (const std::exception& e) {
                qWarning() << "ClientFeeds::fetch() exception: " << e.what();
            } catch (...) {
                qWarning() << "ClientFeeds::fetch() unknown exception";
            }
            QTimer::singleShot(0, raw, [this, raw] {
                raw->m_in_progress.store(false);
                if (m_stopped) return;
                Q_EMIT raw->dataReady();
                if (raw->m_retry_pending.exchange(false)) {
                    raw->requestRefresh();
                }
            });
        });
    });
}

void ClientFeeds::start()
{
    m_worker = new QObject();
    m_worker->moveToThread(m_thread);
    m_thread->start();
    QMetaObject::invokeMethod(m_worker, [] { util::ThreadRename("qt-clientfeed"); });

    for (const auto& source : m_sources) {
        if (source->m_timer) {
            source->m_timer->start(0);
        }
    }
}

void ClientFeeds::stop()
{
    if (m_stopped) {
        return;
    }

    m_stopped = true;
    for (const auto& source : m_sources) {
        if (source->m_timer) {
            source->m_timer->stop();
            source->m_timer = nullptr;
        }
    }
    if (m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait();
    }
    delete m_worker;
    m_worker = nullptr;
}

void ClientFeeds::setSyncing(bool syncing)
{
    if (m_stopped) {
        return;
    }

    for (const auto& source : m_sources) {
        if (source->m_syncing.load() == syncing) {
            continue;
        }
        source->setSyncing(syncing);
        if (source->m_timer && source->m_timer->isActive()) {
            source->m_timer->start(syncing ? source->m_config.m_throttle : source->m_config.m_baseline);
        }
    }
}
