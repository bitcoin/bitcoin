// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DSNOTIFICATIONINTERFACE_H
#define BITCOIN_DSNOTIFICATIONINTERFACE_H

#include <validationinterface.h>

class CConnman;
class CDeterministicMNManager;
class CGovernanceManager;
class CMasternodeSync;
struct CJContext;
struct LLMQContext;

class CDSNotificationInterface : public CValidationInterface
{
public:
    explicit CDSNotificationInterface(CConnman& _connman,
                                      CMasternodeSync& _mn_sync, const std::unique_ptr<CDeterministicMNManager>& _dmnman,
                                      CGovernanceManager& _govman, const std::unique_ptr<LLMQContext>& _llmq_ctx,
                                      const std::unique_ptr<CJContext>& _cj_ctx);
    virtual ~CDSNotificationInterface() = default;

    // a small helper to initialize current block height in sub-modules on startup
    void InitializeCurrentBlockTip();

protected:
    // CValidationInterface
    void AcceptedBlockHeader(const CBlockIndex *pindexNew) override;
    void NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload) override;
    void SynchronousUpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;
    void TransactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime) override;
    void TransactionRemovedFromMempool(const CTransactionRef& ptx, MemPoolRemovalReason reason) override;
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex) override;
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected) override;
    void NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff) override;
    void NotifyChainLock(const CBlockIndex* pindex, const std::shared_ptr<const llmq::CChainLockSig>& clsig) override;

private:
    CConnman& connman;

    CMasternodeSync& m_mn_sync;
    const std::unique_ptr<CDeterministicMNManager>& dmnman;
    CGovernanceManager& govman;

    const std::unique_ptr<LLMQContext>& llmq_ctx;
    const std::unique_ptr<CJContext>& cj_ctx;
};

#endif // BITCOIN_DSNOTIFICATIONINTERFACE_H
