// Copyright (c) 2023-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <coinjoin/walletman.h>
#include <evo/deterministicmns.h>
#include <logging.h>
#include <masternode/meta.h>
#include <masternode/sync.h>
#include <msg_result.h>
#include <net.h>
#include <protocol.h>
#include <scheduler.h>
#include <shutdown.h>
#include <streams.h>

#ifdef ENABLE_WALLET
#include <coinjoin/client.h>
#endif // ENABLE_WALLET

#include <memory>
#include <ranges>

#ifdef ENABLE_WALLET
class CJWalletManagerImpl final : public CJWalletManager
{
public:
    CJWalletManagerImpl(ChainstateManager& chainman, CDeterministicMNManager& dmnman, CMasternodeMetaMan& mn_metaman,
                        CTxMemPool& mempool, const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman,
                        bool relay_txes);
    ~CJWalletManagerImpl() override;

public:
    void Schedule(CConnman& connman, CScheduler& scheduler) override;

public:
    bool hasQueue(const uint256& hash) const override;
    bool doForClient(const std::string& name, const std::function<void(CCoinJoinClientManager&)>& func) override EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map);
    MessageProcessingResult processMessage(CNode& peer, Chainstate& chainstate, CConnman& connman, CTxMemPool& mempool,
                                           std::string_view msg_type, CDataStream& vRecv) override EXCLUSIVE_LOCKS_REQUIRED(!cs_ProcessDSQueue, !cs_wallet_manager_map);
    std::optional<CCoinJoinQueue> getQueueFromHash(const uint256& hash) const override;
    std::optional<int> getQueueSize() const override;
    std::vector<CDeterministicMNCPtr> getMixingMasternodes() override EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map);
    void addWallet(const std::shared_ptr<wallet::CWallet>& wallet) override EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map);
    void removeWallet(const std::string& name) override EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map);
    void flushWallet(const std::string& name) override EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map);

protected:
    // CValidationInterface
    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map);

private:
    const bool m_relay_txes;
    ChainstateManager& m_chainman;
    CDeterministicMNManager& m_dmnman;
    CMasternodeMetaMan& m_mn_metaman;
    CTxMemPool& m_mempool;
    const CMasternodeSync& m_mn_sync;
    const llmq::CInstantSendManager& m_isman;

    // m_queueman is declared before the wallet map so that it is destroyed
    // after all CCoinJoinClientManager instances (which hold a raw pointer to it).
    // Null when relay_txes is false (no queue processing).
    const std::unique_ptr<CoinJoinQueueManager> m_queueman;

    mutable Mutex cs_ProcessDSQueue;

    mutable Mutex cs_wallet_manager_map;
    std::map<const std::string, std::unique_ptr<CCoinJoinClientManager>> m_wallet_manager_map GUARDED_BY(cs_wallet_manager_map);

    void DoMaintenance(CConnman& connman) EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map);
    void CheckPendingObservations() EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map);

    [[nodiscard]] MessageProcessingResult ProcessDSQueue(NodeId from, CConnman& connman, std::string_view msg_type,
                                                         CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!cs_ProcessDSQueue, !cs_wallet_manager_map);

    template <typename Callable>
    void ForEachCJClientMan(Callable&& func) EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map)
    {
        LOCK(cs_wallet_manager_map);
        for (auto& [_, clientman] : m_wallet_manager_map) {
            func(*clientman);
        }
    }

    template <typename Callable>
    bool ForAnyCJClientMan(Callable&& func) EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet_manager_map)
    {
        LOCK(cs_wallet_manager_map);
        return std::ranges::any_of(m_wallet_manager_map, [&](auto& pair) { return func(*pair.second); });
    }
};

CJWalletManagerImpl::CJWalletManagerImpl(ChainstateManager& chainman, CDeterministicMNManager& dmnman,
                                         CMasternodeMetaMan& mn_metaman, CTxMemPool& mempool,
                                         const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman,
                                         bool relay_txes) :
    m_relay_txes{relay_txes},
    m_chainman{chainman},
    m_dmnman{dmnman},
    m_mn_metaman{mn_metaman},
    m_mempool{mempool},
    m_mn_sync{mn_sync},
    m_isman{isman},
    m_queueman{m_relay_txes ? std::make_unique<CoinJoinQueueManager>() : nullptr}
{
}

