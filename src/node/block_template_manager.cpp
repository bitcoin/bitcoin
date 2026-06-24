// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/block_template_manager.h>

#include <chain.h>
#include <consensus/amount.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <core_memusage.h>
#include <interfaces/types.h>
#include <kernel/chainparams.h>
#include <logging/categories.h>
#include <logging/timer.h>
#include <node/kernel_notifications.h>
#include <node/miner.h>
#include <node/mining_args.h>
#include <primitives/block.h>
#include <sync.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/signalinterrupt.h>
#include <validation.h>
#include <validationinterface.h>

#include <algorithm>
#include <compare>
#include <condition_variable>
#include <iterator>
#include <numeric>
#include <ranges>
#include <utility>
#include <vector>

namespace node {

using interfaces::BlockRef;

BlockTemplateManager::BlockTemplateManager(CTxMemPool& mempool, ChainstateManager& chainman,
                                           KernelNotifications& notifications,
                                           BlockCreateOptions block_create_args)
    : m_mempool(mempool), m_chainman(chainman), m_notifications(notifications), m_block_create_args(std::move(block_create_args))
{
}

std::unique_ptr<CBlockTemplate> BlockTemplateManager::CreateNewTemplate(const BlockCreateOptions& options)
{
    return BlockAssembler{
        m_chainman.ActiveChainstate(),
        &m_mempool,
        MergeMiningOptions(options, m_block_create_args),
    }.CreateNewBlock();
}

void BlockTemplateManager::TrackTemplateTransactions(const std::vector<CTransactionRef>& txs)
{
    LOCK(m_mutex);
    // Don't track the dummy coinbase, because it can be modified in-place
    // by submitSolution()
    for (const CTransactionRef& tx : txs | std::views::drop(1)) {
        ++m_template_tx_refs[tx];
    }
}

void BlockTemplateManager::StopTrackingTemplateTransactions(const std::vector<CTransactionRef>& txs)
{
    LOCK(m_mutex);
    for (const CTransactionRef& tx : txs | std::views::drop(1)) {
        auto ref_count{m_template_tx_refs.find(tx)};
        if (!Assume(ref_count != m_template_tx_refs.end())) continue;
        if (--ref_count->second == 0) {
            m_template_tx_refs.erase(ref_count);
        }
    }
}

size_t BlockTemplateManager::GetTemplateMemoryUsage() const
{
    LOG_TIME_MILLIS_WITH_CATEGORY("Calculate template transaction reference memory footprint", BCLog::BENCH);

    // Copy the transaction references so that m_mutex is not held while
    // mempool.exists() below repeatedly locks mempool.cs. Holding it would
    // block template creation and destruction for the duration of this
    // calculation.
    std::vector<CTransactionRef> tx_refs;
    {
        LOCK(m_mutex);
        tx_refs.reserve(m_template_tx_refs.size());
        std::ranges::copy(m_template_tx_refs | std::views::keys, std::back_inserter(tx_refs));
    }

    size_t usage_bytes{0};
    for (const auto& tx_ref : tx_refs) {
        if (!Assume(tx_ref)) continue;
        // mempool.exists() locks mempool.cs each time, which slows down
        // our calculation. This is preferable to potentially blocking
        // the node from processing new transactions while this
        // (non-urgent) calculation is in progress.
        //
        // As a side-effect the result is not an accurate snapshot, because
        // the mempool may change during the loop.
        if (m_mempool.exists(tx_ref->GetWitnessHash())) continue;
        usage_bytes += RecursiveDynamicUsage(*tx_ref);
    }
    return usage_bytes;
}

void BlockTemplateManager::SanityCheck() const
{
    LOCK(m_mutex);
    for (const auto& [tx_ref, count] : m_template_tx_refs) {
        Assume(tx_ref);
        Assume(count > 0);
    }
}

namespace {
class SubmitBlockStateCatcher final : public CValidationInterface
{
public:
    uint256 m_hash;
    bool m_found{false};
    BlockValidationState m_state;

