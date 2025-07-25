// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H
#define BITCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H

#include <validationinterface.h>
#include <list>
#include <memory>

class CBlockIndex;
class CZMQAbstractNotifier;

class CZMQNotificationInterface final : public CValidationInterface
{
public:
    virtual ~CZMQNotificationInterface();

    std::list<const CZMQAbstractNotifier*> GetActiveNotifiers() const;

    static std::unique_ptr<CZMQNotificationInterface> Create();

protected:
    bool Initialize();
    void Shutdown();

    // CValidationInterface
    void TransactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime, uint64_t mempool_sequence) override;
    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) override;
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected) override;
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected) override;
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;
    void NotifyChainLock(const CBlockIndex *pindex, const std::shared_ptr<const chainlock::ChainLockSig>& clsig) override;
    void NotifyTransactionLock(const CTransactionRef &tx, const std::shared_ptr<const instantsend::InstantSendLock>& islock) override;
    void NotifyGovernanceVote(const std::shared_ptr<CDeterministicMNList>& tip_mn_list, const std::shared_ptr<const CGovernanceVote>& vote) override;
    void NotifyGovernanceObject(const std::shared_ptr<const Governance::Object>& object) override;
    void NotifyInstantSendDoubleSpendAttempt(const CTransactionRef& currentTx, const CTransactionRef& previousTx) override;
    void NotifyRecoveredSig(const std::shared_ptr<const llmq::CRecoveredSig>& sig) override;

private:
    CZMQNotificationInterface();

    void *pcontext;
    std::list<std::unique_ptr<CZMQAbstractNotifier>> notifiers;
};

extern std::unique_ptr<CZMQNotificationInterface> g_zmq_notification_interface;

#endif // BITCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H
