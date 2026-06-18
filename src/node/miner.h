// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_MINER_H
#define BITCOIN_NODE_MINER_H

#include <consensus/amount.h>
#include <node/mining_types.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <threadsafety.h>
#include <txmempool.h>
#include <util/feefrac.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

class CBlockIndex;
class CChainParams;
class Chainstate;
class ChainstateManager;

namespace Consensus {
struct Params;
} // namespace Consensus
namespace node {

struct TemplateChunk {
    FeePerWeight feerate;
    std::vector<Wtxid> chunk_wtxids;
    int64_t weight; //!< Actual serialized weight (may differ from feerate.size which is sigops-adjusted).
    int64_t sigops_cost;
};

struct CBlockTemplate
{
    CBlock block;
    // Fees per transaction, not including coinbase transaction (unlike CBlock::vtx).
    std::vector<CAmount> vTxFees;
    // Sigops per transaction, not including coinbase transaction (unlike CBlock::vtx).
    std::vector<int64_t> vTxSigOpsCost;
    /* Chunks included in the block template, ordered by selection sequence.
     * Each entry records the chunk feerate, unadjusted weight, sigops cost,
     * and the wtxids of the transactions in the chunk. */
    std::vector<TemplateChunk> m_template_chunks;
    /*
     * Template containing all coinbase transaction fields that are set by our
     * miner code.
     */
    CoinbaseTx m_coinbase_tx;
    //! Height of the block template.
    int m_height{0};
    //! Median-time-past cutoff used for transaction finality checks.
    int64_t m_lock_time_cutoff{0};
};

/** Generate a new block, without valid proof-of-work */
class BlockAssembler
{
private:
    // The constructed block template
    std::unique_ptr<CBlockTemplate> pblocktemplate;

    // Information on the current status of the block
    uint64_t nBlockWeight;
    uint64_t nBlockTx;
    uint64_t nBlockSigOpsCost;
    CAmount nFees;

    // Chain context for the block
    int nHeight;
    int64_t m_lock_time_cutoff;

    const CChainParams& chainparams;
    const CTxMemPool* const m_mempool;
    Chainstate& m_chainstate;

public:
    explicit BlockAssembler(Chainstate& chainstate,
                            const CTxMemPool* mempool,
                            BlockCreateOptions create_options);

    /** Construct a new block template */
    std::unique_ptr<CBlockTemplate> CreateNewBlock();

    /** The number of transactions in the last assembled block (excluding coinbase transaction) */
    inline static std::optional<int64_t> m_last_block_num_txs{};
    /** The weight of the last assembled block (including reserved weight for block header, txs count and coinbase tx) */
    inline static std::optional<int64_t> m_last_block_weight{};

private:
    const BlockCreateOptions m_options;

    // utility functions
    /** Clear the block's state and prepare for assembling a new block */
    void resetBlock();
    /** Add a tx to the block */
    void AddToBlock(const CTxMemPoolEntry& entry);

    // Methods for how to add transactions to a block.
    /** Add transactions based on chunk feerate
      *
      * @pre BlockAssembler::m_mempool must not be nullptr
    */
    void addChunks() EXCLUSIVE_LOCKS_REQUIRED(m_mempool->cs);

    // helper functions for addChunks()
    /** Test if a new chunk would "fit" in the block */
    bool TestChunkBlockLimits(FeePerWeight chunk_feerate, int64_t chunk_sigops_cost) const;
    /** Perform locktime checks on each transaction in a chunk:
      * This check should always succeed, and is here
      * only as an extra check in case of a bug */
    bool TestChunkTransactions(const std::vector<CTxMemPoolEntryRef>& txs) const;
};

/**
 * Get the minimum time a miner should use in the next block. This always
 * accounts for the BIP94 timewarp rule, so does not necessarily reflect the
 * consensus limit.
 */
int64_t GetMinimumTime(const CBlockIndex* pindexPrev, int64_t difficulty_adjustment_interval);

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev);

/** Update an old GenerateCoinbaseCommitment from CreateNewBlock after the block txs have changed */
void RegenerateCommitments(CBlock& block, ChainstateManager& chainman);

/* Compute the block's merkle root, insert or replace the coinbase transaction and the merkle root into the block */
void AddMerkleRootAndCoinbase(CBlock& block, CTransactionRef coinbase, uint32_t version, uint32_t timestamp, uint32_t nonce);
} // namespace node

#endif // BITCOIN_NODE_MINER_H