    explicit SubmitBlockStateCatcher(const uint256& hash) : m_hash{hash} {}

protected:
    void BlockChecked(const std::shared_ptr<const CBlock>& block, const BlockValidationState& state) override
    {
        if (block->GetHash() != m_hash) return;
        // ProcessNewBlock emits BlockChecked synchronously while holding cs_main,
        // so SubmitBlock can read these fields after ProcessNewBlock returns
        // without extra synchronization.
        m_found = true;
        m_state = state;
    }
};
} // namespace

bool BlockTemplateManager::SubmitBlock(const std::shared_ptr<const CBlock>& block, std::string& reason, std::string& debug)
{
    reason.clear();
    debug.clear();

    // This follows the submitblock RPC's validation-state capture pattern, but
    // is intentionally kept separate from the RPC implementation. The RPC entry
    // point decodes hex, formats BIP22/JSONRPC results, and calls
    // UpdateUncommittedBlockStructures() for legacy witness handling. IPC
    // callers submit already-formed blocks and need bool + reason/debug
    // results.
    auto sc = std::make_shared<SubmitBlockStateCatcher>(block->GetHash());
    CHECK_NONFATAL(m_chainman.m_options.signals)->RegisterSharedValidationInterface(sc);
    bool new_block;
    bool accepted = m_chainman.ProcessNewBlock(block, /*force_processing=*/true, /*min_pow_checked=*/true, /*new_block=*/&new_block);
    // No queue drain is needed. The BlockChecked notification used above is
    // emitted synchronously by ProcessNewBlock, unlike most validation signals.
    CHECK_NONFATAL(m_chainman.m_options.signals)->UnregisterSharedValidationInterface(sc);

    if (!new_block && accepted) {
        reason = "duplicate";
    } else if (!accepted && (!sc->m_found || sc->m_state.IsValid())) {
        // ProcessNewBlock can fail without a validation result, for example
        // from an activation or system error. It can also fail after a valid
        // BlockChecked result. In these cases the validation result is
        // inconclusive.
        reason = "inconclusive";
    } else if (!sc->m_found) {
        // The block was accepted but not connected, for example if it does not
        // have more work than the current tip.
        reason = "inconclusive";
    } else if (!sc->m_state.IsValid()) {
        reason = sc->m_state.GetRejectReason();
        debug = sc->m_state.GetDebugMessage();
    }
    const bool result{accepted && new_block && reason.empty()};
    CHECK_NONFATAL(result == reason.empty());
    return result;
}

std::optional<BlockRef> BlockTemplateManager::GetTip()
{
    LOCK(::cs_main);
    CBlockIndex* tip{m_chainman.ActiveChain().Tip()};
    if (!tip) return {};
    return BlockRef{tip->GetBlockHash(), tip->nHeight};
}

void BlockTemplateManager::InterruptWait(bool& interrupt_wait)
{
    LOCK(m_notifications.m_tip_block_mutex);
    interrupt_wait = true;
    m_notifications.m_tip_block_cv.notify_all();
}

std::unique_ptr<CBlockTemplate> BlockTemplateManager::WaitAndCreateNewBlock(
    const std::unique_ptr<CBlockTemplate>& block_template,
    const BlockWaitOptions& wait_options,
    const BlockCreateOptions& create_options,
    bool& interrupt_wait)
{
    // Delay calculating the current template fees, just in case a new block
    // comes in before the next tick.
    CAmount current_fees = -1;

    // Alternate waiting for a new tip and checking if fees have risen.
    // The latter check is expensive so we only run it once per second.
    auto now{NodeClock::now()};
    const auto deadline = now + wait_options.timeout;
    const MillisecondsDouble tick{1000};
    const bool allow_min_difficulty{m_chainman.GetParams().GetConsensus().fPowAllowMinDifficultyBlocks};

    do {
        bool tip_changed{false};
        {
            WAIT_LOCK(m_notifications.m_tip_block_mutex, lock);
            // Note that wait_until() checks the predicate before waiting
            m_notifications.m_tip_block_cv.wait_until(lock, std::min(now + tick, deadline), [&]() EXCLUSIVE_LOCKS_REQUIRED(m_notifications.m_tip_block_mutex) {
                AssertLockHeld(m_notifications.m_tip_block_mutex);
                const auto tip_block{m_notifications.TipBlock()};
                // We assume tip_block is set, because this is an instance
                // method on BlockTemplate and no template could have been
                // generated before a tip exists.
                tip_changed = Assume(tip_block) && tip_block != block_template->block.hashPrevBlock;
                return tip_changed || m_chainman.m_interrupt || interrupt_wait;
            });
            if (interrupt_wait) {
                interrupt_wait = false;
                return nullptr;
            }
        }

        if (m_chainman.m_interrupt) return nullptr;
        // At this point the tip changed, a full tick went by or we reached
        // the deadline.

        // Must release m_tip_block_mutex before locking cs_main, to avoid deadlocks.
        LOCK(::cs_main);

        // On test networks return a minimum difficulty block after 20 minutes
        if (!tip_changed && allow_min_difficulty) {
            const NodeClock::time_point tip_time{std::chrono::seconds{m_chainman.ActiveChain().Tip()->GetBlockTime()}};
            if (now > tip_time + 20min) {
                tip_changed = true;
            }
        }

        /**
         * We determine if fees increased compared to the previous template by generating
         * a fresh template. There may be more efficient ways to determine how much
         * (approximate) fees for the next block increased, perhaps more so after
         * Cluster Mempool.
         *
         * We'll also create a new template if the tip changed during this iteration.
         */
        if (wait_options.fee_threshold < MAX_MONEY || tip_changed) {
            auto new_tmpl{CreateNewTemplate(create_options)};

            // If the tip changed, return the new template regardless of its fees.
            if (tip_changed) return new_tmpl;

            // Calculate the original template total fees if we haven't already
            if (current_fees == -1) {
                current_fees = std::accumulate(block_template->vTxFees.begin(), block_template->vTxFees.end(), CAmount{0});
            }

            // Check if fees increased enough to return the new template
            const CAmount new_fees = std::accumulate(new_tmpl->vTxFees.begin(), new_tmpl->vTxFees.end(), CAmount{0});
            Assume(wait_options.fee_threshold != MAX_MONEY);
            if (new_fees >= current_fees + wait_options.fee_threshold) return new_tmpl;
        }

        now = NodeClock::now();
    } while (now < deadline);

    return nullptr;
}

bool BlockTemplateManager::CooldownIfHeadersAhead(const BlockRef& last_tip, bool& interrupt_mining)
{
    uint256 last_tip_hash{last_tip.hash};

    while (const std::optional<int> remaining = m_chainman.BlocksAheadOfTip()) {
        const int cooldown_seconds = std::clamp(*remaining, 3, 20);
        const auto cooldown_deadline{MockableSteadyClock::now() + std::chrono::seconds{cooldown_seconds}};

        {
            WAIT_LOCK(m_notifications.m_tip_block_mutex, lock);
            m_notifications.m_tip_block_cv.wait_until(lock, cooldown_deadline, [&]() EXCLUSIVE_LOCKS_REQUIRED(m_notifications.m_tip_block_mutex) {
                const auto tip_block = m_notifications.TipBlock();
                return m_chainman.m_interrupt || interrupt_mining || (tip_block && *tip_block != last_tip_hash);
            });
            if (m_chainman.m_interrupt || interrupt_mining) {
                interrupt_mining = false;
                return false;
            }

            // If the tip changed during the wait, extend the deadline
            const auto tip_block = m_notifications.TipBlock();
            if (tip_block && *tip_block != last_tip_hash) {
                last_tip_hash = *tip_block;
                continue;
            }
        }

        // No tip change and the cooldown window has expired.
        if (MockableSteadyClock::now() >= cooldown_deadline) break;
    }

    return true;
}

std::optional<BlockRef> BlockTemplateManager::WaitTipChanged(const uint256& current_tip, MillisecondsDouble timeout)
{
    bool interrupt_wait{false};
    return WaitTipChanged(current_tip, timeout, interrupt_wait);
}

std::optional<BlockRef> BlockTemplateManager::WaitTipChanged(const uint256& current_tip, MillisecondsDouble& timeout, bool& interrupt)
{
    Assume(timeout >= 0ms); // No internal callers should use a negative timeout
    if (timeout < 0ms) timeout = 0ms;
    if (timeout > std::chrono::years{100}) timeout = std::chrono::years{100}; // Upper bound to avoid UB in std::chrono
    auto deadline{std::chrono::steady_clock::now() + timeout};
    {
        WAIT_LOCK(m_notifications.m_tip_block_mutex, lock);
        // For callers convenience, wait longer than the provided timeout
        // during startup for the tip to be non-null. That way this function
        // always returns valid tip information when possible and only
        // returns null when shutting down, not when timing out.
        m_notifications.m_tip_block_cv.wait(lock, [&]() EXCLUSIVE_LOCKS_REQUIRED(m_notifications.m_tip_block_mutex) {
            AssertLockHeld(m_notifications.m_tip_block_mutex);
            return m_notifications.TipBlock() || m_chainman.m_interrupt || interrupt;
        });
        if (m_chainman.m_interrupt || interrupt) {
            interrupt = false;
            return {};
        }
        // At this point TipBlock is set, so continue to wait until it is
        // different from `current_tip` provided by caller.
        m_notifications.m_tip_block_cv.wait_until(lock, deadline, [&]() EXCLUSIVE_LOCKS_REQUIRED(m_notifications.m_tip_block_mutex) {
            return Assume(m_notifications.TipBlock()) != current_tip || m_chainman.m_interrupt || interrupt;
        });
        if (m_chainman.m_interrupt || interrupt) {
            interrupt = false;
            return {};
        }
    }

    // Must release m_tip_block_mutex before GetTip() locks cs_main, to
    // avoid deadlocks.
    return GetTip();
}

} // namespace node
