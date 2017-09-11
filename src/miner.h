// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MINER_H
#define BITCOIN_MINER_H

#include "primitives/block.h"
#include "txmempool.h"

#include <stdint.h>
#include <memory>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>

class CBlockIndex;
class CChainParams;
class CScript;

namespace Consensus { struct Params; };

static const bool DEFAULT_PRINTPRIORITY = false;

struct CBlockTemplate
{
    CBlock block;
    std::vector<CAmount> vTxFees;
    std::vector<int64_t> vTxSigOpsCost;
    std::vector<unsigned char> vchCoinbaseCommitment;
};

// Container for tracking updates to ancestor feerate as we include (parent)
// transactions in a block
struct CTxMemPoolModifiedEntry {
    explicit CTxMemPoolModifiedEntry(CTxMemPool::txiter entry)
    {
        iter = entry;
        nSizeWithAncestors = entry->GetSizeWithAncestors();
        nModFeesWithAncestors = entry->GetModFeesWithAncestors();
        nSigOpCostWithAncestors = entry->GetSigOpCostWithAncestors();
    }

    CTxMemPool::txiter iter;
    uint64_t nSizeWithAncestors;
    CAmount nModFeesWithAncestors;
    int64_t nSigOpCostWithAncestors;
};

/** Comparator for CTxMemPool::txiter objects.
 *  It simply compares the internal memory address of the CTxMemPoolEntry object
 *  pointed to. This means it has no meaning, and is only useful for using them
 *  as key in other indexes.
 */
struct CompareCTxMemPoolIter {
    bool operator()(const CTxMemPool::txiter& a, const CTxMemPool::txiter& b) const
    {
        return &(*a) < &(*b);
    }
};

struct modifiedentry_iter {
    typedef CTxMemPool::txiter result_type;
    result_type operator() (const CTxMemPoolModifiedEntry &entry) const
    {
        return entry.iter;
    }
};

// This matches the calculation in CompareTxMemPoolEntryByAncestorFee,
// except operating on CTxMemPoolModifiedEntry.
// TODO: refactor to avoid duplication of this logic.
struct CompareModifiedEntry {
    bool operator()(const CTxMemPoolModifiedEntry &a, const CTxMemPoolModifiedEntry &b)
    {
        double f1 = (double)a.nModFeesWithAncestors * b.nSizeWithAncestors;
        double f2 = (double)b.nModFeesWithAncestors * a.nSizeWithAncestors;
        if (f1 == f2) {
            return CTxMemPool::CompareIteratorByHash()(a.iter, b.iter);
        }
        return f1 > f2;
    }
};

// A comparator that sorts transactions based on number of ancestors.
// This is sufficient to sort an ancestor package in an order that is valid
// to appear in a block.
struct CompareTxIterByAncestorCount {
    bool operator()(const CTxMemPool::txiter &a, const CTxMemPool::txiter &b)
    {
        if (a->GetCountWithAncestors() != b->GetCountWithAncestors())
            return a->GetCountWithAncestors() < b->GetCountWithAncestors();
        return CTxMemPool::CompareIteratorByHash()(a, b);
    }
};

typedef boost::multi_index_container<
    CTxMemPoolModifiedEntry,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            modifiedentry_iter,
            CompareCTxMemPoolIter
        >,
        // sorted by modified ancestor fee rate
        boost::multi_index::ordered_non_unique<
            // Reuse same tag from CTxMemPool's similar index
            boost::multi_index::tag<ancestor_score>,
            boost::multi_index::identity<CTxMemPoolModifiedEntry>,
            CompareModifiedEntry
        >
    >
> indexed_modified_transaction_set;

typedef indexed_modified_transaction_set::nth_index<0>::type::iterator modtxiter;
typedef indexed_modified_transaction_set::index<ancestor_score>::type::iterator modtxscoreiter;

struct update_for_parent_inclusion
{
    explicit update_for_parent_inclusion(CTxMemPool::txiter it) : iter(it) {}

    void operator() (CTxMemPoolModifiedEntry &e)
    {
        e.nModFeesWithAncestors -= iter->GetFee();
        e.nSizeWithAncestors -= iter->GetTxSize();
        e.nSigOpCostWithAncestors -= iter->GetSigOpCost();
    }

    CTxMemPool::txiter iter;
};

struct update_for_parent_removal
{
    update_for_parent_removal(CTxMemPool::txiter it) : iter(it) {}

    void operator() (CTxMemPoolModifiedEntry &e)
    {
        e.nModFeesWithAncestors += iter->GetFee();
        e.nSizeWithAncestors += iter->GetTxSize();
        e.nSigOpCostWithAncestors += iter->GetSigOpCost();
    }

    CTxMemPool::txiter iter;
};

/** Generate a new block, without valid proof-of-work */
class BlockAssembler
{
private:
    // The state associated with construction of a block.
    struct WorkingState {
        WorkingState() :
            pblocktemplate(new CBlockTemplate()),
            pblock(&pblocktemplate->block),
            // Reserve space for coinbase tx
            nBlockWeight(4000),
            nBlockSize(1000),
            nBlockSigOpsCost(400),
            // These counters do not include coinbase tx
            nBlockTx(0),
            nFees(0),
            nModifiedFees(0) { }

