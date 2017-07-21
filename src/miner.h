// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MINER_H
#define BITCOIN_MINER_H

#include "primitives/block.h"
#include "txmempool.h"

#include <memory>
#include <stdint.h>

class CBlockIndex;
class CChainParams;
class CReserveKey;
class CScript;
class CWallet;

namespace Consensus
{
struct Params;
};

static const bool DEFAULT_PRINTPRIORITY = false;

struct CBlockTemplate
{
    CBlock block;
    std::vector<CAmount> vTxFees;
    std::vector<int64_t> vTxSigOps;
};

/** Generate a new block, without valid proof-of-work */
class BlockAssembler
{
private:
    const CChainParams &chainparams;

    // Configuration parameters for the block size
    uint64_t nBlockMaxSize, nBlockMinSize;

    // Information on the current status of the block
    uint64_t nBlockSize;
    uint64_t nBlockTx;
    unsigned int nBlockSigOps;
    CAmount nFees;
    CTxMemPool::setEntries inBlock;

    // Chain context for the block
    int nHeight;
    int64_t nLockTimeCutoff;

    // Variables used for addScoreTxs and addPriorityTxs
    int lastFewTxs;
    bool blockFinished;

    bool buip055ChainBlock;

public:
    BlockAssembler(const CChainParams &chainparams);
    /** Construct a new block template with coinbase to scriptPubKeyIn */
    CBlockTemplate *CreateNewBlock(const CScript &scriptPubKeyIn);

private:
    // utility functions
    /** Clear the block's state and prepare for assembling a new block */
    void resetBlock(const CScript &scriptPubKeyIn);
    /** Add a tx to the block */
    void AddToBlock(CBlockTemplate *, CTxMemPool::txiter iter);

    // Methods for how to add transactions to a block.
    /** Add transactions based on modified feerate */
    void addScoreTxs(CBlockTemplate *);
    /** Add transactions based on tx "priority" */
    void addPriorityTxs(CBlockTemplate *);

    // helper function for addScoreTxs and addPriorityTxs
    bool IsIncrementallyGood(uint64_t nExtraSize, unsigned int nExtraSigOps);
    /** Test if tx will still "fit" in the block */
    bool TestForBlock(CTxMemPool::txiter iter);
    /** Test if tx still has unconfirmed parents not yet in block */
    bool isStillDependent(CTxMemPool::txiter iter);
    /** Bytes to reserve for coinbase and block header */
    uint64_t reserveBlockSize(const CScript &scriptPubKeyIn);
    /** Internal method to construct a new block template */
    CBlockTemplate *CreateNewBlock(const CScript &scriptPubKeyIn, bool blockstreamCoreCompatible);
    /** Constructs a coinbase transaction */
    CMutableTransaction coinbaseTx(const CScript &scriptPubKeyIn, int nHeight, CAmount nValue);
};

/** Modify the extranonce in a block */
void IncrementExtraNonce(CBlock *pblock, unsigned int &nExtraNonce);
int64_t UpdateTime(CBlockHeader *pblock, const Consensus::Params &consensusParams, const CBlockIndex *pindexPrev);

#endif // BITCOIN_MINER_H
