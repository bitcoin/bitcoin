// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H
#define SYSCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H

#include <validationinterface.h>
#include <list>
#include <memory>

class CBlockIndex;
class CZMQAbstractNotifier;
// SYSCOIN
class CNEVMBlock;
class uint256;
class CZMQNotificationInterface final : public CValidationInterface
{
public:
    virtual ~CZMQNotificationInterface();

    std::list<const CZMQAbstractNotifier*> GetActiveNotifiers() const;

    static CZMQNotificationInterface* Create();

protected:
    bool Initialize();
    void Shutdown();

    // CValidationInterface
    void TransactionAddedToMempool(const CTransactionRef& tx, uint64_t mempool_sequence) override;
    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) override;
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected) override;
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected) override;
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;
    // SYSCOIN
    void NotifyGovernanceVote(const std::shared_ptr<const CGovernanceVote>& vote) override;
    void NotifyGovernanceObject(const std::shared_ptr<const CGovernanceObject>& object) override;
    void NotifyEVMBlockConnect(const CNEVMBlock &evmBlock, BlockValidationState &state, const uint256& nBlockHash, const bool bWaitForResponse) override;
    void NotifyEVMBlockDisconnect(const CNEVMBlock &evmBlock, BlockValidationState &state, const uint256& nBlockHash, const bool bWaitForResponse) override;
    void NotifyGetNEVMBlock(CNEVMBlock &evmBlock, BlockValidationState& state) override;
private:
    CZMQNotificationInterface();

    void *pcontext;
    // SYSCOIN
    void *pcontextsub;
    std::list<std::unique_ptr<CZMQAbstractNotifier>> notifiers;
};

extern CZMQNotificationInterface* g_zmq_notification_interface;

#endif // SYSCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H
