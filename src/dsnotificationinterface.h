// Copyright (c) 2015-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DSNOTIFICATIONINTERFACE_H
#define BITCOIN_DSNOTIFICATIONINTERFACE_H

#include <validationinterface.h>

class CConnman;
class CDSTXManager;
class CDeterministicMNManager;
class CGovernanceManager;
class ChainstateManager;
class CMasternodeSync;
struct LLMQContext;

class CDSNotificationInterface : public CValidationInterface
{
public:
    CDSNotificationInterface() = delete;
    CDSNotificationInterface(const CDSNotificationInterface&) = delete;
    CDSNotificationInterface& operator=(const CDSNotificationInterface&) = delete;
    explicit CDSNotificationInterface(CConnman& connman,
                                      CDSTXManager& dstxman,
                                      CMasternodeSync& mn_sync,
                                      CGovernanceManager& govman,
                                      const ChainstateManager& chainman,
                                      const std::unique_ptr<CDeterministicMNManager>& dmnman,
                                      const std::unique_ptr<LLMQContext>& llmq_ctx);
    virtual ~CDSNotificationInterface();

    // a small helper to initialize current block height in sub-modules on startup
    void InitializeCurrentBlockTip();

protected:
    // CValidationInterface
    void AcceptedBlockHeader(const CBlockIndex *pindexNew) override;
    void NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload) override;
    void SynchronousUpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;
    void TransactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime, uint64_t mempool_sequence) override;
    void TransactionRemovedFromMempool(const CTransactionRef& ptx, MemPoolRemovalReason reason,
                                       uint64_t mempool_sequence) override;
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex) override;
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected) override;
    void NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff) override;
    void NotifyChainLock(const CBlockIndex* pindex, const std::shared_ptr<const chainlock::ChainLockSig>& clsig) override;

private:
    CConnman& m_connman;
    CDSTXManager& m_dstxman;
    CMasternodeSync& m_mn_sync;
    CGovernanceManager& m_govman;
    const ChainstateManager& m_chainman;
    const std::unique_ptr<CDeterministicMNManager>& m_dmnman;
    const std::unique_ptr<LLMQContext>& m_llmq_ctx;
};

extern std::unique_ptr<CDSNotificationInterface> g_ds_notification_interface;

#endif // BITCOIN_DSNOTIFICATIONINTERFACE_H