CJWalletManagerImpl::~CJWalletManagerImpl()
{
    LOCK(cs_wallet_manager_map);
    for (auto& [_, clientman] : m_wallet_manager_map) {
        clientman.reset();
    }
}

void CJWalletManagerImpl::Schedule(CConnman& connman, CScheduler& scheduler)
{
    // Inputs of a successfully completed session stay locked until the finalized mixing
    // transaction is observed and those locks are persisted, so they are restored with
    // the wallet. Releasing them has to keep running independently of mixing itself:
    // even with CoinJoin disabled, mixing stopped or transaction relay unavailable
    // (block-only mode, where nothing below is scheduled at all) nothing else would ever
    // unlock them. NOTE: no CJWalletManager exists on a masternode (see init.cpp), a
    // wallet with pending observations opened there keeps its inputs locked until it is
    // opened on a regular node again or the user unlocks them via `lockunspent`.
    scheduler.scheduleEvery(std::bind(&CJWalletManagerImpl::CheckPendingObservations, this), std::chrono::minutes{1});

    // Mixing needs transaction relay
    if (!m_relay_txes) return;

    scheduler.scheduleEvery(std::bind(&CJWalletManagerImpl::DoMaintenance, this, std::ref(connman)),
                            std::chrono::seconds{1});
}

void CJWalletManagerImpl::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;

    ForEachCJClientMan([&pindexNew](CCoinJoinClientManager& clientman) { clientman.UpdatedBlockTip(pindexNew); });
}

bool CJWalletManagerImpl::hasQueue(const uint256& hash) const
{
    if (m_queueman) {
        return m_queueman->HasQueue(hash);
    }
    return false;
}

bool CJWalletManagerImpl::doForClient(const std::string& name, const std::function<void(CCoinJoinClientManager&)>& func)
{
    LOCK(cs_wallet_manager_map);
    auto it = m_wallet_manager_map.find(name);
    if (it == m_wallet_manager_map.end()) return false;
    func(*it->second);
    return true;
}

std::optional<CCoinJoinQueue> CJWalletManagerImpl::getQueueFromHash(const uint256& hash) const
{
    if (m_queueman) {
        return m_queueman->GetQueueFromHash(hash);
    }
    return std::nullopt;
}

std::optional<int> CJWalletManagerImpl::getQueueSize() const
{
    if (m_queueman) {
        return m_queueman->GetQueueSize();
    }
    return std::nullopt;
}

std::vector<CDeterministicMNCPtr> CJWalletManagerImpl::getMixingMasternodes()
{
    std::vector<CDeterministicMNCPtr> ret{};
    ForEachCJClientMan([&](const CCoinJoinClientManager& clientman) { clientman.GetMixingMasternodesInfo(ret); });
    return ret;
}

void CJWalletManagerImpl::addWallet(const std::shared_ptr<wallet::CWallet>& wallet)
{
    LOCK(cs_wallet_manager_map);
    m_wallet_manager_map.try_emplace(wallet->GetName(),
                                     std::make_unique<CCoinJoinClientManager>(wallet, m_dmnman, m_mn_metaman, m_mn_sync,
                                                                              m_isman, m_queueman.get()));
}

void CJWalletManagerImpl::flushWallet(const std::string& name)
{
    doForClient(name, [](CCoinJoinClientManager& clientman) {
        clientman.resetPool();
        clientman.stopMixing();
    });
}

void CJWalletManagerImpl::removeWallet(const std::string& name)
{
    LOCK(cs_wallet_manager_map);
    m_wallet_manager_map.erase(name);
}

void CJWalletManagerImpl::DoMaintenance(CConnman& connman)
{
    if (m_queueman && m_mn_sync.IsBlockchainSynced() && !ShutdownRequested()) {
        m_queueman->CheckQueue();
    }
    LOCK(cs_wallet_manager_map);
    for (auto& [_, clientman] : m_wallet_manager_map) {
        clientman->DoMaintenance(m_chainman, connman, m_mempool);
    }
}

void CJWalletManagerImpl::CheckPendingObservations()
{
    if (ShutdownRequested()) return;
    LOCK(cs_wallet_manager_map);
    for (auto& [_, clientman] : m_wallet_manager_map) {
        clientman->CheckPendingObservations(m_mempool);
    }
}

