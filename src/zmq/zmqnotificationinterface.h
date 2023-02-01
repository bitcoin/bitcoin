// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H
#define SYSCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H

#include <primitives/transaction.h>
#include <validationinterface.h>

#include <cstdint>
#include <list>
#include <memory>

class CBlock;
class CBlockIndex;
class CZMQAbstractNotifier;
// SYSCOIN
class CNEVMBlock;
class CNEVMHeader;
class CBlock;
class uint256;
class ChainstateManager;
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
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, ChainstateManager& chainman, bool fInitialDownload) override;
    // SYSCOIN
    void NotifyGovernanceVote(const uint256& vote) override;
    void NotifyGovernanceObject(const uint256& object) override;
    void NotifyNEVMBlockConnect(const CNEVMHeader &evmBlock, const CBlock& block, std::string &state, const uint256& nBlockHash, NEVMDataVec &NEVMDataVecOut, const uint32_t& nHeight, bool bSkipValidation) override;
    void NotifyNEVMBlockDisconnect(std::string &state, const uint256& nBlockHash) override;
    void NotifyGetNEVMBlockInfo(uint64_t &nHeight, std::string& state) override;
    void NotifyGetNEVMBlock(CNEVMBlock &evmBlock, std::string& state) override;
    void NotifyNEVMComms(const std::string& commMessage, bool &bResponse) override;
private:
    CZMQNotificationInterface();

    void* pcontext{nullptr};
       // SYSCOIN
    void *pcontextsub{nullptr};
    std::list<std::unique_ptr<CZMQAbstractNotifier>> notifiers;
};

extern CZMQNotificationInterface* g_zmq_notification_interface;

#endif // SYSCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H
