// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_MINER_H
#define BITCOIN_NODE_MINER_H

#include <interfaces/types.h>
#include <kernel/types.h>
#include <node/types.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <txmempool.h>
#include <util/feefrac.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index_container.hpp>

#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <utility>

class ArgsManager;
class CBlockIndex;
class CChainParams;
class CScript;

namespace Consensus { struct Params; };

using interfaces::BlockRef;
using kernel::ChainstateRole;

namespace node {
class KernelNotifications;

static const bool DEFAULT_PRINT_MODIFIED_FEE = false;
static constexpr size_t DEFAULT_BLOCK_TEMPLATE_CACHE_SIZE{10};

// Return true if current time is greater or equal to `prev_time + time_interval`, or if
// `prev_time` is greater than the current time (indicating clock moved backward; only possible in test).
static inline bool TimeIntervalElapsed(const MockableSteadyClock::time_point& prev_time, MillisecondsDouble time_interval)
{
    const auto now = MockableSteadyClock::now();
    return now < prev_time || MillisecondsDouble{now - prev_time} >= time_interval;
}

struct CBlockTemplate
{
    CBlock block;
    // Fees per transaction, not including coinbase transaction (unlike CBlock::vtx).
    std::vector<CAmount> vTxFees;
    // Sigops per transaction, not including coinbase transaction (unlike CBlock::vtx).
    std::vector<int64_t> vTxSigOpsCost;
    std::vector<unsigned char> vchCoinbaseCommitment;
    /* A vector of package fee rates, ordered by the sequence in which
     * packages are selected for inclusion in the block template.*/
    std::vector<FeePerVSize> m_package_feerates;
    /*
     * Template containing all coinbase transaction fields that are set by our
     * miner code.
     */
    CoinbaseTx m_coinbase_tx;

    /* Uses steady clock because it is monotonic and unaffected by rare system clock changes.
     * Intended for checking template staleness with respect to fee increases.
     * Uses the mockable version to allow for testing. */
    MockableSteadyClock::time_point m_creation_time;
};

using BlockTemplateRef = std::shared_ptr<const CBlockTemplate>;

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
    struct Options : BlockCreateOptions {
        // Configuration parameters for the block size
        size_t nBlockMaxWeight{DEFAULT_BLOCK_MAX_WEIGHT};
        CFeeRate blockMinFeeRate{DEFAULT_BLOCK_MIN_TX_FEE};
        // Whether to call TestBlockValidity() at the end of CreateNewBlock().
        bool test_block_validity{true};
        bool print_modified_fee{DEFAULT_PRINT_MODIFIED_FEE};
        bool operator==(const Options&) const = default;
    };

    explicit BlockAssembler(Chainstate& chainstate, const CTxMemPool* mempool, const Options& options);

    /** Construct a new block template */
    std::unique_ptr<CBlockTemplate> CreateNewBlock();

    /** The number of transactions in the last assembled block (excluding coinbase transaction) */
    inline static std::optional<int64_t> m_last_block_num_txs{};
    /** The weight of the last assembled block (including reserved weight for block header, txs count and coinbase tx) */
    inline static std::optional<int64_t> m_last_block_weight{};

private:
    const Options m_options;

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

/*
 * BlockTemplateCache provides a thread-safe interface for creating and reusing
 * block templates with configurable cache size.
 *
 * The cache stores block templates along with their respective block creation options.
 * When a block template is requested with a specific block creation option and a desired maximum age:
 * - Return the most recent cached template whose block creation option matches the request block creation option
 *   and age (the time elapsed since its creation) does not exceed the specified maximum.
 * - If no such cached template exists, create a new block template, store it in the cache along with
 *   its block creation option, and return it.
 * - When the cache exceeds the configured maximum cache size after an insertion, the oldest template
 *   is evicted.

 * The cache inherits from CValidationInterface to receive notifications
 * about connected and disconnected blocks, which triggers cache invalidation,
 * ensuring stale templates are not returned.
 */
class BlockTemplateCache : public CValidationInterface
{
private:
    std::deque<std::pair<BlockAssembler::Options, BlockTemplateRef>> m_block_templates;
    CTxMemPool& m_mempool;
    ChainstateManager& m_chainman;
    size_t m_block_template_cache_size;
    mutable Mutex m_mutex;

    BlockTemplateRef CreateBlockTemplateInternal(const BlockAssembler::Options& options) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, m_mutex);

public:
    BlockTemplateCache(CTxMemPool& mempool, ChainstateManager& chainman, size_t block_template_cache_size = DEFAULT_BLOCK_TEMPLATE_CACHE_SIZE);
    virtual ~BlockTemplateCache() = default;

    /**
     * If a cached template exists with identical options and its age is less than
     * the specified interval, the cached template is returned.
     * Otherwise, a new template is created and stored in the cache.
     *
     * @param options The block assembly options to use.
     * @param max_template_age This indicate the maximum age of the desired block template in milliseconds (Default 0).
     * @return a BlockTemplateRef.
     */
    BlockTemplateRef GetBlockTemplate(const BlockAssembler::Options& options, MillisecondsDouble max_template_age = MillisecondsDouble{0}) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Overridden from CValidationInterface. */
    void BlockConnected(const ChainstateRole& role, const std::shared_ptr<const CBlock>& /*unused*/, const CBlockIndex* /*unused*/)
        override EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    void BlockDisconnected(const std::shared_ptr<const CBlock>& /*unused*/, const CBlockIndex* /*unused*/)
        override EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
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

/** Apply -blockmintxfee and -blockmaxweight options from ArgsManager to BlockAssembler options. */
void ApplyArgsManOptions(const ArgsManager& gArgs, BlockAssembler::Options& options);

/* Compute the block's merkle root, insert or replace the coinbase transaction and the merkle root into the block */
void AddMerkleRootAndCoinbase(CBlock& block, CTransactionRef coinbase, uint32_t version, uint32_t timestamp, uint32_t nonce);


/* Interrupt the current wait for the next block template. */
void InterruptWait(KernelNotifications& kernel_notifications, bool& interrupt_wait);
/**
 * Return a new block template when fees rise to a certain threshold or after a
 * new tip; return nullopt if timeout is reached.
 */
std::unique_ptr<CBlockTemplate> WaitAndCreateNewBlock(ChainstateManager& chainman,
                                                      KernelNotifications& kernel_notifications,
                                                      CTxMemPool* mempool,
                                                      const std::unique_ptr<CBlockTemplate>& block_template,
                                                      const BlockWaitOptions& options,
                                                      const BlockAssembler::Options& assemble_options,
                                                      bool& interrupt_wait);

/* Locks cs_main and returns the block hash and block height of the active chain if it exists; otherwise, returns nullopt.*/
std::optional<BlockRef> GetTip(ChainstateManager& chainman);

/* Waits for the connected tip to change until timeout has elapsed. During node initialization, this will wait until the tip is connected (regardless of `timeout`).
 * Returns the current tip, or nullopt if the node is shutting down. */
std::optional<BlockRef> WaitTipChanged(ChainstateManager& chainman, KernelNotifications& kernel_notifications, const uint256& current_tip, MillisecondsDouble& timeout);

} // namespace node

#endif // BITCOIN_NODE_MINER_H
