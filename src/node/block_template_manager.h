// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCK_TEMPLATE_MANAGER_H
#define BITCOIN_NODE_BLOCK_TEMPLATE_MANAGER_H

#include <consensus/amount.h>
#include <node/mining_types.h>
#include <sync.h>
#include <uint256.h>
#include <util/feefrac.h>
#include <util/time.h>
#include <validationinterface.h>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>

class CBlock;
class ChainstateManager;
class CTxMemPool;

namespace interfaces {
struct BlockRef;
} // namespace interfaces

namespace node {
class KernelNotifications;
struct CBlockTemplate;

/** Data structure for incrementally tracking chunks from a block template.
 *
 * Chunks are indexed by hash for mempool-removal updates, and by feerate for
 * trimming when a higher-feerate chunk arrives. The feerate index is maintained
 * incrementally, avoiding a full sort on every TrimToFit() call.
 *
 * Invariant: template_chunks and chunks_by_feerate contain the same chunk
 * hashes, and total_fees/total_weight/total_sigops are their aggregate totals.
 */
struct TemplateSnapshot {
    /** Chunk metadata captured from the template: adjusted feerate, weight,
     *  and sigops. */
    struct TrackedChunk {
        FeePerWeight feerate;
        int64_t weight;
        int64_t sigops_cost;
    };
    /** Feerate index key. The hash distinguishes chunks with equal feerates
     *  and allows direct erasure using metadata from template_chunks. */
    struct FeerateIndexEntry {
        FeePerWeight feerate;
        uint256 hash;
        friend bool operator<(const FeerateIndexEntry& left, const FeerateIndexEntry& right)
        {
            const auto feerate_order = ByRatio{left.feerate} <=> ByRatio{right.feerate};
            return feerate_order != 0 ? feerate_order < 0 : left.hash < right.hash;
        }
    };
    /** Owns tracked chunk metadata keyed by chunk hash. */
    std::map<uint256, TrackedChunk> template_chunks;
    /** Keeps chunks sorted so TrimToFit() can scan lowest-feerate chunks first. */
    std::set<FeerateIndexEntry> chunks_by_feerate;
    /** Active chain tip this template was built on. */
    uint256 tip_hash;
    /** Options identify which template request this snapshot belongs to. */
    BlockCreateOptions options;
    /** Minimum feerate accepted by the original template. */
    FeePerWeight min_feerate;
    /** Exclusive block weight limit used when deciding if new chunks fit. */
    int64_t max_weight{0};
    /** Weight reserved before transaction selection. */
    int64_t reserved_weight{0};
    /** Aggregate fees of all currently tracked chunks. */
    CAmount total_fees{0};
    /** Reserved weight plus the weight of all currently tracked chunks. */
    int64_t total_weight{0};
    /** Aggregate sigops cost of all currently tracked chunks. */
    int64_t total_sigops{0};
    /** Sigops reserved before transaction selection. */
    int64_t reserved_sigops{0};
    /** Chain height used for finality checks against new chunks. */
    int height{0};
    /** Median-time-past cutoff used for finality checks against new chunks. */
    int64_t lock_time_cutoff{0};
    /** Remove a chunk by hash in O(log n). */
    void RemoveChunk(const uint256& hash);
    /** Add a chunk in O(log n). */
    void AddChunk(const uint256& hash, const TrackedChunk& chunk);
    /** Evict lowest-feerate chunks to make room for @p needed_weight and @p needed_sigops.
     *  Only commits evictions if the chunk will fit afterward.
     *  Returns true if the incoming chunk fits after eviction.
     *  Runs in O(s log n), where s is the number of scanned chunks. */
    bool TrimToFit(int64_t needed_weight, int64_t needed_sigops, const FeePerWeight& incoming_feerate);
    /** Verify the chunk indexes and cached fee, weight, and sigops totals.
     *  Runs in O(n log n). */
    void SanityCheck() const;
};

/** Block template creation and fee-inflow tracking.
 *
 * Tracks chunk feerates and total fees at creation time. On every mempool
 * update, chunks that left the mempool are dropped from the record. New
 * chunks at or above the block minimum fee rate that pass sigops and finality
 * checks are added: directly if weight remains, or by evicting tracked
 * chunks whose feerate is lower than the incoming chunk.
 *
 * Block connection (non-historical) and disconnection prune records built on
 * the old tip of each transition.
 */
class BlockTemplateManager : public CValidationInterface
{
private:
    CTxMemPool& m_mempool;
    ChainstateManager& m_chainman;
    KernelNotifications& m_notifications;
    const BlockCreateOptions m_init_block_create_options;
    mutable Mutex m_mutex;
    /** Tracked template snapshots keyed by unique nonzero identifier. */
    std::map<uint64_t, TemplateSnapshot> m_template_snapshots GUARDED_BY(m_mutex);
    uint64_t m_next_template_id GUARDED_BY(m_mutex){1};

public:
    explicit BlockTemplateManager(CTxMemPool& mempool,
                                  ChainstateManager& chainman,
                                  KernelNotifications& notifications,
                                  BlockCreateOptions init_block_create_options = {});
    virtual ~BlockTemplateManager() = default;