        WorkingState(const WorkingState &ws) :
            pblocktemplate(new CBlockTemplate(*ws.pblocktemplate)),
            pblock(&pblocktemplate->block),
            nBlockWeight(ws.nBlockWeight),
            nBlockSize(ws.nBlockSize),
            nBlockSigOpsCost(ws.nBlockSigOpsCost),
            nBlockTx(ws.nBlockTx),
            nFees(ws.nFees),
            nModifiedFees(ws.nModifiedFees),
            inBlock(ws.inBlock) { }

        // The constructed block template
        std::unique_ptr<CBlockTemplate> pblocktemplate;
        // A convenience pointer that always refers to the CBlock in pblocktemplate
        CBlock* pblock;

        // Information on the current status of the block
        uint64_t nBlockWeight;
        uint64_t nBlockSize;
        uint64_t nBlockSigOpsCost;
        uint64_t nBlockTx;
        CAmount nFees;
        CAmount nModifiedFees;
        CTxMemPool::setEntries inBlock;
    };

    // Configuration parameters for the block size
    bool fIncludeWitness;
    unsigned int nBlockMaxWeight, nBlockMaxSize;
    // Exclude transactions received in nRecentTxWindow if the income
    // reduction from doing so is below (1-dRecentTxStaleRate).
    // See comment above CreateNewBlock().
    int64_t nRecentTxWindow;
    double dIncomeThreshold;
    bool fNeedSizeAccounting;
    CFeeRate blockMinFeeRate;

    // Chain context for the block
    int nHeight;
    int64_t nLockTimeCutoff;
    const CChainParams& chainparams;

public:
    struct Options {
        Options();
        size_t nBlockMaxWeight;
        size_t nBlockMaxSize;
        CFeeRate blockMinFeeRate;
        int64_t nRecentTxWindow;
        double dRecentTxStaleRate;
    };

    explicit BlockAssembler(const CChainParams& params);
    BlockAssembler(const CChainParams& params, const Options& options);

    /** Construct a new block template with coinbase to scriptPubKeyIn */
    std::unique_ptr<CBlockTemplate> CreateNewBlock(const CScript& scriptPubKeyIn, bool fMineWitnessTx=true);

private:
    // utility functions
    /** Add a tx to the block */
    void AddToBlock(WorkingState &workState, CTxMemPool::txiter iter) const;
    /** Remove recent transactions from a block, including any descendants */
    void RemoveRecentTransactionsFromBlockAndUpdatePackages(WorkingState &workState, int64_t timeCutoff, indexed_modified_transaction_set &mapModifiedTx);

    // Methods for how to add transactions to a block.
    /** Add transactions based on feerate including unconfirmed ancestors
      * Increments nPackagesSelected / nDescendantsUpdated with corresponding
      * statistics from the package selection (for logging statistics).
     *  mapModifiedTx will track the updated ancestor feerate score of
     *  not-in-block transactions that have parents in the block */
    void addPackageTxs(WorkingState &workState, int &nPackagesSelected, int &nDescendantsUpdated, int64_t nTimeCutoff, indexed_modified_transaction_set &mapModifiedTx) const;

    // helper functions for addPackageTxs()
    /** Remove confirmed (inBlock) entries from given set */
    void onlyUnconfirmed(WorkingState &workState, CTxMemPool::setEntries& testSet) const;
    /** Test if a new package would "fit" in the block */
    bool TestPackage(const WorkingState &workState, uint64_t packageSize, int64_t packageSigOpsCost) const;
    /** Perform checks on each transaction in a package:
      * locktime, premature-witness, serialized size (if necessary)
      * These checks should always succeed, and they're here
      * only as an extra check in case of suboptimal node configuration */
    bool TestPackageTransactions(WorkingState &workState, const CTxMemPool::setEntries& package) const;
    /** Return true if given transaction from mapTx has already been evaluated,
      * or if the transaction's cached data in mapTx is incorrect. */
    bool SkipMapTxEntry(WorkingState &workState, CTxMemPool::txiter it, indexed_modified_transaction_set &mapModifiedTx, CTxMemPool::setEntries &failedTx) const;
    /** Sort the package in an order that is valid to appear in a block */
    void SortForBlock(const CTxMemPool::setEntries& package, CTxMemPool::txiter entry, std::vector<CTxMemPool::txiter>& sortedEntries) const;
    /** Add descendants of given transactions to mapModifiedTx with ancestor
      * state updated assuming given transactions are inBlock. Returns number
      * of updated descendants. */
    int UpdatePackagesForAdded(const CTxMemPool::setEntries& alreadyAdded, indexed_modified_transaction_set &mapModifiedTx) const;
};

/** Modify the extranonce in a block */
void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce);
int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev);

#endif // BITCOIN_MINER_H
