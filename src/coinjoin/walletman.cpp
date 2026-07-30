// Copyright (c) 2023-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <coinjoin/walletman.h>

#include <net.h>
#include <scheduler.h>
#include <streams.h>

#include <evo/deterministicmns.h>

#ifdef ENABLE_WALLET
#include <coinjoin/client.h>
#endif // ENABLE_WALLET

#include <memory>

#ifdef ENABLE_WALLET
class CJWalletManagerImpl final : public CJWalletManager
{
public:
    CJWalletManagerImpl(ChainstateManager& chainman, CDeterministicMNManager& dmnman, CMasternodeMetaMan& mn_metaman,
                        CTxMemPool& mempool, const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman,
                        bool relay_txes);
    virtual ~CJWalletManagerImpl() = default;

public:
    void Schedule(CConnman& connman, CScheduler& scheduler) override;

public:
    bool hasQueue(const uint256& hash) const override;
    bool doForClient(const std::string& name, const std::function<void(CCoinJoinClientManager&)>& func) override;
    MessageProcessingResult processMessage(CNode& peer, CChainState& chainstate, CConnman& connman, CTxMemPool& mempool,
                                           std::string_view msg_type, CDataStream& vRecv) override;
    std::optional<CCoinJoinQueue> getQueueFromHash(const uint256& hash) const override;
    std::optional<int> getQueueSize() const override;
    std::vector<CDeterministicMNCPtr> getMixingMasternodes() override;
    void addWallet(const std::shared_ptr<wallet::CWallet>& wallet) override;
    void removeWallet(const std::string& name) override;
    void flushWallet(const std::string& name) override;

protected:
    // CValidationInterface
    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override;

private:
    const bool m_relay_txes;

    CoinJoinWalletManager walletman;
    const std::unique_ptr<CCoinJoinClientQueueManager> queueman;
};

CJWalletManagerImpl::CJWalletManagerImpl(ChainstateManager& chainman, CDeterministicMNManager& dmnman,
                                         CMasternodeMetaMan& mn_metaman, CTxMemPool& mempool,
                                         const CMasternodeSync& mn_sync, const llmq::CInstantSendManager& isman,
                                         bool relay_txes) :
    m_relay_txes{relay_txes},
    walletman{chainman, dmnman, mn_metaman, mempool, mn_sync, isman, queueman},
    queueman{m_relay_txes ? std::make_unique<CCoinJoinClientQueueManager>(walletman, dmnman, mn_metaman, mn_sync) : nullptr}
{
}

void CJWalletManagerImpl::Schedule(CConnman& connman, CScheduler& scheduler)
{
    if (!m_relay_txes) return;
    scheduler.scheduleEvery(std::bind(&CCoinJoinClientQueueManager::DoMaintenance, std::ref(*queueman)),
                            std::chrono::seconds{1});
    scheduler.scheduleEvery(std::bind(&CoinJoinWalletManager::DoMaintenance, std::ref(walletman), std::ref(connman)),
                            std::chrono::seconds{1});
}

void CJWalletManagerImpl::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;

    walletman.ForEachCJClientMan(
        [&pindexNew](std::unique_ptr<CCoinJoinClientManager>& clientman) { clientman->UpdatedBlockTip(pindexNew); });
}

bool CJWalletManagerImpl::hasQueue(const uint256& hash) const
{
    if (queueman) {
        return queueman->HasQueue(hash);
    }
    return false;
}

bool CJWalletManagerImpl::doForClient(const std::string& name, const std::function<void(CCoinJoinClientManager&)>& func)
{
    return walletman.DoForClient(name, func);
}

MessageProcessingResult CJWalletManagerImpl::processMessage(CNode& pfrom, CChainState& chainstate, CConnman& connman,
                                                            CTxMemPool& mempool, std::string_view msg_type,
                                                            CDataStream& vRecv)
{
    walletman.ForEachCJClientMan([&](std::unique_ptr<CCoinJoinClientManager>& clientman) {
        clientman->ProcessMessage(pfrom, chainstate, connman, mempool, msg_type, vRecv);
    });
    if (queueman) {
        return queueman->ProcessMessage(pfrom.GetId(), connman, msg_type, vRecv);
    }
    return {};
}

std::optional<CCoinJoinQueue> CJWalletManagerImpl::getQueueFromHash(const uint256& hash) const
{
    if (queueman) {
        return queueman->GetQueueFromHash(hash);
    }
    return std::nullopt;
}

std::optional<int> CJWalletManagerImpl::getQueueSize() const
{
    if (queueman) {
        return queueman->GetQueueSize();
    }
    return std::nullopt;
}

std::vector<CDeterministicMNCPtr> CJWalletManagerImpl::getMixingMasternodes()
{
    std::vector<CDeterministicMNCPtr> ret{};
    walletman.ForEachCJClientMan(
        [&](const std::unique_ptr<CCoinJoinClientManager>& clientman) { clientman->GetMixingMasternodesInfo(ret); });
    return ret;
}

void CJWalletManagerImpl::addWallet(const std::shared_ptr<wallet::CWallet>& wallet)
{
    walletman.Add(wallet);
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
    walletman.Remove(name);
}

std::unique_ptr<CJWalletManager> CJWalletManager::make(ChainstateManager& chainman, CDeterministicMNManager& dmnman,
                                                       CMasternodeMetaMan& mn_metaman, CTxMemPool& mempool,
                                                       const CMasternodeSync& mn_sync,
                                                       const llmq::CInstantSendManager& isman, bool relay_txes)
{
    return std::make_unique<CJWalletManagerImpl>(chainman, dmnman, mn_metaman, mempool, mn_sync, isman, relay_txes);
}
#endif // ENABLE_WALLET
