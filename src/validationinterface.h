// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VALIDATIONINTERFACE_H
#define BITCOIN_VALIDATIONINTERFACE_H

#include <boost/signals2/signal.hpp>
#include <memory>

#include "primitives/transaction.h" // CTransaction(Ref)

class CBlock;
class CBlockIndex;
struct CBlockLocator;
class CConnman;
class CReserveScript;
class CValidationInterface;
class CValidationState;
class CGovernanceVote;
class CGovernanceObject;
class CDeterministicMNList;
class CDeterministicMNListDiff;
class uint256;

namespace llmq {
    class CChainLockSig;
    class CInstantSendLock;
}

// These functions dispatch to one or all registered wallets

/** Register a wallet to receive updates from core */
void RegisterValidationInterface(CValidationInterface* pwalletIn);
/** Unregister a wallet from core */
void UnregisterValidationInterface(CValidationInterface* pwalletIn);
/** Unregister all wallets from core */
void UnregisterAllValidationInterfaces();

class CValidationInterface {
protected:
    virtual void AcceptedBlockHeader(const CBlockIndex *pindexNew) {}
    virtual void NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload) {}
    virtual void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) {}
    virtual void TransactionAddedToMempool(const CTransactionRef &ptxn) {}
    virtual void BlockConnected(const std::shared_ptr<const CBlock> &block, const CBlockIndex *pindex, const std::vector<CTransactionRef> &txnConflicted) {}
    virtual void BlockDisconnected(const std::shared_ptr<const CBlock> &block, const CBlockIndex *pindexDisconnected) {}
    virtual void NotifyTransactionLock(const CTransaction &tx, const llmq::CInstantSendLock& islock) {}
    virtual void NotifyChainLock(const CBlockIndex* pindex, const llmq::CChainLockSig& clsig) {}
    virtual void NotifyGovernanceVote(const CGovernanceVote &vote) {}
    virtual void NotifyGovernanceObject(const CGovernanceObject &object) {}
    virtual void NotifyInstantSendDoubleSpendAttempt(const CTransaction &currentTx, const CTransaction &previousTx) {}
    virtual void NotifyMasternodeListChanged(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff) {}
    virtual void SetBestChain(const CBlockLocator &locator) {}
    virtual bool UpdatedTransaction(const uint256 &hash) { return false;}
    virtual void Inventory(const uint256 &hash) {}
    virtual void ResendWalletTransactions(int64_t nBestBlockTime, CConnman* connman) {}
    virtual void BlockChecked(const CBlock&, const CValidationState&) {}
    virtual void GetScriptForMining(std::shared_ptr<CReserveScript>&) {};
    virtual void NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock>& block) {}
    friend void ::RegisterValidationInterface(CValidationInterface*);
    friend void ::UnregisterValidationInterface(CValidationInterface*);
    friend void ::UnregisterAllValidationInterfaces();
};

struct CMainSignals {
    /** Notifies listeners of accepted block header */
    boost::signals2::signal<void (const CBlockIndex *)> AcceptedBlockHeader;
    /** Notifies listeners of updated block header tip */
    boost::signals2::signal<void (const CBlockIndex *, bool fInitialDownload)> NotifyHeaderTip;
    /** Notifies listeners of updated block chain tip */
    boost::signals2::signal<void (const CBlockIndex *, const CBlockIndex *, bool fInitialDownload)> UpdatedBlockTip;
    /** Notifies listeners of a transaction having been added to mempool. */
    boost::signals2::signal<void (const CTransactionRef &)> TransactionAddedToMempool;
    /**
     * Notifies listeners of a block being connected.
     * Provides a vector of transactions evicted from the mempool as a result.
     */
    boost::signals2::signal<void (const std::shared_ptr<const CBlock> &, const CBlockIndex *pindex, const std::vector<CTransactionRef> &)> BlockConnected;
    /** Notifies listeners of a block being disconnected */
    boost::signals2::signal<void (const std::shared_ptr<const CBlock> &, const CBlockIndex* pindexDisconnected)> BlockDisconnected;
    /** Notifies listeners of an updated transaction lock without new data. */
    boost::signals2::signal<void (const CTransaction &, const llmq::CInstantSendLock& islock)> NotifyTransactionLock;
    /** Notifies listeners of a ChainLock. */
    boost::signals2::signal<void (const CBlockIndex* pindex, const llmq::CChainLockSig& clsig)> NotifyChainLock;
    /** Notifies listeners of a new governance vote. */
    boost::signals2::signal<void (const CGovernanceVote &)> NotifyGovernanceVote;
    /** Notifies listeners of a new governance object. */
    boost::signals2::signal<void (const CGovernanceObject &)> NotifyGovernanceObject;
    /** Notifies listeners of a attempted InstantSend double spend*/
    boost::signals2::signal<void(const CTransaction &currentTx, const CTransaction &previousTx)> NotifyInstantSendDoubleSpendAttempt;
    /** Notifies listeners that the MN list changed */
    boost::signals2::signal<void(bool undo, const CDeterministicMNList& oldMNList, const CDeterministicMNListDiff& diff)> NotifyMasternodeListChanged;
    /** Notifies listeners of an updated transaction without new data (for now: a coinbase potentially becoming visible). */
    boost::signals2::signal<bool (const uint256 &)> UpdatedTransaction;
    /** Notifies listeners of a new active block chain. */
    boost::signals2::signal<void (const CBlockLocator &)> SetBestChain;
    /** Notifies listeners about an inventory item being seen on the network. */
    boost::signals2::signal<void (const uint256 &)> Inventory;
    /** Tells listeners to broadcast their data. */
    boost::signals2::signal<void (int64_t nBestBlockTime, CConnman* connman)> Broadcast;
    /**
     * Notifies listeners of a block validation result.
     * If the provided CValidationState IsValid, the provided block
     * is guaranteed to be the current best block at the time the
     * callback was generated (not necessarily now)
     */
    boost::signals2::signal<void (const CBlock&, const CValidationState&)> BlockChecked;
    /** Notifies listeners that a key for mining is required (coinbase) */
    boost::signals2::signal<void (std::shared_ptr<CReserveScript>&)> ScriptForMining;
    /**
     * Notifies listeners that a block which builds directly on our current tip
     * has been received and connected to the headers tree, though not validated yet */
    boost::signals2::signal<void (const CBlockIndex *, const std::shared_ptr<const CBlock>&)> NewPoWValidBlock;
};

CMainSignals& GetMainSignals();

#endif // BITCOIN_VALIDATIONINTERFACE_H