MessageProcessingResult CJWalletManagerImpl::processMessage(CNode& pfrom, Chainstate& chainstate, CConnman& connman,
                                                            CTxMemPool& mempool, std::string_view msg_type,
                                                            CDataStream& vRecv)
{
    ForEachCJClientMan([&](CCoinJoinClientManager& clientman) {
        clientman.ProcessMessage(pfrom, chainstate, connman, mempool, msg_type, vRecv);
    });
    if (m_queueman) {
        return ProcessDSQueue(pfrom.GetId(), connman, msg_type, vRecv);
    }
    return {};
}

MessageProcessingResult CJWalletManagerImpl::ProcessDSQueue(NodeId from, CConnman& connman, std::string_view msg_type,
                                                            CDataStream& vRecv)
{
    assert(m_queueman);

    if (msg_type != NetMsgType::DSQUEUE) {
        return {};
    }
    if (!m_mn_sync.IsBlockchainSynced()) return {};

    assert(m_mn_metaman.IsValid());

    CCoinJoinQueue dsq;
    vRecv >> dsq;

    MessageProcessingResult ret{};
    ret.m_to_erase = CInv{MSG_DSQ, dsq.GetHash()};

    const auto tip_mn_list = m_dmnman.GetListAtChainTip();

    {
        LOCK(cs_ProcessDSQueue);

        const auto validation = CoinJoin::ValidateDSQueue(dsq, tip_mn_list, [this, from](const CCoinJoinQueue& q) {
            if (m_queueman->HasQueue(q.GetHash())) return true;
            if (m_queueman->HasQueueFromMasternode(q.masternodeOutpoint, q.fReady)) {
                // no way the same mn can send another dsq with the same readiness this soon
                LogPrint(BCLog::COINJOIN, /* Continued */
                         "DSQUEUE -- Peer %d is sending WAY too many dsq messages for a masternode with collateral %s\n",
                         from, q.masternodeOutpoint.ToStringShort());
                return true;
            }
            return false;
        });
        if (validation.misbehavior > 0) {
            ret.m_error = MisbehavingError{validation.misbehavior};
            return ret;
        }
        const auto& dmn = validation.dmn;
        if (!dmn) return ret;

        // if the queue is ready, submit if we can
        if (dsq.fReady && ForAnyCJClientMan([&connman, &dmn](CCoinJoinClientManager& clientman) {
                return clientman.TrySubmitDenominate(dmn->proTxHash, connman);
            })) {
            LogPrint(BCLog::COINJOIN, "DSQUEUE -- CoinJoin queue is ready, masternode=%s, queue=%s\n",
                     dmn->proTxHash.ToString(), dsq.ToString());
            return ret;
        } else {
            if (m_mn_metaman.IsMixingThresholdExceeded(dmn->proTxHash, tip_mn_list.GetCounts().enabled())) {
                LogPrint(BCLog::COINJOIN, "DSQUEUE -- Masternode %s is sending too many dsq messages\n",
                         dmn->proTxHash.ToString());
                return ret;
            }
            m_mn_metaman.AllowMixing(dmn->proTxHash);

            LogPrint(BCLog::COINJOIN, "DSQUEUE -- new CoinJoin queue, masternode=%s, queue=%s\n",
                     dmn->proTxHash.ToString(), dsq.ToString());

            ForAnyCJClientMan(
                [&dsq](const CCoinJoinClientManager& clientman) { return clientman.MarkAlreadyJoinedQueueAsTried(dsq); });

            m_queueman->AddQueue(dsq);
        }
    } // cs_ProcessDSQueue
    return ret;
}

std::unique_ptr<CJWalletManager> CJWalletManager::make(ChainstateManager& chainman, CDeterministicMNManager& dmnman,
                                                       CMasternodeMetaMan& mn_metaman, CTxMemPool& mempool,
                                                       const CMasternodeSync& mn_sync,
                                                       const llmq::CInstantSendManager& isman, bool relay_txes)
{
    return std::make_unique<CJWalletManagerImpl>(chainman, dmnman, mn_metaman, mempool, mn_sync, isman, relay_txes);
}
#endif // ENABLE_WALLET