    /** @return a copy of the block create options set during node init. */
    BlockCreateOptions GetInitBlockCreateOptions() const { return m_init_block_create_options; }

    /** Create a fresh block template, applying init-time defaults to any unset
     *  options. When @p tracking_id is non-null, the template is tracked for
     *  fee inflow and its identifier is returned there. */
    std::unique_ptr<CBlockTemplate> CreateNewTemplate(const BlockCreateOptions& options, uint64_t* tracking_id = nullptr)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** @return true if fee-inflow tracking is active for the given template id. */
    bool IsTrackingFeeInflow(uint64_t template_id) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Stop tracking fee inflow for the given template id. */
    void StopTrackingFeeInflow(uint64_t template_id)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Test-only: verify invariants. */
    void SanityCheck() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    // CValidationInterface overrides
    void BlockConnected(const kernel::ChainstateRole& role, const std::shared_ptr<const CBlock>&, const CBlockIndex*) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    void BlockDisconnected(const std::shared_ptr<const CBlock>&, const CBlockIndex*) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    void MempoolUpdated(const MemPoolChunksUpdate& mempool_chunks) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Submit a block via ProcessNewBlock and capture validation state. */
    bool SubmitBlock(const std::shared_ptr<const CBlock>& block, bool* new_block, std::string& reason, std::string& debug);

    /** @return the active chain tip, or nullopt if none exists. */
    std::optional<interfaces::BlockRef> GetTip();

    /** Wait for the tip to differ from @p current_tip or timeout.
     *  Waits indefinitely during startup for a non-null tip. */
    std::optional<interfaces::BlockRef> WaitTipChanged(const uint256& current_tip, MillisecondsDouble& timeout, bool& interrupt);

    /** Convenience overload for in-process callers (e.g. RPC) that have no
     *  interrupt handle. Takes the timeout by value so the caller's variable is
     *  left unchanged; the wait still ends on chain shutdown. */
    std::optional<interfaces::BlockRef> WaitTipChanged(const uint256& current_tip, MillisecondsDouble timeout = MillisecondsDouble::max());

    /**
     * Wait while the best known header extends the current chain tip AND at
     * least one block is being added to the tip every 3 seconds. If the tip is
     * sufficiently far behind, allow up to 20 seconds for the next tip update.
     *
     * It's not safe to keep waiting, because a malicious miner could announce
     * a header and delay revealing the block, causing all other miners using
     * this software to stall. At the same time, we need to balance between the
     * default waiting time being brief, but not ending the cooldown prematurely
     * when a random block is slow to download (or process).
     *
     * The cooldown only applies to createNewBlock(), which is typically called
     * once per connected client. Subsequent templates are provided by
     * waitNext().
     *
     * @param last_tip tip at the start of the cooldown window.
     * @param interrupt_mining set to true to interrupt the cooldown.
     *
     * @returns false if interrupted.
     */
    bool CooldownIfHeadersAhead(const interfaces::BlockRef& last_tip, bool& interrupt_mining);

    /** Interrupt a blocking wait. */
    void InterruptWait(bool& interrupt_wait);

    /** Return a new block template when fees rise to a certain threshold or
     *  after a new tip; return nullptr if timeout is reached. */
    std::unique_ptr<CBlockTemplate> WaitAndCreateNewBlock(
        const std::unique_ptr<CBlockTemplate>& block_template,
        const BlockWaitOptions& wait_options,
        const BlockCreateOptions& create_options,
        bool& interrupt_wait)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};
} // namespace node

#endif // BITCOIN_NODE_BLOCK_TEMPLATE_MANAGER_H
