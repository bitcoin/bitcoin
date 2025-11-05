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
static constexpr size_t DEFAULT_BLOCK_TEMPLATE_CACHE_LIMIT{10};

struct CBlockTemplate
{
    CBlock block;
    // Fees per transaction, not including coinbase transaction (unlike CBlock::vtx).
    std::vector<CAmount> vTxFees;
    // Sigops per transaction, not including coinbase transaction (unlike CBlock::vtx).
    std::vector<int64_t> vTxSigOpsCost;
    /* A vector of package fee rates, ordered by the sequence in which
     * packages are selected for inclusion in the block template.*/
    std::vector<FeePerVSize> m_package_feerates;
    /*
     * Template containing all coinbase transaction fields that are set by our
     * miner code.
     */
    CoinbaseTx m_coinbase_tx;

    /* Uses steady clock because it is monotonic and unaffected by rare system clock changes.
     * It uses the mockable version to allow for testing.
     * Used to decide whether a cached template is eligible for reuse based on its age.*/
    MockableSteadyClock::time_point m_creation_time;
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

using BlockTemplateRef = std::shared_ptr<const CBlockTemplate>;

struct TemplateEntry {
    BlockAssembler::Options options;
    BlockTemplateRef block_template;
};

/*
 * BlockTemplateCache provides a thread-safe cache for block templates.
 *
 * Each cached entry is identified by its BlockAssembler::Options. At most one
 * template is stored per set of options, and the cache has a fixed maximum size.
 *
 * When requesting a block template:
 * - If template_max_age is std::nullopt, a new block template is always
 *   created and returned, and nothing is cached.
 *
 * - Otherwise, the cache is checked for a template with matching options.
 *   If a cached template exists and its age (elapsed time since creation)
 *   doesn't exceed template_max_age, it is reused.
 *
 * - If a cached template exists but it is too old, it is removed and replaced
 *   with a newly created template.
 *
 * - If no matching template exists, a new template is created and cached.
 *
 * When adding a new entry causes the cache to exceed its size limit, the
 * oldest cached template (by insertion order) is evicted.
 *
 * The cache inherits from CValidationInterface to receive notifications
 * about connected and disconnected blocks, which triggers cache invalidation,
 * ensuring stale templates are not returned.
 */
class BlockTemplateCache : public CValidationInterface
{
    CTxMemPool& m_mempool;
    ChainstateManager& m_chainman;
    const size_t m_block_template_cache_limit;
    mutable Mutex m_mutex;
    // FIFO cache; at most one template per BlockAssembler::Options.
    std::deque<TemplateEntry> m_block_templates GUARDED_BY(m_mutex);
    /**
     * Search for a cached template matching the provided options.
     *
     * - If found and not too old, return it.
     * - If found but too old, evict it and return nullptr.
     * - If not found, return nullptr.
     */
    BlockTemplateRef getCachedTemplate(const BlockAssembler::Options& options,
                                       MillisecondsDouble template_max_age)
        EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

public:
    explicit BlockTemplateCache(CTxMemPool& mempool,
                                ChainstateManager& chainman,
                                size_t block_template_cache_limit = DEFAULT_BLOCK_TEMPLATE_CACHE_LIMIT);
    virtual ~BlockTemplateCache() = default;
    /**
     * Get a block template from the cache or create a new one.
     *
     * Cache lookup requires an exact match on all block creation options.
     *
     * @param options Block assembly options to match
     * @param template_max_age Maximum age for cached templates in milliseconds.
     *        If nullopt, bypasses cache entirely (doesn't insert into, read or delete from cache).
     * @return A shared pointer to the block template
     */
    BlockTemplateRef GetBlockTemplate(const BlockAssembler::Options& options,
                                      std::optional<MillisecondsDouble> template_max_age = MillisecondsDouble::max())
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Test-only: verify cache invariants */
    void SanityCheck() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

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


/* Interrupt a blocking call. */
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
 * Returns the current tip, or nullopt if the node is shutting down or interrupt()
 * is called.
 */
std::optional<BlockRef> WaitTipChanged(ChainstateManager& chainman, KernelNotifications& kernel_notifications, const uint256& current_tip, MillisecondsDouble& timeout, bool& interrupt);

/**
 * Wait while the best known header extends the current chain tip AND at least
 * one block is being added to the tip every 3 seconds. If the tip is
 * sufficiently far behind, allow up to 20 seconds for the next tip update.
 *
 * It’s not safe to keep waiting, because a malicious miner could announce a
 * header and delay revealing the block, causing all other miners using this
 * software to stall. At the same time, we need to balance between the default
 * waiting time being brief, but not ending the cooldown prematurely when a
 * random block is slow to download (or process).
 *
 * The cooldown only applies to createNewBlock(), which is typically called
 * once per connected client. Subsequent templates are provided by waitNext().
 *
 * @param last_tip tip at the start of the cooldown window.
 * @param interrupt_mining set to true to interrupt the cooldown.
 *
 * @returns false if interrupted.
 */
bool CooldownIfHeadersAhead(ChainstateManager& chainman, KernelNotifications& kernel_notifications, const BlockRef& last_tip, bool& interrupt_mining);
} // namespace node

#endif // BITCOIN_NODE_MINER_H
