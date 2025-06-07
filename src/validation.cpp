// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <validation.h>

#include <arith_uint256.h>
#include <chain.h>
#include <checkqueue.h>
#include <clientversion.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_check.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <cuckoocache.h>
#include <flatfile.h>
#include <hash.h>
#include <kernel/chain.h>
#include <kernel/chainparams.h>
#include <kernel/coinstats.h>
#include <kernel/disconnected_transactions.h>
#include <kernel/mempool_entry.h>
#include <kernel/messagestartchars.h>
#include <kernel/notifications_interface.h>
#include <kernel/warning.h>
#include <logging.h>
#include <logging/timer.h>
#include <node/blockstorage.h>
#include <node/utxo_snapshot.h>
#include <policy/ephemeral_policy.h>
#include <policy/policy.h>
#include <policy/rbf.h>
#include <policy/settings.h>
#include <policy/truc_policy.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/script.h>
#include <script/sigcache.h>
#include <signet.h>
#include <tinyformat.h>
#include <txdb.h>
#include <txmempool.h>
#include <uint256.h>
#include <undo.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/hasher.h>
#include <util/moneystr.h>
#include <util/rbf.h>
#include <util/result.h>
#include <util/signalinterrupt.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>
#include <util/trace.h>
#include <util/translation.h>
#include <validationinterface.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <deque>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <tuple>
#include <utility>

using kernel::CCoinsStats;
using kernel::CoinStatsHashType;
using kernel::ComputeUTXOStats;
using kernel::Notifications;

using fsbridge::FopenFn;
using node::BlockManager;
using node::BlockMap;
using node::CBlockIndexHeightOnlyComparator;
using node::CBlockIndexWorkComparator;
using node::SnapshotMetadata;

/** Size threshold for warning about slow UTXO set flush to disk. */
static constexpr size_t WARN_FLUSH_COINS_SIZE = 1 << 30; // 1 GiB
/** Time window to wait between writing blocks/block index and chainstate to disk.
 *  Randomize writing time inside the window to prevent a situation where the
 *  network over time settles into a few cohorts of synchronized writers.
*/
static constexpr auto DATABASE_WRITE_INTERVAL_MIN{50min};
static constexpr auto DATABASE_WRITE_INTERVAL_MAX{70min};
/** Maximum age of our tip for us to be considered current for fee estimation */
static constexpr std::chrono::hours MAX_FEE_ESTIMATION_TIP_AGE{3};
const std::vector<std::string> CHECKLEVEL_DOC {
    "level 0 reads the blocks from disk",
    "level 1 verifies block validity",
    "level 2 verifies undo data",
    "level 3 checks disconnection of tip blocks",
    "level 4 tries to reconnect the blocks",
    "each level includes the checks of the previous levels",
};
/** The number of blocks to keep below the deepest prune lock.
 *  There is nothing special about this number. It is higher than what we
 *  expect to see in regular mainnet reorgs, but not so high that it would
 *  noticeably interfere with the pruning mechanism.
 * */
static constexpr int PRUNE_LOCK_BUFFER{10};

TRACEPOINT_SEMAPHORE(validation, block_connected);
TRACEPOINT_SEMAPHORE(utxocache, flush);
TRACEPOINT_SEMAPHORE(mempool, replaced);
TRACEPOINT_SEMAPHORE(mempool, rejected);

const CBlockIndex* Chainstate::FindForkInGlobalIndex(const CBlockLocator& locator) const
{
    AssertLockHeld(cs_main);

    // Find the latest block common to locator and chain - we expect that
    // locator.vHave is sorted descending by height.
    for (const uint256& hash : locator.vHave) {
        const CBlockIndex* pindex{m_blockman.LookupBlockIndex(hash)};
        if (pindex) {
            if (m_chain.Contains(pindex)) {
                return pindex;
            }
            if (pindex->GetAncestor(m_chain.Height()) == m_chain.Tip()) {
                return m_chain.Tip();
            }
        }
    }
    return m_chain.Genesis();
}

bool CheckInputScripts(const CTransaction& tx, TxValidationState& state,
                       const CCoinsViewCache& inputs, unsigned int flags, bool cacheSigStore,
                       bool cacheFullScriptStore, PrecomputedTransactionData& txdata,
                       ValidationCache& validation_cache,
                       std::vector<CScriptCheck>* pvChecks = nullptr)
                       EXCLUSIVE_LOCKS_REQUIRED(cs_main);

bool CheckFinalTxAtTip(const CBlockIndex& active_chain_tip, const CTransaction& tx)
{
    AssertLockHeld(cs_main);

    // CheckFinalTxAtTip() uses active_chain_tip.Height()+1 to evaluate
    // nLockTime because when IsFinalTx() is called within
    // AcceptBlock(), the height of the block *being*
    // evaluated is what is used. Thus if we want to know if a
    // transaction can be part of the *next* block, we need to call
    // IsFinalTx() with one more than active_chain_tip.Height().
    const int nBlockHeight = active_chain_tip.nHeight + 1;

    // BIP113 requires that time-locked transactions have nLockTime set to
    // less than the median time of the previous block they're contained in.
    // When the next block is created its previous block will be the current
    // chain tip, so we use that to calculate the median time passed to
    // IsFinalTx().
    const int64_t nBlockTime{active_chain_tip.GetMedianTimePast()};

    return IsFinalTx(tx, nBlockHeight, nBlockTime);
}

namespace {
/**
 * A helper which calculates heights of inputs of a given transaction.
 *
 * @param[in] tip    The current chain tip. If an input belongs to a mempool
 *                   transaction, we assume it will be confirmed in the next block.
 * @param[in] coins  Any CCoinsView that provides access to the relevant coins.
 * @param[in] tx     The transaction being evaluated.
 *
 * @returns A vector of input heights or nullopt, in case of an error.
 */
std::optional<std::vector<int>> CalculatePrevHeights(
    const CBlockIndex& tip,
    const CCoinsView& coins,
    const CTransaction& tx)
{
    std::vector<int> prev_heights;
    prev_heights.resize(tx.vin.size());
    for (size_t i = 0; i < tx.vin.size(); ++i) {
        if (auto coin{coins.GetCoin(tx.vin[i].prevout)}) {
            prev_heights[i] = coin->nHeight == MEMPOOL_HEIGHT
                              ? tip.nHeight + 1 // Assume all mempool transaction confirm in the next block.
                              : coin->nHeight;
        } else {
            LogPrintf("ERROR: %s: Missing input %d in transaction \'%s\'\n", __func__, i, tx.GetHash().GetHex());
            return std::nullopt;
        }
    }
    return prev_heights;
}
} // namespace

std::optional<LockPoints> CalculateLockPointsAtTip(
    CBlockIndex* tip,
    const CCoinsView& coins_view,
    const CTransaction& tx)
{
    assert(tip);

    auto prev_heights{CalculatePrevHeights(*tip, coins_view, tx)};
    if (!prev_heights.has_value()) return std::nullopt;

    CBlockIndex next_tip;
    next_tip.pprev = tip;
    // When SequenceLocks() is called within ConnectBlock(), the height
    // of the block *being* evaluated is what is used.
    // Thus if we want to know if a transaction can be part of the
    // *next* block, we need to use one more than active_chainstate.m_chain.Height()
    next_tip.nHeight = tip->nHeight + 1;
    const auto [min_height, min_time] = CalculateSequenceLocks(tx, STANDARD_LOCKTIME_VERIFY_FLAGS, prev_heights.value(), next_tip);

    // Also store the hash of the block with the highest height of
    // all the blocks which have sequence locked prevouts.
    // This hash needs to still be on the chain
    // for these LockPoint calculations to be valid
    // Note: It is impossible to correctly calculate a maxInputBlock
    // if any of the sequence locked inputs depend on unconfirmed txs,
    // except in the special case where the relative lock time/height
    // is 0, which is equivalent to no sequence lock. Since we assume
    // input height of tip+1 for mempool txs and test the resulting
    // min_height and min_time from CalculateSequenceLocks against tip+1.
    int max_input_height{0};
    for (const int height : prev_heights.value()) {
        // Can ignore mempool inputs since we'll fail if they had non-zero locks
        if (height != next_tip.nHeight) {
            max_input_height = std::max(max_input_height, height);
        }
    }

    // tip->GetAncestor(max_input_height) should never return a nullptr
    // because max_input_height is always less than the tip height.
    // It would, however, be a bad bug to continue execution, since a
    // LockPoints object with the maxInputBlock member set to nullptr
    // signifies no relative lock time.
    return LockPoints{min_height, min_time, Assert(tip->GetAncestor(max_input_height))};
}

bool CheckSequenceLocksAtTip(CBlockIndex* tip,
                             const LockPoints& lock_points)
{
    assert(tip != nullptr);

    CBlockIndex index;
    index.pprev = tip;
    // CheckSequenceLocksAtTip() uses active_chainstate.m_chain.Height()+1 to evaluate
    // height based locks because when SequenceLocks() is called within
    // ConnectBlock(), the height of the block *being*
    // evaluated is what is used.
    // Thus if we want to know if a transaction can be part of the
    // *next* block, we need to use one more than active_chainstate.m_chain.Height()
    index.nHeight = tip->nHeight + 1;

    return EvaluateSequenceLocks(index, {lock_points.height, lock_points.time});
}

// Returns the script flags which should be checked for a given block
static unsigned int GetBlockScriptFlags(const CBlockIndex& block_index, const ChainstateManager& chainman);

static void LimitMempoolSize(CTxMemPool& pool, CCoinsViewCache& coins_cache)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main, pool.cs)
{
    AssertLockHeld(::cs_main);
    AssertLockHeld(pool.cs);
    int expired = pool.Expire(GetTime<std::chrono::seconds>() - pool.m_opts.expiry);
    if (expired != 0) {
        LogDebug(BCLog::MEMPOOL, "Expired %i transactions from the memory pool\n", expired);
    }

    std::vector<COutPoint> vNoSpendsRemaining;
    pool.TrimToSize(pool.m_opts.max_size_bytes, &vNoSpendsRemaining);
    for (const COutPoint& removed : vNoSpendsRemaining)
        coins_cache.Uncache(removed);
}

static bool IsCurrentForFeeEstimation(Chainstate& active_chainstate) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    if (active_chainstate.m_chainman.IsInitialBlockDownload()) {
        return false;
    }
    if (active_chainstate.m_chain.Tip()->GetBlockTime() < count_seconds(GetTime<std::chrono::seconds>() - MAX_FEE_ESTIMATION_TIP_AGE))
        return false;
    if (active_chainstate.m_chain.Height() < active_chainstate.m_chainman.m_best_header->nHeight - 1) {
        return false;
    }
    return true;
}

void Chainstate::MaybeUpdateMempoolForReorg(
    DisconnectedBlockTransactions& disconnectpool,
    bool fAddToMempool)
{
    if (!m_mempool) return;

    AssertLockHeld(cs_main);
    AssertLockHeld(m_mempool->cs);
    std::vector<uint256> vHashUpdate;
    {
        // disconnectpool is ordered so that the front is the most recently-confirmed
        // transaction (the last tx of the block at the tip) in the disconnected chain.
        // Iterate disconnectpool in reverse, so that we add transactions
        // back to the mempool starting with the earliest transaction that had
        // been previously seen in a block.
        const auto queuedTx = disconnectpool.take();
        auto it = queuedTx.rbegin();
        while (it != queuedTx.rend()) {
            // ignore validation errors in resurrected transactions
            if (!fAddToMempool || (*it)->IsCoinBase() ||
                AcceptToMemoryPool(*this, *it, GetTime(),
                    /*bypass_limits=*/true, /*test_accept=*/false).m_result_type !=
                        MempoolAcceptResult::ResultType::VALID) {
                // If the transaction doesn't make it in to the mempool, remove any
                // transactions that depend on it (which would now be orphans).
                m_mempool->removeRecursive(**it, MemPoolRemovalReason::REORG);
            } else if (m_mempool->exists(GenTxid::Txid((*it)->GetHash()))) {
                vHashUpdate.push_back((*it)->GetHash());
            }
            ++it;
        }
    }

    // AcceptToMemoryPool/addNewTransaction all assume that new mempool entries have
    // no in-mempool children, which is generally not true when adding
    // previously-confirmed transactions back to the mempool.
    // UpdateTransactionsFromBlock finds descendants of any transactions in
    // the disconnectpool that were added back and cleans up the mempool state.
    m_mempool->UpdateTransactionsFromBlock(vHashUpdate);

    // Predicate to use for filtering transactions in removeForReorg.
    // Checks whether the transaction is still final and, if it spends a coinbase output, mature.
    // Also updates valid entries' cached LockPoints if needed.
    // If false, the tx is still valid and its lockpoints are updated.
    // If true, the tx would be invalid in the next block; remove this entry and all of its descendants.
    // Note that TRUC rules are not applied here, so reorgs may cause violations of TRUC inheritance or
    // topology restrictions.
    const auto filter_final_and_mature = [&](CTxMemPool::txiter it)
        EXCLUSIVE_LOCKS_REQUIRED(m_mempool->cs, ::cs_main) {
        AssertLockHeld(m_mempool->cs);
        AssertLockHeld(::cs_main);
        const CTransaction& tx = it->GetTx();

        // The transaction must be final.
        if (!CheckFinalTxAtTip(*Assert(m_chain.Tip()), tx)) return true;

        const LockPoints& lp = it->GetLockPoints();
        // CheckSequenceLocksAtTip checks if the transaction will be final in the next block to be
        // created on top of the new chain.
        if (TestLockPointValidity(m_chain, lp)) {
            if (!CheckSequenceLocksAtTip(m_chain.Tip(), lp)) {
                return true;
            }
        } else {
            const CCoinsViewMemPool view_mempool{&CoinsTip(), *m_mempool};
            const std::optional<LockPoints> new_lock_points{CalculateLockPointsAtTip(m_chain.Tip(), view_mempool, tx)};
            if (new_lock_points.has_value() && CheckSequenceLocksAtTip(m_chain.Tip(), *new_lock_points)) {
                // Now update the mempool entry lockpoints as well.
                it->UpdateLockPoints(*new_lock_points);
            } else {
                return true;
            }
        }

        // If the transaction spends any coinbase outputs, it must be mature.
        if (it->GetSpendsCoinbase()) {
            for (const CTxIn& txin : tx.vin) {
                if (m_mempool->exists(GenTxid::Txid(txin.prevout.hash))) continue;
                const Coin& coin{CoinsTip().AccessCoin(txin.prevout)};
                assert(!coin.IsSpent());
                const auto mempool_spend_height{m_chain.Tip()->nHeight + 1};
                if (coin.IsCoinBase() && mempool_spend_height - coin.nHeight < COINBASE_MATURITY) {
                    return true;
                }
            }
        }
        // Transaction is still valid and cached LockPoints are updated.
        return false;
    };

    // We also need to remove any now-immature transactions
    m_mempool->removeForReorg(m_chain, filter_final_and_mature);
    // Re-limit mempool size, in case we added any transactions
    LimitMempoolSize(*m_mempool, this->CoinsTip());
}

/**
* Checks to avoid mempool polluting consensus critical paths since cached
* signature and script validity results will be reused if we validate this
* transaction again during block validation.
* */
static bool CheckInputsFromMempoolAndCache(const CTransaction& tx, TxValidationState& state,
                const CCoinsViewCache& view, const CTxMemPool& pool,
                unsigned int flags, PrecomputedTransactionData& txdata, CCoinsViewCache& coins_tip,
                ValidationCache& validation_cache)
                EXCLUSIVE_LOCKS_REQUIRED(cs_main, pool.cs)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(pool.cs);

    assert(!tx.IsCoinBase());
    for (const CTxIn& txin : tx.vin) {
        const Coin& coin = view.AccessCoin(txin.prevout);

        // This coin was checked in PreChecks and MemPoolAccept
        // has been holding cs_main since then.
        Assume(!coin.IsSpent());
        if (coin.IsSpent()) return false;

        // If the Coin is available, there are 2 possibilities:
        // it is available in our current ChainstateActive UTXO set,
        // or it's a UTXO provided by a transaction in our mempool.
        // Ensure the scriptPubKeys in Coins from CoinsView are correct.
        const CTransactionRef& txFrom = pool.get(txin.prevout.hash);
        if (txFrom) {
            assert(txFrom->GetHash() == txin.prevout.hash);
            assert(txFrom->vout.size() > txin.prevout.n);
            assert(txFrom->vout[txin.prevout.n] == coin.out);
        } else {
            const Coin& coinFromUTXOSet = coins_tip.AccessCoin(txin.prevout);
            assert(!coinFromUTXOSet.IsSpent());
            assert(coinFromUTXOSet.out == coin.out);
        }
    }

    // Call CheckInputScripts() to cache signature and script validity against current tip consensus rules.
    return CheckInputScripts(tx, state, view, flags, /* cacheSigStore= */ true, /* cacheFullScriptStore= */ true, txdata, validation_cache);
}

namespace {

class MemPoolAccept
{
public:
    explicit MemPoolAccept(CTxMemPool& mempool, Chainstate& active_chainstate) :
        m_pool(mempool),
        m_view(&m_dummy),
        m_viewmempool(&active_chainstate.CoinsTip(), m_pool),
        m_active_chainstate(active_chainstate)
    {
    }

    // We put the arguments we're handed into a struct, so we can pass them
    // around easier.
    struct ATMPArgs {
        const CChainParams& m_chainparams;
        const int64_t m_accept_time;
        const bool m_bypass_limits;
        /*
         * Return any outpoints which were not previously present in the coins
         * cache, but were added as a result of validating the tx for mempool
         * acceptance. This allows the caller to optionally remove the cache
         * additions if the associated transaction ends up being rejected by
         * the mempool.
         */
        std::vector<COutPoint>& m_coins_to_uncache;
        /** When true, the transaction or package will not be submitted to the mempool. */
        const bool m_test_accept;
        /** Whether we allow transactions to replace mempool transactions. If false,
         * any transaction spending the same inputs as a transaction in the mempool is considered
         * a conflict. */
        const bool m_allow_replacement;
        /** When true, allow sibling eviction. This only occurs in single transaction package settings. */
        const bool m_allow_sibling_eviction;
        /** When true, the mempool will not be trimmed when any transactions are submitted in
         * Finalize(). Instead, limits should be enforced at the end to ensure the package is not
         * partially submitted.
         */
        const bool m_package_submission;
        /** When true, use package feerates instead of individual transaction feerates for fee-based
         * policies such as mempool min fee and min relay fee.
         */
        const bool m_package_feerates;
        /** Used for local submission of transactions to catch "absurd" fees
         * due to fee miscalculation by wallets. std:nullopt implies unset, allowing any feerates.
         * Any individual transaction failing this check causes immediate failure.
         */
        const std::optional<CFeeRate> m_client_maxfeerate;

        /** Whether CPFP carveout and RBF carveout are granted. */
        const bool m_allow_carveouts;

        /** Parameters for single transaction mempool validation. */
        static ATMPArgs SingleAccept(const CChainParams& chainparams, int64_t accept_time,
                                     bool bypass_limits, std::vector<COutPoint>& coins_to_uncache,
                                     bool test_accept) {
            return ATMPArgs{/* m_chainparams */ chainparams,
                            /* m_accept_time */ accept_time,
                            /* m_bypass_limits */ bypass_limits,
                            /* m_coins_to_uncache */ coins_to_uncache,
                            /* m_test_accept */ test_accept,
                            /* m_allow_replacement */ true,
                            /* m_allow_sibling_eviction */ true,
                            /* m_package_submission */ false,
                            /* m_package_feerates */ false,
                            /* m_client_maxfeerate */ {}, // checked by caller
                            /* m_allow_carveouts */ true,
            };
        }

        /** Parameters for test package mempool validation through testmempoolaccept. */
        static ATMPArgs PackageTestAccept(const CChainParams& chainparams, int64_t accept_time,
                                          std::vector<COutPoint>& coins_to_uncache) {
            return ATMPArgs{/* m_chainparams */ chainparams,
                            /* m_accept_time */ accept_time,
                            /* m_bypass_limits */ false,
                            /* m_coins_to_uncache */ coins_to_uncache,
                            /* m_test_accept */ true,
                            /* m_allow_replacement */ false,
                            /* m_allow_sibling_eviction */ false,
                            /* m_package_submission */ false, // not submitting to mempool
                            /* m_package_feerates */ false,
                            /* m_client_maxfeerate */ {}, // checked by caller
                            /* m_allow_carveouts */ false,
            };
        }

        /** Parameters for child-with-unconfirmed-parents package validation. */
        static ATMPArgs PackageChildWithParents(const CChainParams& chainparams, int64_t accept_time,
                                                std::vector<COutPoint>& coins_to_uncache, const std::optional<CFeeRate>& client_maxfeerate) {
            return ATMPArgs{/* m_chainparams */ chainparams,
                            /* m_accept_time */ accept_time,
                            /* m_bypass_limits */ false,
                            /* m_coins_to_uncache */ coins_to_uncache,
                            /* m_test_accept */ false,
                            /* m_allow_replacement */ true,
                            /* m_allow_sibling_eviction */ false,
                            /* m_package_submission */ true,
                            /* m_package_feerates */ true,
                            /* m_client_maxfeerate */ client_maxfeerate,
                            /* m_allow_carveouts */ false,
            };
        }

        /** Parameters for a single transaction within a package. */
        static ATMPArgs SingleInPackageAccept(const ATMPArgs& package_args) {
            return ATMPArgs{/* m_chainparams */ package_args.m_chainparams,
                            /* m_accept_time */ package_args.m_accept_time,
                            /* m_bypass_limits */ false,
                            /* m_coins_to_uncache */ package_args.m_coins_to_uncache,
                            /* m_test_accept */ package_args.m_test_accept,
                            /* m_allow_replacement */ true,
                            /* m_allow_sibling_eviction */ true,
                            /* m_package_submission */ true, // do not LimitMempoolSize in Finalize()
                            /* m_package_feerates */ false, // only 1 transaction
                            /* m_client_maxfeerate */ package_args.m_client_maxfeerate,
                            /* m_allow_carveouts */ false,
            };
        }

    private:
        // Private ctor to avoid exposing details to clients and allowing the possibility of
        // mixing up the order of the arguments. Use static functions above instead.
        ATMPArgs(const CChainParams& chainparams,
                 int64_t accept_time,
                 bool bypass_limits,
                 std::vector<COutPoint>& coins_to_uncache,
                 bool test_accept,
                 bool allow_replacement,
                 bool allow_sibling_eviction,
                 bool package_submission,
                 bool package_feerates,
                 std::optional<CFeeRate> client_maxfeerate,
                 bool allow_carveouts)
            : m_chainparams{chainparams},
              m_accept_time{accept_time},
              m_bypass_limits{bypass_limits},
              m_coins_to_uncache{coins_to_uncache},
              m_test_accept{test_accept},
              m_allow_replacement{allow_replacement},
              m_allow_sibling_eviction{allow_sibling_eviction},
              m_package_submission{package_submission},
              m_package_feerates{package_feerates},
              m_client_maxfeerate{client_maxfeerate},
              m_allow_carveouts{allow_carveouts}
        {
            // If we are using package feerates, we must be doing package submission.
            // It also means carveouts and sibling eviction are not permitted.
            if (m_package_feerates) {
                Assume(m_package_submission);
                Assume(!m_allow_carveouts);
                Assume(!m_allow_sibling_eviction);
            }
            if (m_allow_sibling_eviction) Assume(m_allow_replacement);
        }
    };

    /** Clean up all non-chainstate coins from m_view and m_viewmempool. */
    void CleanupTemporaryCoins() EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    // Single transaction acceptance
    MempoolAcceptResult AcceptSingleTransaction(const CTransactionRef& ptx, ATMPArgs& args) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /**
    * Multiple transaction acceptance. Transactions may or may not be interdependent, but must not
    * conflict with each other, and the transactions cannot already be in the mempool. Parents must
    * come before children if any dependencies exist.
    */
    PackageMempoolAcceptResult AcceptMultipleTransactions(const std::vector<CTransactionRef>& txns, ATMPArgs& args) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /**
     * Submission of a subpackage.
     * If subpackage size == 1, calls AcceptSingleTransaction() with adjusted ATMPArgs to avoid
     * package policy restrictions like no CPFP carve out (PackageMempoolChecks)
     * and creates a PackageMempoolAcceptResult wrapping the result.
     *
     * If subpackage size > 1, calls AcceptMultipleTransactions() with the provided ATMPArgs.
     *
     * Also cleans up all non-chainstate coins from m_view at the end.
    */
    PackageMempoolAcceptResult AcceptSubPackage(const std::vector<CTransactionRef>& subpackage, ATMPArgs& args)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    /**
     * Package (more specific than just multiple transactions) acceptance. Package must be a child
     * with all of its unconfirmed parents, and topologically sorted.
     */
    PackageMempoolAcceptResult AcceptPackage(const Package& package, ATMPArgs& args) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

private:
    // All the intermediate state that gets passed between the various levels
    // of checking a given transaction.
    struct Workspace {
        explicit Workspace(const CTransactionRef& ptx) : m_ptx(ptx), m_hash(ptx->GetHash()) {}
        /** Txids of mempool transactions that this transaction directly conflicts with or may
         * replace via sibling eviction. */
        std::set<Txid> m_conflicts;
        /** Iterators to mempool entries that this transaction directly conflicts with or may
         * replace via sibling eviction. */
        CTxMemPool::setEntries m_iters_conflicting;
        /** All mempool ancestors of this transaction. */
        CTxMemPool::setEntries m_ancestors;
        /* Handle to the tx in the changeset */
        CTxMemPool::ChangeSet::TxHandle m_tx_handle;
        /** Whether RBF-related data structures (m_conflicts, m_iters_conflicting,
         * m_replaced_transactions) include a sibling in addition to txns with conflicting inputs. */
        bool m_sibling_eviction{false};

        /** Virtual size of the transaction as used by the mempool, calculated using serialized size
         * of the transaction and sigops. */
        int64_t m_vsize;
        /** Fees paid by this transaction: total input amounts subtracted by total output amounts. */
        CAmount m_base_fees;
        /** Base fees + any fee delta set by the user with prioritisetransaction. */
        CAmount m_modified_fees;

        /** If we're doing package validation (i.e. m_package_feerates=true), the "effective"
         * package feerate of this transaction is the total fees divided by the total size of
         * transactions (which may include its ancestors and/or descendants). */
        CFeeRate m_package_feerate{0};

        const CTransactionRef& m_ptx;
        /** Txid. */
        const Txid& m_hash;
        TxValidationState m_state;
        /** A temporary cache containing serialized transaction data for signature verification.
         * Reused across PolicyScriptChecks and ConsensusScriptChecks. */
        PrecomputedTransactionData m_precomputed_txdata;
    };

    // Run the policy checks on a given transaction, excluding any script checks.
    // Looks up inputs, calculates feerate, considers replacement, evaluates
    // package limits, etc. As this function can be invoked for "free" by a peer,
    // only tests that are fast should be done here (to avoid CPU DoS).
    bool PreChecks(ATMPArgs& args, Workspace& ws) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    // Run checks for mempool replace-by-fee, only used in AcceptSingleTransaction.
    bool ReplacementChecks(Workspace& ws) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    // Enforce package mempool ancestor/descendant limits (distinct from individual
    // ancestor/descendant limits done in PreChecks) and run Package RBF checks.
    bool PackageMempoolChecks(const std::vector<CTransactionRef>& txns,
                              std::vector<Workspace>& workspaces,
                              int64_t total_vsize,
                              PackageValidationState& package_state) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    // Run the script checks using our policy flags. As this can be slow, we should
    // only invoke this on transactions that have otherwise passed policy checks.
    bool PolicyScriptChecks(const ATMPArgs& args, Workspace& ws) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    // Re-run the script checks, using consensus flags, and try to cache the
    // result in the scriptcache. This should be done after
    // PolicyScriptChecks(). This requires that all inputs either be in our
    // utxo set or in the mempool.
    bool ConsensusScriptChecks(const ATMPArgs& args, Workspace& ws) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    // Try to add the transaction to the mempool, removing any conflicts first.
    void FinalizeSubpackage(const ATMPArgs& args) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    // Submit all transactions to the mempool and call ConsensusScriptChecks to add to the script
    // cache - should only be called after successful validation of all transactions in the package.
    // Does not call LimitMempoolSize(), so mempool max_size_bytes may be temporarily exceeded.
    bool SubmitPackage(const ATMPArgs& args, std::vector<Workspace>& workspaces, PackageValidationState& package_state,
                       std::map<Wtxid, MempoolAcceptResult>& results)
         EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    // Compare a package's feerate against minimum allowed.
    bool CheckFeeRate(size_t package_size, CAmount package_fee, TxValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, m_pool.cs)
    {
        AssertLockHeld(::cs_main);
        AssertLockHeld(m_pool.cs);
        CAmount mempoolRejectFee = m_pool.GetMinFee().GetFee(package_size);
        if (mempoolRejectFee > 0 && package_fee < mempoolRejectFee) {
            return state.Invalid(TxValidationResult::TX_RECONSIDERABLE, "mempool min fee not met", strprintf("%d < %d", package_fee, mempoolRejectFee));
        }

        if (package_fee < m_pool.m_opts.min_relay_feerate.GetFee(package_size)) {
            return state.Invalid(TxValidationResult::TX_RECONSIDERABLE, "min relay fee not met",
                                 strprintf("%d < %d", package_fee, m_pool.m_opts.min_relay_feerate.GetFee(package_size)));
        }
        return true;
    }

    ValidationCache& GetValidationCache()
    {
        return m_active_chainstate.m_chainman.m_validation_cache;
    }

private:
    CTxMemPool& m_pool;
    CCoinsViewCache m_view;
    CCoinsViewMemPool m_viewmempool;
    CCoinsView m_dummy;

    Chainstate& m_active_chainstate;

    // Fields below are per *sub*package state and must be reset prior to subsequent
    // AcceptSingleTransaction and AcceptMultipleTransactions invocations
    struct SubPackageState {
        /** Aggregated modified fees of all transactions, used to calculate package feerate. */
        CAmount m_total_modified_fees{0};
        /** Aggregated virtual size of all transactions, used to calculate package feerate. */
        int64_t m_total_vsize{0};

        // RBF-related members
        /** Whether the transaction(s) would replace any mempool transactions and/or evict any siblings.
         * If so, RBF rules apply. */
        bool m_rbf{false};
        /** Mempool transactions that were replaced. */
        std::list<CTransactionRef> m_replaced_transactions;
        /* Changeset representing adding transactions and removing their conflicts. */
        std::unique_ptr<CTxMemPool::ChangeSet> m_changeset;

        /** Total modified fees of mempool transactions being replaced. */
        CAmount m_conflicting_fees{0};
        /** Total size (in virtual bytes) of mempool transactions being replaced. */
        size_t m_conflicting_size{0};
    };

    struct SubPackageState m_subpackage;

    /** Re-set sub-package state to not leak between evaluations */
    void ClearSubPackageState() EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs)
    {
        m_subpackage = SubPackageState{};

        // And clean coins while at it
        CleanupTemporaryCoins();
    }
};

bool MemPoolAccept::PreChecks(ATMPArgs& args, Workspace& ws)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(m_pool.cs);
    const CTransactionRef& ptx = ws.m_ptx;
    const CTransaction& tx = *ws.m_ptx;
    const Txid& hash = ws.m_hash;

    // Copy/alias what we need out of args
    const int64_t nAcceptTime = args.m_accept_time;
    const bool bypass_limits = args.m_bypass_limits;
    std::vector<COutPoint>& coins_to_uncache = args.m_coins_to_uncache;

    // Alias what we need out of ws
    TxValidationState& state = ws.m_state;

    if (!CheckTransaction(tx, state)) {
        return false; // state filled in by CheckTransaction
    }

    // Coinbase is only valid in a block, not as a loose transaction
    if (tx.IsCoinBase())
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "coinbase");

    // Rather not work on nonstandard transactions (unless -testnet/-regtest)
    std::string reason;
    if (m_pool.m_opts.require_standard && !IsStandardTx(tx, m_pool.m_opts.max_datacarrier_bytes, m_pool.m_opts.permit_bare_multisig, m_pool.m_opts.dust_relay_feerate, reason)) {
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, reason);
    }

    // Transactions smaller than 65 non-witness bytes are not relayed to mitigate CVE-2017-12842.
    if (::GetSerializeSize(TX_NO_WITNESS(tx)) < MIN_STANDARD_TX_NONWITNESS_SIZE)
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "tx-size-small");

    // Only accept nLockTime-using transactions that can be mined in the next
    // block; we don't want our mempool filled up with transactions that can't
    // be mined yet.
    if (!CheckFinalTxAtTip(*Assert(m_active_chainstate.m_chain.Tip()), tx)) {
        return state.Invalid(TxValidationResult::TX_PREMATURE_SPEND, "non-final");
    }

    if (m_pool.exists(GenTxid::Wtxid(tx.GetWitnessHash()))) {
        // Exact transaction already exists in the mempool.
        return state.Invalid(TxValidationResult::TX_CONFLICT, "txn-already-in-mempool");
    } else if (m_pool.exists(GenTxid::Txid(tx.GetHash()))) {
        // Transaction with the same non-witness data but different witness (same txid, different
        // wtxid) already exists in the mempool.
        return state.Invalid(TxValidationResult::TX_CONFLICT, "txn-same-nonwitness-data-in-mempool");
    }

    // Check for conflicts with in-memory transactions
    for (const CTxIn &txin : tx.vin)
    {
        const CTransaction* ptxConflicting = m_pool.GetConflictTx(txin.prevout);
        if (ptxConflicting) {
            if (!args.m_allow_replacement) {
                // Transaction conflicts with a mempool tx, but we're not allowing replacements in this context.
                return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "bip125-replacement-disallowed");
            }
            ws.m_conflicts.insert(ptxConflicting->GetHash());
        }
    }

    m_view.SetBackend(m_viewmempool);

    const CCoinsViewCache& coins_cache = m_active_chainstate.CoinsTip();
    // do all inputs exist?
    for (const CTxIn& txin : tx.vin) {
        if (!coins_cache.HaveCoinInCache(txin.prevout)) {
            coins_to_uncache.push_back(txin.prevout);
        }

        // Note: this call may add txin.prevout to the coins cache
        // (coins_cache.cacheCoins) by way of FetchCoin(). It should be removed
        // later (via coins_to_uncache) if this tx turns out to be invalid.
        if (!m_view.HaveCoin(txin.prevout)) {
            // Are inputs missing because we already have the tx?
            for (size_t out = 0; out < tx.vout.size(); out++) {
                // Optimistically just do efficient check of cache for outputs
                if (coins_cache.HaveCoinInCache(COutPoint(hash, out))) {
                    return state.Invalid(TxValidationResult::TX_CONFLICT, "txn-already-known");
                }
            }
            // Otherwise assume this might be an orphan tx for which we just haven't seen parents yet
            return state.Invalid(TxValidationResult::TX_MISSING_INPUTS, "bad-txns-inputs-missingorspent");
        }
    }

    // This is const, but calls into the back end CoinsViews. The CCoinsViewDB at the bottom of the
    // hierarchy brings the best block into scope. See CCoinsViewDB::GetBestBlock().
    m_view.GetBestBlock();

    // we have all inputs cached now, so switch back to dummy (to protect
    // against bugs where we pull more inputs from disk that miss being added
    // to coins_to_uncache)
    m_view.SetBackend(m_dummy);

    assert(m_active_chainstate.m_blockman.LookupBlockIndex(m_view.GetBestBlock()) == m_active_chainstate.m_chain.Tip());

    // Only accept BIP68 sequence locked transactions that can be mined in the next
    // block; we don't want our mempool filled up with transactions that can't
    // be mined yet.
    // Pass in m_view which has all of the relevant inputs cached. Note that, since m_view's
    // backend was removed, it no longer pulls coins from the mempool.
    const std::optional<LockPoints> lock_points{CalculateLockPointsAtTip(m_active_chainstate.m_chain.Tip(), m_view, tx)};
    if (!lock_points.has_value() || !CheckSequenceLocksAtTip(m_active_chainstate.m_chain.Tip(), *lock_points)) {
        return state.Invalid(TxValidationResult::TX_PREMATURE_SPEND, "non-BIP68-final");
    }

    // The mempool holds txs for the next block, so pass height+1 to CheckTxInputs
    if (!Consensus::CheckTxInputs(tx, state, m_view, m_active_chainstate.m_chain.Height() + 1, ws.m_base_fees)) {
        return false; // state filled in by CheckTxInputs
    }

    if (m_pool.m_opts.require_standard && !AreInputsStandard(tx, m_view)) {
        return state.Invalid(TxValidationResult::TX_INPUTS_NOT_STANDARD, "bad-txns-nonstandard-inputs");
    }

    // Check for non-standard witnesses.
    if (tx.HasWitness() && m_pool.m_opts.require_standard && !IsWitnessStandard(tx, m_view)) {
        return state.Invalid(TxValidationResult::TX_WITNESS_MUTATED, "bad-witness-nonstandard");
    }

    int64_t nSigOpsCost = GetTransactionSigOpCost(tx, m_view, STANDARD_SCRIPT_VERIFY_FLAGS);

    // Keep track of transactions that spend a coinbase, which we re-scan
    // during reorgs to ensure COINBASE_MATURITY is still met.
    bool fSpendsCoinbase = false;
    for (const CTxIn &txin : tx.vin) {
        const Coin &coin = m_view.AccessCoin(txin.prevout);
        if (coin.IsCoinBase()) {
            fSpendsCoinbase = true;
            break;
        }
    }

    // Set entry_sequence to 0 when bypass_limits is used; this allows txs from a block
    // reorg to be marked earlier than any child txs that were already in the mempool.
    const uint64_t entry_sequence = bypass_limits ? 0 : m_pool.GetSequence();
    if (!m_subpackage.m_changeset) {
        m_subpackage.m_changeset = m_pool.GetChangeSet();
    }
    ws.m_tx_handle = m_subpackage.m_changeset->StageAddition(ptx, ws.m_base_fees, nAcceptTime, m_active_chainstate.m_chain.Height(), entry_sequence, fSpendsCoinbase, nSigOpsCost, lock_points.value());

    // ws.m_modified_fees includes any fee deltas from PrioritiseTransaction
    ws.m_modified_fees = ws.m_tx_handle->GetModifiedFee();

    ws.m_vsize = ws.m_tx_handle->GetTxSize();

    // Enforces 0-fee for dust transactions, no incentive to be mined alone
    if (m_pool.m_opts.require_standard) {
        if (!PreCheckEphemeralTx(*ptx, m_pool.m_opts.dust_relay_feerate, ws.m_base_fees, ws.m_modified_fees, state)) {
            return false; // state filled in by PreCheckEphemeralTx
        }
    }

    if (nSigOpsCost > MAX_STANDARD_TX_SIGOPS_COST)
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "bad-txns-too-many-sigops",
                strprintf("%d", nSigOpsCost));

    // No individual transactions are allowed below the min relay feerate except from disconnected blocks.
    // This requirement, unlike CheckFeeRate, cannot be bypassed using m_package_feerates because,
    // while a tx could be package CPFP'd when entering the mempool, we do not have a DoS-resistant
    // method of ensuring the tx remains bumped. For example, the fee-bumping child could disappear
    // due to a replacement.
    // The only exception is TRUC transactions.
    if (!bypass_limits && ws.m_ptx->version != TRUC_VERSION && ws.m_modified_fees < m_pool.m_opts.min_relay_feerate.GetFee(ws.m_vsize)) {
        // Even though this is a fee-related failure, this result is TX_MEMPOOL_POLICY, not
        // TX_RECONSIDERABLE, because it cannot be bypassed using package validation.
        return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "min relay fee not met",
                             strprintf("%d < %d", ws.m_modified_fees, m_pool.m_opts.min_relay_feerate.GetFee(ws.m_vsize)));
    }
    // No individual transactions are allowed below the mempool min feerate except from disconnected
    // blocks and transactions in a package. Package transactions will be checked using package
    // feerate later.
    if (!bypass_limits && !args.m_package_feerates && !CheckFeeRate(ws.m_vsize, ws.m_modified_fees, state)) return false;

    ws.m_iters_conflicting = m_pool.GetIterSet(ws.m_conflicts);

    // Note that these modifications are only applicable to single transaction scenarios;
    // carve-outs are disabled for multi-transaction evaluations.
    CTxMemPool::Limits maybe_rbf_limits = m_pool.m_opts.limits;

    // Calculate in-mempool ancestors, up to a limit.
    if (ws.m_conflicts.size() == 1 && args.m_allow_carveouts) {
        // In general, when we receive an RBF transaction with mempool conflicts, we want to know whether we
        // would meet the chain limits after the conflicts have been removed. However, there isn't a practical
        // way to do this short of calculating the ancestor and descendant sets with an overlay cache of
        // changed mempool entries. Due to both implementation and runtime complexity concerns, this isn't
        // very realistic, thus we only ensure a limited set of transactions are RBF'able despite mempool
        // conflicts here. Importantly, we need to ensure that some transactions which were accepted using
        // the below carve-out are able to be RBF'ed, without impacting the security the carve-out provides
        // for off-chain contract systems (see link in the comment below).
        //
        // Specifically, the subset of RBF transactions which we allow despite chain limits are those which
        // conflict directly with exactly one other transaction (but may evict children of said transaction),
        // and which are not adding any new mempool dependencies. Note that the "no new mempool dependencies"
        // check is accomplished later, so we don't bother doing anything about it here, but if our
        // policy changes, we may need to move that check to here instead of removing it wholesale.
        //
        // Such transactions are clearly not merging any existing packages, so we are only concerned with
        // ensuring that (a) no package is growing past the package size (not count) limits and (b) we are
        // not allowing something to effectively use the (below) carve-out spot when it shouldn't be allowed
        // to.
        //
        // To check these we first check if we meet the RBF criteria, above, and increment the descendant
        // limits by the direct conflict and its descendants (as these are recalculated in
        // CalculateMempoolAncestors by assuming the new transaction being added is a new descendant, with no
        // removals, of each parent's existing dependent set). The ancestor count limits are unmodified (as
        // the ancestor limits should be the same for both our new transaction and any conflicts).
        // We don't bother incrementing m_limit_descendants by the full removal count as that limit never comes
        // into force here (as we're only adding a single transaction).
        assert(ws.m_iters_conflicting.size() == 1);
        CTxMemPool::txiter conflict = *ws.m_iters_conflicting.begin();

        maybe_rbf_limits.descendant_count += 1;
        maybe_rbf_limits.descendant_size_vbytes += conflict->GetSizeWithDescendants();
    }

    if (auto ancestors{m_subpackage.m_changeset->CalculateMemPoolAncestors(ws.m_tx_handle, maybe_rbf_limits)}) {
        ws.m_ancestors = std::move(*ancestors);
    } else {
        // If CalculateMemPoolAncestors fails second time, we want the original error string.
        const auto error_message{util::ErrorString(ancestors).original};

        // Carve-out is not allowed in this context; fail
        if (!args.m_allow_carveouts) {
            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "too-long-mempool-chain", error_message);
        }

        // Contracting/payment channels CPFP carve-out:
        // If the new transaction is relatively small (up to 40k weight)
        // and has at most one ancestor (ie ancestor limit of 2, including
        // the new transaction), allow it if its parent has exactly the
        // descendant limit descendants. The transaction also cannot be TRUC,
        // as its topology restrictions do not allow a second child.
        //
        // This allows protocols which rely on distrusting counterparties
        // being able to broadcast descendants of an unconfirmed transaction
        // to be secure by simply only having two immediately-spendable
        // outputs - one for each counterparty. For more info on the uses for
        // this, see https://lists.linuxfoundation.org/pipermail/bitcoin-dev/2018-November/016518.html
        CTxMemPool::Limits cpfp_carve_out_limits{
            .ancestor_count = 2,
            .ancestor_size_vbytes = maybe_rbf_limits.ancestor_size_vbytes,
            .descendant_count = maybe_rbf_limits.descendant_count + 1,
            .descendant_size_vbytes = maybe_rbf_limits.descendant_size_vbytes + EXTRA_DESCENDANT_TX_SIZE_LIMIT,
        };
        if (ws.m_vsize > EXTRA_DESCENDANT_TX_SIZE_LIMIT || ws.m_ptx->version == TRUC_VERSION) {
            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "too-long-mempool-chain", error_message);
        }
        if (auto ancestors_retry{m_subpackage.m_changeset->CalculateMemPoolAncestors(ws.m_tx_handle, cpfp_carve_out_limits)}) {
            ws.m_ancestors = std::move(*ancestors_retry);
        } else {
            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "too-long-mempool-chain", error_message);
        }
    }

    // Even though just checking direct mempool parents for inheritance would be sufficient, we
    // check using the full ancestor set here because it's more convenient to use what we have
    // already calculated.
    if (const auto err{SingleTRUCChecks(ws.m_ptx, ws.m_ancestors, ws.m_conflicts, ws.m_vsize)}) {
        // Single transaction contexts only.
        if (args.m_allow_sibling_eviction && err->second != nullptr) {
            // We should only be considering where replacement is considered valid as well.
            Assume(args.m_allow_replacement);

            // Potential sibling eviction. Add the sibling to our list of mempool conflicts to be
            // included in RBF checks.
            ws.m_conflicts.insert(err->second->GetHash());
            // Adding the sibling to m_iters_conflicting here means that it doesn't count towards
            // RBF Carve Out above. This is correct, since removing to-be-replaced transactions from
            // the descendant count is done separately in SingleTRUCChecks for TRUC transactions.
            ws.m_iters_conflicting.insert(m_pool.GetIter(err->second->GetHash()).value());
            ws.m_sibling_eviction = true;
            // The sibling will be treated as part of the to-be-replaced set in ReplacementChecks.
            // Note that we are not checking whether it opts in to replaceability via BIP125 or TRUC
            // (which is normally done in PreChecks). However, the only way a TRUC transaction can
            // have a non-TRUC and non-BIP125 descendant is due to a reorg.
        } else {
            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "TRUC-violation", err->first);
        }
    }

    // A transaction that spends outputs that would be replaced by it is invalid. Now
    // that we have the set of all ancestors we can detect this
    // pathological case by making sure ws.m_conflicts and ws.m_ancestors don't
    // intersect.
    if (const auto err_string{EntriesAndTxidsDisjoint(ws.m_ancestors, ws.m_conflicts, hash)}) {
        // We classify this as a consensus error because a transaction depending on something it
        // conflicts with would be inconsistent.
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-spends-conflicting-tx", *err_string);
    }

    // We want to detect conflicts in any tx in a package to trigger package RBF logic
    m_subpackage.m_rbf |= !ws.m_conflicts.empty();
    return true;
}

bool MemPoolAccept::ReplacementChecks(Workspace& ws)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(m_pool.cs);

    const CTransaction& tx = *ws.m_ptx;
    const uint256& hash = ws.m_hash;
    TxValidationState& state = ws.m_state;

    CFeeRate newFeeRate(ws.m_modified_fees, ws.m_vsize);
    // Enforce Rule #6. The replacement transaction must have a higher feerate than its direct conflicts.
    // - The motivation for this check is to ensure that the replacement transaction is preferable for
    //   block-inclusion, compared to what would be removed from the mempool.
    // - This logic predates ancestor feerate-based transaction selection, which is why it doesn't
    //   consider feerates of descendants.
    // - Note: Ancestor feerate-based transaction selection has made this comparison insufficient to
    //   guarantee that this is incentive-compatible for miners, because it is possible for a
    //   descendant transaction of a direct conflict to pay a higher feerate than the transaction that
    //   might replace them, under these rules.
    if (const auto err_string{PaysMoreThanConflicts(ws.m_iters_conflicting, newFeeRate, hash)}) {
        // This fee-related failure is TX_RECONSIDERABLE because validating in a package may change
        // the result.
        return state.Invalid(TxValidationResult::TX_RECONSIDERABLE,
                             strprintf("insufficient fee%s", ws.m_sibling_eviction ? " (including sibling eviction)" : ""), *err_string);
    }

    CTxMemPool::setEntries all_conflicts;

    // Calculate all conflicting entries and enforce Rule #5.
    if (const auto err_string{GetEntriesForConflicts(tx, m_pool, ws.m_iters_conflicting, all_conflicts)}) {
        return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY,
                             strprintf("too many potential replacements%s", ws.m_sibling_eviction ? " (including sibling eviction)" : ""), *err_string);
    }
    // Enforce Rule #2.
    if (const auto err_string{HasNoNewUnconfirmed(tx, m_pool, all_conflicts)}) {
        // Sibling eviction is only done for TRUC transactions, which cannot have multiple ancestors.
        Assume(!ws.m_sibling_eviction);
        return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY,
                             strprintf("replacement-adds-unconfirmed%s", ws.m_sibling_eviction ? " (including sibling eviction)" : ""), *err_string);
    }

    // Check if it's economically rational to mine this transaction rather than the ones it
    // replaces and pays for its own relay fees. Enforce Rules #3 and #4.
    for (CTxMemPool::txiter it : all_conflicts) {
        m_subpackage.m_conflicting_fees += it->GetModifiedFee();
        m_subpackage.m_conflicting_size += it->GetTxSize();
    }
    if (const auto err_string{PaysForRBF(m_subpackage.m_conflicting_fees, ws.m_modified_fees, ws.m_vsize,
                                         m_pool.m_opts.incremental_relay_feerate, hash)}) {
        // Result may change in a package context
        return state.Invalid(TxValidationResult::TX_RECONSIDERABLE,
                             strprintf("insufficient fee%s", ws.m_sibling_eviction ? " (including sibling eviction)" : ""), *err_string);
    }

    // Add all the to-be-removed transactions to the changeset.
    for (auto it : all_conflicts) {
        m_subpackage.m_changeset->StageRemoval(it);
    }
    return true;
}

bool MemPoolAccept::PackageMempoolChecks(const std::vector<CTransactionRef>& txns,
                                         std::vector<Workspace>& workspaces,
                                         const int64_t total_vsize,
                                         PackageValidationState& package_state)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(m_pool.cs);

    // CheckPackageLimits expects the package transactions to not already be in the mempool.
    assert(std::all_of(txns.cbegin(), txns.cend(), [this](const auto& tx)
                       { return !m_pool.exists(GenTxid::Txid(tx->GetHash()));}));

    assert(txns.size() == workspaces.size());

    auto result = m_pool.CheckPackageLimits(txns, total_vsize);
    if (!result) {
        // This is a package-wide error, separate from an individual transaction error.
        return package_state.Invalid(PackageValidationResult::PCKG_POLICY, "package-mempool-limits", util::ErrorString(result).original);
    }

    // No conflicts means we're finished. Further checks are all RBF-only.
    if (!m_subpackage.m_rbf) return true;

    // We're in package RBF context; replacement proposal must be size 2
    if (workspaces.size() != 2 || !Assume(IsChildWithParents(txns))) {
        return package_state.Invalid(PackageValidationResult::PCKG_POLICY, "package RBF failed: package must be 1-parent-1-child");
    }

    // If the package has in-mempool ancestors, we won't consider a package RBF
    // since it would result in a cluster larger than 2.
    // N.B. To relax this constraint we will need to revisit how CCoinsViewMemPool::PackageAddTransaction
    // is being used inside AcceptMultipleTransactions to track available inputs while processing a package.
    for (const auto& ws : workspaces) {
        if (!ws.m_ancestors.empty()) {
            return package_state.Invalid(PackageValidationResult::PCKG_POLICY, "package RBF failed: new transaction cannot have mempool ancestors");
        }
    }

    // Aggregate all conflicts into one set.
    CTxMemPool::setEntries direct_conflict_iters;
    for (Workspace& ws : workspaces) {
        // Aggregate all conflicts into one set.
        direct_conflict_iters.merge(ws.m_iters_conflicting);
    }

    const auto& parent_ws = workspaces[0];
    const auto& child_ws = workspaces[1];

    // Don't consider replacements that would cause us to remove a large number of mempool entries.
    // This limit is not increased in a package RBF. Use the aggregate number of transactions.
    CTxMemPool::setEntries all_conflicts;
    if (const auto err_string{GetEntriesForConflicts(*child_ws.m_ptx, m_pool, direct_conflict_iters,
                                                     all_conflicts)}) {
        return package_state.Invalid(PackageValidationResult::PCKG_POLICY,
                                     "package RBF failed: too many potential replacements", *err_string);
    }


    for (CTxMemPool::txiter it : all_conflicts) {
        m_subpackage.m_changeset->StageRemoval(it);
        m_subpackage.m_conflicting_fees += it->GetModifiedFee();
        m_subpackage.m_conflicting_size += it->GetTxSize();
    }

    // Use the child as the transaction for attributing errors to.
    const Txid& child_hash = child_ws.m_ptx->GetHash();
    if (const auto err_string{PaysForRBF(/*original_fees=*/m_subpackage.m_conflicting_fees,
                                         /*replacement_fees=*/m_subpackage.m_total_modified_fees,
                                         /*replacement_vsize=*/m_subpackage.m_total_vsize,
                                         m_pool.m_opts.incremental_relay_feerate, child_hash)}) {
        return package_state.Invalid(PackageValidationResult::PCKG_POLICY,
                                     "package RBF failed: insufficient anti-DoS fees", *err_string);
    }

    // Ensure this two transaction package is a "chunk" on its own; we don't want the child
    // to be only paying anti-DoS fees
    const CFeeRate parent_feerate(parent_ws.m_modified_fees, parent_ws.m_vsize);
    const CFeeRate package_feerate(m_subpackage.m_total_modified_fees, m_subpackage.m_total_vsize);
    if (package_feerate <= parent_feerate) {
        return package_state.Invalid(PackageValidationResult::PCKG_POLICY,
                                     "package RBF failed: package feerate is less than or equal to parent feerate",
                                     strprintf("package feerate %s <= parent feerate is %s", package_feerate.ToString(), parent_feerate.ToString()));
    }

    // Check if it's economically rational to mine this package rather than the ones it replaces.
    // This takes the place of ReplacementChecks()'s PaysMoreThanConflicts() in the package RBF setting.
    if (const auto err_tup{ImprovesFeerateDiagram(*m_subpackage.m_changeset)}) {
        return package_state.Invalid(PackageValidationResult::PCKG_POLICY,
                                     "package RBF failed: " + err_tup.value().second, "");
    }

    LogDebug(BCLog::TXPACKAGES, "package RBF checks passed: parent %s (wtxid=%s), child %s (wtxid=%s), package hash (%s)\n",
        txns.front()->GetHash().ToString(), txns.front()->GetWitnessHash().ToString(),
        txns.back()->GetHash().ToString(), txns.back()->GetWitnessHash().ToString(),
        GetPackageHash(txns).ToString());


    return true;
}

bool MemPoolAccept::PolicyScriptChecks(const ATMPArgs& args, Workspace& ws)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(m_pool.cs);
    const CTransaction& tx = *ws.m_ptx;
    TxValidationState& state = ws.m_state;

    constexpr unsigned int scriptVerifyFlags = STANDARD_SCRIPT_VERIFY_FLAGS;

    // Check input scripts and signatures.
    // This is done last to help prevent CPU exhaustion denial-of-service attacks.
    if (!CheckInputScripts(tx, state, m_view, scriptVerifyFlags, true, false, ws.m_precomputed_txdata, GetValidationCache())) {
        // SCRIPT_VERIFY_CLEANSTACK requires SCRIPT_VERIFY_WITNESS, so we
        // need to turn both off, and compare against just turning off CLEANSTACK
        // to see if the failure is specifically due to witness validation.
        TxValidationState state_dummy; // Want reported failures to be from first CheckInputScripts
        if (!tx.HasWitness() && CheckInputScripts(tx, state_dummy, m_view, scriptVerifyFlags & ~(SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_CLEANSTACK), true, false, ws.m_precomputed_txdata, GetValidationCache()) &&
                !CheckInputScripts(tx, state_dummy, m_view, scriptVerifyFlags & ~SCRIPT_VERIFY_CLEANSTACK, true, false, ws.m_precomputed_txdata, GetValidationCache())) {
            // Only the witness is missing, so the transaction itself may be fine.
            state.Invalid(TxValidationResult::TX_WITNESS_STRIPPED,
                    state.GetRejectReason(), state.GetDebugMessage());
        }
        return false; // state filled in by CheckInputScripts
    }

    return true;
}

bool MemPoolAccept::ConsensusScriptChecks(const ATMPArgs& args, Workspace& ws)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(m_pool.cs);
    const CTransaction& tx = *ws.m_ptx;
    const uint256& hash = ws.m_hash;
    TxValidationState& state = ws.m_state;

    // Check again against the current block tip's script verification
    // flags to cache our script execution flags. This is, of course,
    // useless if the next block has different script flags from the
    // previous one, but because the cache tracks script flags for us it
    // will auto-invalidate and we'll just have a few blocks of extra
    // misses on soft-fork activation.
    //
    // This is also useful in case of bugs in the standard flags that cause
    // transactions to pass as valid when they're actually invalid. For
    // instance the STRICTENC flag was incorrectly allowing certain
    // CHECKSIG NOT scripts to pass, even though they were invalid.
    //
    // There is a similar check in CreateNewBlock() to prevent creating
    // invalid blocks (using TestBlockValidity), however allowing such
    // transactions into the mempool can be exploited as a DoS attack.
    unsigned int currentBlockScriptVerifyFlags{GetBlockScriptFlags(*m_active_chainstate.m_chain.Tip(), m_active_chainstate.m_chainman)};
    if (!CheckInputsFromMempoolAndCache(tx, state, m_view, m_pool, currentBlockScriptVerifyFlags,
                                        ws.m_precomputed_txdata, m_active_chainstate.CoinsTip(), GetValidationCache())) {
        LogPrintf("BUG! PLEASE REPORT THIS! CheckInputScripts failed against latest-block but not STANDARD flags %s, %s\n", hash.ToString(), state.ToString());
        return Assume(false);
    }

    return true;
}

void MemPoolAccept::FinalizeSubpackage(const ATMPArgs& args)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(m_pool.cs);

    if (!m_subpackage.m_changeset->GetRemovals().empty()) Assume(args.m_allow_replacement);
    // Remove conflicting transactions from the mempool
    for (CTxMemPool::txiter it : m_subpackage.m_changeset->GetRemovals())
    {
        std::string log_string = strprintf("replacing mempool tx %s (wtxid=%s, fees=%s, vsize=%s). ",
                                      it->GetTx().GetHash().ToString(),
                                      it->GetTx().GetWitnessHash().ToString(),
                                      it->GetFee(),
                                      it->GetTxSize());
        FeeFrac feerate{m_subpackage.m_total_modified_fees, int32_t(m_subpackage.m_total_vsize)};
        uint256 tx_or_package_hash{};
        const bool replaced_with_tx{m_subpackage.m_changeset->GetTxCount() == 1};
        if (replaced_with_tx) {
            const CTransaction& tx = m_subpackage.m_changeset->GetAddedTxn(0);
            tx_or_package_hash = tx.GetHash();
            log_string += strprintf("New tx %s (wtxid=%s, fees=%s, vsize=%s)",
                                    tx.GetHash().ToString(),
                                    tx.GetWitnessHash().ToString(),
                                    feerate.fee,
                                    feerate.size);
        } else {
            tx_or_package_hash = GetPackageHash(m_subpackage.m_changeset->GetAddedTxns());
            log_string += strprintf("New package %s with %lu txs, fees=%s, vsize=%s",
                                    tx_or_package_hash.ToString(),
                                    m_subpackage.m_changeset->GetTxCount(),
                                    feerate.fee,
                                    feerate.size);

        }
        LogDebug(BCLog::MEMPOOL, "%s\n", log_string);
        TRACEPOINT(mempool, replaced,
                it->GetTx().GetHash().data(),
                it->GetTxSize(),
                it->GetFee(),
                std::chrono::duration_cast<std::chrono::duration<std::uint64_t>>(it->GetTime()).count(),
                tx_or_package_hash.data(),
                feerate.size,
                feerate.fee,
                replaced_with_tx
        );
        m_subpackage.m_replaced_transactions.push_back(it->GetSharedTx());
    }
    m_subpackage.m_changeset->Apply();
    m_subpackage.m_changeset.reset();
}

bool MemPoolAccept::SubmitPackage(const ATMPArgs& args, std::vector<Workspace>& workspaces,
                                  PackageValidationState& package_state,
                                  std::map<Wtxid, MempoolAcceptResult>& results)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(m_pool.cs);
    // Sanity check: none of the transactions should be in the mempool, and none of the transactions
    // should have a same-txid-different-witness equivalent in the mempool.
    assert(std::all_of(workspaces.cbegin(), workspaces.cend(), [this](const auto& ws){
        return !m_pool.exists(GenTxid::Txid(ws.m_ptx->GetHash())); }));

    bool all_submitted = true;
    FinalizeSubpackage(args);
    // ConsensusScriptChecks adds to the script cache and is therefore consensus-critical;
    // CheckInputsFromMempoolAndCache asserts that transactions only spend coins available from the
    // mempool or UTXO set. Submit each transaction to the mempool immediately after calling
    // ConsensusScriptChecks to make the outputs available for subsequent transactions.
    for (Workspace& ws : workspaces) {
        if (!ConsensusScriptChecks(args, ws)) {
            results.emplace(ws.m_ptx->GetWitnessHash(), MempoolAcceptResult::Failure(ws.m_state));
            // Since PolicyScriptChecks() passed, this should never fail.
            Assume(false);
            all_submitted = false;
            package_state.Invalid(PackageValidationResult::PCKG_MEMPOOL_ERROR,
                                  strprintf("BUG! PolicyScriptChecks succeeded but ConsensusScriptChecks failed: %s",
                                            ws.m_ptx->GetHash().ToString()));
            // Remove the transaction from the mempool.
            if (!m_subpackage.m_changeset) m_subpackage.m_changeset = m_pool.GetChangeSet();
            m_subpackage.m_changeset->StageRemoval(m_pool.GetIter(ws.m_ptx->GetHash()).value());
        }
    }
    if (!all_submitted) {
        Assume(m_subpackage.m_changeset);
        // This code should be unreachable; it's here as belt-and-suspenders
        // to try to ensure we have no consensus-invalid transactions in the
        // mempool.
        m_subpackage.m_changeset->Apply();
        m_subpackage.m_changeset.reset();
        return false;
    }

    std::vector<Wtxid> all_package_wtxids;
    all_package_wtxids.reserve(workspaces.size());
    std::transform(workspaces.cbegin(), workspaces.cend(), std::back_inserter(all_package_wtxids),
                   [](const auto& ws) { return ws.m_ptx->GetWitnessHash(); });

    if (!m_subpackage.m_replaced_transactions.empty()) {
        LogDebug(BCLog::MEMPOOL, "replaced %u mempool transactions with %u new one(s) for %s additional fees, %d delta bytes\n",
                 m_subpackage.m_replaced_transactions.size(), workspaces.size(),
                 m_subpackage.m_total_modified_fees - m_subpackage.m_conflicting_fees,
                 m_subpackage.m_total_vsize - static_cast<int>(m_subpackage.m_conflicting_size));
    }

    // Add successful results. The returned results may change later if LimitMempoolSize() evicts them.
    for (Workspace& ws : workspaces) {
        auto iter = m_pool.GetIter(ws.m_ptx->GetHash());
        Assume(iter.has_value());
        const auto effective_feerate = args.m_package_feerates ? ws.m_package_feerate :
            CFeeRate{ws.m_modified_fees, static_cast<uint32_t>(ws.m_vsize)};
        const auto effective_feerate_wtxids = args.m_package_feerates ? all_package_wtxids :
            std::vector<Wtxid>{ws.m_ptx->GetWitnessHash()};
        results.emplace(ws.m_ptx->GetWitnessHash(),
                        MempoolAcceptResult::Success(std::move(m_subpackage.m_replaced_transactions), ws.m_vsize,
                                         ws.m_base_fees, effective_feerate, effective_feerate_wtxids));
        if (!m_pool.m_opts.signals) continue;
        const CTransaction& tx = *ws.m_ptx;
        const auto tx_info = NewMempoolTransactionInfo(ws.m_ptx, ws.m_base_fees,
                                                       ws.m_vsize, (*iter)->GetHeight(),
                                                       args.m_bypass_limits, args.m_package_submission,
                                                       IsCurrentForFeeEstimation(m_active_chainstate),
                                                       m_pool.HasNoInputsOf(tx));
        m_pool.m_opts.signals->TransactionAddedToMempool(tx_info, m_pool.GetAndIncrementSequence());
    }
    return all_submitted;
}

MempoolAcceptResult MemPoolAccept::AcceptSingleTransaction(const CTransactionRef& ptx, ATMPArgs& args)
{
    AssertLockHeld(cs_main);
    LOCK(m_pool.cs); // mempool "read lock" (held through m_pool.m_opts.signals->TransactionAddedToMempool())

    Workspace ws(ptx);
    const std::vector<Wtxid> single_wtxid{ws.m_ptx->GetWitnessHash()};

    if (!PreChecks(args, ws)) {
        if (ws.m_state.GetResult() == TxValidationResult::TX_RECONSIDERABLE) {
            // Failed for fee reasons. Provide the effective feerate and which tx was included.
            return MempoolAcceptResult::FeeFailure(ws.m_state, CFeeRate(ws.m_modified_fees, ws.m_vsize), single_wtxid);
        }
        return MempoolAcceptResult::Failure(ws.m_state);
    }

    m_subpackage.m_total_vsize = ws.m_vsize;
    m_subpackage.m_total_modified_fees = ws.m_modified_fees;

    // Individual modified feerate exceeded caller-defined max; abort
    if (args.m_client_maxfeerate && CFeeRate(ws.m_modified_fees, ws.m_vsize) > args.m_client_maxfeerate.value()) {
        ws.m_state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "max feerate exceeded", "");
        return MempoolAcceptResult::Failure(ws.m_state);
    }

    if (m_pool.m_opts.require_standard) {
        Wtxid dummy_wtxid;
        if (!CheckEphemeralSpends(/*package=*/{ptx}, m_pool.m_opts.dust_relay_feerate, m_pool, ws.m_state, dummy_wtxid)) {
            return MempoolAcceptResult::Failure(ws.m_state);
        }
    }

    if (m_subpackage.m_rbf && !ReplacementChecks(ws)) {
        if (ws.m_state.GetResult() == TxValidationResult::TX_RECONSIDERABLE) {
            // Failed for incentives-based fee reasons. Provide the effective feerate and which tx was included.
            return MempoolAcceptResult::FeeFailure(ws.m_state, CFeeRate(ws.m_modified_fees, ws.m_vsize), single_wtxid);
        }
        return MempoolAcceptResult::Failure(ws.m_state);
    }

    // Perform the inexpensive checks first and avoid hashing and signature verification unless
    // those checks pass, to mitigate CPU exhaustion denial-of-service attacks.
    if (!PolicyScriptChecks(args, ws)) return MempoolAcceptResult::Failure(ws.m_state);

    if (!ConsensusScriptChecks(args, ws)) return MempoolAcceptResult::Failure(ws.m_state);

    const CFeeRate effective_feerate{ws.m_modified_fees, static_cast<uint32_t>(ws.m_vsize)};
    // Tx was accepted, but not added
    if (args.m_test_accept) {
        return MempoolAcceptResult::Success(std::move(m_subpackage.m_replaced_transactions), ws.m_vsize,
                                            ws.m_base_fees, effective_feerate, single_wtxid);
    }

    FinalizeSubpackage(args);

    // Limit the mempool, if appropriate.
    if (!args.m_package_submission && !args.m_bypass_limits) {
        LimitMempoolSize(m_pool, m_active_chainstate.CoinsTip());
        if (!m_pool.exists(GenTxid::Txid(ws.m_hash))) {
            // The tx no longer meets our (new) mempool minimum feerate but could be reconsidered in a package.
            ws.m_state.Invalid(TxValidationResult::TX_RECONSIDERABLE, "mempool full");
            return MempoolAcceptResult::FeeFailure(ws.m_state, CFeeRate(ws.m_modified_fees, ws.m_vsize), {ws.m_ptx->GetWitnessHash()});
        }
    }

    if (m_pool.m_opts.signals) {
        const CTransaction& tx = *ws.m_ptx;
        auto iter = m_pool.GetIter(tx.GetHash());
        Assume(iter.has_value());
        const auto tx_info = NewMempoolTransactionInfo(ws.m_ptx, ws.m_base_fees,
                                                       ws.m_vsize, (*iter)->GetHeight(),
                                                       args.m_bypass_limits, args.m_package_submission,
                                                       IsCurrentForFeeEstimation(m_active_chainstate),
                                                       m_pool.HasNoInputsOf(tx));
        m_pool.m_opts.signals->TransactionAddedToMempool(tx_info, m_pool.GetAndIncrementSequence());
    }

    if (!m_subpackage.m_replaced_transactions.empty()) {
        LogDebug(BCLog::MEMPOOL, "replaced %u mempool transactions with 1 new transaction for %s additional fees, %d delta bytes\n",
                 m_subpackage.m_replaced_transactions.size(),
                 ws.m_modified_fees - m_subpackage.m_conflicting_fees,
                 ws.m_vsize - static_cast<int>(m_subpackage.m_conflicting_size));
    }

    return MempoolAcceptResult::Success(std::move(m_subpackage.m_replaced_transactions), ws.m_vsize, ws.m_base_fees,
                                        effective_feerate, single_wtxid);
}

PackageMempoolAcceptResult MemPoolAccept::AcceptMultipleTransactions(const std::vector<CTransactionRef>& txns, ATMPArgs& args)
{
    AssertLockHeld(cs_main);

    // These context-free package limits can be done before taking the mempool lock.
    PackageValidationState package_state;
    if (!IsWellFormedPackage(txns, package_state, /*require_sorted=*/true)) return PackageMempoolAcceptResult(package_state, {});

    std::vector<Workspace> workspaces{};
    workspaces.reserve(txns.size());
    std::transform(txns.cbegin(), txns.cend(), std::back_inserter(workspaces),
                   [](const auto& tx) { return Workspace(tx); });
    std::map<Wtxid, MempoolAcceptResult> results;

    LOCK(m_pool.cs);

    // Do all PreChecks first and fail fast to avoid running expensive script checks when unnecessary.
    for (Workspace& ws : workspaces) {
        if (!PreChecks(args, ws)) {
            package_state.Invalid(PackageValidationResult::PCKG_TX, "transaction failed");
            // Exit early to avoid doing pointless work. Update the failed tx result; the rest are unfinished.
            results.emplace(ws.m_ptx->GetWitnessHash(), MempoolAcceptResult::Failure(ws.m_state));
            return PackageMempoolAcceptResult(package_state, std::move(results));
        }

        // Individual modified feerate exceeded caller-defined max; abort
        // N.B. this doesn't take into account CPFPs. Chunk-aware validation may be more robust.
        if (args.m_client_maxfeerate && CFeeRate(ws.m_modified_fees, ws.m_vsize) > args.m_client_maxfeerate.value()) {
            // Need to set failure here both individually and at package level
            ws.m_state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "max feerate exceeded", "");
            package_state.Invalid(PackageValidationResult::PCKG_TX, "transaction failed");
            // Exit early to avoid doing pointless work. Update the failed tx result; the rest are unfinished.
            results.emplace(ws.m_ptx->GetWitnessHash(), MempoolAcceptResult::Failure(ws.m_state));
            return PackageMempoolAcceptResult(package_state, std::move(results));
        }

        // Make the coins created by this transaction available for subsequent transactions in the
        // package to spend. If there are no conflicts within the package, no transaction can spend a coin
        // needed by another transaction in the package. We also need to make sure that no package
        // tx replaces (or replaces the ancestor of) the parent of another package tx. As long as we
        // check these two things, we don't need to track the coins spent.
        // If a package tx conflicts with a mempool tx, PackageMempoolChecks() ensures later that any package RBF attempt
        // has *no* in-mempool ancestors, so we don't have to worry about subsequent transactions in
        // same package spending the same in-mempool outpoints. This needs to be revisited for general
        // package RBF.
        m_viewmempool.PackageAddTransaction(ws.m_ptx);
    }

    // At this point we have all in-mempool ancestors, and we know every transaction's vsize.
    // Run the TRUC checks on the package.
    for (Workspace& ws : workspaces) {
        if (auto err{PackageTRUCChecks(ws.m_ptx, ws.m_vsize, txns, ws.m_ancestors)}) {
            package_state.Invalid(PackageValidationResult::PCKG_POLICY, "TRUC-violation", err.value());
            return PackageMempoolAcceptResult(package_state, {});
        }
    }

    // Transactions must meet two minimum feerates: the mempool minimum fee and min relay fee.
    // For transactions consisting of exactly one child and its parents, it suffices to use the
    // package feerate (total modified fees / total virtual size) to check this requirement.
    // Note that this is an aggregate feerate; this function has not checked that there are transactions
    // too low feerate to pay for themselves, or that the child transactions are higher feerate than
    // their parents. Using aggregate feerate may allow "parents pay for child" behavior and permit
    // a child that is below mempool minimum feerate. To avoid these behaviors, callers of
    // AcceptMultipleTransactions need to restrict txns topology (e.g. to ancestor sets) and check
    // the feerates of individuals and subsets.
    m_subpackage.m_total_vsize = std::accumulate(workspaces.cbegin(), workspaces.cend(), int64_t{0},
        [](int64_t sum, auto& ws) { return sum + ws.m_vsize; });
    m_subpackage.m_total_modified_fees = std::accumulate(workspaces.cbegin(), workspaces.cend(), CAmount{0},
        [](CAmount sum, auto& ws) { return sum + ws.m_modified_fees; });
    const CFeeRate package_feerate(m_subpackage.m_total_modified_fees, m_subpackage.m_total_vsize);
    std::vector<Wtxid> all_package_wtxids;
    all_package_wtxids.reserve(workspaces.size());
    std::transform(workspaces.cbegin(), workspaces.cend(), std::back_inserter(all_package_wtxids),
                   [](const auto& ws) { return ws.m_ptx->GetWitnessHash(); });
    TxValidationState placeholder_state;
    if (args.m_package_feerates &&
        !CheckFeeRate(m_subpackage.m_total_vsize, m_subpackage.m_total_modified_fees, placeholder_state)) {
        package_state.Invalid(PackageValidationResult::PCKG_TX, "transaction failed");
        return PackageMempoolAcceptResult(package_state, {{workspaces.back().m_ptx->GetWitnessHash(),
            MempoolAcceptResult::FeeFailure(placeholder_state, CFeeRate(m_subpackage.m_total_modified_fees, m_subpackage.m_total_vsize), all_package_wtxids)}});
    }

    // Apply package mempool ancestor/descendant limits. Skip if there is only one transaction,
    // because it's unnecessary.
    if (txns.size() > 1 && !PackageMempoolChecks(txns, workspaces, m_subpackage.m_total_vsize, package_state)) {
        return PackageMempoolAcceptResult(package_state, std::move(results));
    }

    // Now that we've bounded the resulting possible ancestry count, check package for dust spends
    if (m_pool.m_opts.require_standard) {
        TxValidationState child_state;
        Wtxid child_wtxid;
        if (!CheckEphemeralSpends(txns, m_pool.m_opts.dust_relay_feerate, m_pool, child_state, child_wtxid)) {
            package_state.Invalid(PackageValidationResult::PCKG_TX, "unspent-dust");
            results.emplace(child_wtxid, MempoolAcceptResult::Failure(child_state));
            return PackageMempoolAcceptResult(package_state, std::move(results));
        }
    }

    for (Workspace& ws : workspaces) {
        ws.m_package_feerate = package_feerate;
        if (!PolicyScriptChecks(args, ws)) {
            // Exit early to avoid doing pointless work. Update the failed tx result; the rest are unfinished.
            package_state.Invalid(PackageValidationResult::PCKG_TX, "transaction failed");
            results.emplace(ws.m_ptx->GetWitnessHash(), MempoolAcceptResult::Failure(ws.m_state));
            return PackageMempoolAcceptResult(package_state, std::move(results));
        }
        if (args.m_test_accept) {
            const auto effective_feerate = args.m_package_feerates ? ws.m_package_feerate :
                CFeeRate{ws.m_modified_fees, static_cast<uint32_t>(ws.m_vsize)};
            const auto effective_feerate_wtxids = args.m_package_feerates ? all_package_wtxids :
                std::vector<Wtxid>{ws.m_ptx->GetWitnessHash()};
            results.emplace(ws.m_ptx->GetWitnessHash(),
                            MempoolAcceptResult::Success(std::move(m_subpackage.m_replaced_transactions),
                                                         ws.m_vsize, ws.m_base_fees, effective_feerate,
                                                         effective_feerate_wtxids));
        }
    }

    if (args.m_test_accept) return PackageMempoolAcceptResult(package_state, std::move(results));

    if (!SubmitPackage(args, workspaces, package_state, results)) {
        // PackageValidationState filled in by SubmitPackage().
        return PackageMempoolAcceptResult(package_state, std::move(results));
    }

    return PackageMempoolAcceptResult(package_state, std::move(results));
}

void MemPoolAccept::CleanupTemporaryCoins()
{
    // There are 3 kinds of coins in m_view:
    // (1) Temporary coins from the transactions in subpackage, constructed by m_viewmempool.
    // (2) Mempool coins from transactions in the mempool, constructed by m_viewmempool.
    // (3) Confirmed coins fetched from our current UTXO set.
    //
    // (1) Temporary coins need to be removed, regardless of whether the transaction was submitted.
    // If the transaction was submitted to the mempool, m_viewmempool will be able to fetch them from
    // there. If it wasn't submitted to mempool, it is incorrect to keep them - future calls may try
    // to spend those coins that don't actually exist.
    // (2) Mempool coins also need to be removed. If the mempool contents have changed as a result
    // of submitting or replacing transactions, coins previously fetched from mempool may now be
    // spent or nonexistent. Those coins need to be deleted from m_view.
    // (3) Confirmed coins don't need to be removed. The chainstate has not changed (we are
    // holding cs_main and no blocks have been processed) so the confirmed tx cannot disappear like
    // a mempool tx can. The coin may now be spent after we submitted a tx to mempool, but
    // we have already checked that the package does not have 2 transactions spending the same coin.
    // Keeping them in m_view is an optimization to not re-fetch confirmed coins if we later look up
    // inputs for this transaction again.
    for (const auto& outpoint : m_viewmempool.GetNonBaseCoins()) {
        // In addition to resetting m_viewmempool, we also need to manually delete these coins from
        // m_view because it caches copies of the coins it fetched from m_viewmempool previously.
        m_view.Uncache(outpoint);
    }
    // This deletes the temporary and mempool coins.
    m_viewmempool.Reset();
}

PackageMempoolAcceptResult MemPoolAccept::AcceptSubPackage(const std::vector<CTransactionRef>& subpackage, ATMPArgs& args)
{
    AssertLockHeld(::cs_main);
    AssertLockHeld(m_pool.cs);
    auto result = [&]() EXCLUSIVE_LOCKS_REQUIRED(::cs_main, m_pool.cs) {
        if (subpackage.size() > 1) {
            return AcceptMultipleTransactions(subpackage, args);
        }
        const auto& tx = subpackage.front();
        ATMPArgs single_args = ATMPArgs::SingleInPackageAccept(args);
        const auto single_res = AcceptSingleTransaction(tx, single_args);
        PackageValidationState package_state_wrapped;
        if (single_res.m_result_type != MempoolAcceptResult::ResultType::VALID) {
            package_state_wrapped.Invalid(PackageValidationResult::PCKG_TX, "transaction failed");
        }
        return PackageMempoolAcceptResult(package_state_wrapped, {{tx->GetWitnessHash(), single_res}});
    }();

    // Clean up m_view and m_viewmempool so that other subpackage evaluations don't have access to
    // coins they shouldn't. Keep some coins in order to minimize re-fetching coins from the UTXO set.
    // Clean up package feerate and rbf calculations
    ClearSubPackageState();

    return result;
}

PackageMempoolAcceptResult MemPoolAccept::AcceptPackage(const Package& package, ATMPArgs& args)
{
    Assert(!package.empty());
    AssertLockHeld(cs_main);
    // Used if returning a PackageMempoolAcceptResult directly from this function.
    PackageValidationState package_state_quit_early;

    // There are two topologies we are able to handle through this function:
    // (1) A single transaction
    // (2) A child-with-unconfirmed-parents package.
    // Check that the package is well-formed. If it isn't, we won't try to validate any of the
    // transactions and thus won't return any MempoolAcceptResults, just a package-wide error.

    // Context-free package checks.
    if (!IsWellFormedPackage(package, package_state_quit_early, /*require_sorted=*/true)) {
        return PackageMempoolAcceptResult(package_state_quit_early, {});
    }

    if (package.size() > 1) {
        // All transactions in the package must be a parent of the last transaction. This is just an
        // opportunity for us to fail fast on a context-free check without taking the mempool lock.
        if (!IsChildWithParents(package)) {
            package_state_quit_early.Invalid(PackageValidationResult::PCKG_POLICY, "package-not-child-with-parents");
            return PackageMempoolAcceptResult(package_state_quit_early, {});
        }

        // IsChildWithParents() guarantees the package is > 1 transactions.
        assert(package.size() > 1);
        // The package must be 1 child with all of its unconfirmed parents. The package is expected to
        // be sorted, so the last transaction is the child.
        const auto& child = package.back();
        std::unordered_set<uint256, SaltedTxidHasher> unconfirmed_parent_txids;
        std::transform(package.cbegin(), package.cend() - 1,
                       std::inserter(unconfirmed_parent_txids, unconfirmed_parent_txids.end()),
                       [](const auto& tx) { return tx->GetHash(); });

        // All child inputs must refer to a preceding package transaction or a confirmed UTXO. The only
        // way to verify this is to look up the child's inputs in our current coins view (not including
        // mempool), and enforce that all parents not present in the package be available at chain tip.
        // Since this check can bring new coins into the coins cache, keep track of these coins and
        // uncache them if we don't end up submitting this package to the mempool.
        const CCoinsViewCache& coins_tip_cache = m_active_chainstate.CoinsTip();
        for (const auto& input : child->vin) {
            if (!coins_tip_cache.HaveCoinInCache(input.prevout)) {
                args.m_coins_to_uncache.push_back(input.prevout);
            }
        }
        // Using the MemPoolAccept m_view cache allows us to look up these same coins faster later.
        // This should be connecting directly to CoinsTip, not to m_viewmempool, because we specifically
        // require inputs to be confirmed if they aren't in the package.
        m_view.SetBackend(m_active_chainstate.CoinsTip());
        const auto package_or_confirmed = [this, &unconfirmed_parent_txids](const auto& input) {
             return unconfirmed_parent_txids.count(input.prevout.hash) > 0 || m_view.HaveCoin(input.prevout);
        };
        if (!std::all_of(child->vin.cbegin(), child->vin.cend(), package_or_confirmed)) {
            package_state_quit_early.Invalid(PackageValidationResult::PCKG_POLICY, "package-not-child-with-unconfirmed-parents");
            return PackageMempoolAcceptResult(package_state_quit_early, {});
        }
        // Protect against bugs where we pull more inputs from disk that miss being added to
        // coins_to_uncache. The backend will be connected again when needed in PreChecks.
        m_view.SetBackend(m_dummy);
    }

    LOCK(m_pool.cs);
    // Stores results from which we will create the returned PackageMempoolAcceptResult.
    // A result may be changed if a mempool transaction is evicted later due to LimitMempoolSize().
    std::map<Wtxid, MempoolAcceptResult> results_final;
    // Results from individual validation which will be returned if no other result is available for
    // this transaction. "Nonfinal" because if a transaction fails by itself but succeeds later
    // (i.e. when evaluated with a fee-bumping child), the result in this map may be discarded.
    std::map<Wtxid, MempoolAcceptResult> individual_results_nonfinal;
    // Tracks whether we think package submission could result in successful entry to the mempool
    bool quit_early{false};
    std::vector<CTransactionRef> txns_package_eval;
    for (const auto& tx : package) {
        const auto& wtxid = tx->GetWitnessHash();
        const auto& txid = tx->GetHash();
        // There are 3 possibilities: already in mempool, same-txid-diff-wtxid already in mempool,
        // or not in mempool. An already confirmed tx is treated as one not in mempool, because all
        // we know is that the inputs aren't available.
        if (m_pool.exists(GenTxid::Wtxid(wtxid))) {
            // Exact transaction already exists in the mempool.
            // Node operators are free to set their mempool policies however they please, nodes may receive
            // transactions in different orders, and malicious counterparties may try to take advantage of
            // policy differences to pin or delay propagation of transactions. As such, it's possible for
            // some package transaction(s) to already be in the mempool, and we don't want to reject the
            // entire package in that case (as that could be a censorship vector). De-duplicate the
            // transactions that are already in the mempool, and only call AcceptMultipleTransactions() with
            // the new transactions. This ensures we don't double-count transaction counts and sizes when
            // checking ancestor/descendant limits, or double-count transaction fees for fee-related policy.
            const auto& entry{*Assert(m_pool.GetEntry(txid))};
            results_final.emplace(wtxid, MempoolAcceptResult::MempoolTx(entry.GetTxSize(), entry.GetFee()));
        } else if (m_pool.exists(GenTxid::Txid(txid))) {
            // Transaction with the same non-witness data but different witness (same txid,
            // different wtxid) already exists in the mempool.
            //
            // We don't allow replacement transactions right now, so just swap the package
            // transaction for the mempool one. Note that we are ignoring the validity of the
            // package transaction passed in.
            // TODO: allow witness replacement in packages.
            const auto& entry{*Assert(m_pool.GetEntry(txid))};
            // Provide the wtxid of the mempool tx so that the caller can look it up in the mempool.
            results_final.emplace(wtxid, MempoolAcceptResult::MempoolTxDifferentWitness(entry.GetTx().GetWitnessHash()));
        } else {
            // Transaction does not already exist in the mempool.
            // Try submitting the transaction on its own.
            const auto single_package_res = AcceptSubPackage({tx}, args);
            const auto& single_res = single_package_res.m_tx_results.at(wtxid);
            if (single_res.m_result_type == MempoolAcceptResult::ResultType::VALID) {
                // The transaction succeeded on its own and is now in the mempool. Don't include it
                // in package validation, because its fees should only be "used" once.
                assert(m_pool.exists(GenTxid::Wtxid(wtxid)));
                results_final.emplace(wtxid, single_res);
            } else if (package.size() == 1 || // If there is only one transaction, no need to retry it "as a package"
                       (single_res.m_state.GetResult() != TxValidationResult::TX_RECONSIDERABLE &&
                       single_res.m_state.GetResult() != TxValidationResult::TX_MISSING_INPUTS)) {
                // Package validation policy only differs from individual policy in its evaluation
                // of feerate. For example, if a transaction fails here due to violation of a
                // consensus rule, the result will not change when it is submitted as part of a
                // package. To minimize the amount of repeated work, unless the transaction fails
                // due to feerate or missing inputs (its parent is a previous transaction in the
                // package that failed due to feerate), don't run package validation. Note that this
                // decision might not make sense if different types of packages are allowed in the
                // future.  Continue individually validating the rest of the transactions, because
                // some of them may still be valid.
                quit_early = true;
                package_state_quit_early.Invalid(PackageValidationResult::PCKG_TX, "transaction failed");
                individual_results_nonfinal.emplace(wtxid, single_res);
            } else {
                individual_results_nonfinal.emplace(wtxid, single_res);
                txns_package_eval.push_back(tx);
            }
        }
    }

    auto multi_submission_result = quit_early || txns_package_eval.empty() ? PackageMempoolAcceptResult(package_state_quit_early, {}) :
        AcceptSubPackage(txns_package_eval, args);
    PackageValidationState& package_state_final = multi_submission_result.m_state;

    // This is invoked by AcceptSubPackage() already, so this is just here for
    // clarity (since it's not permitted to invoke LimitMempoolSize() while a
    // changeset is outstanding).
    ClearSubPackageState();

    // Make sure we haven't exceeded max mempool size.
    // Package transactions that were submitted to mempool or already in mempool may be evicted.
    LimitMempoolSize(m_pool, m_active_chainstate.CoinsTip());

    for (const auto& tx : package) {
        const auto& wtxid = tx->GetWitnessHash();
        if (multi_submission_result.m_tx_results.count(wtxid) > 0) {
            // We shouldn't have re-submitted if the tx result was already in results_final.
            Assume(results_final.count(wtxid) == 0);
            // If it was submitted, check to see if the tx is still in the mempool. It could have
            // been evicted due to LimitMempoolSize() above.
            const auto& txresult = multi_submission_result.m_tx_results.at(wtxid);
            if (txresult.m_result_type == MempoolAcceptResult::ResultType::VALID && !m_pool.exists(GenTxid::Wtxid(wtxid))) {
                package_state_final.Invalid(PackageValidationResult::PCKG_TX, "transaction failed");
                TxValidationState mempool_full_state;
                mempool_full_state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "mempool full");
                results_final.emplace(wtxid, MempoolAcceptResult::Failure(mempool_full_state));
            } else {
                results_final.emplace(wtxid, txresult);
            }
        } else if (const auto it{results_final.find(wtxid)}; it != results_final.end()) {
            // Already-in-mempool transaction. Check to see if it's still there, as it could have
            // been evicted when LimitMempoolSize() was called.
            Assume(it->second.m_result_type != MempoolAcceptResult::ResultType::INVALID);
            Assume(individual_results_nonfinal.count(wtxid) == 0);
            // Query by txid to include the same-txid-different-witness ones.
            if (!m_pool.exists(GenTxid::Txid(tx->GetHash()))) {
                package_state_final.Invalid(PackageValidationResult::PCKG_TX, "transaction failed");
                TxValidationState mempool_full_state;
                mempool_full_state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "mempool full");
                // Replace the previous result.
                results_final.erase(wtxid);
                results_final.emplace(wtxid, MempoolAcceptResult::Failure(mempool_full_state));
            }
        } else if (const auto it{individual_results_nonfinal.find(wtxid)}; it != individual_results_nonfinal.end()) {
            Assume(it->second.m_result_type == MempoolAcceptResult::ResultType::INVALID);
            // Interesting result from previous processing.
            results_final.emplace(wtxid, it->second);
        }
    }
    Assume(results_final.size() == package.size());
    return PackageMempoolAcceptResult(package_state_final, std::move(results_final));
}

} // anon namespace

MempoolAcceptResult AcceptToMemoryPool(Chainstate& active_chainstate, const CTransactionRef& tx,
                                       int64_t accept_time, bool bypass_limits, bool test_accept)
{
    AssertLockHeld(::cs_main);
    const CChainParams& chainparams{active_chainstate.m_chainman.GetParams()};
    assert(active_chainstate.GetMempool() != nullptr);
    CTxMemPool& pool{*active_chainstate.GetMempool()};

    std::vector<COutPoint> coins_to_uncache;
    auto args = MemPoolAccept::ATMPArgs::SingleAccept(chainparams, accept_time, bypass_limits, coins_to_uncache, test_accept);
    MempoolAcceptResult result = MemPoolAccept(pool, active_chainstate).AcceptSingleTransaction(tx, args);
    if (result.m_result_type != MempoolAcceptResult::ResultType::VALID) {
        // Remove coins that were not present in the coins cache before calling
        // AcceptSingleTransaction(); this is to prevent memory DoS in case we receive a large
        // number of invalid transactions that attempt to overrun the in-memory coins cache
        // (`CCoinsViewCache::cacheCoins`).

        for (const COutPoint& hashTx : coins_to_uncache)
            active_chainstate.CoinsTip().Uncache(hashTx);
        TRACEPOINT(mempool, rejected,
                tx->GetHash().data(),
                result.m_state.GetRejectReason().c_str()
        );
    }
    // After we've (potentially) uncached entries, ensure our coins cache is still within its size limits
    BlockValidationState state_dummy;
    active_chainstate.FlushStateToDisk(state_dummy, FlushStateMode::PERIODIC);
    return result;
}

PackageMempoolAcceptResult ProcessNewPackage(Chainstate& active_chainstate, CTxMemPool& pool,
                                                   const Package& package, bool test_accept, const std::optional<CFeeRate>& client_maxfeerate)
{
    AssertLockHeld(cs_main);
    assert(!package.empty());
    assert(std::all_of(package.cbegin(), package.cend(), [](const auto& tx){return tx != nullptr;}));

    std::vector<COutPoint> coins_to_uncache;
    const CChainParams& chainparams = active_chainstate.m_chainman.GetParams();
    auto result = [&]() EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
        AssertLockHeld(cs_main);
        if (test_accept) {
            auto args = MemPoolAccept::ATMPArgs::PackageTestAccept(chainparams, GetTime(), coins_to_uncache);
            return MemPoolAccept(pool, active_chainstate).AcceptMultipleTransactions(package, args);
        } else {
            auto args = MemPoolAccept::ATMPArgs::PackageChildWithParents(chainparams, GetTime(), coins_to_uncache, client_maxfeerate);
            return MemPoolAccept(pool, active_chainstate).AcceptPackage(package, args);
        }
    }();

    // Uncache coins pertaining to transactions that were not submitted to the mempool.
    if (test_accept || result.m_state.IsInvalid()) {
        for (const COutPoint& hashTx : coins_to_uncache) {
            active_chainstate.CoinsTip().Uncache(hashTx);
        }
    }
    // Ensure the coins cache is still within limits.
    BlockValidationState state_dummy;
    active_chainstate.FlushStateToDisk(state_dummy, FlushStateMode::PERIODIC);
    return result;
}

CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& consensusParams)
{
    int halvings = nHeight / consensusParams.nSubsidyHalvingInterval;
    // Force block reward to zero when right shift is undefined.
    if (halvings >= 64)
        return 0;

    CAmount nSubsidy = 50 * COIN;
    // Subsidy is cut in half every 210,000 blocks which will occur approximately every 4 years.
    nSubsidy >>= halvings;
    return nSubsidy;
}

CoinsViews::CoinsViews(DBParams db_params, CoinsViewOptions options)
    : m_dbview{std::move(db_params), std::move(options)},
      m_catcherview(&m_dbview) {}

void CoinsViews::InitCache()
{
    AssertLockHeld(::cs_main);
    m_cacheview = std::make_unique<CCoinsViewCache>(&m_catcherview);
}

Chainstate::Chainstate(
    CTxMemPool* mempool,
    BlockManager& blockman,
    ChainstateManager& chainman,
    std::optional<uint256> from_snapshot_blockhash)
    : m_mempool(mempool),
      m_blockman(blockman),
      m_chainman(chainman),
      m_from_snapshot_blockhash(from_snapshot_blockhash) {}

const CBlockIndex* Chainstate::SnapshotBase() const
{
    if (!m_from_snapshot_blockhash) return nullptr;
    if (!m_cached_snapshot_base) m_cached_snapshot_base = Assert(m_chainman.m_blockman.LookupBlockIndex(*m_from_snapshot_blockhash));
    return m_cached_snapshot_base;
}

void Chainstate::InitCoinsDB(
    size_t cache_size_bytes,
    bool in_memory,
    bool should_wipe,
    fs::path leveldb_name)
{
    if (m_from_snapshot_blockhash) {
        leveldb_name += node::SNAPSHOT_CHAINSTATE_SUFFIX;
    }

    m_coins_views = std::make_unique<CoinsViews>(
        DBParams{
            .path = m_chainman.m_options.datadir / leveldb_name,
            .cache_bytes = cache_size_bytes,
            .memory_only = in_memory,
            .wipe_data = should_wipe,
            .obfuscate = true,
            .options = m_chainman.m_options.coins_db},
        m_chainman.m_options.coins_view);

    m_coinsdb_cache_size_bytes = cache_size_bytes;
}

void Chainstate::InitCoinsCache(size_t cache_size_bytes)
{
    AssertLockHeld(::cs_main);
    assert(m_coins_views != nullptr);
    m_coinstip_cache_size_bytes = cache_size_bytes;
    m_coins_views->InitCache();
}

// Note that though this is marked const, we may end up modifying `m_cached_finished_ibd`, which
// is a performance-related implementation detail. This function must be marked
// `const` so that `CValidationInterface` clients (which are given a `const Chainstate*`)
// can call it.
//
bool ChainstateManager::IsInitialBlockDownload() const
{
    // Optimization: pre-test latch before taking the lock.
    if (m_cached_finished_ibd.load(std::memory_order_relaxed))
        return false;

    LOCK(cs_main);
    if (m_cached_finished_ibd.load(std::memory_order_relaxed))
        return false;
    if (m_blockman.LoadingBlocks()) {
        return true;
    }
    CChain& chain{ActiveChain()};
    if (chain.Tip() == nullptr) {
        return true;
    }
    if (chain.Tip()->nChainWork < MinimumChainWork()) {
        return true;
    }
    if (chain.Tip()->Time() < Now<NodeSeconds>() - m_options.max_tip_age) {
        return true;
    }
    LogPrintf("Leaving InitialBlockDownload (latching to false)\n");
    m_cached_finished_ibd.store(true, std::memory_order_relaxed);
    return false;
}

void Chainstate::CheckForkWarningConditions()
{
    AssertLockHeld(cs_main);

    // Before we get past initial download, we cannot reliably alert about forks
    // (we assume we don't get stuck on a fork before finishing our initial sync)
    // Also not applicable to the background chainstate
    if (m_chainman.IsInitialBlockDownload() || this->GetRole() == ChainstateRole::BACKGROUND) {
        return;
    }

    if (m_chainman.m_best_invalid && m_chainman.m_best_invalid->nChainWork > m_chain.Tip()->nChainWork + (GetBlockProof(*m_chain.Tip()) * 6)) {
        LogPrintf("%s: Warning: Found invalid chain at least ~6 blocks longer than our best chain.\nChain state database corruption likely.\n", __func__);
        m_chainman.GetNotifications().warningSet(
            kernel::Warning::LARGE_WORK_INVALID_CHAIN,
            _("Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade."));
    } else {
        m_chainman.GetNotifications().warningUnset(kernel::Warning::LARGE_WORK_INVALID_CHAIN);
    }
}

// Called both upon regular invalid block discovery *and* InvalidateBlock
void Chainstate::InvalidChainFound(CBlockIndex* pindexNew)
{
    AssertLockHeld(cs_main);
    if (!m_chainman.m_best_invalid || pindexNew->nChainWork > m_chainman.m_best_invalid->nChainWork) {
        m_chainman.m_best_invalid = pindexNew;
    }
    SetBlockFailureFlags(pindexNew);
    if (m_chainman.m_best_header != nullptr && m_chainman.m_best_header->GetAncestor(pindexNew->nHeight) == pindexNew) {
        m_chainman.RecalculateBestHeader();
    }

    LogPrintf("%s: invalid block=%s  height=%d  log2_work=%f  date=%s\n", __func__,
      pindexNew->GetBlockHash().ToString(), pindexNew->nHeight,
      log(pindexNew->nChainWork.getdouble())/log(2.0), FormatISO8601DateTime(pindexNew->GetBlockTime()));
    CBlockIndex *tip = m_chain.Tip();
    assert (tip);
    LogPrintf("%s:  current best=%s  height=%d  log2_work=%f  date=%s\n", __func__,
      tip->GetBlockHash().ToString(), m_chain.Height(), log(tip->nChainWork.getdouble())/log(2.0),
      FormatISO8601DateTime(tip->GetBlockTime()));
    CheckForkWarningConditions();
}

// Same as InvalidChainFound, above, except not called directly from InvalidateBlock,
// which does its own setBlockIndexCandidates management.
void Chainstate::InvalidBlockFound(CBlockIndex* pindex, const BlockValidationState& state)
{
    AssertLockHeld(cs_main);
    if (state.GetResult() != BlockValidationResult::BLOCK_MUTATED) {
        pindex->nStatus |= BLOCK_FAILED_VALID;
        m_chainman.m_failed_blocks.insert(pindex);
        m_blockman.m_dirty_blockindex.insert(pindex);
        setBlockIndexCandidates.erase(pindex);
        InvalidChainFound(pindex);
    }
}

void UpdateCoins(const CTransaction& tx, CCoinsViewCache& inputs, CTxUndo &txundo, int nHeight)
{
    // mark inputs spent
    if (!tx.IsCoinBase()) {
        txundo.vprevout.reserve(tx.vin.size());
        for (const CTxIn &txin : tx.vin) {
            txundo.vprevout.emplace_back();
            bool is_spent = inputs.SpendCoin(txin.prevout, &txundo.vprevout.back());
            assert(is_spent);
        }
    }
    // add outputs
    AddCoins(inputs, tx, nHeight);
}

std::optional<std::pair<ScriptError, std::string>> CScriptCheck::operator()() {
    const CScript &scriptSig = ptxTo->vin[nIn].scriptSig;
    const CScriptWitness *witness = &ptxTo->vin[nIn].scriptWitness;
    ScriptError error{SCRIPT_ERR_UNKNOWN_ERROR};
    if (VerifyScript(scriptSig, m_tx_out.scriptPubKey, witness, nFlags, CachingTransactionSignatureChecker(ptxTo, nIn, m_tx_out.nValue, cacheStore, *m_signature_cache, *txdata), &error)) {
        return std::nullopt;
    } else {
        auto debug_str = strprintf("input %i of %s (wtxid %s), spending %s:%i", nIn, ptxTo->GetHash().ToString(), ptxTo->GetWitnessHash().ToString(), ptxTo->vin[nIn].prevout.hash.ToString(), ptxTo->vin[nIn].prevout.n);
        return std::make_pair(error, std::move(debug_str));
    }
}

ValidationCache::ValidationCache(const size_t script_execution_cache_bytes, const size_t signature_cache_bytes)
    : m_signature_cache{signature_cache_bytes}
{
    // Setup the salted hasher
    uint256 nonce = GetRandHash();
    // We want the nonce to be 64 bytes long to force the hasher to process
    // this chunk, which makes later hash computations more efficient. We
    // just write our 32-byte entropy twice to fill the 64 bytes.
    m_script_execution_cache_hasher.Write(nonce.begin(), 32);
    m_script_execution_cache_hasher.Write(nonce.begin(), 32);

    const auto [num_elems, approx_size_bytes] = m_script_execution_cache.setup_bytes(script_execution_cache_bytes);
    LogPrintf("Using %zu MiB out of %zu MiB requested for script execution cache, able to store %zu elements\n",
              approx_size_bytes >> 20, script_execution_cache_bytes >> 20, num_elems);
}

/**
 * Check whether all of this transaction's input scripts succeed.
 *
 * This involves ECDSA signature checks so can be computationally intensive. This function should
 * only be called after the cheap sanity checks in CheckTxInputs passed.
 *
 * If pvChecks is not nullptr, script checks are pushed onto it instead of being performed inline. Any
 * script checks which are not necessary (eg due to script execution cache hits) are, obviously,
 * not pushed onto pvChecks/run.
 *
 * Setting cacheSigStore/cacheFullScriptStore to false will remove elements from the corresponding cache
 * which are matched. This is useful for checking blocks where we will likely never need the cache
 * entry again.
 *
 * Note that we may set state.reason to NOT_STANDARD for extra soft-fork flags in flags, block-checking
 * callers should probably reset it to CONSENSUS in such cases.
 *
 * Non-static (and redeclared) in src/test/txvalidationcache_tests.cpp
 */
bool CheckInputScripts(const CTransaction& tx, TxValidationState& state,
                       const CCoinsViewCache& inputs, unsigned int flags, bool cacheSigStore,
                       bool cacheFullScriptStore, PrecomputedTransactionData& txdata,
                       ValidationCache& validation_cache,
                       std::vector<CScriptCheck>* pvChecks)
{
    if (tx.IsCoinBase()) return true;

    if (pvChecks) {
        pvChecks->reserve(tx.vin.size());
    }

    // First check if script executions have been cached with the same
    // flags. Note that this assumes that the inputs provided are
    // correct (ie that the transaction hash which is in tx's prevouts
    // properly commits to the scriptPubKey in the inputs view of that
    // transaction).
    uint256 hashCacheEntry;
    CSHA256 hasher = validation_cache.ScriptExecutionCacheHasher();
    hasher.Write(UCharCast(tx.GetWitnessHash().begin()), 32).Write((unsigned char*)&flags, sizeof(flags)).Finalize(hashCacheEntry.begin());
    AssertLockHeld(cs_main); //TODO: Remove this requirement by making CuckooCache not require external locks
    if (validation_cache.m_script_execution_cache.contains(hashCacheEntry, !cacheFullScriptStore)) {
        return true;
    }

    if (!txdata.m_spent_outputs_ready) {
        std::vector<CTxOut> spent_outputs;
        spent_outputs.reserve(tx.vin.size());

        for (const auto& txin : tx.vin) {
            const COutPoint& prevout = txin.prevout;
            const Coin& coin = inputs.AccessCoin(prevout);
            assert(!coin.IsSpent());
            spent_outputs.emplace_back(coin.out);
        }
        txdata.Init(tx, std::move(spent_outputs));
    }
    assert(txdata.m_spent_outputs.size() == tx.vin.size());

    for (unsigned int i = 0; i < tx.vin.size(); i++) {

        // We very carefully only pass in things to CScriptCheck which
        // are clearly committed to by tx' witness hash. This provides
        // a sanity check that our caching is not introducing consensus
        // failures through additional data in, eg, the coins being
        // spent being checked as a part of CScriptCheck.

        // Verify signature
        CScriptCheck check(txdata.m_spent_outputs[i], tx, validation_cache.m_signature_cache, i, flags, cacheSigStore, &txdata);
        if (pvChecks) {
            pvChecks->emplace_back(std::move(check));
        } else if (auto result = check(); result.has_value()) {
            if (flags & STANDARD_NOT_MANDATORY_VERIFY_FLAGS) {
                // Check whether the failure was caused by a
                // non-mandatory script verification check, such as
                // non-standard DER encodings or non-null dummy
                // arguments; if so, ensure we return NOT_STANDARD
                // instead of CONSENSUS to avoid downstream users
                // splitting the network between upgraded and
                // non-upgraded nodes by banning CONSENSUS-failing
                // data providers.
                CScriptCheck check2(txdata.m_spent_outputs[i], tx, validation_cache.m_signature_cache, i,
                        flags & ~STANDARD_NOT_MANDATORY_VERIFY_FLAGS, cacheSigStore, &txdata);
                auto mandatory_result = check2();
                if (!mandatory_result.has_value()) {
                    return state.Invalid(TxValidationResult::TX_NOT_STANDARD, strprintf("non-mandatory-script-verify-flag (%s)", ScriptErrorString(result->first)), result->second);
                } else {
                    // If the second check failed, it failed due to a mandatory script verification
                    // flag, but the first check might have failed on a non-mandatory script
                    // verification flag.
                    //
                    // Avoid reporting a mandatory script check failure with a non-mandatory error
                    // string by reporting the error from the second check.
                    result = mandatory_result;
                }
            }

            // MANDATORY flag failures correspond to
            // TxValidationResult::TX_CONSENSUS.
            return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("mandatory-script-verify-flag-failed (%s)", ScriptErrorString(result->first)), result->second);
        }
    }

    if (cacheFullScriptStore && !pvChecks) {
        // We executed all of the provided scripts, and were told to
        // cache the result. Do so now.
        validation_cache.m_script_execution_cache.insert(hashCacheEntry);
    }

    return true;
}

bool FatalError(Notifications& notifications, BlockValidationState& state, const bilingual_str& message)
{
    notifications.fatalError(message);
    return state.Error(message.original);
}

/**
 * Restore the UTXO in a Coin at a given COutPoint
 * @param undo The Coin to be restored.
 * @param view The coins view to which to apply the changes.
 * @param out The out point that corresponds to the tx input.
 * @return A DisconnectResult as an int
 */
int ApplyTxInUndo(Coin&& undo, CCoinsViewCache& view, const COutPoint& out)
{
    bool fClean = true;

    if (view.HaveCoin(out)) fClean = false; // overwriting transaction output

    if (undo.nHeight == 0) {
        // Missing undo metadata (height and coinbase). Older versions included this
        // information only in undo records for the last spend of a transactions'
        // outputs. This implies that it must be present for some other output of the same tx.
        const Coin& alternate = AccessByTxid(view, out.hash);
        if (!alternate.IsSpent()) {
            undo.nHeight = alternate.nHeight;
            undo.fCoinBase = alternate.fCoinBase;
        } else {
            return DISCONNECT_FAILED; // adding output for transaction without known metadata
        }
    }
    // If the coin already exists as an unspent coin in the cache, then the
    // possible_overwrite parameter to AddCoin must be set to true. We have
    // already checked whether an unspent coin exists above using HaveCoin, so
    // we don't need to guess. When fClean is false, an unspent coin already
    // existed and it is an overwrite.
    view.AddCoin(out, std::move(undo), !fClean);

    return fClean ? DISCONNECT_OK : DISCONNECT_UNCLEAN;
}

/** Undo the effects of this block (with given index) on the UTXO set represented by coins.
 *  When FAILED is returned, view is left in an indeterminate state. */
DisconnectResult Chainstate::DisconnectBlock(const CBlock& block, const CBlockIndex* pindex, CCoinsViewCache& view)
{
    AssertLockHeld(::cs_main);
    bool fClean = true;

    CBlockUndo blockUndo;
    if (!m_blockman.ReadBlockUndo(blockUndo, *pindex)) {
        LogError("DisconnectBlock(): failure reading undo data\n");
        return DISCONNECT_FAILED;
    }

    if (blockUndo.vtxundo.size() + 1 != block.vtx.size()) {
        LogError("DisconnectBlock(): block and undo data inconsistent\n");
        return DISCONNECT_FAILED;
    }

    // Ignore blocks that contain transactions which are 'overwritten' by later transactions,
    // unless those are already completely spent.
    // See https://github.com/bitcoin/bitcoin/issues/22596 for additional information.
    // Note: the blocks specified here are different than the ones used in ConnectBlock because DisconnectBlock
    // unwinds the blocks in reverse. As a result, the inconsistency is not discovered until the earlier
    // blocks with the duplicate coinbase transactions are disconnected.
    bool fEnforceBIP30 = !((pindex->nHeight==91722 && pindex->GetBlockHash() == uint256{"00000000000271a2dc26e7667f8419f2e15416dc6955e5a6c6cdf3f2574dd08e"}) ||
                           (pindex->nHeight==91812 && pindex->GetBlockHash() == uint256{"00000000000af0aed4792b1acee3d966af36cf5def14935db8de83d6f9306f2f"}));

    // undo transactions in reverse order
    for (int i = block.vtx.size() - 1; i >= 0; i--) {
        const CTransaction &tx = *(block.vtx[i]);
        Txid hash = tx.GetHash();
        bool is_coinbase = tx.IsCoinBase();
        bool is_bip30_exception = (is_coinbase && !fEnforceBIP30);

        // Check that all outputs are available and match the outputs in the block itself
        // exactly.
        for (size_t o = 0; o < tx.vout.size(); o++) {
            if (!tx.vout[o].scriptPubKey.IsUnspendable()) {
                COutPoint out(hash, o);
                Coin coin;
                bool is_spent = view.SpendCoin(out, &coin);
                if (!is_spent || tx.vout[o] != coin.out || pindex->nHeight != coin.nHeight || is_coinbase != coin.fCoinBase) {
                    if (!is_bip30_exception) {
                        fClean = false; // transaction output mismatch
                    }
                }
            }
        }

        // restore inputs
        if (i > 0) { // not coinbases
            CTxUndo &txundo = blockUndo.vtxundo[i-1];
            if (txundo.vprevout.size() != tx.vin.size()) {
                LogError("DisconnectBlock(): transaction and undo data inconsistent\n");
                return DISCONNECT_FAILED;
            }
            for (unsigned int j = tx.vin.size(); j > 0;) {
                --j;
                const COutPoint& out = tx.vin[j].prevout;
                int res = ApplyTxInUndo(std::move(txundo.vprevout[j]), view, out);
                if (res == DISCONNECT_FAILED) return DISCONNECT_FAILED;
                fClean = fClean && res != DISCONNECT_UNCLEAN;
            }
            // At this point, all of txundo.vprevout should have been moved out.
        }
    }

    // move best block pointer to prevout block
    view.SetBestBlock(pindex->pprev->GetBlockHash());

    return fClean ? DISCONNECT_OK : DISCONNECT_UNCLEAN;
}

static unsigned int GetBlockScriptFlags(const CBlockIndex& block_index, const ChainstateManager& chainman)
{
    const Consensus::Params& consensusparams = chainman.GetConsensus();

    // BIP16 didn't become active until Apr 1 2012 (on mainnet, and
    // retroactively applied to testnet)
    // However, only one historical block violated the P2SH rules (on both
    // mainnet and testnet).
    // Similarly, only one historical block violated the TAPROOT rules on
    // mainnet.
    // For simplicity, always leave P2SH+WITNESS+TAPROOT on except for the two
    // violating blocks.
    uint32_t flags{SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_TAPROOT};
    const auto it{consensusparams.script_flag_exceptions.find(*Assert(block_index.phashBlock))};
    if (it != consensusparams.script_flag_exceptions.end()) {
        flags = it->second;
    }

    // Enforce the DERSIG (BIP66) rule
    if (DeploymentActiveAt(block_index, chainman, Consensus::DEPLOYMENT_DERSIG)) {
        flags |= SCRIPT_VERIFY_DERSIG;
    }

    // Enforce CHECKLOCKTIMEVERIFY (BIP65)
    if (DeploymentActiveAt(block_index, chainman, Consensus::DEPLOYMENT_CLTV)) {
        flags |= SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY;
    }

    // Enforce CHECKSEQUENCEVERIFY (BIP112)
    if (DeploymentActiveAt(block_index, chainman, Consensus::DEPLOYMENT_CSV)) {
        flags |= SCRIPT_VERIFY_CHECKSEQUENCEVERIFY;
    }

    // Enforce BIP147 NULLDUMMY (activated simultaneously with segwit)
    if (DeploymentActiveAt(block_index, chainman, Consensus::DEPLOYMENT_SEGWIT)) {
        flags |= SCRIPT_VERIFY_NULLDUMMY;
    }

    return flags;
}


/** Apply the effects of this block (with given index) on the UTXO set represented by coins.
 *  Validity checks that depend on the UTXO set are also done; ConnectBlock()
 *  can fail if those validity checks fail (among other reasons). */
bool Chainstate::ConnectBlock(const CBlock& block, BlockValidationState& state, CBlockIndex* pindex,
                               CCoinsViewCache& view, bool fJustCheck)
{
    AssertLockHeld(cs_main);
    assert(pindex);

    uint256 block_hash{block.GetHash()};
    assert(*pindex->phashBlock == block_hash);

    const auto time_start{SteadyClock::now()};
    const CChainParams& params{m_chainman.GetParams()};

    // Check it again in case a previous version let a bad block in
    // NOTE: We don't currently (re-)invoke ContextualCheckBlock() or
    // ContextualCheckBlockHeader() here. This means that if we add a new
    // consensus rule that is enforced in one of those two functions, then we
    // may have let in a block that violates the rule prior to updating the
    // software, and we would NOT be enforcing the rule here. Fully solving
    // upgrade from one software version to the next after a consensus rule
    // change is potentially tricky and issue-specific (see NeedsRedownload()
    // for one approach that was used for BIP 141 deployment).
    // Also, currently the rule against blocks more than 2 hours in the future
    // is enforced in ContextualCheckBlockHeader(); we wouldn't want to
    // re-enforce that rule here (at least until we make it impossible for
    // the clock to go backward).
    if (!CheckBlock(block, state, params.GetConsensus(), !fJustCheck, !fJustCheck)) {
        if (state.GetResult() == BlockValidationResult::BLOCK_MUTATED) {
            // We don't write down blocks to disk if they may have been
            // corrupted, so this should be impossible unless we're having hardware
            // problems.
            return FatalError(m_chainman.GetNotifications(), state, _("Corrupt block found indicating potential hardware failure."));
        }
        LogError("%s: Consensus::CheckBlock: %s\n", __func__, state.ToString());
        return false;
    }

    // verify that the view's current state corresponds to the previous block
    uint256 hashPrevBlock = pindex->pprev == nullptr ? uint256() : pindex->pprev->GetBlockHash();
    assert(hashPrevBlock == view.GetBestBlock());

    m_chainman.num_blocks_total++;

    // Special case for the genesis block, skipping connection of its transactions
    // (its coinbase is unspendable)
    if (block_hash == params.GetConsensus().hashGenesisBlock) {
        if (!fJustCheck)
            view.SetBestBlock(pindex->GetBlockHash());
        return true;
    }

    bool fScriptChecks = true;
    if (!m_chainman.AssumedValidBlock().IsNull()) {
        // We've been configured with the hash of a block which has been externally verified to have a valid history.
        // A suitable default value is included with the software and updated from time to time.  Because validity
        //  relative to a piece of software is an objective fact these defaults can be easily reviewed.
        // This setting doesn't force the selection of any particular chain but makes validating some faster by
        //  effectively caching the result of part of the verification.
        BlockMap::const_iterator it{m_blockman.m_block_index.find(m_chainman.AssumedValidBlock())};
        if (it != m_blockman.m_block_index.end()) {
            if (it->second.GetAncestor(pindex->nHeight) == pindex &&
                m_chainman.m_best_header->GetAncestor(pindex->nHeight) == pindex &&
                m_chainman.m_best_header->nChainWork >= m_chainman.MinimumChainWork()) {
                // This block is a member of the assumed verified chain and an ancestor of the best header.
                // Script verification is skipped when connecting blocks under the
                // assumevalid block. Assuming the assumevalid block is valid this
                // is safe because block merkle hashes are still computed and checked,
                // Of course, if an assumed valid block is invalid due to false scriptSigs
                // this optimization would allow an invalid chain to be accepted.
                // The equivalent time check discourages hash power from extorting the network via DOS attack
                //  into accepting an invalid block through telling users they must manually set assumevalid.
                //  Requiring a software change or burying the invalid block, regardless of the setting, makes
                //  it hard to hide the implication of the demand.  This also avoids having release candidates
                //  that are hardly doing any signature verification at all in testing without having to
                //  artificially set the default assumed verified block further back.
                // The test against the minimum chain work prevents the skipping when denied access to any chain at
                //  least as good as the expected chain.
                fScriptChecks = (GetBlockProofEquivalentTime(*m_chainman.m_best_header, *pindex, *m_chainman.m_best_header, params.GetConsensus()) <= 60 * 60 * 24 * 7 * 2);
            }
        }
    }

    const auto time_1{SteadyClock::now()};
    m_chainman.time_check += time_1 - time_start;
    LogDebug(BCLog::BENCH, "    - Sanity checks: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_1 - time_start),
             Ticks<SecondsDouble>(m_chainman.time_check),
             Ticks<MillisecondsDouble>(m_chainman.time_check) / m_chainman.num_blocks_total);

    // Do not allow blocks that contain transactions which 'overwrite' older transactions,
    // unless those are already completely spent.
    // If such overwrites are allowed, coinbases and transactions depending upon those
    // can be duplicated to remove the ability to spend the first instance -- even after
    // being sent to another address.
    // See BIP30, CVE-2012-1909, and https://r6.ca/blog/20120206T005236Z.html for more information.
    // This rule was originally applied to all blocks with a timestamp after March 15, 2012, 0:00 UTC.
    // Now that the whole chain is irreversibly beyond that time it is applied to all blocks except the
    // two in the chain that violate it. This prevents exploiting the issue against nodes during their
    // initial block download.
    bool fEnforceBIP30 = !IsBIP30Repeat(*pindex);

    // Once BIP34 activated it was not possible to create new duplicate coinbases and thus other than starting
    // with the 2 existing duplicate coinbase pairs, not possible to create overwriting txs.  But by the
    // time BIP34 activated, in each of the existing pairs the duplicate coinbase had overwritten the first
    // before the first had been spent.  Since those coinbases are sufficiently buried it's no longer possible to create further
    // duplicate transactions descending from the known pairs either.
    // If we're on the known chain at height greater than where BIP34 activated, we can save the db accesses needed for the BIP30 check.

    // BIP34 requires that a block at height X (block X) has its coinbase
    // scriptSig start with a CScriptNum of X (indicated height X).  The above
    // logic of no longer requiring BIP30 once BIP34 activates is flawed in the
    // case that there is a block X before the BIP34 height of 227,931 which has
    // an indicated height Y where Y is greater than X.  The coinbase for block
    // X would also be a valid coinbase for block Y, which could be a BIP30
    // violation.  An exhaustive search of all mainnet coinbases before the
    // BIP34 height which have an indicated height greater than the block height
    // reveals many occurrences. The 3 lowest indicated heights found are
    // 209,921, 490,897, and 1,983,702 and thus coinbases for blocks at these 3
    // heights would be the first opportunity for BIP30 to be violated.

    // The search reveals a great many blocks which have an indicated height
    // greater than 1,983,702, so we simply remove the optimization to skip
    // BIP30 checking for blocks at height 1,983,702 or higher.  Before we reach
    // that block in another 25 years or so, we should take advantage of a
    // future consensus change to do a new and improved version of BIP34 that
    // will actually prevent ever creating any duplicate coinbases in the
    // future.
    static constexpr int BIP34_IMPLIES_BIP30_LIMIT = 1983702;

    // There is no potential to create a duplicate coinbase at block 209,921
    // because this is still before the BIP34 height and so explicit BIP30
    // checking is still active.

    // The final case is block 176,684 which has an indicated height of
    // 490,897. Unfortunately, this issue was not discovered until about 2 weeks
    // before block 490,897 so there was not much opportunity to address this
    // case other than to carefully analyze it and determine it would not be a
    // problem. Block 490,897 was, in fact, mined with a different coinbase than
    // block 176,684, but it is important to note that even if it hadn't been or
    // is remined on an alternate fork with a duplicate coinbase, we would still
    // not run into a BIP30 violation.  This is because the coinbase for 176,684
    // is spent in block 185,956 in transaction
    // d4f7fbbf92f4a3014a230b2dc70b8058d02eb36ac06b4a0736d9d60eaa9e8781.  This
    // spending transaction can't be duplicated because it also spends coinbase
    // 0328dd85c331237f18e781d692c92de57649529bd5edf1d01036daea32ffde29.  This
    // coinbase has an indicated height of over 4.2 billion, and wouldn't be
    // duplicatable until that height, and it's currently impossible to create a
    // chain that long. Nevertheless we may wish to consider a future soft fork
    // which retroactively prevents block 490,897 from creating a duplicate
    // coinbase. The two historical BIP30 violations often provide a confusing
    // edge case when manipulating the UTXO and it would be simpler not to have
    // another edge case to deal with.

    // testnet3 has no blocks before the BIP34 height with indicated heights
    // post BIP34 before approximately height 486,000,000. After block
    // 1,983,702 testnet3 starts doing unnecessary BIP30 checking again.
    assert(pindex->pprev);
    CBlockIndex* pindexBIP34height = pindex->pprev->GetAncestor(params.GetConsensus().BIP34Height);
    //Only continue to enforce if we're below BIP34 activation height or the block hash at that height doesn't correspond.
    fEnforceBIP30 = fEnforceBIP30 && (!pindexBIP34height || !(pindexBIP34height->GetBlockHash() == params.GetConsensus().BIP34Hash));

    // TODO: Remove BIP30 checking from block height 1,983,702 on, once we have a
    // consensus change that ensures coinbases at those heights cannot
    // duplicate earlier coinbases.
    if (fEnforceBIP30 || pindex->nHeight >= BIP34_IMPLIES_BIP30_LIMIT) {
        for (const auto& tx : block.vtx) {
            for (size_t o = 0; o < tx->vout.size(); o++) {
                if (view.HaveCoin(COutPoint(tx->GetHash(), o))) {
                    state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-txns-BIP30",
                                  "tried to overwrite transaction");
                }
            }
        }
    }

    // Enforce BIP68 (sequence locks)
    int nLockTimeFlags = 0;
    if (DeploymentActiveAt(*pindex, m_chainman, Consensus::DEPLOYMENT_CSV)) {
        nLockTimeFlags |= LOCKTIME_VERIFY_SEQUENCE;
    }

    // Get the script flags for this block
    unsigned int flags{GetBlockScriptFlags(*pindex, m_chainman)};

    const auto time_2{SteadyClock::now()};
    m_chainman.time_forks += time_2 - time_1;
    LogDebug(BCLog::BENCH, "    - Fork checks: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_2 - time_1),
             Ticks<SecondsDouble>(m_chainman.time_forks),
             Ticks<MillisecondsDouble>(m_chainman.time_forks) / m_chainman.num_blocks_total);

    CBlockUndo blockundo;

    // Precomputed transaction data pointers must not be invalidated
    // until after `control` has run the script checks (potentially
    // in multiple threads). Preallocate the vector size so a new allocation
    // doesn't invalidate pointers into the vector, and keep txsdata in scope
    // for as long as `control`.
    std::optional<CCheckQueueControl<CScriptCheck>> control;
    if (auto& queue = m_chainman.GetCheckQueue(); queue.HasThreads() && fScriptChecks) control.emplace(queue);

    std::vector<PrecomputedTransactionData> txsdata(block.vtx.size());

    std::vector<int> prevheights;
    CAmount nFees = 0;
    int nInputs = 0;
    int64_t nSigOpsCost = 0;
    blockundo.vtxundo.reserve(block.vtx.size() - 1);
    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        if (!state.IsValid()) break;
        const CTransaction &tx = *(block.vtx[i]);

        nInputs += tx.vin.size();

        if (!tx.IsCoinBase())
        {
            CAmount txfee = 0;
            TxValidationState tx_state;
            if (!Consensus::CheckTxInputs(tx, tx_state, view, pindex->nHeight, txfee)) {
                // Any transaction validation failure in ConnectBlock is a block consensus failure
                state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                              tx_state.GetRejectReason(),
                              tx_state.GetDebugMessage() + " in transaction " + tx.GetHash().ToString());
                break;
            }
            nFees += txfee;
            if (!MoneyRange(nFees)) {
                state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-txns-accumulated-fee-outofrange",
                              "accumulated fee in the block out of range");
                break;
            }

            // Check that transaction is BIP68 final
            // BIP68 lock checks (as opposed to nLockTime checks) must
            // be in ConnectBlock because they require the UTXO set
            prevheights.resize(tx.vin.size());
            for (size_t j = 0; j < tx.vin.size(); j++) {
                prevheights[j] = view.AccessCoin(tx.vin[j].prevout).nHeight;
            }

            if (!SequenceLocks(tx, nLockTimeFlags, prevheights, *pindex)) {
                state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-txns-nonfinal",
                              "contains a non-BIP68-final transaction " + tx.GetHash().ToString());
                break;
            }
        }

        // GetTransactionSigOpCost counts 3 types of sigops:
        // * legacy (always)
        // * p2sh (when P2SH enabled in flags and excludes coinbase)
        // * witness (when witness enabled in flags and excludes coinbase)
        nSigOpsCost += GetTransactionSigOpCost(tx, view, flags);
        if (nSigOpsCost > MAX_BLOCK_SIGOPS_COST) {
            state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-blk-sigops", "too many sigops");
            break;
        }

        if (!tx.IsCoinBase() && fScriptChecks)
        {
            bool fCacheResults = fJustCheck; /* Don't cache results if we're actually connecting blocks (still consult the cache, though) */
            bool tx_ok;
            TxValidationState tx_state;
            // If CheckInputScripts is called with a pointer to a checks vector, the resulting checks are appended to it. In that case
            // they need to be added to control which runs them asynchronously. Otherwise, CheckInputScripts runs the checks before returning.
            if (control) {
                std::vector<CScriptCheck> vChecks;
                tx_ok = CheckInputScripts(tx, tx_state, view, flags, fCacheResults, fCacheResults, txsdata[i], m_chainman.m_validation_cache, &vChecks);
                if (tx_ok) control->Add(std::move(vChecks));
            } else {
                tx_ok = CheckInputScripts(tx, tx_state, view, flags, fCacheResults, fCacheResults, txsdata[i], m_chainman.m_validation_cache);
            }
            if (!tx_ok) {
                // Any transaction validation failure in ConnectBlock is a block consensus failure
                state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                              tx_state.GetRejectReason(), tx_state.GetDebugMessage());
                break;
            }
        }

        CTxUndo undoDummy;
        if (i > 0) {
            blockundo.vtxundo.emplace_back();
        }
        UpdateCoins(tx, view, i == 0 ? undoDummy : blockundo.vtxundo.back(), pindex->nHeight);
    }
    const auto time_3{SteadyClock::now()};
    m_chainman.time_connect += time_3 - time_2;
    LogDebug(BCLog::BENCH, "      - Connect %u transactions: %.2fms (%.3fms/tx, %.3fms/txin) [%.2fs (%.2fms/blk)]\n", (unsigned)block.vtx.size(),
             Ticks<MillisecondsDouble>(time_3 - time_2), Ticks<MillisecondsDouble>(time_3 - time_2) / block.vtx.size(),
             nInputs <= 1 ? 0 : Ticks<MillisecondsDouble>(time_3 - time_2) / (nInputs - 1),
             Ticks<SecondsDouble>(m_chainman.time_connect),
             Ticks<MillisecondsDouble>(m_chainman.time_connect) / m_chainman.num_blocks_total);

    CAmount blockReward = nFees + GetBlockSubsidy(pindex->nHeight, params.GetConsensus());
    if (block.vtx[0]->GetValueOut() > blockReward && state.IsValid()) {
        state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-amount",
                      strprintf("coinbase pays too much (actual=%d vs limit=%d)", block.vtx[0]->GetValueOut(), blockReward));
    }
    if (control) {
        auto parallel_result = control->Complete();
        if (parallel_result.has_value() && state.IsValid()) {
            state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, strprintf("mandatory-script-verify-flag-failed (%s)", ScriptErrorString(parallel_result->first)), parallel_result->second);
        }
    }
    if (!state.IsValid()) {
        LogInfo("Block validation error: %s", state.ToString());
        return false;
    }
    const auto time_4{SteadyClock::now()};
    m_chainman.time_verify += time_4 - time_2;
    LogDebug(BCLog::BENCH, "    - Verify %u txins: %.2fms (%.3fms/txin) [%.2fs (%.2fms/blk)]\n", nInputs - 1,
             Ticks<MillisecondsDouble>(time_4 - time_2),
             nInputs <= 1 ? 0 : Ticks<MillisecondsDouble>(time_4 - time_2) / (nInputs - 1),
             Ticks<SecondsDouble>(m_chainman.time_verify),
             Ticks<MillisecondsDouble>(m_chainman.time_verify) / m_chainman.num_blocks_total);

    if (fJustCheck) {
        return true;
    }

    if (!m_blockman.WriteBlockUndo(blockundo, state, *pindex)) {
        return false;
    }

    const auto time_5{SteadyClock::now()};
    m_chainman.time_undo += time_5 - time_4;
    LogDebug(BCLog::BENCH, "    - Write undo data: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_5 - time_4),
             Ticks<SecondsDouble>(m_chainman.time_undo),
             Ticks<MillisecondsDouble>(m_chainman.time_undo) / m_chainman.num_blocks_total);

    if (!pindex->IsValid(BLOCK_VALID_SCRIPTS)) {
        pindex->RaiseValidity(BLOCK_VALID_SCRIPTS);
        m_blockman.m_dirty_blockindex.insert(pindex);
    }

    // add this block to the view's block chain
    view.SetBestBlock(pindex->GetBlockHash());

    const auto time_6{SteadyClock::now()};
    m_chainman.time_index += time_6 - time_5;
    LogDebug(BCLog::BENCH, "    - Index writing: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_6 - time_5),
             Ticks<SecondsDouble>(m_chainman.time_index),
             Ticks<MillisecondsDouble>(m_chainman.time_index) / m_chainman.num_blocks_total);

    TRACEPOINT(validation, block_connected,
        block_hash.data(),
        pindex->nHeight,
        block.vtx.size(),
        nInputs,
        nSigOpsCost,
        Ticks<std::chrono::nanoseconds>(time_5 - time_start)
    );

    return true;
}

CoinsCacheSizeState Chainstate::GetCoinsCacheSizeState()
{
    AssertLockHeld(::cs_main);
    return this->GetCoinsCacheSizeState(
        m_coinstip_cache_size_bytes,
        m_mempool ? m_mempool->m_opts.max_size_bytes : 0);
}

CoinsCacheSizeState Chainstate::GetCoinsCacheSizeState(
    size_t max_coins_cache_size_bytes,
    size_t max_mempool_size_bytes)
{
    AssertLockHeld(::cs_main);
    const int64_t nMempoolUsage = m_mempool ? m_mempool->DynamicMemoryUsage() : 0;
    int64_t cacheSize = CoinsTip().DynamicMemoryUsage();
    int64_t nTotalSpace =
        max_coins_cache_size_bytes + std::max<int64_t>(int64_t(max_mempool_size_bytes) - nMempoolUsage, 0);

    //! No need to periodic flush if at least this much space still available.
    static constexpr int64_t MAX_BLOCK_COINSDB_USAGE_BYTES = 10 * 1024 * 1024;  // 10MB
    int64_t large_threshold =
        std::max((9 * nTotalSpace) / 10, nTotalSpace - MAX_BLOCK_COINSDB_USAGE_BYTES);

    if (cacheSize > nTotalSpace) {
        LogPrintf("Cache size (%s) exceeds total space (%s)\n", cacheSize, nTotalSpace);
        return CoinsCacheSizeState::CRITICAL;
    } else if (cacheSize > large_threshold) {
        return CoinsCacheSizeState::LARGE;
    }
    return CoinsCacheSizeState::OK;
}

bool Chainstate::FlushStateToDisk(
    BlockValidationState &state,
    FlushStateMode mode,
    int nManualPruneHeight)
{
    LOCK(cs_main);
    assert(this->CanFlushToDisk());
    std::set<int> setFilesToPrune;
    bool full_flush_completed = false;

    const size_t coins_count = CoinsTip().GetCacheSize();
    const size_t coins_mem_usage = CoinsTip().DynamicMemoryUsage();

    try {
    {
        bool fFlushForPrune = false;

        CoinsCacheSizeState cache_state = GetCoinsCacheSizeState();
        LOCK(m_blockman.cs_LastBlockFile);
        if (m_blockman.IsPruneMode() && (m_blockman.m_check_for_pruning || nManualPruneHeight > 0) && m_chainman.m_blockman.m_blockfiles_indexed) {
            // make sure we don't prune above any of the prune locks bestblocks
            // pruning is height-based
            int last_prune{m_chain.Height()}; // last height we can prune
            std::optional<std::string> limiting_lock; // prune lock that actually was the limiting factor, only used for logging

            for (const auto& prune_lock : m_blockman.m_prune_locks) {
                if (prune_lock.second.height_first == std::numeric_limits<int>::max()) continue;
                // Remove the buffer and one additional block here to get actual height that is outside of the buffer
                const int lock_height{prune_lock.second.height_first - PRUNE_LOCK_BUFFER - 1};
                last_prune = std::max(1, std::min(last_prune, lock_height));
                if (last_prune == lock_height) {
                    limiting_lock = prune_lock.first;
                }
            }

            if (limiting_lock) {
                LogDebug(BCLog::PRUNE, "%s limited pruning to height %d\n", limiting_lock.value(), last_prune);
            }

            if (nManualPruneHeight > 0) {
                LOG_TIME_MILLIS_WITH_CATEGORY("find files to prune (manual)", BCLog::BENCH);

                m_blockman.FindFilesToPruneManual(
                    setFilesToPrune,
                    std::min(last_prune, nManualPruneHeight),
                    *this, m_chainman);
            } else {
                LOG_TIME_MILLIS_WITH_CATEGORY("find files to prune", BCLog::BENCH);

                m_blockman.FindFilesToPrune(setFilesToPrune, last_prune, *this, m_chainman);
                m_blockman.m_check_for_pruning = false;
            }
            if (!setFilesToPrune.empty()) {
                fFlushForPrune = true;
                if (!m_blockman.m_have_pruned) {
                    m_blockman.m_block_tree_db->WriteFlag("prunedblockfiles", true);
                    m_blockman.m_have_pruned = true;
                }
            }
        }
        const auto nNow{NodeClock::now()};
        // The cache is large and we're within 10% and 10 MiB of the limit, but we have time now (not in the middle of a block processing).
        bool fCacheLarge = mode == FlushStateMode::PERIODIC && cache_state >= CoinsCacheSizeState::LARGE;
        // The cache is over the limit, we have to write now.
        bool fCacheCritical = mode == FlushStateMode::IF_NEEDED && cache_state >= CoinsCacheSizeState::CRITICAL;
        // It's been a while since we wrote the block index and chain state to disk. Do this frequently, so we don't need to redownload or reindex after a crash.
        bool fPeriodicWrite = mode == FlushStateMode::PERIODIC && nNow >= m_next_write;
        // Combine all conditions that result in a write to disk.
        bool should_write = (mode == FlushStateMode::ALWAYS) || fCacheLarge || fCacheCritical || fPeriodicWrite || fFlushForPrune;
        // Write blocks, block index and best chain related state to disk.
        if (should_write) {
            LogDebug(BCLog::COINDB, "Writing chainstate to disk: flush mode=%s, prune=%d, large=%d, critical=%d, periodic=%d",
                     FlushStateModeNames[size_t(mode)], fFlushForPrune, fCacheLarge, fCacheCritical, fPeriodicWrite);

            // Ensure we can write block index
            if (!CheckDiskSpace(m_blockman.m_opts.blocks_dir)) {
                return FatalError(m_chainman.GetNotifications(), state, _("Disk space is too low!"));
            }
            {
                LOG_TIME_MILLIS_WITH_CATEGORY("write block and undo data to disk", BCLog::BENCH);

                // First make sure all block and undo data is flushed to disk.
                // TODO: Handle return error, or add detailed comment why it is
                // safe to not return an error upon failure.
                if (!m_blockman.FlushChainstateBlockFile(m_chain.Height())) {
                    LogPrintLevel(BCLog::VALIDATION, BCLog::Level::Warning, "%s: Failed to flush block file.\n", __func__);
                }
            }

            // Then update all block file information (which may refer to block and undo files).
            {
                LOG_TIME_MILLIS_WITH_CATEGORY("write block index to disk", BCLog::BENCH);

                if (!m_blockman.WriteBlockIndexDB()) {
                    return FatalError(m_chainman.GetNotifications(), state, _("Failed to write to block index database."));
                }
            }
            // Finally remove any pruned files
            if (fFlushForPrune) {
                LOG_TIME_MILLIS_WITH_CATEGORY("unlink pruned files", BCLog::BENCH);

                m_blockman.UnlinkPrunedFiles(setFilesToPrune);
            }

            if (!CoinsTip().GetBestBlock().IsNull()) {
                if (coins_mem_usage >= WARN_FLUSH_COINS_SIZE) LogWarning("Flushing large (%d GiB) UTXO set to disk, it may take several minutes", coins_mem_usage >> 30);
                LOG_TIME_MILLIS_WITH_CATEGORY(strprintf("write coins cache to disk (%d coins, %.2fKiB)",
                    coins_count, coins_mem_usage >> 10), BCLog::BENCH);

                // Typical Coin structures on disk are around 48 bytes in size.
                // Pushing a new one to the database can cause it to be written
                // twice (once in the log, and once in the tables). This is already
                // an overestimation, as most will delete an existing entry or
                // overwrite one. Still, use a conservative safety factor of 2.
                if (!CheckDiskSpace(m_chainman.m_options.datadir, 48 * 2 * 2 * CoinsTip().GetCacheSize())) {
                    return FatalError(m_chainman.GetNotifications(), state, _("Disk space is too low!"));
                }
                // Flush the chainstate (which may refer to block index entries).
                const auto empty_cache{(mode == FlushStateMode::ALWAYS) || fCacheLarge || fCacheCritical};
                if (empty_cache ? !CoinsTip().Flush() : !CoinsTip().Sync()) {
                    return FatalError(m_chainman.GetNotifications(), state, _("Failed to write to coin database."));
                }
                full_flush_completed = true;
                TRACEPOINT(utxocache, flush,
                    int64_t{Ticks<std::chrono::microseconds>(NodeClock::now() - nNow)},
                    (uint32_t)mode,
                    (uint64_t)coins_count,
                    (uint64_t)coins_mem_usage,
                    (bool)fFlushForPrune);
            }
        }

        if (should_write || m_next_write == NodeClock::time_point::max()) {
            constexpr auto range{DATABASE_WRITE_INTERVAL_MAX - DATABASE_WRITE_INTERVAL_MIN};
            m_next_write = FastRandomContext().rand_uniform_delay(NodeClock::now() + DATABASE_WRITE_INTERVAL_MIN, range);
        }
    }
    if (full_flush_completed && m_chainman.m_options.signals) {
        // Update best block in wallet (so we can detect restored wallets).
        m_chainman.m_options.signals->ChainStateFlushed(this->GetRole(), m_chain.GetLocator());
    }
    } catch (const std::runtime_error& e) {
        return FatalError(m_chainman.GetNotifications(), state, strprintf(_("System error while flushing: %s"), e.what()));
    }
    return true;
}

void Chainstate::ForceFlushStateToDisk()
{
    BlockValidationState state;
    if (!this->FlushStateToDisk(state, FlushStateMode::ALWAYS)) {
        LogPrintf("%s: failed to flush state (%s)\n", __func__, state.ToString());
    }
}

void Chainstate::PruneAndFlush()
{
    BlockValidationState state;
    m_blockman.m_check_for_pruning = true;
    if (!this->FlushStateToDisk(state, FlushStateMode::NONE)) {
        LogPrintf("%s: failed to flush state (%s)\n", __func__, state.ToString());
    }
}

static void UpdateTipLog(
    const ChainstateManager& chainman,
    const CCoinsViewCache& coins_tip,
    const CBlockIndex* tip,
    const std::string& func_name,
    const std::string& prefix,
    const std::string& warning_messages) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{

    AssertLockHeld(::cs_main);

    // We use the private LogPrintLevel_ macro here because we want to disable rate limiting from this location. This code
    // could use LogPrintLevel with the Info level to bypass rate-limiting, but it is more opaque. Using LogPrintLevel_
    // allows us to explicitly disable rate limiting rather than relying on implicit LogPrintLevel behavior.
    LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Info, /*should_ratelimit=*/false, "%s%s: new best=%s height=%d version=0x%08x log2_work=%f tx=%lu date='%s' progress=%f cache=%.1fMiB(%utxo)%s\n",
                  prefix, func_name,
                  tip->GetBlockHash().ToString(), tip->nHeight, tip->nVersion,
                  log(tip->nChainWork.getdouble()) / log(2.0), tip->m_chain_tx_count,
                  FormatISO8601DateTime(tip->GetBlockTime()),
                  chainman.GuessVerificationProgress(tip),
                  coins_tip.DynamicMemoryUsage() * (1.0 / (1 << 20)),
                  coins_tip.GetCacheSize(),
                  !warning_messages.empty() ? strprintf(" warning='%s'", warning_messages) : "");
}

void Chainstate::UpdateTip(const CBlockIndex* pindexNew)
{
    AssertLockHeld(::cs_main);
    const auto& coins_tip = this->CoinsTip();

    // The remainder of the function isn't relevant if we are not acting on
    // the active chainstate, so return if need be.
    if (this != &m_chainman.ActiveChainstate()) {
        // Only log every so often so that we don't bury log messages at the tip.
        constexpr int BACKGROUND_LOG_INTERVAL = 2000;
        if (pindexNew->nHeight % BACKGROUND_LOG_INTERVAL == 0) {
            UpdateTipLog(m_chainman, coins_tip, pindexNew, __func__, "[background validation] ", "");
        }
        return;
    }

    // New best block
    if (m_mempool) {
        m_mempool->AddTransactionsUpdated(1);
    }

    std::vector<bilingual_str> warning_messages;
    if (!m_chainman.IsInitialBlockDownload()) {
        auto bits = m_chainman.m_versionbitscache.CheckUnknownActivations(pindexNew, m_chainman.GetParams());
        for (auto [bit, active] : bits) {
            const bilingual_str warning = strprintf(_("Unknown new rules activated (versionbit %i)"), bit);
            if (active) {
                m_chainman.GetNotifications().warningSet(kernel::Warning::UNKNOWN_NEW_RULES_ACTIVATED, warning);
            } else {
                warning_messages.push_back(warning);
            }
        }
    }
    UpdateTipLog(m_chainman, coins_tip, pindexNew, __func__, "",
                 util::Join(warning_messages, Untranslated(", ")).original);
}

/** Disconnect m_chain's tip.
  * After calling, the mempool will be in an inconsistent state, with
  * transactions from disconnected blocks being added to disconnectpool.  You
  * should make the mempool consistent again by calling MaybeUpdateMempoolForReorg.
  * with cs_main held.
  *
  * If disconnectpool is nullptr, then no disconnected transactions are added to
  * disconnectpool (note that the caller is responsible for mempool consistency
  * in any case).
  */
bool Chainstate::DisconnectTip(BlockValidationState& state, DisconnectedBlockTransactions* disconnectpool)
{
    AssertLockHeld(cs_main);
    if (m_mempool) AssertLockHeld(m_mempool->cs);

    CBlockIndex *pindexDelete = m_chain.Tip();
    assert(pindexDelete);
    assert(pindexDelete->pprev);
    // Read block from disk.
    std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
    CBlock& block = *pblock;
    if (!m_blockman.ReadBlock(block, *pindexDelete)) {
        LogError("DisconnectTip(): Failed to read block\n");
        return false;
    }
    // Apply the block atomically to the chain state.
    const auto time_start{SteadyClock::now()};
    {
        CCoinsViewCache view(&CoinsTip());
        assert(view.GetBestBlock() == pindexDelete->GetBlockHash());
        if (DisconnectBlock(block, pindexDelete, view) != DISCONNECT_OK) {
            LogError("DisconnectTip(): DisconnectBlock %s failed\n", pindexDelete->GetBlockHash().ToString());
            return false;
        }
        bool flushed = view.Flush();
        assert(flushed);
    }
    LogDebug(BCLog::BENCH, "- Disconnect block: %.2fms\n",
             Ticks<MillisecondsDouble>(SteadyClock::now() - time_start));

    {
        // Prune locks that began at or after the tip should be moved backward so they get a chance to reorg
        const int max_height_first{pindexDelete->nHeight - 1};
        for (auto& prune_lock : m_blockman.m_prune_locks) {
            if (prune_lock.second.height_first <= max_height_first) continue;

            prune_lock.second.height_first = max_height_first;
            LogDebug(BCLog::PRUNE, "%s prune lock moved back to %d\n", prune_lock.first, max_height_first);
        }
    }

    // Write the chain state to disk, if necessary.
    if (!FlushStateToDisk(state, FlushStateMode::IF_NEEDED)) {
        return false;
    }

    if (disconnectpool && m_mempool) {
        // Save transactions to re-add to mempool at end of reorg. If any entries are evicted for
        // exceeding memory limits, remove them and their descendants from the mempool.
        for (auto&& evicted_tx : disconnectpool->AddTransactionsFromBlock(block.vtx)) {
            m_mempool->removeRecursive(*evicted_tx, MemPoolRemovalReason::REORG);
        }
    }

    m_chain.SetTip(*pindexDelete->pprev);

    UpdateTip(pindexDelete->pprev);
    // Let wallets know transactions went from 1-confirmed to
    // 0-confirmed or conflicted:
    if (m_chainman.m_options.signals) {
        m_chainman.m_options.signals->BlockDisconnected(pblock, pindexDelete);
    }
    return true;
}

struct PerBlockConnectTrace {
    CBlockIndex* pindex = nullptr;
    std::shared_ptr<const CBlock> pblock;
    PerBlockConnectTrace() = default;
};
/**
 * Used to track blocks whose transactions were applied to the UTXO state as a
 * part of a single ActivateBestChainStep call.
 *
 * This class is single-use, once you call GetBlocksConnected() you have to throw
 * it away and make a new one.
 */
class ConnectTrace {
private:
    std::vector<PerBlockConnectTrace> blocksConnected;

public:
    explicit ConnectTrace() : blocksConnected(1) {}

    void BlockConnected(CBlockIndex* pindex, std::shared_ptr<const CBlock> pblock) {
        assert(!blocksConnected.back().pindex);
        assert(pindex);
        assert(pblock);
        blocksConnected.back().pindex = pindex;
        blocksConnected.back().pblock = std::move(pblock);
        blocksConnected.emplace_back();
    }

    std::vector<PerBlockConnectTrace>& GetBlocksConnected() {
        // We always keep one extra block at the end of our list because
        // blocks are added after all the conflicted transactions have
        // been filled in. Thus, the last entry should always be an empty
        // one waiting for the transactions from the next block. We pop
        // the last entry here to make sure the list we return is sane.
        assert(!blocksConnected.back().pindex);
        blocksConnected.pop_back();
        return blocksConnected;
    }
};

/**
 * Connect a new block to m_chain. pblock is either nullptr or a pointer to a CBlock
 * corresponding to pindexNew, to bypass loading it again from disk.
 *
 * The block is added to connectTrace if connection succeeds.
 */
bool Chainstate::ConnectTip(BlockValidationState& state, CBlockIndex* pindexNew, const std::shared_ptr<const CBlock>& pblock, ConnectTrace& connectTrace, DisconnectedBlockTransactions& disconnectpool)
{
    AssertLockHeld(cs_main);
    if (m_mempool) AssertLockHeld(m_mempool->cs);

    assert(pindexNew->pprev == m_chain.Tip());
    // Read block from disk.
    const auto time_1{SteadyClock::now()};
    std::shared_ptr<const CBlock> pthisBlock;
    if (!pblock) {
        std::shared_ptr<CBlock> pblockNew = std::make_shared<CBlock>();
        if (!m_blockman.ReadBlock(*pblockNew, *pindexNew)) {
            return FatalError(m_chainman.GetNotifications(), state, _("Failed to read block."));
        }
        pthisBlock = pblockNew;
    } else {
        LogDebug(BCLog::BENCH, "  - Using cached block\n");
        pthisBlock = pblock;
    }
    const CBlock& blockConnecting = *pthisBlock;
    // Apply the block atomically to the chain state.
    const auto time_2{SteadyClock::now()};
    SteadyClock::time_point time_3;
    // When adding aggregate statistics in the future, keep in mind that
    // num_blocks_total may be zero until the ConnectBlock() call below.
    LogDebug(BCLog::BENCH, "  - Load block from disk: %.2fms\n",
             Ticks<MillisecondsDouble>(time_2 - time_1));
    {
        CCoinsViewCache view(&CoinsTip());
        bool rv = ConnectBlock(blockConnecting, state, pindexNew, view);
        if (m_chainman.m_options.signals) {
            m_chainman.m_options.signals->BlockChecked(blockConnecting, state);
        }
        if (!rv) {
            if (state.IsInvalid())
                InvalidBlockFound(pindexNew, state);
            LogError("%s: ConnectBlock %s failed, %s\n", __func__, pindexNew->GetBlockHash().ToString(), state.ToString());
            return false;
        }
        time_3 = SteadyClock::now();
        m_chainman.time_connect_total += time_3 - time_2;
        assert(m_chainman.num_blocks_total > 0);
        LogDebug(BCLog::BENCH, "  - Connect total: %.2fms [%.2fs (%.2fms/blk)]\n",
                 Ticks<MillisecondsDouble>(time_3 - time_2),
                 Ticks<SecondsDouble>(m_chainman.time_connect_total),
                 Ticks<MillisecondsDouble>(m_chainman.time_connect_total) / m_chainman.num_blocks_total);
        bool flushed = view.Flush();
        assert(flushed);
    }
    const auto time_4{SteadyClock::now()};
    m_chainman.time_flush += time_4 - time_3;
    LogDebug(BCLog::BENCH, "  - Flush: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_4 - time_3),
             Ticks<SecondsDouble>(m_chainman.time_flush),
             Ticks<MillisecondsDouble>(m_chainman.time_flush) / m_chainman.num_blocks_total);
    // Write the chain state to disk, if necessary.
    if (!FlushStateToDisk(state, FlushStateMode::IF_NEEDED)) {
        return false;
    }
    const auto time_5{SteadyClock::now()};
    m_chainman.time_chainstate += time_5 - time_4;
    LogDebug(BCLog::BENCH, "  - Writing chainstate: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_5 - time_4),
             Ticks<SecondsDouble>(m_chainman.time_chainstate),
             Ticks<MillisecondsDouble>(m_chainman.time_chainstate) / m_chainman.num_blocks_total);
    // Remove conflicting transactions from the mempool.;
    if (m_mempool) {
        m_mempool->removeForBlock(blockConnecting.vtx, pindexNew->nHeight);
        disconnectpool.removeForBlock(blockConnecting.vtx);
    }
    // Update m_chain & related variables.
    m_chain.SetTip(*pindexNew);
    UpdateTip(pindexNew);

    const auto time_6{SteadyClock::now()};
    m_chainman.time_post_connect += time_6 - time_5;
    m_chainman.time_total += time_6 - time_1;
    LogDebug(BCLog::BENCH, "  - Connect postprocess: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_6 - time_5),
             Ticks<SecondsDouble>(m_chainman.time_post_connect),
             Ticks<MillisecondsDouble>(m_chainman.time_post_connect) / m_chainman.num_blocks_total);
    LogDebug(BCLog::BENCH, "- Connect block: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_6 - time_1),
             Ticks<SecondsDouble>(m_chainman.time_total),
             Ticks<MillisecondsDouble>(m_chainman.time_total) / m_chainman.num_blocks_total);

    // If we are the background validation chainstate, check to see if we are done
    // validating the snapshot (i.e. our tip has reached the snapshot's base block).
    if (this != &m_chainman.ActiveChainstate()) {
        // This call may set `m_disabled`, which is referenced immediately afterwards in
        // ActivateBestChain, so that we stop connecting blocks past the snapshot base.
        m_chainman.MaybeCompleteSnapshotValidation();
    }

    connectTrace.BlockConnected(pindexNew, std::move(pthisBlock));
    return true;
}

/**
 * Return the tip of the chain with the most work in it, that isn't
 * known to be invalid (it's however far from certain to be valid).
 */
CBlockIndex* Chainstate::FindMostWorkChain()
{
    AssertLockHeld(::cs_main);
    do {
        CBlockIndex *pindexNew = nullptr;

        // Find the best candidate header.
        {
            std::set<CBlockIndex*, CBlockIndexWorkComparator>::reverse_iterator it = setBlockIndexCandidates.rbegin();
            if (it == setBlockIndexCandidates.rend())
                return nullptr;
            pindexNew = *it;
        }

        // Check whether all blocks on the path between the currently active chain and the candidate are valid.
        // Just going until the active chain is an optimization, as we know all blocks in it are valid already.
        CBlockIndex *pindexTest = pindexNew;
        bool fInvalidAncestor = false;
        while (pindexTest && !m_chain.Contains(pindexTest)) {
            assert(pindexTest->HaveNumChainTxs() || pindexTest->nHeight == 0);

            // Pruned nodes may have entries in setBlockIndexCandidates for
            // which block files have been deleted.  Remove those as candidates
            // for the most work chain if we come across them; we can't switch
            // to a chain unless we have all the non-active-chain parent blocks.
            bool fFailedChain = pindexTest->nStatus & BLOCK_FAILED_MASK;
            bool fMissingData = !(pindexTest->nStatus & BLOCK_HAVE_DATA);
            if (fFailedChain || fMissingData) {
                // Candidate chain is not usable (either invalid or missing data)
                if (fFailedChain && (m_chainman.m_best_invalid == nullptr || pindexNew->nChainWork > m_chainman.m_best_invalid->nChainWork)) {
                    m_chainman.m_best_invalid = pindexNew;
                }
                CBlockIndex *pindexFailed = pindexNew;
                // Remove the entire chain from the set.
                while (pindexTest != pindexFailed) {
                    if (fFailedChain) {
                        pindexFailed->nStatus |= BLOCK_FAILED_CHILD;
                        m_blockman.m_dirty_blockindex.insert(pindexFailed);
                    } else if (fMissingData) {
                        // If we're missing data, then add back to m_blocks_unlinked,
                        // so that if the block arrives in the future we can try adding
                        // to setBlockIndexCandidates again.
                        m_blockman.m_blocks_unlinked.insert(
                            std::make_pair(pindexFailed->pprev, pindexFailed));
                    }
                    setBlockIndexCandidates.erase(pindexFailed);
                    pindexFailed = pindexFailed->pprev;
                }
                setBlockIndexCandidates.erase(pindexTest);
                fInvalidAncestor = true;
                break;
            }
            pindexTest = pindexTest->pprev;
        }
        if (!fInvalidAncestor)
            return pindexNew;
    } while(true);
}

/** Delete all entries in setBlockIndexCandidates that are worse than the current tip. */
void Chainstate::PruneBlockIndexCandidates() {
    // Note that we can't delete the current block itself, as we may need to return to it later in case a
    // reorganization to a better block fails.
    std::set<CBlockIndex*, CBlockIndexWorkComparator>::iterator it = setBlockIndexCandidates.begin();
    while (it != setBlockIndexCandidates.end() && setBlockIndexCandidates.value_comp()(*it, m_chain.Tip())) {
        setBlockIndexCandidates.erase(it++);
    }
    // Either the current tip or a successor of it we're working towards is left in setBlockIndexCandidates.
    assert(!setBlockIndexCandidates.empty());
}

/**
 * Try to make some progress towards making pindexMostWork the active block.
 * pblock is either nullptr or a pointer to a CBlock corresponding to pindexMostWork.
 *
 * @returns true unless a system error occurred
 */
bool Chainstate::ActivateBestChainStep(BlockValidationState& state, CBlockIndex* pindexMostWork, const std::shared_ptr<const CBlock>& pblock, bool& fInvalidFound, ConnectTrace& connectTrace)
{
    AssertLockHeld(cs_main);
    if (m_mempool) AssertLockHeld(m_mempool->cs);

    const CBlockIndex* pindexOldTip = m_chain.Tip();
    const CBlockIndex* pindexFork = m_chain.FindFork(pindexMostWork);

    // Disconnect active blocks which are no longer in the best chain.
    bool fBlocksDisconnected = false;
    DisconnectedBlockTransactions disconnectpool{MAX_DISCONNECTED_TX_POOL_BYTES};
    while (m_chain.Tip() && m_chain.Tip() != pindexFork) {
        if (!DisconnectTip(state, &disconnectpool)) {
            // This is likely a fatal error, but keep the mempool consistent,
            // just in case. Only remove from the mempool in this case.
            MaybeUpdateMempoolForReorg(disconnectpool, false);

            // If we're unable to disconnect a block during normal operation,
            // then that is a failure of our local system -- we should abort
            // rather than stay on a less work chain.
            FatalError(m_chainman.GetNotifications(), state, _("Failed to disconnect block."));
            return false;
        }
        fBlocksDisconnected = true;
    }

    // Build list of new blocks to connect (in descending height order).
    std::vector<CBlockIndex*> vpindexToConnect;
    bool fContinue = true;
    int nHeight = pindexFork ? pindexFork->nHeight : -1;
    while (fContinue && nHeight != pindexMostWork->nHeight) {
        // Don't iterate the entire list of potential improvements toward the best tip, as we likely only need
        // a few blocks along the way.
        int nTargetHeight = std::min(nHeight + 32, pindexMostWork->nHeight);
        vpindexToConnect.clear();
        vpindexToConnect.reserve(nTargetHeight - nHeight);
        CBlockIndex* pindexIter = pindexMostWork->GetAncestor(nTargetHeight);
        while (pindexIter && pindexIter->nHeight != nHeight) {
            vpindexToConnect.push_back(pindexIter);
            pindexIter = pindexIter->pprev;
        }
        nHeight = nTargetHeight;

        // Connect new blocks.
        for (CBlockIndex* pindexConnect : vpindexToConnect | std::views::reverse) {
            if (!ConnectTip(state, pindexConnect, pindexConnect == pindexMostWork ? pblock : std::shared_ptr<const CBlock>(), connectTrace, disconnectpool)) {
                if (state.IsInvalid()) {
                    // The block violates a consensus rule.
                    if (state.GetResult() != BlockValidationResult::BLOCK_MUTATED) {
                        InvalidChainFound(vpindexToConnect.front());
                    }
                    state = BlockValidationState();
                    fInvalidFound = true;
                    fContinue = false;
                    break;
                } else {
                    // A system error occurred (disk space, database error, ...).
                    // Make the mempool consistent with the current tip, just in case
                    // any observers try to use it before shutdown.
                    MaybeUpdateMempoolForReorg(disconnectpool, false);
                    return false;
                }
            } else {
                PruneBlockIndexCandidates();
                if (!pindexOldTip || m_chain.Tip()->nChainWork > pindexOldTip->nChainWork) {
                    // We're in a better position than we were. Return temporarily to release the lock.
                    fContinue = false;
                    break;
                }
            }
        }
    }

    if (fBlocksDisconnected) {
        // If any blocks were disconnected, disconnectpool may be non empty.  Add
        // any disconnected transactions back to the mempool.
        MaybeUpdateMempoolForReorg(disconnectpool, true);
    }
    if (m_mempool) m_mempool->check(this->CoinsTip(), this->m_chain.Height() + 1);

    CheckForkWarningConditions();

    return true;
}

static SynchronizationState GetSynchronizationState(bool init, bool blockfiles_indexed)
{
    if (!init) return SynchronizationState::POST_INIT;
    if (!blockfiles_indexed) return SynchronizationState::INIT_REINDEX;
    return SynchronizationState::INIT_DOWNLOAD;
}

bool ChainstateManager::NotifyHeaderTip()
{
    bool fNotify = false;
    bool fInitialBlockDownload = false;
    CBlockIndex* pindexHeader = nullptr;
    {
        LOCK(GetMutex());
        pindexHeader = m_best_header;

        if (pindexHeader != m_last_notified_header) {
            fNotify = true;
            fInitialBlockDownload = IsInitialBlockDownload();
            m_last_notified_header = pindexHeader;
        }
    }
    // Send block tip changed notifications without the lock held
    if (fNotify) {
        GetNotifications().headerTip(GetSynchronizationState(fInitialBlockDownload, m_blockman.m_blockfiles_indexed), pindexHeader->nHeight, pindexHeader->nTime, false);
    }
    return fNotify;
}

static void LimitValidationInterfaceQueue(ValidationSignals& signals) LOCKS_EXCLUDED(cs_main) {
    AssertLockNotHeld(cs_main);

    if (signals.CallbacksPending() > 10) {
        signals.SyncWithValidationInterfaceQueue();
    }
}

bool Chainstate::ActivateBestChain(BlockValidationState& state, std::shared_ptr<const CBlock> pblock)
{
    AssertLockNotHeld(m_chainstate_mutex);

    // Note that while we're often called here from ProcessNewBlock, this is
    // far from a guarantee. Things in the P2P/RPC will often end up calling
    // us in the middle of ProcessNewBlock - do not assume pblock is set
    // sanely for performance or correctness!
    AssertLockNotHeld(::cs_main);

    // ABC maintains a fair degree of expensive-to-calculate internal state
    // because this function periodically releases cs_main so that it does not lock up other threads for too long
    // during large connects - and to allow for e.g. the callback queue to drain
    // we use m_chainstate_mutex to enforce mutual exclusion so that only one caller may execute this function at a time
    LOCK(m_chainstate_mutex);

    // Belt-and-suspenders check that we aren't attempting to advance the background
    // chainstate past the snapshot base block.
    if (WITH_LOCK(::cs_main, return m_disabled)) {
        LogPrintf("m_disabled is set - this chainstate should not be in operation. "
            "Please report this as a bug. %s\n", CLIENT_BUGREPORT);
        return false;
    }

    CBlockIndex *pindexMostWork = nullptr;
    CBlockIndex *pindexNewTip = nullptr;
    bool exited_ibd{false};
    do {
        // Block until the validation queue drains. This should largely
        // never happen in normal operation, however may happen during
        // reindex, causing memory blowup if we run too far ahead.
        // Note that if a validationinterface callback ends up calling
        // ActivateBestChain this may lead to a deadlock! We should
        // probably have a DEBUG_LOCKORDER test for this in the future.
        if (m_chainman.m_options.signals) LimitValidationInterfaceQueue(*m_chainman.m_options.signals);

        {
            LOCK(cs_main);
            {
            // Lock transaction pool for at least as long as it takes for connectTrace to be consumed
            LOCK(MempoolMutex());
            const bool was_in_ibd = m_chainman.IsInitialBlockDownload();
            CBlockIndex* starting_tip = m_chain.Tip();
            bool blocks_connected = false;
            do {
                // We absolutely may not unlock cs_main until we've made forward progress
                // (with the exception of shutdown due to hardware issues, low disk space, etc).
                ConnectTrace connectTrace; // Destructed before cs_main is unlocked

                if (pindexMostWork == nullptr) {
                    pindexMostWork = FindMostWorkChain();
                }

                // Whether we have anything to do at all.
                if (pindexMostWork == nullptr || pindexMostWork == m_chain.Tip()) {
                    break;
                }

                bool fInvalidFound = false;
                std::shared_ptr<const CBlock> nullBlockPtr;
                // BlockConnected signals must be sent for the original role;
                // in case snapshot validation is completed during ActivateBestChainStep, the
                // result of GetRole() changes from BACKGROUND to NORMAL.
               const ChainstateRole chainstate_role{this->GetRole()};
                if (!ActivateBestChainStep(state, pindexMostWork, pblock && pblock->GetHash() == pindexMostWork->GetBlockHash() ? pblock : nullBlockPtr, fInvalidFound, connectTrace)) {
                    // A system error occurred
                    return false;
                }
                blocks_connected = true;

                if (fInvalidFound) {
                    // Wipe cache, we may need another branch now.
                    pindexMostWork = nullptr;
                }
                pindexNewTip = m_chain.Tip();

                for (const PerBlockConnectTrace& trace : connectTrace.GetBlocksConnected()) {
                    assert(trace.pblock && trace.pindex);
                    if (m_chainman.m_options.signals) {
                        m_chainman.m_options.signals->BlockConnected(chainstate_role, trace.pblock, trace.pindex);
                    }
                }

                // This will have been toggled in
                // ActivateBestChainStep -> ConnectTip -> MaybeCompleteSnapshotValidation,
                // if at all, so we should catch it here.
                //
                // Break this do-while to ensure we don't advance past the base snapshot.
                if (m_disabled) {
                    break;
                }
            } while (!m_chain.Tip() || (starting_tip && CBlockIndexWorkComparator()(m_chain.Tip(), starting_tip)));
            if (!blocks_connected) return true;

            const CBlockIndex* pindexFork = m_chain.FindFork(starting_tip);
            bool still_in_ibd = m_chainman.IsInitialBlockDownload();

            if (was_in_ibd && !still_in_ibd) {
                // Active chainstate has exited IBD.
                exited_ibd = true;
            }

            // Notify external listeners about the new tip.
            // Enqueue while holding cs_main to ensure that UpdatedBlockTip is called in the order in which blocks are connected
            if (this == &m_chainman.ActiveChainstate() && pindexFork != pindexNewTip) {
                // Notify ValidationInterface subscribers
                if (m_chainman.m_options.signals) {
                    m_chainman.m_options.signals->UpdatedBlockTip(pindexNewTip, pindexFork, still_in_ibd);
                }

                if (kernel::IsInterrupted(m_chainman.GetNotifications().blockTip(
                        /*state=*/GetSynchronizationState(still_in_ibd, m_chainman.m_blockman.m_blockfiles_indexed),
                        /*index=*/*pindexNewTip,
                        /*verification_progress=*/m_chainman.GuessVerificationProgress(pindexNewTip))))
                {
                    // Just breaking and returning success for now. This could
                    // be changed to bubble up the kernel::Interrupted value to
                    // the caller so the caller could distinguish between
                    // completed and interrupted operations.
                    break;
                }
            }
            } // release MempoolMutex
            // Notify external listeners about the new tip, even if pindexFork == pindexNewTip.
            if (m_chainman.m_options.signals && this == &m_chainman.ActiveChainstate()) {
                m_chainman.m_options.signals->ActiveTipChange(*Assert(pindexNewTip), m_chainman.IsInitialBlockDownload());
            }
        } // release cs_main
        // When we reach this point, we switched to a new tip (stored in pindexNewTip).

        if (exited_ibd) {
            // If a background chainstate is in use, we may need to rebalance our
            // allocation of caches once a chainstate exits initial block download.
            LOCK(::cs_main);
            m_chainman.MaybeRebalanceCaches();
        }

        if (WITH_LOCK(::cs_main, return m_disabled)) {
            // Background chainstate has reached the snapshot base block, so exit.

            // Restart indexes to resume indexing for all blocks unique to the snapshot
            // chain. This resumes indexing "in order" from where the indexing on the
            // background validation chain left off.
            //
            // This cannot be done while holding cs_main (within
            // MaybeCompleteSnapshotValidation) or a cs_main deadlock will occur.
            if (m_chainman.snapshot_download_completed) {
                m_chainman.snapshot_download_completed();
            }
            break;
        }

        // We check interrupt only after giving ActivateBestChainStep a chance to run once so that we
        // never interrupt before connecting the genesis block during LoadChainTip(). Previously this
        // caused an assert() failure during interrupt in such cases as the UTXO DB flushing checks
        // that the best block hash is non-null.
        if (m_chainman.m_interrupt) break;
    } while (pindexNewTip != pindexMostWork);

    m_chainman.CheckBlockIndex();

    // Write changes periodically to disk, after relay.
    if (!FlushStateToDisk(state, FlushStateMode::PERIODIC)) {
        return false;
    }

    return true;
}

bool Chainstate::PreciousBlock(BlockValidationState& state, CBlockIndex* pindex)
{
    AssertLockNotHeld(m_chainstate_mutex);
    AssertLockNotHeld(::cs_main);
    {
        LOCK(cs_main);
        if (pindex->nChainWork < m_chain.Tip()->nChainWork) {
            // Nothing to do, this block is not at the tip.
            return true;
        }
        if (m_chain.Tip()->nChainWork > m_chainman.nLastPreciousChainwork) {
            // The chain has been extended since the last call, reset the counter.
            m_chainman.nBlockReverseSequenceId = -1;
        }
        m_chainman.nLastPreciousChainwork = m_chain.Tip()->nChainWork;
        setBlockIndexCandidates.erase(pindex);
        pindex->nSequenceId = m_chainman.nBlockReverseSequenceId;
        if (m_chainman.nBlockReverseSequenceId > std::numeric_limits<int32_t>::min()) {
            // We can't keep reducing the counter if somebody really wants to
            // call preciousblock 2**31-1 times on the same set of tips...
            m_chainman.nBlockReverseSequenceId--;
        }
        if (pindex->IsValid(BLOCK_VALID_TRANSACTIONS) && pindex->HaveNumChainTxs()) {
            setBlockIndexCandidates.insert(pindex);
            PruneBlockIndexCandidates();
        }
    }

    return ActivateBestChain(state, std::shared_ptr<const CBlock>());
}

bool Chainstate::InvalidateBlock(BlockValidationState& state, CBlockIndex* pindex)
{
    AssertLockNotHeld(m_chainstate_mutex);
    AssertLockNotHeld(::cs_main);

    // Genesis block can't be invalidated
    assert(pindex);
    if (pindex->nHeight == 0) return false;

    CBlockIndex* to_mark_failed = pindex;
    bool pindex_was_in_chain = false;
    int disconnected = 0;

    // We do not allow ActivateBestChain() to run while InvalidateBlock() is
    // running, as that could cause the tip to change while we disconnect
    // blocks.
    LOCK(m_chainstate_mutex);

    // We'll be acquiring and releasing cs_main below, to allow the validation
    // callbacks to run. However, we should keep the block index in a
    // consistent state as we disconnect blocks -- in particular we need to
    // add equal-work blocks to setBlockIndexCandidates as we disconnect.
    // To avoid walking the block index repeatedly in search of candidates,
    // build a map once so that we can look up candidate blocks by chain
    // work as we go.
    std::multimap<const arith_uint256, CBlockIndex *> candidate_blocks_by_work;

    {
        LOCK(cs_main);
        for (auto& entry : m_blockman.m_block_index) {
            CBlockIndex* candidate = &entry.second;
            // We don't need to put anything in our active chain into the
            // multimap, because those candidates will be found and considered
            // as we disconnect.
            // Instead, consider only non-active-chain blocks that have at
            // least as much work as where we expect the new tip to end up.
            if (!m_chain.Contains(candidate) &&
                    !CBlockIndexWorkComparator()(candidate, pindex->pprev) &&
                    candidate->IsValid(BLOCK_VALID_TRANSACTIONS) &&
                    candidate->HaveNumChainTxs()) {
                candidate_blocks_by_work.insert(std::make_pair(candidate->nChainWork, candidate));
            }
        }
    }

    // Disconnect (descendants of) pindex, and mark them invalid.
    while (true) {
        if (m_chainman.m_interrupt) break;

        // Make sure the queue of validation callbacks doesn't grow unboundedly.
        if (m_chainman.m_options.signals) LimitValidationInterfaceQueue(*m_chainman.m_options.signals);

        LOCK(cs_main);
        // Lock for as long as disconnectpool is in scope to make sure MaybeUpdateMempoolForReorg is
        // called after DisconnectTip without unlocking in between
        LOCK(MempoolMutex());
        if (!m_chain.Contains(pindex)) break;
        pindex_was_in_chain = true;
        CBlockIndex *invalid_walk_tip = m_chain.Tip();

        // ActivateBestChain considers blocks already in m_chain
        // unconditionally valid already, so force disconnect away from it.
        DisconnectedBlockTransactions disconnectpool{MAX_DISCONNECTED_TX_POOL_BYTES};
        bool ret = DisconnectTip(state, &disconnectpool);
        // DisconnectTip will add transactions to disconnectpool.
        // Adjust the mempool to be consistent with the new tip, adding
        // transactions back to the mempool if disconnecting was successful,
        // and we're not doing a very deep invalidation (in which case
        // keeping the mempool up to date is probably futile anyway).
        MaybeUpdateMempoolForReorg(disconnectpool, /* fAddToMempool = */ (++disconnected <= 10) && ret);
        if (!ret) return false;
        assert(invalid_walk_tip->pprev == m_chain.Tip());

        // We immediately mark the disconnected blocks as invalid.
        // This prevents a case where pruned nodes may fail to invalidateblock
        // and be left unable to start as they have no tip candidates (as there
        // are no blocks that meet the "have data and are not invalid per
        // nStatus" criteria for inclusion in setBlockIndexCandidates).
        invalid_walk_tip->nStatus |= BLOCK_FAILED_VALID;
        m_blockman.m_dirty_blockindex.insert(invalid_walk_tip);
        setBlockIndexCandidates.erase(invalid_walk_tip);
        setBlockIndexCandidates.insert(invalid_walk_tip->pprev);
        if (invalid_walk_tip == to_mark_failed->pprev && (to_mark_failed->nStatus & BLOCK_FAILED_VALID)) {
            // We only want to mark the last disconnected block as BLOCK_FAILED_VALID; its children
            // need to be BLOCK_FAILED_CHILD instead.
            to_mark_failed->nStatus = (to_mark_failed->nStatus ^ BLOCK_FAILED_VALID) | BLOCK_FAILED_CHILD;
            m_blockman.m_dirty_blockindex.insert(to_mark_failed);
        }

        // Add any equal or more work headers to setBlockIndexCandidates
        auto candidate_it = candidate_blocks_by_work.lower_bound(invalid_walk_tip->pprev->nChainWork);
        while (candidate_it != candidate_blocks_by_work.end()) {
            if (!CBlockIndexWorkComparator()(candidate_it->second, invalid_walk_tip->pprev)) {
                setBlockIndexCandidates.insert(candidate_it->second);
                candidate_it = candidate_blocks_by_work.erase(candidate_it);
            } else {
                ++candidate_it;
            }
        }

        // Track the last disconnected block, so we can correct its BLOCK_FAILED_CHILD status in future
        // iterations, or, if it's the last one, call InvalidChainFound on it.
        to_mark_failed = invalid_walk_tip;
    }

    m_chainman.CheckBlockIndex();

    {
        LOCK(cs_main);
        if (m_chain.Contains(to_mark_failed)) {
            // If the to-be-marked invalid block is in the active chain, something is interfering and we can't proceed.
            return false;
        }

        // Mark pindex as invalid if it never was in the main chain
        if (!pindex_was_in_chain && !(pindex->nStatus & BLOCK_FAILED_MASK)) {
            pindex->nStatus |= BLOCK_FAILED_VALID;
            m_blockman.m_dirty_blockindex.insert(pindex);
            setBlockIndexCandidates.erase(pindex);
            m_chainman.m_failed_blocks.insert(pindex);
        }

        // If any new blocks somehow arrived while we were disconnecting
        // (above), then the pre-calculation of what should go into
        // setBlockIndexCandidates may have missed entries. This would
        // technically be an inconsistency in the block index, but if we clean
        // it up here, this should be an essentially unobservable error.
        // Loop back over all block index entries and add any missing entries
        // to setBlockIndexCandidates.
        for (auto& [_, block_index] : m_blockman.m_block_index) {
            if (block_index.IsValid(BLOCK_VALID_TRANSACTIONS) && block_index.HaveNumChainTxs() && !setBlockIndexCandidates.value_comp()(&block_index, m_chain.Tip())) {
                setBlockIndexCandidates.insert(&block_index);
            }
        }

        InvalidChainFound(to_mark_failed);
    }

    // Only notify about a new block tip if the active chain was modified.
    if (pindex_was_in_chain) {
        // Ignoring return value for now, this could be changed to bubble up
        // kernel::Interrupted value to the caller so the caller could
        // distinguish between completed and interrupted operations. It might
        // also make sense for the blockTip notification to have an enum
        // parameter indicating the source of the tip change so hooks can
        // distinguish user-initiated invalidateblock changes from other
        // changes.
        (void)m_chainman.GetNotifications().blockTip(
            /*state=*/GetSynchronizationState(m_chainman.IsInitialBlockDownload(), m_chainman.m_blockman.m_blockfiles_indexed),
            /*index=*/*to_mark_failed->pprev,
            /*verification_progress=*/WITH_LOCK(m_chainman.GetMutex(), return m_chainman.GuessVerificationProgress(to_mark_failed->pprev)));

        // Fire ActiveTipChange now for the current chain tip to make sure clients are notified.
        // ActivateBestChain may call this as well, but not necessarily.
        if (m_chainman.m_options.signals) {
            m_chainman.m_options.signals->ActiveTipChange(*Assert(m_chain.Tip()), m_chainman.IsInitialBlockDownload());
        }
    }
    return true;
}

void Chainstate::SetBlockFailureFlags(CBlockIndex* invalid_block)
{
    AssertLockHeld(cs_main);

    for (auto& [_, block_index] : m_blockman.m_block_index) {
        if (invalid_block != &block_index && block_index.GetAncestor(invalid_block->nHeight) == invalid_block) {
            block_index.nStatus = (block_index.nStatus & ~BLOCK_FAILED_VALID) | BLOCK_FAILED_CHILD;
            m_blockman.m_dirty_blockindex.insert(&block_index);
        }
    }
}

void Chainstate::ResetBlockFailureFlags(CBlockIndex *pindex) {
    AssertLockHeld(cs_main);

    int nHeight = pindex->nHeight;

    // Remove the invalidity flag from this block and all its descendants.
    for (auto& [_, block_index] : m_blockman.m_block_index) {
        if (!block_index.IsValid() && block_index.GetAncestor(nHeight) == pindex) {
            block_index.nStatus &= ~BLOCK_FAILED_MASK;
            m_blockman.m_dirty_blockindex.insert(&block_index);
            if (block_index.IsValid(BLOCK_VALID_TRANSACTIONS) && block_index.HaveNumChainTxs() && setBlockIndexCandidates.value_comp()(m_chain.Tip(), &block_index)) {
                setBlockIndexCandidates.insert(&block_index);
            }
            if (&block_index == m_chainman.m_best_invalid) {
                // Reset invalid block marker if it was pointing to one of those.
                m_chainman.m_best_invalid = nullptr;
            }
            m_chainman.m_failed_blocks.erase(&block_index);
        }
    }

    // Remove the invalidity flag from all ancestors too.
    while (pindex != nullptr) {
        if (pindex->nStatus & BLOCK_FAILED_MASK) {
            pindex->nStatus &= ~BLOCK_FAILED_MASK;
            m_blockman.m_dirty_blockindex.insert(pindex);
            m_chainman.m_failed_blocks.erase(pindex);
        }
        pindex = pindex->pprev;
    }
}

void Chainstate::TryAddBlockIndexCandidate(CBlockIndex* pindex)
{
    AssertLockHeld(cs_main);
    // The block only is a candidate for the most-work-chain if it has the same
    // or more work than our current tip.
    if (m_chain.Tip() != nullptr && setBlockIndexCandidates.value_comp()(pindex, m_chain.Tip())) {
        return;
    }

    bool is_active_chainstate = this == &m_chainman.ActiveChainstate();
    if (is_active_chainstate) {
        // The active chainstate should always add entries that have more
        // work than the tip.
        setBlockIndexCandidates.insert(pindex);
    } else if (!m_disabled) {
        // For the background chainstate, we only consider connecting blocks
        // towards the snapshot base (which can't be nullptr or else we'll
        // never make progress).
        const CBlockIndex* snapshot_base{Assert(m_chainman.GetSnapshotBaseBlock())};
        if (snapshot_base->GetAncestor(pindex->nHeight) == pindex) {
            setBlockIndexCandidates.insert(pindex);
        }
    }
}

/** Mark a block as having its data received and checked (up to BLOCK_VALID_TRANSACTIONS). */
void ChainstateManager::ReceivedBlockTransactions(const CBlock& block, CBlockIndex* pindexNew, const FlatFilePos& pos)
{
    AssertLockHeld(cs_main);
    pindexNew->nTx = block.vtx.size();
    // Typically m_chain_tx_count will be 0 at this point, but it can be nonzero if this
    // is a pruned block which is being downloaded again, or if this is an
    // assumeutxo snapshot block which has a hardcoded m_chain_tx_count value from the
    // snapshot metadata. If the pindex is not the snapshot block and the
    // m_chain_tx_count value is not zero, assert that value is actually correct.
    auto prev_tx_sum = [](CBlockIndex& block) { return block.nTx + (block.pprev ? block.pprev->m_chain_tx_count : 0); };
    if (!Assume(pindexNew->m_chain_tx_count == 0 || pindexNew->m_chain_tx_count == prev_tx_sum(*pindexNew) ||
                pindexNew == GetSnapshotBaseBlock())) {
        LogWarning("Internal bug detected: block %d has unexpected m_chain_tx_count %i that should be %i (%s %s). Please report this issue here: %s\n",
            pindexNew->nHeight, pindexNew->m_chain_tx_count, prev_tx_sum(*pindexNew), CLIENT_NAME, FormatFullVersion(), CLIENT_BUGREPORT);
        pindexNew->m_chain_tx_count = 0;
    }
    pindexNew->nFile = pos.nFile;
    pindexNew->nDataPos = pos.nPos;
    pindexNew->nUndoPos = 0;
    pindexNew->nStatus |= BLOCK_HAVE_DATA;
    if (DeploymentActiveAt(*pindexNew, *this, Consensus::DEPLOYMENT_SEGWIT)) {
        pindexNew->nStatus |= BLOCK_OPT_WITNESS;
    }
    pindexNew->RaiseValidity(BLOCK_VALID_TRANSACTIONS);
    m_blockman.m_dirty_blockindex.insert(pindexNew);

    if (pindexNew->pprev == nullptr || pindexNew->pprev->HaveNumChainTxs()) {
        // If pindexNew is the genesis block or all parents are BLOCK_VALID_TRANSACTIONS.
        std::deque<CBlockIndex*> queue;
        queue.push_back(pindexNew);

        // Recursively process any descendant blocks that now may be eligible to be connected.
        while (!queue.empty()) {
            CBlockIndex *pindex = queue.front();
            queue.pop_front();
            // Before setting m_chain_tx_count, assert that it is 0 or already set to
            // the correct value. This assert will fail after receiving the
            // assumeutxo snapshot block if assumeutxo snapshot metadata has an
            // incorrect hardcoded AssumeutxoData::m_chain_tx_count value.
            if (!Assume(pindex->m_chain_tx_count == 0 || pindex->m_chain_tx_count == prev_tx_sum(*pindex))) {
                LogWarning("Internal bug detected: block %d has unexpected m_chain_tx_count %i that should be %i (%s %s). Please report this issue here: %s\n",
                   pindex->nHeight, pindex->m_chain_tx_count, prev_tx_sum(*pindex), CLIENT_NAME, FormatFullVersion(), CLIENT_BUGREPORT);
            }
            pindex->m_chain_tx_count = prev_tx_sum(*pindex);
            pindex->nSequenceId = nBlockSequenceId++;
            for (Chainstate *c : GetAll()) {
                c->TryAddBlockIndexCandidate(pindex);
            }
            std::pair<std::multimap<CBlockIndex*, CBlockIndex*>::iterator, std::multimap<CBlockIndex*, CBlockIndex*>::iterator> range = m_blockman.m_blocks_unlinked.equal_range(pindex);
            while (range.first != range.second) {
                std::multimap<CBlockIndex*, CBlockIndex*>::iterator it = range.first;
                queue.push_back(it->second);
                range.first++;
                m_blockman.m_blocks_unlinked.erase(it);
            }
        }
    } else {
        if (pindexNew->pprev && pindexNew->pprev->IsValid(BLOCK_VALID_TREE)) {
            m_blockman.m_blocks_unlinked.insert(std::make_pair(pindexNew->pprev, pindexNew));
        }
    }
}

static bool CheckBlockHeader(const CBlockHeader& block, BlockValidationState& state, const Consensus::Params& consensusParams, bool fCheckPOW = true)
{
    // Check proof of work matches claimed amount
    if (fCheckPOW && !CheckProofOfWork(block.GetHash(), block.nBits, consensusParams))
        return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "high-hash", "proof of work failed");

    return true;
}

static bool CheckMerkleRoot(const CBlock& block, BlockValidationState& state)
{
    if (block.m_checked_merkle_root) return true;

    bool mutated;
    uint256 merkle_root = BlockMerkleRoot(block, &mutated);
    if (block.hashMerkleRoot != merkle_root) {
        return state.Invalid(
            /*result=*/BlockValidationResult::BLOCK_MUTATED,
            /*reject_reason=*/"bad-txnmrklroot",
            /*debug_message=*/"hashMerkleRoot mismatch");
    }

    // Check for merkle tree malleability (CVE-2012-2459): repeating sequences
    // of transactions in a block without affecting the merkle root of a block,
    // while still invalidating it.
    if (mutated) {
        return state.Invalid(
            /*result=*/BlockValidationResult::BLOCK_MUTATED,
            /*reject_reason=*/"bad-txns-duplicate",
            /*debug_message=*/"duplicate transaction");
    }

    block.m_checked_merkle_root = true;
    return true;
}

/** CheckWitnessMalleation performs checks for block malleation with regard to
 * its witnesses.
 *
 * Note: If the witness commitment is expected (i.e. `expect_witness_commitment
 * = true`), then the block is required to have at least one transaction and the
 * first transaction needs to have at least one input. */
static bool CheckWitnessMalleation(const CBlock& block, bool expect_witness_commitment, BlockValidationState& state)
{
    if (expect_witness_commitment) {
        if (block.m_checked_witness_commitment) return true;

        int commitpos = GetWitnessCommitmentIndex(block);
        if (commitpos != NO_WITNESS_COMMITMENT) {
            assert(!block.vtx.empty() && !block.vtx[0]->vin.empty());
            const auto& witness_stack{block.vtx[0]->vin[0].scriptWitness.stack};

            if (witness_stack.size() != 1 || witness_stack[0].size() != 32) {
                return state.Invalid(
                    /*result=*/BlockValidationResult::BLOCK_MUTATED,
                    /*reject_reason=*/"bad-witness-nonce-size",
                    /*debug_message=*/strprintf("%s : invalid witness reserved value size", __func__));
            }

            // The malleation check is ignored; as the transaction tree itself
            // already does not permit it, it is impossible to trigger in the
            // witness tree.
            uint256 hash_witness = BlockWitnessMerkleRoot(block, /*mutated=*/nullptr);

            CHash256().Write(hash_witness).Write(witness_stack[0]).Finalize(hash_witness);
            if (memcmp(hash_witness.begin(), &block.vtx[0]->vout[commitpos].scriptPubKey[6], 32)) {
                return state.Invalid(
                    /*result=*/BlockValidationResult::BLOCK_MUTATED,
                    /*reject_reason=*/"bad-witness-merkle-match",
                    /*debug_message=*/strprintf("%s : witness merkle commitment mismatch", __func__));
            }

            block.m_checked_witness_commitment = true;
            return true;
        }
    }

    // No witness data is allowed in blocks that don't commit to witness data, as this would otherwise leave room for spam
    for (const auto& tx : block.vtx) {
        if (tx->HasWitness()) {
            return state.Invalid(
                /*result=*/BlockValidationResult::BLOCK_MUTATED,
                /*reject_reason=*/"unexpected-witness",
                /*debug_message=*/strprintf("%s : unexpected witness data found", __func__));
        }
    }

    return true;
}

bool CheckBlock(const CBlock& block, BlockValidationState& state, const Consensus::Params& consensusParams, bool fCheckPOW, bool fCheckMerkleRoot)
{
    // These are checks that are independent of context.

    if (block.fChecked)
        return true;

    // Check that the header is valid (particularly PoW).  This is mostly
    // redundant with the call in AcceptBlockHeader.
    if (!CheckBlockHeader(block, state, consensusParams, fCheckPOW))
        return false;

    // Signet only: check block solution
    if (consensusParams.signet_blocks && fCheckPOW && !CheckSignetBlockSolution(block, consensusParams)) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-signet-blksig", "signet block signature validation failure");
    }

    // Check the merkle root.
    if (fCheckMerkleRoot && !CheckMerkleRoot(block, state)) {
        return false;
    }

    // All potential-corruption validation must be done before we do any
    // transaction validation, as otherwise we may mark the header as invalid
    // because we receive the wrong transactions for it.
    // Note that witness malleability is checked in ContextualCheckBlock, so no
    // checks that use witness data may be performed here.

    // Size limits
    if (block.vtx.empty() || block.vtx.size() * WITNESS_SCALE_FACTOR > MAX_BLOCK_WEIGHT || ::GetSerializeSize(TX_NO_WITNESS(block)) * WITNESS_SCALE_FACTOR > MAX_BLOCK_WEIGHT)
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-blk-length", "size limits failed");

    // First transaction must be coinbase, the rest must not be
    if (block.vtx.empty() || !block.vtx[0]->IsCoinBase())
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-missing", "first tx is not coinbase");
    for (unsigned int i = 1; i < block.vtx.size(); i++)
        if (block.vtx[i]->IsCoinBase())
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-multiple", "more than one coinbase");

    // Check transactions
    // Must check for duplicate inputs (see CVE-2018-17144)
    for (const auto& tx : block.vtx) {
        TxValidationState tx_state;
        if (!CheckTransaction(*tx, tx_state)) {
            // CheckBlock() does context-free validation checks. The only
            // possible failures are consensus failures.
            assert(tx_state.GetResult() == TxValidationResult::TX_CONSENSUS);
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, tx_state.GetRejectReason(),
                                 strprintf("Transaction check failed (tx hash %s) %s", tx->GetHash().ToString(), tx_state.GetDebugMessage()));
        }
    }
    // This underestimates the number of sigops, because unlike ConnectBlock it
    // does not count witness and p2sh sigops.
    unsigned int nSigOps = 0;
    for (const auto& tx : block.vtx)
    {
        nSigOps += GetLegacySigOpCount(*tx);
    }
    if (nSigOps * WITNESS_SCALE_FACTOR > MAX_BLOCK_SIGOPS_COST)
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-blk-sigops", "out-of-bounds SigOpCount");

    if (fCheckPOW && fCheckMerkleRoot)
        block.fChecked = true;

    return true;
}

void ChainstateManager::UpdateUncommittedBlockStructures(CBlock& block, const CBlockIndex* pindexPrev) const
{
    int commitpos = GetWitnessCommitmentIndex(block);
    static const std::vector<unsigned char> nonce(32, 0x00);
    if (commitpos != NO_WITNESS_COMMITMENT && DeploymentActiveAfter(pindexPrev, *this, Consensus::DEPLOYMENT_SEGWIT) && !block.vtx[0]->HasWitness()) {
        CMutableTransaction tx(*block.vtx[0]);
        tx.vin[0].scriptWitness.stack.resize(1);
        tx.vin[0].scriptWitness.stack[0] = nonce;
        block.vtx[0] = MakeTransactionRef(std::move(tx));
    }
}

std::vector<unsigned char> ChainstateManager::GenerateCoinbaseCommitment(CBlock& block, const CBlockIndex* pindexPrev) const
{
    std::vector<unsigned char> commitment;
    int commitpos = GetWitnessCommitmentIndex(block);
    std::vector<unsigned char> ret(32, 0x00);
    if (commitpos == NO_WITNESS_COMMITMENT) {
        uint256 witnessroot = BlockWitnessMerkleRoot(block, nullptr);
        CHash256().Write(witnessroot).Write(ret).Finalize(witnessroot);
        CTxOut out;
        out.nValue = 0;
        out.scriptPubKey.resize(MINIMUM_WITNESS_COMMITMENT);
        out.scriptPubKey[0] = OP_RETURN;
        out.scriptPubKey[1] = 0x24;
        out.scriptPubKey[2] = 0xaa;
        out.scriptPubKey[3] = 0x21;
        out.scriptPubKey[4] = 0xa9;
        out.scriptPubKey[5] = 0xed;
        memcpy(&out.scriptPubKey[6], witnessroot.begin(), 32);
        commitment = std::vector<unsigned char>(out.scriptPubKey.begin(), out.scriptPubKey.end());
        CMutableTransaction tx(*block.vtx[0]);
        tx.vout.push_back(out);
        block.vtx[0] = MakeTransactionRef(std::move(tx));
    }
    UpdateUncommittedBlockStructures(block, pindexPrev);
    return commitment;
}

bool HasValidProofOfWork(const std::vector<CBlockHeader>& headers, const Consensus::Params& consensusParams)
{
    return std::all_of(headers.cbegin(), headers.cend(),
            [&](const auto& header) { return CheckProofOfWork(header.GetHash(), header.nBits, consensusParams);});
}

bool IsBlockMutated(const CBlock& block, bool check_witness_root)
{
    BlockValidationState state;
    if (!CheckMerkleRoot(block, state)) {
        LogDebug(BCLog::VALIDATION, "Block mutated: %s\n", state.ToString());
        return true;
    }

    if (block.vtx.empty() || !block.vtx[0]->IsCoinBase()) {
        // Consider the block mutated if any transaction is 64 bytes in size (see 3.1
        // in "Weaknesses in Bitcoin’s Merkle Root Construction":
        // https://lists.linuxfoundation.org/pipermail/bitcoin-dev/attachments/20190225/a27d8837/attachment-0001.pdf).
        //
        // Note: This is not a consensus change as this only applies to blocks that
        // don't have a coinbase transaction and would therefore already be invalid.
        return std::any_of(block.vtx.begin(), block.vtx.end(),
                           [](auto& tx) { return GetSerializeSize(TX_NO_WITNESS(tx)) == 64; });
    } else {
        // Theoretically it is still possible for a block with a 64 byte
        // coinbase transaction to be mutated but we neglect that possibility
        // here as it requires at least 224 bits of work.
    }

    if (!CheckWitnessMalleation(block, check_witness_root, state)) {
        LogDebug(BCLog::VALIDATION, "Block mutated: %s\n", state.ToString());
        return true;
    }

    return false;
}

arith_uint256 CalculateClaimedHeadersWork(std::span<const CBlockHeader> headers)
{
    arith_uint256 total_work{0};
    for (const CBlockHeader& header : headers) {
        CBlockIndex dummy(header);
        total_work += GetBlockProof(dummy);
    }
    return total_work;
}

/** Context-dependent validity checks.
 *  By "context", we mean only the previous block headers, but not the UTXO
 *  set; UTXO-related validity checks are done in ConnectBlock().
 *  NOTE: This function is not currently invoked by ConnectBlock(), so we
 *  should consider upgrade issues if we change which consensus rules are
 *  enforced in this function (eg by adding a new consensus rule). See comment
 *  in ConnectBlock().
 *  Note that -reindex-chainstate skips the validation that happens here!
 *
 *  NOTE: failing to check the header's height against the last checkpoint's opened a DoS vector between
 *  v0.12 and v0.15 (when no additional protection was in place) whereby an attacker could unboundedly
 *  grow our in-memory block index. See https://bitcoincore.org/en/2024/07/03/disclose-header-spam.
 */
static bool ContextualCheckBlockHeader(const CBlockHeader& block, BlockValidationState& state, BlockManager& blockman, const ChainstateManager& chainman, const CBlockIndex* pindexPrev) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    AssertLockHeld(::cs_main);
    assert(pindexPrev != nullptr);
    const int nHeight = pindexPrev->nHeight + 1;

    // Check proof of work
    const Consensus::Params& consensusParams = chainman.GetConsensus();
    if (block.nBits != GetNextWorkRequired(pindexPrev, &block, consensusParams))
        return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "bad-diffbits", "incorrect proof of work");

    // Check timestamp against prev
    if (block.GetBlockTime() <= pindexPrev->GetMedianTimePast())
        return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "time-too-old", "block's timestamp is too early");

    // Testnet4 and regtest only: Check timestamp against prev for difficulty-adjustment
    // blocks to prevent timewarp attacks (see https://github.com/bitcoin/bitcoin/pull/15482).
    if (consensusParams.enforce_BIP94) {
        // Check timestamp for the first block of each difficulty adjustment
        // interval, except the genesis block.
        if (nHeight % consensusParams.DifficultyAdjustmentInterval() == 0) {
            if (block.GetBlockTime() < pindexPrev->GetBlockTime() - MAX_TIMEWARP) {
                return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "time-timewarp-attack", "block's timestamp is too early on diff adjustment block");
            }
        }
    }

    // Check timestamp
    if (block.Time() > NodeClock::now() + std::chrono::seconds{MAX_FUTURE_BLOCK_TIME}) {
        return state.Invalid(BlockValidationResult::BLOCK_TIME_FUTURE, "time-too-new", "block timestamp too far in the future");
    }

    // Reject blocks with outdated version
    if ((block.nVersion < 2 && DeploymentActiveAfter(pindexPrev, chainman, Consensus::DEPLOYMENT_HEIGHTINCB)) ||
        (block.nVersion < 3 && DeploymentActiveAfter(pindexPrev, chainman, Consensus::DEPLOYMENT_DERSIG)) ||
        (block.nVersion < 4 && DeploymentActiveAfter(pindexPrev, chainman, Consensus::DEPLOYMENT_CLTV))) {
            return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, strprintf("bad-version(0x%08x)", block.nVersion),
                                 strprintf("rejected nVersion=0x%08x block", block.nVersion));
    }

    return true;
}

/** NOTE: This function is not currently invoked by ConnectBlock(), so we
 *  should consider upgrade issues if we change which consensus rules are
 *  enforced in this function (eg by adding a new consensus rule). See comment
 *  in ConnectBlock().
 *  Note that -reindex-chainstate skips the validation that happens here!
 */
static bool ContextualCheckBlock(const CBlock& block, BlockValidationState& state, const ChainstateManager& chainman, const CBlockIndex* pindexPrev)
{
    const int nHeight = pindexPrev == nullptr ? 0 : pindexPrev->nHeight + 1;

    // Enforce BIP113 (Median Time Past).
    bool enforce_locktime_median_time_past{false};
    if (DeploymentActiveAfter(pindexPrev, chainman, Consensus::DEPLOYMENT_CSV)) {
        assert(pindexPrev != nullptr);
        enforce_locktime_median_time_past = true;
    }

    const int64_t nLockTimeCutoff{enforce_locktime_median_time_past ?
                                      pindexPrev->GetMedianTimePast() :
                                      block.GetBlockTime()};

    // Check that all transactions are finalized
    for (const auto& tx : block.vtx) {
        if (!IsFinalTx(*tx, nHeight, nLockTimeCutoff)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-txns-nonfinal", "non-final transaction");
        }
    }

    // Enforce rule that the coinbase starts with serialized block height
    if (DeploymentActiveAfter(pindexPrev, chainman, Consensus::DEPLOYMENT_HEIGHTINCB))
    {
        CScript expect = CScript() << nHeight;
        if (block.vtx[0]->vin[0].scriptSig.size() < expect.size() ||
            !std::equal(expect.begin(), expect.end(), block.vtx[0]->vin[0].scriptSig.begin())) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-height", "block height mismatch in coinbase");
        }
    }

    // Validation for witness commitments.
    // * We compute the witness hash (which is the hash including witnesses) of all the block's transactions, except the
    //   coinbase (where 0x0000....0000 is used instead).
    // * The coinbase scriptWitness is a stack of a single 32-byte vector, containing a witness reserved value (unconstrained).
    // * We build a merkle tree with all those witness hashes as leaves (similar to the hashMerkleRoot in the block header).
    // * There must be at least one output whose scriptPubKey is a single 36-byte push, the first 4 bytes of which are
    //   {0xaa, 0x21, 0xa9, 0xed}, and the following 32 bytes are SHA256^2(witness root, witness reserved value). In case there are
    //   multiple, the last one is used.
    if (!CheckWitnessMalleation(block, DeploymentActiveAfter(pindexPrev, chainman, Consensus::DEPLOYMENT_SEGWIT), state)) {
        return false;
    }

    // After the coinbase witness reserved value and commitment are verified,
    // we can check if the block weight passes (before we've checked the
    // coinbase witness, it would be possible for the weight to be too
    // large by filling up the coinbase witness, which doesn't change
    // the block hash, so we couldn't mark the block as permanently
    // failed).
    if (GetBlockWeight(block) > MAX_BLOCK_WEIGHT) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-blk-weight", strprintf("%s : weight limit failed", __func__));
    }

    return true;
}

bool ChainstateManager::AcceptBlockHeader(const CBlockHeader& block, BlockValidationState& state, CBlockIndex** ppindex, bool min_pow_checked)
{
    AssertLockHeld(cs_main);

    // Check for duplicate
    uint256 hash = block.GetHash();
    BlockMap::iterator miSelf{m_blockman.m_block_index.find(hash)};
    if (hash != GetConsensus().hashGenesisBlock) {
        if (miSelf != m_blockman.m_block_index.end()) {
            // Block header is already known.
            CBlockIndex* pindex = &(miSelf->second);
            if (ppindex)
                *ppindex = pindex;
            if (pindex->nStatus & BLOCK_FAILED_MASK) {
                LogDebug(BCLog::VALIDATION, "%s: block %s is marked invalid\n", __func__, hash.ToString());
                return state.Invalid(BlockValidationResult::BLOCK_CACHED_INVALID, "duplicate-invalid");
            }
            return true;
        }

        if (!CheckBlockHeader(block, state, GetConsensus())) {
            LogDebug(BCLog::VALIDATION, "%s: Consensus::CheckBlockHeader: %s, %s\n", __func__, hash.ToString(), state.ToString());
            return false;
        }

        // Get prev block index
        CBlockIndex* pindexPrev = nullptr;
        BlockMap::iterator mi{m_blockman.m_block_index.find(block.hashPrevBlock)};
        if (mi == m_blockman.m_block_index.end()) {
            LogDebug(BCLog::VALIDATION, "header %s has prev block not found: %s\n", hash.ToString(), block.hashPrevBlock.ToString());
            return state.Invalid(BlockValidationResult::BLOCK_MISSING_PREV, "prev-blk-not-found");
        }
        pindexPrev = &((*mi).second);
        if (pindexPrev->nStatus & BLOCK_FAILED_MASK) {
            LogDebug(BCLog::VALIDATION, "header %s has prev block invalid: %s\n", hash.ToString(), block.hashPrevBlock.ToString());
            return state.Invalid(BlockValidationResult::BLOCK_INVALID_PREV, "bad-prevblk");
        }
        if (!ContextualCheckBlockHeader(block, state, m_blockman, *this, pindexPrev)) {
            LogDebug(BCLog::VALIDATION, "%s: Consensus::ContextualCheckBlockHeader: %s, %s\n", __func__, hash.ToString(), state.ToString());
            return false;
        }

        /* Determine if this block descends from any block which has been found
         * invalid (m_failed_blocks), then mark pindexPrev and any blocks between
         * them as failed. For example:
         *
         *                D3
         *              /
         *      B2 - C2
         *    /         \
         *  A             D2 - E2 - F2
         *    \
         *      B1 - C1 - D1 - E1
         *
         * In the case that we attempted to reorg from E1 to F2, only to find
         * C2 to be invalid, we would mark D2, E2, and F2 as BLOCK_FAILED_CHILD
         * but NOT D3 (it was not in any of our candidate sets at the time).
         *
         * In any case D3 will also be marked as BLOCK_FAILED_CHILD at restart
         * in LoadBlockIndex.
         */
        if (!pindexPrev->IsValid(BLOCK_VALID_SCRIPTS)) {
            // The above does not mean "invalid": it checks if the previous block
            // hasn't been validated up to BLOCK_VALID_SCRIPTS. This is a performance
            // optimization, in the common case of adding a new block to the tip,
            // we don't need to iterate over the failed blocks list.
            for (const CBlockIndex* failedit : m_failed_blocks) {
                if (pindexPrev->GetAncestor(failedit->nHeight) == failedit) {
                    assert(failedit->nStatus & BLOCK_FAILED_VALID);
                    CBlockIndex* invalid_walk = pindexPrev;
                    while (invalid_walk != failedit) {
                        invalid_walk->nStatus |= BLOCK_FAILED_CHILD;
                        m_blockman.m_dirty_blockindex.insert(invalid_walk);
                        invalid_walk = invalid_walk->pprev;
                    }
                    LogDebug(BCLog::VALIDATION, "header %s has prev block invalid: %s\n", hash.ToString(), block.hashPrevBlock.ToString());
                    return state.Invalid(BlockValidationResult::BLOCK_INVALID_PREV, "bad-prevblk");
                }
            }
        }
    }
    if (!min_pow_checked) {
        LogDebug(BCLog::VALIDATION, "%s: not adding new block header %s, missing anti-dos proof-of-work validation\n", __func__, hash.ToString());
        return state.Invalid(BlockValidationResult::BLOCK_HEADER_LOW_WORK, "too-little-chainwork");
    }
    CBlockIndex* pindex{m_blockman.AddToBlockIndex(block, m_best_header)};

    if (ppindex)
        *ppindex = pindex;

    return true;
}

// Exposed wrapper for AcceptBlockHeader
bool ChainstateManager::ProcessNewBlockHeaders(std::span<const CBlockHeader> headers, bool min_pow_checked, BlockValidationState& state, const CBlockIndex** ppindex)
{
    AssertLockNotHeld(cs_main);
    {
        LOCK(cs_main);
        for (const CBlockHeader& header : headers) {
            CBlockIndex *pindex = nullptr; // Use a temp pindex instead of ppindex to avoid a const_cast
            bool accepted{AcceptBlockHeader(header, state, &pindex, min_pow_checked)};
            CheckBlockIndex();

            if (!accepted) {
                return false;
            }
            if (ppindex) {
                *ppindex = pindex;
            }
        }
    }
    if (NotifyHeaderTip()) {
        if (IsInitialBlockDownload() && ppindex && *ppindex) {
            const CBlockIndex& last_accepted{**ppindex};
            int64_t blocks_left{(NodeClock::now() - last_accepted.Time()) / GetConsensus().PowTargetSpacing()};
            blocks_left = std::max<int64_t>(0, blocks_left);
            const double progress{100.0 * last_accepted.nHeight / (last_accepted.nHeight + blocks_left)};
            LogInfo("Synchronizing blockheaders, height: %d (~%.2f%%)\n", last_accepted.nHeight, progress);
        }
    }
    return true;
}

void ChainstateManager::ReportHeadersPresync(const arith_uint256& work, int64_t height, int64_t timestamp)
{
    AssertLockNotHeld(GetMutex());
    {
        LOCK(GetMutex());
        // Don't report headers presync progress if we already have a post-minchainwork header chain.
        // This means we lose reporting for potentially legitimate, but unlikely, deep reorgs, but
        // prevent attackers that spam low-work headers from filling our logs.
        if (m_best_header->nChainWork >= UintToArith256(GetConsensus().nMinimumChainWork)) return;
        // Rate limit headers presync updates to 4 per second, as these are not subject to DoS
        // protection.
        auto now = MockableSteadyClock::now();
        if (now < m_last_presync_update + std::chrono::milliseconds{250}) return;
        m_last_presync_update = now;
    }
    bool initial_download = IsInitialBlockDownload();
    GetNotifications().headerTip(GetSynchronizationState(initial_download, m_blockman.m_blockfiles_indexed), height, timestamp, /*presync=*/true);
    if (initial_download) {
        int64_t blocks_left{(NodeClock::now() - NodeSeconds{std::chrono::seconds{timestamp}}) / GetConsensus().PowTargetSpacing()};
        blocks_left = std::max<int64_t>(0, blocks_left);
        const double progress{100.0 * height / (height + blocks_left)};
        LogInfo("Pre-synchronizing blockheaders, height: %d (~%.2f%%)\n", height, progress);
    }
}

/** Store block on disk. If dbp is non-nullptr, the file is known to already reside on disk */
bool ChainstateManager::AcceptBlock(const std::shared_ptr<const CBlock>& pblock, BlockValidationState& state, CBlockIndex** ppindex, bool fRequested, const FlatFilePos* dbp, bool* fNewBlock, bool min_pow_checked)
{
    const CBlock& block = *pblock;

    if (fNewBlock) *fNewBlock = false;
    AssertLockHeld(cs_main);

    CBlockIndex *pindexDummy = nullptr;
    CBlockIndex *&pindex = ppindex ? *ppindex : pindexDummy;

    bool accepted_header{AcceptBlockHeader(block, state, &pindex, min_pow_checked)};
    CheckBlockIndex();

    if (!accepted_header)
        return false;

    // Check all requested blocks that we do not already have for validity and
    // save them to disk. Skip processing of unrequested blocks as an anti-DoS
    // measure, unless the blocks have more work than the active chain tip, and
    // aren't too far ahead of it, so are likely to be attached soon.
    bool fAlreadyHave = pindex->nStatus & BLOCK_HAVE_DATA;
    bool fHasMoreOrSameWork = (ActiveTip() ? pindex->nChainWork >= ActiveTip()->nChainWork : true);
    // Blocks that are too out-of-order needlessly limit the effectiveness of
    // pruning, because pruning will not delete block files that contain any
    // blocks which are too close in height to the tip.  Apply this test
    // regardless of whether pruning is enabled; it should generally be safe to
    // not process unrequested blocks.
    bool fTooFarAhead{pindex->nHeight > ActiveHeight() + int(MIN_BLOCKS_TO_KEEP)};

    // TODO: Decouple this function from the block download logic by removing fRequested
    // This requires some new chain data structure to efficiently look up if a
    // block is in a chain leading to a candidate for best tip, despite not
    // being such a candidate itself.
    // Note that this would break the getblockfrompeer RPC

    // TODO: deal better with return value and error conditions for duplicate
    // and unrequested blocks.
    if (fAlreadyHave) return true;
    if (!fRequested) {  // If we didn't ask for it:
        if (pindex->nTx != 0) return true;    // This is a previously-processed block that was pruned
        if (!fHasMoreOrSameWork) return true; // Don't process less-work chains
        if (fTooFarAhead) return true;        // Block height is too high

        // Protect against DoS attacks from low-work chains.
        // If our tip is behind, a peer could try to send us
        // low-work blocks on a fake chain that we would never
        // request; don't process these.
        if (pindex->nChainWork < MinimumChainWork()) return true;
    }

    const CChainParams& params{GetParams()};

    if (!CheckBlock(block, state, params.GetConsensus()) ||
        !ContextualCheckBlock(block, state, *this, pindex->pprev)) {
        if (state.IsInvalid() && state.GetResult() != BlockValidationResult::BLOCK_MUTATED) {
            pindex->nStatus |= BLOCK_FAILED_VALID;
            m_blockman.m_dirty_blockindex.insert(pindex);
        }
        LogError("%s: %s\n", __func__, state.ToString());
        return false;
    }

    // Header is valid/has work, merkle tree and segwit merkle tree are good...RELAY NOW
    // (but if it does not build on our best tip, let the SendMessages loop relay it)
    if (!IsInitialBlockDownload() && ActiveTip() == pindex->pprev && m_options.signals) {
        m_options.signals->NewPoWValidBlock(pindex, pblock);
    }

    // Write block to history file
    if (fNewBlock) *fNewBlock = true;
    try {
        FlatFilePos blockPos{};
        if (dbp) {
            blockPos = *dbp;
            m_blockman.UpdateBlockInfo(block, pindex->nHeight, blockPos);
        } else {
            blockPos = m_blockman.WriteBlock(block, pindex->nHeight);
            if (blockPos.IsNull()) {
                state.Error(strprintf("%s: Failed to find position to write new block to disk", __func__));
                return false;
            }
        }
        ReceivedBlockTransactions(block, pindex, blockPos);
    } catch (const std::runtime_error& e) {
        return FatalError(GetNotifications(), state, strprintf(_("System error while saving block to disk: %s"), e.what()));
    }

    // TODO: FlushStateToDisk() handles flushing of both block and chainstate
    // data, so we should move this to ChainstateManager so that we can be more
    // intelligent about how we flush.
    // For now, since FlushStateMode::NONE is used, all that can happen is that
    // the block files may be pruned, so we can just call this on one
    // chainstate (particularly if we haven't implemented pruning with
    // background validation yet).
    ActiveChainstate().FlushStateToDisk(state, FlushStateMode::NONE);

    CheckBlockIndex();

    return true;
}

bool ChainstateManager::ProcessNewBlock(const std::shared_ptr<const CBlock>& block, bool force_processing, bool min_pow_checked, bool* new_block)
{
    AssertLockNotHeld(cs_main);

    {
        CBlockIndex *pindex = nullptr;
        if (new_block) *new_block = false;
        BlockValidationState state;

        // CheckBlock() does not support multi-threaded block validation because CBlock::fChecked can cause data race.
        // Therefore, the following critical section must include the CheckBlock() call as well.
        LOCK(cs_main);

        // Skipping AcceptBlock() for CheckBlock() failures means that we will never mark a block as invalid if
        // CheckBlock() fails.  This is protective against consensus failure if there are any unknown forms of block
        // malleability that cause CheckBlock() to fail; see e.g. CVE-2012-2459 and
        // https://lists.linuxfoundation.org/pipermail/bitcoin-dev/2019-February/016697.html.  Because CheckBlock() is
        // not very expensive, the anti-DoS benefits of caching failure (of a definitely-invalid block) are not substantial.
        bool ret = CheckBlock(*block, state, GetConsensus());
        if (ret) {
            // Store to disk
            ret = AcceptBlock(block, state, &pindex, force_processing, nullptr, new_block, min_pow_checked);
        }
        if (!ret) {
            if (m_options.signals) {
                m_options.signals->BlockChecked(*block, state);
            }
            LogError("%s: AcceptBlock FAILED (%s)\n", __func__, state.ToString());
            return false;
        }
    }

    NotifyHeaderTip();

    BlockValidationState state; // Only used to report errors, not invalidity - ignore it
    if (!ActiveChainstate().ActivateBestChain(state, block)) {
        LogError("%s: ActivateBestChain failed (%s)\n", __func__, state.ToString());
        return false;
    }

    Chainstate* bg_chain{WITH_LOCK(cs_main, return BackgroundSyncInProgress() ? m_ibd_chainstate.get() : nullptr)};
    BlockValidationState bg_state;
    if (bg_chain && !bg_chain->ActivateBestChain(bg_state, block)) {
        LogError("%s: [background] ActivateBestChain failed (%s)\n", __func__, bg_state.ToString());
        return false;
     }

    return true;
}

MempoolAcceptResult ChainstateManager::ProcessTransaction(const CTransactionRef& tx, bool test_accept)
{
    AssertLockHeld(cs_main);
    Chainstate& active_chainstate = ActiveChainstate();
    if (!active_chainstate.GetMempool()) {
        TxValidationState state;
        state.Invalid(TxValidationResult::TX_NO_MEMPOOL, "no-mempool");
        return MempoolAcceptResult::Failure(state);
    }
    auto result = AcceptToMemoryPool(active_chainstate, tx, GetTime(), /*bypass_limits=*/ false, test_accept);
    active_chainstate.GetMempool()->check(active_chainstate.CoinsTip(), active_chainstate.m_chain.Height() + 1);
    return result;
}

bool TestBlockValidity(BlockValidationState& state,
                       const CChainParams& chainparams,
                       Chainstate& chainstate,
                       const CBlock& block,
                       CBlockIndex* pindexPrev,
                       bool fCheckPOW,
                       bool fCheckMerkleRoot)
{
    AssertLockHeld(cs_main);
    assert(pindexPrev && pindexPrev == chainstate.m_chain.Tip());
    CCoinsViewCache viewNew(&chainstate.CoinsTip());
    uint256 block_hash(block.GetHash());
    CBlockIndex indexDummy(block);
    indexDummy.pprev = pindexPrev;
    indexDummy.nHeight = pindexPrev->nHeight + 1;
    indexDummy.phashBlock = &block_hash;

    // NOTE: CheckBlockHeader is called by CheckBlock
    if (!ContextualCheckBlockHeader(block, state, chainstate.m_blockman, chainstate.m_chainman, pindexPrev)) {
        LogError("%s: Consensus::ContextualCheckBlockHeader: %s\n", __func__, state.ToString());
        return false;
    }
    if (!CheckBlock(block, state, chainparams.GetConsensus(), fCheckPOW, fCheckMerkleRoot)) {
        LogError("%s: Consensus::CheckBlock: %s\n", __func__, state.ToString());
        return false;
    }
    if (!ContextualCheckBlock(block, state, chainstate.m_chainman, pindexPrev)) {
        LogError("%s: Consensus::ContextualCheckBlock: %s\n", __func__, state.ToString());
        return false;
    }
    if (!chainstate.ConnectBlock(block, state, &indexDummy, viewNew, true)) {
        return false;
    }
    assert(state.IsValid());

    return true;
}

/* This function is called from the RPC code for pruneblockchain */
void PruneBlockFilesManual(Chainstate& active_chainstate, int nManualPruneHeight)
{
    BlockValidationState state;
    if (!active_chainstate.FlushStateToDisk(
            state, FlushStateMode::NONE, nManualPruneHeight)) {
        LogPrintf("%s: failed to flush state (%s)\n", __func__, state.ToString());
    }
}

bool Chainstate::LoadChainTip()
{
    AssertLockHeld(cs_main);
    const CCoinsViewCache& coins_cache = CoinsTip();
    assert(!coins_cache.GetBestBlock().IsNull()); // Never called when the coins view is empty
    const CBlockIndex* tip = m_chain.Tip();

    if (tip && tip->GetBlockHash() == coins_cache.GetBestBlock()) {
        return true;
    }

    // Load pointer to end of best chain
    CBlockIndex* pindex = m_blockman.LookupBlockIndex(coins_cache.GetBestBlock());
    if (!pindex) {
        return false;
    }
    m_chain.SetTip(*pindex);
    PruneBlockIndexCandidates();

    tip = m_chain.Tip();
    LogPrintf("Loaded best chain: hashBestChain=%s height=%d date=%s progress=%f\n",
              tip->GetBlockHash().ToString(),
              m_chain.Height(),
              FormatISO8601DateTime(tip->GetBlockTime()),
              m_chainman.GuessVerificationProgress(tip));

    // Ensure KernelNotifications m_tip_block is set even if no new block arrives.
    if (this->GetRole() != ChainstateRole::BACKGROUND) {
        // Ignoring return value for now.
        (void)m_chainman.GetNotifications().blockTip(
            /*state=*/GetSynchronizationState(/*init=*/true, m_chainman.m_blockman.m_blockfiles_indexed),
            /*index=*/*pindex,
            /*verification_progress=*/m_chainman.GuessVerificationProgress(tip));
    }

    return true;
}

CVerifyDB::CVerifyDB(Notifications& notifications)
    : m_notifications{notifications}
{
    m_notifications.progress(_("Verifying blocks…"), 0, false);
}

CVerifyDB::~CVerifyDB()
{
    m_notifications.progress(bilingual_str{}, 100, false);
}

VerifyDBResult CVerifyDB::VerifyDB(
    Chainstate& chainstate,
    const Consensus::Params& consensus_params,
    CCoinsView& coinsview,
    int nCheckLevel, int nCheckDepth)
{
    AssertLockHeld(cs_main);

    if (chainstate.m_chain.Tip() == nullptr || chainstate.m_chain.Tip()->pprev == nullptr) {
        return VerifyDBResult::SUCCESS;
    }

    // Verify blocks in the best chain
    if (nCheckDepth <= 0 || nCheckDepth > chainstate.m_chain.Height()) {
        nCheckDepth = chainstate.m_chain.Height();
    }
    nCheckLevel = std::max(0, std::min(4, nCheckLevel));
    LogPrintf("Verifying last %i blocks at level %i\n", nCheckDepth, nCheckLevel);
    CCoinsViewCache coins(&coinsview);
    CBlockIndex* pindex;
    CBlockIndex* pindexFailure = nullptr;
    int nGoodTransactions = 0;
    BlockValidationState state;
    int reportDone = 0;
    bool skipped_no_block_data{false};
    bool skipped_l3_checks{false};
    LogPrintf("Verification progress: 0%%\n");

    const bool is_snapshot_cs{chainstate.m_from_snapshot_blockhash};

    for (pindex = chainstate.m_chain.Tip(); pindex && pindex->pprev; pindex = pindex->pprev) {
        const int percentageDone = std::max(1, std::min(99, (int)(((double)(chainstate.m_chain.Height() - pindex->nHeight)) / (double)nCheckDepth * (nCheckLevel >= 4 ? 50 : 100))));
        if (reportDone < percentageDone / 10) {
            // report every 10% step
            LogPrintf("Verification progress: %d%%\n", percentageDone);
            reportDone = percentageDone / 10;
        }
        m_notifications.progress(_("Verifying blocks…"), percentageDone, false);
        if (pindex->nHeight <= chainstate.m_chain.Height() - nCheckDepth) {
            break;
        }
        if ((chainstate.m_blockman.IsPruneMode() || is_snapshot_cs) && !(pindex->nStatus & BLOCK_HAVE_DATA)) {
            // If pruning or running under an assumeutxo snapshot, only go
            // back as far as we have data.
            LogPrintf("VerifyDB(): block verification stopping at height %d (no data). This could be due to pruning or use of an assumeutxo snapshot.\n", pindex->nHeight);
            skipped_no_block_data = true;
            break;
        }
        CBlock block;
        // check level 0: read from disk
        if (!chainstate.m_blockman.ReadBlock(block, *pindex)) {
            LogPrintf("Verification error: ReadBlock failed at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
            return VerifyDBResult::CORRUPTED_BLOCK_DB;
        }
        // check level 1: verify block validity
        if (nCheckLevel >= 1 && !CheckBlock(block, state, consensus_params)) {
            LogPrintf("Verification error: found bad block at %d, hash=%s (%s)\n",
                      pindex->nHeight, pindex->GetBlockHash().ToString(), state.ToString());
            return VerifyDBResult::CORRUPTED_BLOCK_DB;
        }
        // check level 2: verify undo validity
        if (nCheckLevel >= 2 && pindex) {
            CBlockUndo undo;
            if (!pindex->GetUndoPos().IsNull()) {
                if (!chainstate.m_blockman.ReadBlockUndo(undo, *pindex)) {
                    LogPrintf("Verification error: found bad undo data at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                    return VerifyDBResult::CORRUPTED_BLOCK_DB;
                }
            }
        }
        // check level 3: check for inconsistencies during memory-only disconnect of tip blocks
        size_t curr_coins_usage = coins.DynamicMemoryUsage() + chainstate.CoinsTip().DynamicMemoryUsage();

        if (nCheckLevel >= 3) {
            if (curr_coins_usage <= chainstate.m_coinstip_cache_size_bytes) {
                assert(coins.GetBestBlock() == pindex->GetBlockHash());
                DisconnectResult res = chainstate.DisconnectBlock(block, pindex, coins);
                if (res == DISCONNECT_FAILED) {
                    LogPrintf("Verification error: irrecoverable inconsistency in block data at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                    return VerifyDBResult::CORRUPTED_BLOCK_DB;
                }
                if (res == DISCONNECT_UNCLEAN) {
                    nGoodTransactions = 0;
                    pindexFailure = pindex;
                } else {
                    nGoodTransactions += block.vtx.size();
                }
            } else {
                skipped_l3_checks = true;
            }
        }
        if (chainstate.m_chainman.m_interrupt) return VerifyDBResult::INTERRUPTED;
    }
    if (pindexFailure) {
        LogPrintf("Verification error: coin database inconsistencies found (last %i blocks, %i good transactions before that)\n", chainstate.m_chain.Height() - pindexFailure->nHeight + 1, nGoodTransactions);
        return VerifyDBResult::CORRUPTED_BLOCK_DB;
    }
    if (skipped_l3_checks) {
        LogPrintf("Skipped verification of level >=3 (insufficient database cache size). Consider increasing -dbcache.\n");
    }

    // store block count as we move pindex at check level >= 4
    int block_count = chainstate.m_chain.Height() - pindex->nHeight;

    // check level 4: try reconnecting blocks
    if (nCheckLevel >= 4 && !skipped_l3_checks) {
        while (pindex != chainstate.m_chain.Tip()) {
            const int percentageDone = std::max(1, std::min(99, 100 - (int)(((double)(chainstate.m_chain.Height() - pindex->nHeight)) / (double)nCheckDepth * 50)));
            if (reportDone < percentageDone / 10) {
                // report every 10% step
                LogPrintf("Verification progress: %d%%\n", percentageDone);
                reportDone = percentageDone / 10;
            }
            m_notifications.progress(_("Verifying blocks…"), percentageDone, false);
            pindex = chainstate.m_chain.Next(pindex);
            CBlock block;
            if (!chainstate.m_blockman.ReadBlock(block, *pindex)) {
                LogPrintf("Verification error: ReadBlock failed at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                return VerifyDBResult::CORRUPTED_BLOCK_DB;
            }
            if (!chainstate.ConnectBlock(block, state, pindex, coins)) {
                LogPrintf("Verification error: found unconnectable block at %d, hash=%s (%s)\n", pindex->nHeight, pindex->GetBlockHash().ToString(), state.ToString());
                return VerifyDBResult::CORRUPTED_BLOCK_DB;
            }
            if (chainstate.m_chainman.m_interrupt) return VerifyDBResult::INTERRUPTED;
        }
    }

    LogPrintf("Verification: No coin database inconsistencies in last %i blocks (%i transactions)\n", block_count, nGoodTransactions);

    if (skipped_l3_checks) {
        return VerifyDBResult::SKIPPED_L3_CHECKS;
    }
    if (skipped_no_block_data) {
        return VerifyDBResult::SKIPPED_MISSING_BLOCKS;
    }
    return VerifyDBResult::SUCCESS;
}

/** Apply the effects of a block on the utxo cache, ignoring that it may already have been applied. */
bool Chainstate::RollforwardBlock(const CBlockIndex* pindex, CCoinsViewCache& inputs)
{
    AssertLockHeld(cs_main);
    // TODO: merge with ConnectBlock
    CBlock block;
    if (!m_blockman.ReadBlock(block, *pindex)) {
        LogError("ReplayBlock(): ReadBlock failed at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
        return false;
    }

    for (const CTransactionRef& tx : block.vtx) {
        if (!tx->IsCoinBase()) {
            for (const CTxIn &txin : tx->vin) {
                inputs.SpendCoin(txin.prevout);
            }
        }
        // Pass check = true as every addition may be an overwrite.
        AddCoins(inputs, *tx, pindex->nHeight, true);
    }
    return true;
}

bool Chainstate::ReplayBlocks()
{
    LOCK(cs_main);

    CCoinsView& db = this->CoinsDB();
    CCoinsViewCache cache(&db);

    std::vector<uint256> hashHeads = db.GetHeadBlocks();
    if (hashHeads.empty()) return true; // We're already in a consistent state.
    if (hashHeads.size() != 2) {
        LogError("ReplayBlocks(): unknown inconsistent state\n");
        return false;
    }

    m_chainman.GetNotifications().progress(_("Replaying blocks…"), 0, false);
    LogPrintf("Replaying blocks\n");

    const CBlockIndex* pindexOld = nullptr;  // Old tip during the interrupted flush.
    const CBlockIndex* pindexNew;            // New tip during the interrupted flush.
    const CBlockIndex* pindexFork = nullptr; // Latest block common to both the old and the new tip.

    if (m_blockman.m_block_index.count(hashHeads[0]) == 0) {
        LogError("ReplayBlocks(): reorganization to unknown block requested\n");
        return false;
    }
    pindexNew = &(m_blockman.m_block_index[hashHeads[0]]);

    if (!hashHeads[1].IsNull()) { // The old tip is allowed to be 0, indicating it's the first flush.
        if (m_blockman.m_block_index.count(hashHeads[1]) == 0) {
            LogError("ReplayBlocks(): reorganization from unknown block requested\n");
            return false;
        }
        pindexOld = &(m_blockman.m_block_index[hashHeads[1]]);
        pindexFork = LastCommonAncestor(pindexOld, pindexNew);
        assert(pindexFork != nullptr);
    }

    // Rollback along the old branch.
    while (pindexOld != pindexFork) {
        if (pindexOld->nHeight > 0) { // Never disconnect the genesis block.
            CBlock block;
            if (!m_blockman.ReadBlock(block, *pindexOld)) {
                LogError("RollbackBlock(): ReadBlock() failed at %d, hash=%s\n", pindexOld->nHeight, pindexOld->GetBlockHash().ToString());
                return false;
            }
            LogPrintf("Rolling back %s (%i)\n", pindexOld->GetBlockHash().ToString(), pindexOld->nHeight);
            DisconnectResult res = DisconnectBlock(block, pindexOld, cache);
            if (res == DISCONNECT_FAILED) {
                LogError("RollbackBlock(): DisconnectBlock failed at %d, hash=%s\n", pindexOld->nHeight, pindexOld->GetBlockHash().ToString());
                return false;
            }
            // If DISCONNECT_UNCLEAN is returned, it means a non-existing UTXO was deleted, or an existing UTXO was
            // overwritten. It corresponds to cases where the block-to-be-disconnect never had all its operations
            // applied to the UTXO set. However, as both writing a UTXO and deleting a UTXO are idempotent operations,
            // the result is still a version of the UTXO set with the effects of that block undone.
        }
        pindexOld = pindexOld->pprev;
    }

    // Roll forward from the forking point to the new tip.
    int nForkHeight = pindexFork ? pindexFork->nHeight : 0;
    for (int nHeight = nForkHeight + 1; nHeight <= pindexNew->nHeight; ++nHeight) {
        const CBlockIndex& pindex{*Assert(pindexNew->GetAncestor(nHeight))};

        LogPrintf("Rolling forward %s (%i)\n", pindex.GetBlockHash().ToString(), nHeight);
        m_chainman.GetNotifications().progress(_("Replaying blocks…"), (int)((nHeight - nForkHeight) * 100.0 / (pindexNew->nHeight - nForkHeight)), false);
        if (!RollforwardBlock(&pindex, cache)) return false;
    }

    cache.SetBestBlock(pindexNew->GetBlockHash());
    cache.Flush();
    m_chainman.GetNotifications().progress(bilingual_str{}, 100, false);
    return true;
}

bool Chainstate::NeedsRedownload() const
{
    AssertLockHeld(cs_main);

    // At and above m_params.SegwitHeight, segwit consensus rules must be validated
    CBlockIndex* block{m_chain.Tip()};

    while (block != nullptr && DeploymentActiveAt(*block, m_chainman, Consensus::DEPLOYMENT_SEGWIT)) {
        if (!(block->nStatus & BLOCK_OPT_WITNESS)) {
            // block is insufficiently validated for a segwit client
            return true;
        }
        block = block->pprev;
    }

    return false;
}

void Chainstate::ClearBlockIndexCandidates()
{
    AssertLockHeld(::cs_main);
    setBlockIndexCandidates.clear();
}

bool ChainstateManager::LoadBlockIndex()
{
    AssertLockHeld(cs_main);
    // Load block index from databases
    if (m_blockman.m_blockfiles_indexed) {
        bool ret{m_blockman.LoadBlockIndexDB(SnapshotBlockhash())};
        if (!ret) return false;

        m_blockman.ScanAndUnlinkAlreadyPrunedFiles();

        std::vector<CBlockIndex*> vSortedByHeight{m_blockman.GetAllBlockIndices()};
        std::sort(vSortedByHeight.begin(), vSortedByHeight.end(),
                  CBlockIndexHeightOnlyComparator());

        for (CBlockIndex* pindex : vSortedByHeight) {
            if (m_interrupt) return false;
            // If we have an assumeutxo-based chainstate, then the snapshot
            // block will be a candidate for the tip, but it may not be
            // VALID_TRANSACTIONS (eg if we haven't yet downloaded the block),
            // so we special-case the snapshot block as a potential candidate
            // here.
            if (pindex == GetSnapshotBaseBlock() ||
                    (pindex->IsValid(BLOCK_VALID_TRANSACTIONS) &&
                     (pindex->HaveNumChainTxs() || pindex->pprev == nullptr))) {

                for (Chainstate* chainstate : GetAll()) {
                    chainstate->TryAddBlockIndexCandidate(pindex);
                }
            }
            if (pindex->nStatus & BLOCK_FAILED_MASK && (!m_best_invalid || pindex->nChainWork > m_best_invalid->nChainWork)) {
                m_best_invalid = pindex;
            }
            if (pindex->IsValid(BLOCK_VALID_TREE) && (m_best_header == nullptr || CBlockIndexWorkComparator()(m_best_header, pindex)))
                m_best_header = pindex;
        }
    }
    return true;
}

bool Chainstate::LoadGenesisBlock()
{
    LOCK(cs_main);

    const CChainParams& params{m_chainman.GetParams()};

    // Check whether we're already initialized by checking for genesis in
    // m_blockman.m_block_index. Note that we can't use m_chain here, since it is
    // set based on the coins db, not the block index db, which is the only
    // thing loaded at this point.
    if (m_blockman.m_block_index.count(params.GenesisBlock().GetHash()))
        return true;

    try {
        const CBlock& block = params.GenesisBlock();
        FlatFilePos blockPos{m_blockman.WriteBlock(block, 0)};
        if (blockPos.IsNull()) {
            LogError("%s: writing genesis block to disk failed\n", __func__);
            return false;
        }
        CBlockIndex* pindex = m_blockman.AddToBlockIndex(block, m_chainman.m_best_header);
        m_chainman.ReceivedBlockTransactions(block, pindex, blockPos);
    } catch (const std::runtime_error& e) {
        LogError("%s: failed to write genesis block: %s\n", __func__, e.what());
        return false;
    }

    return true;
}

void ChainstateManager::LoadExternalBlockFile(
    AutoFile& file_in,
    FlatFilePos* dbp,
    std::multimap<uint256, FlatFilePos>* blocks_with_unknown_parent)
{
    // Either both should be specified (-reindex), or neither (-loadblock).
    assert(!dbp == !blocks_with_unknown_parent);

    const auto start{SteadyClock::now()};
    const CChainParams& params{GetParams()};

    int nLoaded = 0;
    try {
        BufferedFile blkdat{file_in, 2 * MAX_BLOCK_SERIALIZED_SIZE, MAX_BLOCK_SERIALIZED_SIZE + 8};
        // nRewind indicates where to resume scanning in case something goes wrong,
        // such as a block fails to deserialize.
        uint64_t nRewind = blkdat.GetPos();
        while (!blkdat.eof()) {
            if (m_interrupt) return;

            blkdat.SetPos(nRewind);
            nRewind++; // start one byte further next time, in case of failure
            blkdat.SetLimit(); // remove former limit
            unsigned int nSize = 0;
            try {
                // locate a header
                MessageStartChars buf;
                blkdat.FindByte(std::byte(params.MessageStart()[0]));
                nRewind = blkdat.GetPos() + 1;
                blkdat >> buf;
                if (buf != params.MessageStart()) {
                    continue;
                }
                // read size
                blkdat >> nSize;
                if (nSize < 80 || nSize > MAX_BLOCK_SERIALIZED_SIZE)
                    continue;
            } catch (const std::exception&) {
                // no valid block header found; don't complain
                // (this happens at the end of every blk.dat file)
                break;
            }
            try {
                // read block header
                const uint64_t nBlockPos{blkdat.GetPos()};
                if (dbp)
                    dbp->nPos = nBlockPos;
                blkdat.SetLimit(nBlockPos + nSize);
                CBlockHeader header;
                blkdat >> header;
                const uint256 hash{header.GetHash()};
                // Skip the rest of this block (this may read from disk into memory); position to the marker before the
                // next block, but it's still possible to rewind to the start of the current block (without a disk read).
                nRewind = nBlockPos + nSize;
                blkdat.SkipTo(nRewind);

                std::shared_ptr<CBlock> pblock{}; // needs to remain available after the cs_main lock is released to avoid duplicate reads from disk

                {
                    LOCK(cs_main);
                    // detect out of order blocks, and store them for later
                    if (hash != params.GetConsensus().hashGenesisBlock && !m_blockman.LookupBlockIndex(header.hashPrevBlock)) {
                        LogDebug(BCLog::REINDEX, "%s: Out of order block %s, parent %s not known\n", __func__, hash.ToString(),
                                 header.hashPrevBlock.ToString());
                        if (dbp && blocks_with_unknown_parent) {
                            blocks_with_unknown_parent->emplace(header.hashPrevBlock, *dbp);
                        }
                        continue;
                    }

                    // process in case the block isn't known yet
                    const CBlockIndex* pindex = m_blockman.LookupBlockIndex(hash);
                    if (!pindex || (pindex->nStatus & BLOCK_HAVE_DATA) == 0) {
                        // This block can be processed immediately; rewind to its start, read and deserialize it.
                        blkdat.SetPos(nBlockPos);
                        pblock = std::make_shared<CBlock>();
                        blkdat >> TX_WITH_WITNESS(*pblock);
                        nRewind = blkdat.GetPos();

                        BlockValidationState state;
                        if (AcceptBlock(pblock, state, nullptr, true, dbp, nullptr, true)) {
                            nLoaded++;
                        }
                        if (state.IsError()) {
                            break;
                        }
                    } else if (hash != params.GetConsensus().hashGenesisBlock && pindex->nHeight % 1000 == 0) {
                        LogDebug(BCLog::REINDEX, "Block Import: already had block %s at height %d\n", hash.ToString(), pindex->nHeight);
                    }
                }

                // Activate the genesis block so normal node progress can continue
                // During first -reindex, this will only connect Genesis since
                // ActivateBestChain only connects blocks which are in the block tree db,
                // which only contains blocks whose parents are in it.
                // But do this only if genesis isn't activated yet, to avoid connecting many blocks
                // without assumevalid in the case of a continuation of a reindex that
                // was interrupted by the user.
                if (hash == params.GetConsensus().hashGenesisBlock && WITH_LOCK(::cs_main, return ActiveHeight()) == -1) {
                    BlockValidationState state;
                    if (!ActiveChainstate().ActivateBestChain(state, nullptr)) {
                        break;
                    }
                }

                if (m_blockman.IsPruneMode() && m_blockman.m_blockfiles_indexed && pblock) {
                    // must update the tip for pruning to work while importing with -loadblock.
                    // this is a tradeoff to conserve disk space at the expense of time
                    // spent updating the tip to be able to prune.
                    // otherwise, ActivateBestChain won't be called by the import process
                    // until after all of the block files are loaded. ActivateBestChain can be
                    // called by concurrent network message processing. but, that is not
                    // reliable for the purpose of pruning while importing.
                    bool activation_failure = false;
                    for (auto c : GetAll()) {
                        BlockValidationState state;
                        if (!c->ActivateBestChain(state, pblock)) {
                            LogDebug(BCLog::REINDEX, "failed to activate chain (%s)\n", state.ToString());
                            activation_failure = true;
                            break;
                        }
                    }
                    if (activation_failure) {
                        break;
                    }
                }

                NotifyHeaderTip();

                if (!blocks_with_unknown_parent) continue;

                // Recursively process earlier encountered successors of this block
                std::deque<uint256> queue;
                queue.push_back(hash);
                while (!queue.empty()) {
                    uint256 head = queue.front();
                    queue.pop_front();
                    auto range = blocks_with_unknown_parent->equal_range(head);
                    while (range.first != range.second) {
                        std::multimap<uint256, FlatFilePos>::iterator it = range.first;
                        std::shared_ptr<CBlock> pblockrecursive = std::make_shared<CBlock>();
                        if (m_blockman.ReadBlock(*pblockrecursive, it->second)) {
                            LogDebug(BCLog::REINDEX, "%s: Processing out of order child %s of %s\n", __func__, pblockrecursive->GetHash().ToString(),
                                    head.ToString());
                            LOCK(cs_main);
                            BlockValidationState dummy;
                            if (AcceptBlock(pblockrecursive, dummy, nullptr, true, &it->second, nullptr, true)) {
                                nLoaded++;
                                queue.push_back(pblockrecursive->GetHash());
                            }
                        }
                        range.first++;
                        blocks_with_unknown_parent->erase(it);
                        NotifyHeaderTip();
                    }
                }
            } catch (const std::exception& e) {
                // historical bugs added extra data to the block files that does not deserialize cleanly.
                // commonly this data is between readable blocks, but it does not really matter. such data is not fatal to the import process.
                // the code that reads the block files deals with invalid data by simply ignoring it.
                // it continues to search for the next {4 byte magic message start bytes + 4 byte length + block} that does deserialize cleanly
                // and passes all of the other block validation checks dealing with POW and the merkle root, etc...
                // we merely note with this informational log message when unexpected data is encountered.
                // we could also be experiencing a storage system read error, or a read of a previous bad write. these are possible, but
                // less likely scenarios. we don't have enough information to tell a difference here.
                // the reindex process is not the place to attempt to clean and/or compact the block files. if so desired, a studious node operator
                // may use knowledge of the fact that the block files are not entirely pristine in order to prepare a set of pristine, and
                // perhaps ordered, block files for later reindexing.
                LogDebug(BCLog::REINDEX, "%s: unexpected data at file offset 0x%x - %s. continuing\n", __func__, (nRewind - 1), e.what());
            }
        }
    } catch (const std::runtime_error& e) {
        GetNotifications().fatalError(strprintf(_("System error while loading external block file: %s"), e.what()));
    }
    LogPrintf("Loaded %i blocks from external file in %dms\n", nLoaded, Ticks<std::chrono::milliseconds>(SteadyClock::now() - start));
}

bool ChainstateManager::ShouldCheckBlockIndex() const
{
    // Assert to verify Flatten() has been called.
    if (!*Assert(m_options.check_block_index)) return false;
    if (FastRandomContext().randrange(*m_options.check_block_index) >= 1) return false;
    return true;
}

void ChainstateManager::CheckBlockIndex() const
{
    if (!ShouldCheckBlockIndex()) {
        return;
    }

    LOCK(cs_main);

    // During a reindex, we read the genesis block and call CheckBlockIndex before ActivateBestChain,
    // so we have the genesis block in m_blockman.m_block_index but no active chain. (A few of the
    // tests when iterating the block tree require that m_chain has been initialized.)
    if (ActiveChain().Height() < 0) {
        assert(m_blockman.m_block_index.size() <= 1);
        return;
    }

    // Build forward-pointing data structure for the entire block tree.
    // For performance reasons, indexes of the best header chain are stored in a vector (within CChain).
    // All remaining blocks are stored in a multimap.
    // The best header chain can differ from the active chain: E.g. its entries may belong to blocks that
    // are not yet validated.
    CChain best_hdr_chain;
    assert(m_best_header);
    best_hdr_chain.SetTip(*m_best_header);

    std::multimap<const CBlockIndex*, const CBlockIndex*> forward;
    for (auto& [_, block_index] : m_blockman.m_block_index) {
        // Only save indexes in forward that are not part of the best header chain.
        if (!best_hdr_chain.Contains(&block_index)) {
            // Only genesis, which must be part of the best header chain, can have a nullptr parent.
            assert(block_index.pprev);
            forward.emplace(block_index.pprev, &block_index);
        }
    }
    assert(forward.size() + best_hdr_chain.Height() + 1 == m_blockman.m_block_index.size());

    const CBlockIndex* pindex = best_hdr_chain[0];
    assert(pindex);
    // Iterate over the entire block tree, using depth-first search.
    // Along the way, remember whether there are blocks on the path from genesis
    // block being explored which are the first to have certain properties.
    size_t nNodes = 0;
    int nHeight = 0;
    const CBlockIndex* pindexFirstInvalid = nullptr;              // Oldest ancestor of pindex which is invalid.
    const CBlockIndex* pindexFirstMissing = nullptr;              // Oldest ancestor of pindex which does not have BLOCK_HAVE_DATA, since assumeutxo snapshot if used.
    const CBlockIndex* pindexFirstNeverProcessed = nullptr;       // Oldest ancestor of pindex for which nTx == 0, since assumeutxo snapshot if used.
    const CBlockIndex* pindexFirstNotTreeValid = nullptr;         // Oldest ancestor of pindex which does not have BLOCK_VALID_TREE (regardless of being valid or not).
    const CBlockIndex* pindexFirstNotTransactionsValid = nullptr; // Oldest ancestor of pindex which does not have BLOCK_VALID_TRANSACTIONS (regardless of being valid or not), since assumeutxo snapshot if used.
    const CBlockIndex* pindexFirstNotChainValid = nullptr;        // Oldest ancestor of pindex which does not have BLOCK_VALID_CHAIN (regardless of being valid or not), since assumeutxo snapshot if used.
    const CBlockIndex* pindexFirstNotScriptsValid = nullptr;      // Oldest ancestor of pindex which does not have BLOCK_VALID_SCRIPTS (regardless of being valid or not), since assumeutxo snapshot if used.

    // After checking an assumeutxo snapshot block, reset pindexFirst pointers
    // to earlier blocks that have not been downloaded or validated yet, so
    // checks for later blocks can assume the earlier blocks were validated and
    // be stricter, testing for more requirements.
    const CBlockIndex* snap_base{GetSnapshotBaseBlock()};
    const CBlockIndex *snap_first_missing{}, *snap_first_notx{}, *snap_first_notv{}, *snap_first_nocv{}, *snap_first_nosv{};
    auto snap_update_firsts = [&] {
        if (pindex == snap_base) {
            std::swap(snap_first_missing, pindexFirstMissing);
            std::swap(snap_first_notx, pindexFirstNeverProcessed);
            std::swap(snap_first_notv, pindexFirstNotTransactionsValid);
            std::swap(snap_first_nocv, pindexFirstNotChainValid);
            std::swap(snap_first_nosv, pindexFirstNotScriptsValid);
        }
    };

    while (pindex != nullptr) {
        nNodes++;
        if (pindexFirstInvalid == nullptr && pindex->nStatus & BLOCK_FAILED_VALID) pindexFirstInvalid = pindex;
        if (pindexFirstMissing == nullptr && !(pindex->nStatus & BLOCK_HAVE_DATA)) {
            pindexFirstMissing = pindex;
        }
        if (pindexFirstNeverProcessed == nullptr && pindex->nTx == 0) pindexFirstNeverProcessed = pindex;
        if (pindex->pprev != nullptr && pindexFirstNotTreeValid == nullptr && (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_TREE) pindexFirstNotTreeValid = pindex;

        if (pindex->pprev != nullptr) {
            if (pindexFirstNotTransactionsValid == nullptr &&
                    (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_TRANSACTIONS) {
                pindexFirstNotTransactionsValid = pindex;
            }

            if (pindexFirstNotChainValid == nullptr &&
                    (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_CHAIN) {
                pindexFirstNotChainValid = pindex;
            }

            if (pindexFirstNotScriptsValid == nullptr &&
                    (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_SCRIPTS) {
                pindexFirstNotScriptsValid = pindex;
            }
        }

        // Begin: actual consistency checks.
        if (pindex->pprev == nullptr) {
            // Genesis block checks.
            assert(pindex->GetBlockHash() == GetConsensus().hashGenesisBlock); // Genesis block's hash must match.
            for (const Chainstate* c : {m_ibd_chainstate.get(), m_snapshot_chainstate.get()}) {
                if (c && c->m_chain.Genesis() != nullptr) {
                    assert(pindex == c->m_chain.Genesis()); // The chain's genesis block must be this block.
                }
            }
        }
        if (!pindex->HaveNumChainTxs()) assert(pindex->nSequenceId <= 0); // nSequenceId can't be set positive for blocks that aren't linked (negative is used for preciousblock)
        // VALID_TRANSACTIONS is equivalent to nTx > 0 for all nodes (whether or not pruning has occurred).
        // HAVE_DATA is only equivalent to nTx > 0 (or VALID_TRANSACTIONS) if no pruning has occurred.
        if (!m_blockman.m_have_pruned) {
            // If we've never pruned, then HAVE_DATA should be equivalent to nTx > 0
            assert(!(pindex->nStatus & BLOCK_HAVE_DATA) == (pindex->nTx == 0));
            assert(pindexFirstMissing == pindexFirstNeverProcessed);
        } else {
            // If we have pruned, then we can only say that HAVE_DATA implies nTx > 0
            if (pindex->nStatus & BLOCK_HAVE_DATA) assert(pindex->nTx > 0);
        }
        if (pindex->nStatus & BLOCK_HAVE_UNDO) assert(pindex->nStatus & BLOCK_HAVE_DATA);
        if (snap_base && snap_base->GetAncestor(pindex->nHeight) == pindex) {
            // Assumed-valid blocks should connect to the main chain.
            assert((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_TREE);
        }
        // There should only be an nTx value if we have
        // actually seen a block's transactions.
        assert(((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_TRANSACTIONS) == (pindex->nTx > 0)); // This is pruning-independent.
        // All parents having had data (at some point) is equivalent to all parents being VALID_TRANSACTIONS, which is equivalent to HaveNumChainTxs().
        // HaveNumChainTxs will also be set in the assumeutxo snapshot block from snapshot metadata.
        assert((pindexFirstNeverProcessed == nullptr || pindex == snap_base) == pindex->HaveNumChainTxs());
        assert((pindexFirstNotTransactionsValid == nullptr || pindex == snap_base) == pindex->HaveNumChainTxs());
        assert(pindex->nHeight == nHeight); // nHeight must be consistent.
        assert(pindex->pprev == nullptr || pindex->nChainWork >= pindex->pprev->nChainWork); // For every block except the genesis block, the chainwork must be larger than the parent's.
        assert(nHeight < 2 || (pindex->pskip && (pindex->pskip->nHeight < nHeight))); // The pskip pointer must point back for all but the first 2 blocks.
        assert(pindexFirstNotTreeValid == nullptr); // All m_blockman.m_block_index entries must at least be TREE valid
        if ((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_TREE) assert(pindexFirstNotTreeValid == nullptr); // TREE valid implies all parents are TREE valid
        if ((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_CHAIN) assert(pindexFirstNotChainValid == nullptr); // CHAIN valid implies all parents are CHAIN valid
        if ((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_SCRIPTS) assert(pindexFirstNotScriptsValid == nullptr); // SCRIPTS valid implies all parents are SCRIPTS valid
        if (pindexFirstInvalid == nullptr) {
            // Checks for not-invalid blocks.
            assert((pindex->nStatus & BLOCK_FAILED_MASK) == 0); // The failed mask cannot be set for blocks without invalid parents.
        }
        // Make sure m_chain_tx_count sum is correctly computed.
        if (!pindex->pprev) {
            // If no previous block, nTx and m_chain_tx_count must be the same.
            assert(pindex->m_chain_tx_count == pindex->nTx);
        } else if (pindex->pprev->m_chain_tx_count > 0 && pindex->nTx > 0) {
            // If previous m_chain_tx_count is set and number of transactions in block is known, sum must be set.
            assert(pindex->m_chain_tx_count == pindex->nTx + pindex->pprev->m_chain_tx_count);
        } else {
            // Otherwise m_chain_tx_count should only be set if this is a snapshot
            // block, and must be set if it is.
            assert((pindex->m_chain_tx_count != 0) == (pindex == snap_base));
        }

        // Chainstate-specific checks on setBlockIndexCandidates
        for (const Chainstate* c : {m_ibd_chainstate.get(), m_snapshot_chainstate.get()}) {
            if (!c || c->m_chain.Tip() == nullptr) continue;
            // Two main factors determine whether pindex is a candidate in
            // setBlockIndexCandidates:
            //
            // - If pindex has less work than the chain tip, it should not be a
            //   candidate, and this will be asserted below. Otherwise it is a
            //   potential candidate.
            //
            // - If pindex or one of its parent blocks back to the genesis block
            //   or an assumeutxo snapshot never downloaded transactions
            //   (pindexFirstNeverProcessed is non-null), it should not be a
            //   candidate, and this will be asserted below. The only exception
            //   is if pindex itself is an assumeutxo snapshot block. Then it is
            //   also a potential candidate.
            if (!CBlockIndexWorkComparator()(pindex, c->m_chain.Tip()) && (pindexFirstNeverProcessed == nullptr || pindex == snap_base)) {
                // If pindex was detected as invalid (pindexFirstInvalid is
                // non-null), it is not required to be in
                // setBlockIndexCandidates.
                if (pindexFirstInvalid == nullptr) {
                    // If pindex and all its parents back to the genesis block
                    // or an assumeutxo snapshot block downloaded transactions,
                    // and the transactions were not pruned (pindexFirstMissing
                    // is null), it is a potential candidate. The check
                    // excludes pruned blocks, because if any blocks were
                    // pruned between pindex and the current chain tip, pindex will
                    // only temporarily be added to setBlockIndexCandidates,
                    // before being moved to m_blocks_unlinked. This check
                    // could be improved to verify that if all blocks between
                    // the chain tip and pindex have data, pindex must be a
                    // candidate.
                    //
                    // If pindex is the chain tip, it also is a potential
                    // candidate.
                    //
                    // If the chainstate was loaded from a snapshot and pindex
                    // is the base of the snapshot, pindex is also a potential
                    // candidate.
                    if (pindexFirstMissing == nullptr || pindex == c->m_chain.Tip() || pindex == c->SnapshotBase()) {
                        // If this chainstate is the active chainstate, pindex
                        // must be in setBlockIndexCandidates. Otherwise, this
                        // chainstate is a background validation chainstate, and
                        // pindex only needs to be added if it is an ancestor of
                        // the snapshot that is being validated.
                        if (c == &ActiveChainstate() || snap_base->GetAncestor(pindex->nHeight) == pindex) {
                            assert(c->setBlockIndexCandidates.contains(const_cast<CBlockIndex*>(pindex)));
                        }
                    }
                    // If some parent is missing, then it could be that this block was in
                    // setBlockIndexCandidates but had to be removed because of the missing data.
                    // In this case it must be in m_blocks_unlinked -- see test below.
                }
            } else { // If this block sorts worse than the current tip or some ancestor's block has never been seen, it cannot be in setBlockIndexCandidates.
                assert(!c->setBlockIndexCandidates.contains(const_cast<CBlockIndex*>(pindex)));
            }
        }
        // Check whether this block is in m_blocks_unlinked.
        auto rangeUnlinked{m_blockman.m_blocks_unlinked.equal_range(pindex->pprev)};
        bool foundInUnlinked = false;
        while (rangeUnlinked.first != rangeUnlinked.second) {
            assert(rangeUnlinked.first->first == pindex->pprev);
            if (rangeUnlinked.first->second == pindex) {
                foundInUnlinked = true;
                break;
            }
            rangeUnlinked.first++;
        }
        if (pindex->pprev && (pindex->nStatus & BLOCK_HAVE_DATA) && pindexFirstNeverProcessed != nullptr && pindexFirstInvalid == nullptr) {
            // If this block has block data available, some parent was never received, and has no invalid parents, it must be in m_blocks_unlinked.
            assert(foundInUnlinked);
        }
        if (!(pindex->nStatus & BLOCK_HAVE_DATA)) assert(!foundInUnlinked); // Can't be in m_blocks_unlinked if we don't HAVE_DATA
        if (pindexFirstMissing == nullptr) assert(!foundInUnlinked); // We aren't missing data for any parent -- cannot be in m_blocks_unlinked.
        if (pindex->pprev && (pindex->nStatus & BLOCK_HAVE_DATA) && pindexFirstNeverProcessed == nullptr && pindexFirstMissing != nullptr) {
            // We HAVE_DATA for this block, have received data for all parents at some point, but we're currently missing data for some parent.
            assert(m_blockman.m_have_pruned);
            // This block may have entered m_blocks_unlinked if:
            //  - it has a descendant that at some point had more work than the
            //    tip, and
            //  - we tried switching to that descendant but were missing
            //    data for some intermediate block between m_chain and the
            //    tip.
            // So if this block is itself better than any m_chain.Tip() and it wasn't in
            // setBlockIndexCandidates, then it must be in m_blocks_unlinked.
            for (const Chainstate* c : {m_ibd_chainstate.get(), m_snapshot_chainstate.get()}) {
                if (!c) continue;
                const bool is_active = c == &ActiveChainstate();
                if (!CBlockIndexWorkComparator()(pindex, c->m_chain.Tip()) && !c->setBlockIndexCandidates.contains(const_cast<CBlockIndex*>(pindex))) {
                    if (pindexFirstInvalid == nullptr) {
                        if (is_active || snap_base->GetAncestor(pindex->nHeight) == pindex) {
                            assert(foundInUnlinked);
                        }
                    }
                }
            }
        }
        // assert(pindex->GetBlockHash() == pindex->GetBlockHeader().GetHash()); // Perhaps too slow
        // End: actual consistency checks.


        // Try descending into the first subnode. Always process forks first and the best header chain after.
        snap_update_firsts();
        auto range{forward.equal_range(pindex)};
        if (range.first != range.second) {
            // A subnode not part of the best header chain was found.
            pindex = range.first->second;
            nHeight++;
            continue;
        } else if (best_hdr_chain.Contains(pindex)) {
            // Descend further into best header chain.
            nHeight++;
            pindex = best_hdr_chain[nHeight];
            if (!pindex) break; // we are finished, since the best header chain is always processed last
            continue;
        }
        // This is a leaf node.
        // Move upwards until we reach a node of which we have not yet visited the last child.
        while (pindex) {
            // We are going to either move to a parent or a sibling of pindex.
            snap_update_firsts();
            // If pindex was the first with a certain property, unset the corresponding variable.
            if (pindex == pindexFirstInvalid) pindexFirstInvalid = nullptr;
            if (pindex == pindexFirstMissing) pindexFirstMissing = nullptr;
            if (pindex == pindexFirstNeverProcessed) pindexFirstNeverProcessed = nullptr;
            if (pindex == pindexFirstNotTreeValid) pindexFirstNotTreeValid = nullptr;
            if (pindex == pindexFirstNotTransactionsValid) pindexFirstNotTransactionsValid = nullptr;
            if (pindex == pindexFirstNotChainValid) pindexFirstNotChainValid = nullptr;
            if (pindex == pindexFirstNotScriptsValid) pindexFirstNotScriptsValid = nullptr;
            // Find our parent.
            CBlockIndex* pindexPar = pindex->pprev;
            // Find which child we just visited.
            auto rangePar{forward.equal_range(pindexPar)};
            while (rangePar.first->second != pindex) {
                assert(rangePar.first != rangePar.second); // Our parent must have at least the node we're coming from as child.
                rangePar.first++;
            }
            // Proceed to the next one.
            rangePar.first++;
            if (rangePar.first != rangePar.second) {
                // Move to a sibling not part of the best header chain.
                pindex = rangePar.first->second;
                break;
            } else if (pindexPar == best_hdr_chain[nHeight - 1]) {
                // Move to pindex's sibling on the best-chain, if it has one.
                pindex = best_hdr_chain[nHeight];
                // There will not be a next block if (and only if) parent block is the best header.
                assert((pindex == nullptr) == (pindexPar == best_hdr_chain.Tip()));
                break;
            } else {
                // Move up further.
                pindex = pindexPar;
                nHeight--;
                continue;
            }
        }
    }

    // Check that we actually traversed the entire block index.
    assert(nNodes == forward.size() + best_hdr_chain.Height() + 1);
}

std::string Chainstate::ToString()
{
    AssertLockHeld(::cs_main);
    CBlockIndex* tip = m_chain.Tip();
    return strprintf("Chainstate [%s] @ height %d (%s)",
                     m_from_snapshot_blockhash ? "snapshot" : "ibd",
                     tip ? tip->nHeight : -1, tip ? tip->GetBlockHash().ToString() : "null");
}

bool Chainstate::ResizeCoinsCaches(size_t coinstip_size, size_t coinsdb_size)
{
    AssertLockHeld(::cs_main);
    if (coinstip_size == m_coinstip_cache_size_bytes &&
            coinsdb_size == m_coinsdb_cache_size_bytes) {
        // Cache sizes are unchanged, no need to continue.
        return true;
    }
    size_t old_coinstip_size = m_coinstip_cache_size_bytes;
    m_coinstip_cache_size_bytes = coinstip_size;
    m_coinsdb_cache_size_bytes = coinsdb_size;
    CoinsDB().ResizeCache(coinsdb_size);

    LogPrintf("[%s] resized coinsdb cache to %.1f MiB\n",
        this->ToString(), coinsdb_size * (1.0 / 1024 / 1024));
    LogPrintf("[%s] resized coinstip cache to %.1f MiB\n",
        this->ToString(), coinstip_size * (1.0 / 1024 / 1024));

    BlockValidationState state;
    bool ret;

    if (coinstip_size > old_coinstip_size) {
        // Likely no need to flush if cache sizes have grown.
        ret = FlushStateToDisk(state, FlushStateMode::IF_NEEDED);
    } else {
        // Otherwise, flush state to disk and deallocate the in-memory coins map.
        ret = FlushStateToDisk(state, FlushStateMode::ALWAYS);
    }
    return ret;
}

double ChainstateManager::GuessVerificationProgress(const CBlockIndex* pindex) const
{
    AssertLockHeld(GetMutex());
    const ChainTxData& data{GetParams().TxData()};
    if (pindex == nullptr) {
        return 0.0;
    }

    if (pindex->m_chain_tx_count == 0) {
        LogDebug(BCLog::VALIDATION, "Block %d has unset m_chain_tx_count. Unable to estimate verification progress.\n", pindex->nHeight);
        return 0.0;
    }

    const int64_t nNow{TicksSinceEpoch<std::chrono::seconds>(NodeClock::now())};
    const auto block_time{
        (Assume(m_best_header) && std::abs(nNow - pindex->GetBlockTime()) <= Ticks<std::chrono::seconds>(2h) &&
         Assume(m_best_header->nHeight >= pindex->nHeight)) ?
            // When the header is known to be recent, switch to a height-based
            // approach. This ensures the returned value is quantized when
            // close to "1.0", because some users expect it to be. This also
            // avoids relying too much on the exact miner-set timestamp, which
            // may be off.
            nNow - (m_best_header->nHeight - pindex->nHeight) * GetConsensus().nPowTargetSpacing :
            pindex->GetBlockTime(),
    };

    double fTxTotal;

    if (pindex->m_chain_tx_count <= data.tx_count) {
        fTxTotal = data.tx_count + (nNow - data.nTime) * data.dTxRate;
    } else {
        fTxTotal = pindex->m_chain_tx_count + (nNow - block_time) * data.dTxRate;
    }

    return std::min<double>(pindex->m_chain_tx_count / fTxTotal, 1.0);
}

std::optional<uint256> ChainstateManager::SnapshotBlockhash() const
{
    LOCK(::cs_main);
    if (m_active_chainstate && m_active_chainstate->m_from_snapshot_blockhash) {
        // If a snapshot chainstate exists, it will always be our active.
        return m_active_chainstate->m_from_snapshot_blockhash;
    }
    return std::nullopt;
}

std::vector<Chainstate*> ChainstateManager::GetAll()
{
    LOCK(::cs_main);
    std::vector<Chainstate*> out;

    for (Chainstate* cs : {m_ibd_chainstate.get(), m_snapshot_chainstate.get()}) {
        if (this->IsUsable(cs)) out.push_back(cs);
    }

    return out;
}

Chainstate& ChainstateManager::InitializeChainstate(CTxMemPool* mempool)
{
    AssertLockHeld(::cs_main);
    assert(!m_ibd_chainstate);
    assert(!m_active_chainstate);

    m_ibd_chainstate = std::make_unique<Chainstate>(mempool, m_blockman, *this);
    m_active_chainstate = m_ibd_chainstate.get();
    return *m_active_chainstate;
}

[[nodiscard]] static bool DeleteCoinsDBFromDisk(const fs::path db_path, bool is_snapshot)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    AssertLockHeld(::cs_main);

    if (is_snapshot) {
        fs::path base_blockhash_path = db_path / node::SNAPSHOT_BLOCKHASH_FILENAME;

        try {
            bool existed = fs::remove(base_blockhash_path);
            if (!existed) {
                LogPrintf("[snapshot] snapshot chainstate dir being removed lacks %s file\n",
                          fs::PathToString(node::SNAPSHOT_BLOCKHASH_FILENAME));
            }
        } catch (const fs::filesystem_error& e) {
            LogWarning("[snapshot] failed to remove file %s: %s\n",
                       fs::PathToString(base_blockhash_path), e.code().message());
        }
    }

    std::string path_str = fs::PathToString(db_path);
    LogPrintf("Removing leveldb dir at %s\n", path_str);

    // We have to destruct before this call leveldb::DB in order to release the db
    // lock, otherwise `DestroyDB` will fail. See `leveldb::~DBImpl()`.
    const bool destroyed = DestroyDB(path_str);

    if (!destroyed) {
        LogPrintf("error: leveldb DestroyDB call failed on %s\n", path_str);
    }

    // Datadir should be removed from filesystem; otherwise initialization may detect
    // it on subsequent statups and get confused.
    //
    // If the base_blockhash_path removal above fails in the case of snapshot
    // chainstates, this will return false since leveldb won't remove a non-empty
    // directory.
    return destroyed && !fs::exists(db_path);
}

util::Result<CBlockIndex*> ChainstateManager::ActivateSnapshot(
        AutoFile& coins_file,
        const SnapshotMetadata& metadata,
        bool in_memory)
{
    uint256 base_blockhash = metadata.m_base_blockhash;

    if (this->SnapshotBlockhash()) {
        return util::Error{Untranslated("Can't activate a snapshot-based chainstate more than once")};
    }

    CBlockIndex* snapshot_start_block{};

    {
        LOCK(::cs_main);

        if (!GetParams().AssumeutxoForBlockhash(base_blockhash).has_value()) {
            auto available_heights = GetParams().GetAvailableSnapshotHeights();
            std::string heights_formatted = util::Join(available_heights, ", ", [&](const auto& i) { return util::ToString(i); });
            return util::Error{Untranslated(strprintf("assumeutxo block hash in snapshot metadata not recognized (hash: %s). The following snapshot heights are available: %s",
                base_blockhash.ToString(),
                heights_formatted))};
        }

        snapshot_start_block = m_blockman.LookupBlockIndex(base_blockhash);
        if (!snapshot_start_block) {
            return util::Error{Untranslated(strprintf("The base block header (%s) must appear in the headers chain. Make sure all headers are syncing, and call loadtxoutset again",
                          base_blockhash.ToString()))};
        }

        bool start_block_invalid = snapshot_start_block->nStatus & BLOCK_FAILED_MASK;
        if (start_block_invalid) {
            return util::Error{Untranslated(strprintf("The base block header (%s) is part of an invalid chain", base_blockhash.ToString()))};
        }

        if (!m_best_header || m_best_header->GetAncestor(snapshot_start_block->nHeight) != snapshot_start_block) {
            return util::Error{Untranslated("A forked headers-chain with more work than the chain with the snapshot base block header exists. Please proceed to sync without AssumeUtxo.")};
        }

        auto mempool{m_active_chainstate->GetMempool()};
        if (mempool && mempool->size() > 0) {
            return util::Error{Untranslated("Can't activate a snapshot when mempool not empty")};
        }
    }

    int64_t current_coinsdb_cache_size{0};
    int64_t current_coinstip_cache_size{0};

    // Cache percentages to allocate to each chainstate.
    //
    // These particular percentages don't matter so much since they will only be
    // relevant during snapshot activation; caches are rebalanced at the conclusion of
    // this function. We want to give (essentially) all available cache capacity to the
    // snapshot to aid the bulk load later in this function.
    static constexpr double IBD_CACHE_PERC = 0.01;
    static constexpr double SNAPSHOT_CACHE_PERC = 0.99;

    {
        LOCK(::cs_main);
        // Resize the coins caches to ensure we're not exceeding memory limits.
        //
        // Allocate the majority of the cache to the incoming snapshot chainstate, since
        // (optimistically) getting to its tip will be the top priority. We'll need to call
        // `MaybeRebalanceCaches()` once we're done with this function to ensure
        // the right allocation (including the possibility that no snapshot was activated
        // and that we should restore the active chainstate caches to their original size).
        //
        current_coinsdb_cache_size = this->ActiveChainstate().m_coinsdb_cache_size_bytes;
        current_coinstip_cache_size = this->ActiveChainstate().m_coinstip_cache_size_bytes;

        // Temporarily resize the active coins cache to make room for the newly-created
        // snapshot chain.
        this->ActiveChainstate().ResizeCoinsCaches(
            static_cast<size_t>(current_coinstip_cache_size * IBD_CACHE_PERC),
            static_cast<size_t>(current_coinsdb_cache_size * IBD_CACHE_PERC));
    }

    auto snapshot_chainstate = WITH_LOCK(::cs_main,
        return std::make_unique<Chainstate>(
            /*mempool=*/nullptr, m_blockman, *this, base_blockhash));

    {
        LOCK(::cs_main);
        snapshot_chainstate->InitCoinsDB(
            static_cast<size_t>(current_coinsdb_cache_size * SNAPSHOT_CACHE_PERC),
            in_memory, false, "chainstate");
        snapshot_chainstate->InitCoinsCache(
            static_cast<size_t>(current_coinstip_cache_size * SNAPSHOT_CACHE_PERC));
    }

    auto cleanup_bad_snapshot = [&](bilingual_str reason) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        this->MaybeRebalanceCaches();

        // PopulateAndValidateSnapshot can return (in error) before the leveldb datadir
        // has been created, so only attempt removal if we got that far.
        if (auto snapshot_datadir = node::FindSnapshotChainstateDir(m_options.datadir)) {
            // We have to destruct leveldb::DB in order to release the db lock, otherwise
            // DestroyDB() (in DeleteCoinsDBFromDisk()) will fail. See `leveldb::~DBImpl()`.
            // Destructing the chainstate (and so resetting the coinsviews object) does this.
            snapshot_chainstate.reset();
            bool removed = DeleteCoinsDBFromDisk(*snapshot_datadir, /*is_snapshot=*/true);
            if (!removed) {
                GetNotifications().fatalError(strprintf(_("Failed to remove snapshot chainstate dir (%s). "
                    "Manually remove it before restarting.\n"), fs::PathToString(*snapshot_datadir)));
            }
        }
        return util::Error{std::move(reason)};
    };

    if (auto res{this->PopulateAndValidateSnapshot(*snapshot_chainstate, coins_file, metadata)}; !res) {
        LOCK(::cs_main);
        return cleanup_bad_snapshot(Untranslated(strprintf("Population failed: %s", util::ErrorString(res).original)));
    }

    LOCK(::cs_main);  // cs_main required for rest of snapshot activation.

    // Do a final check to ensure that the snapshot chainstate is actually a more
    // work chain than the active chainstate; a user could have loaded a snapshot
    // very late in the IBD process, and we wouldn't want to load a useless chainstate.
    if (!CBlockIndexWorkComparator()(ActiveTip(), snapshot_chainstate->m_chain.Tip())) {
        return cleanup_bad_snapshot(Untranslated("work does not exceed active chainstate"));
    }
    // If not in-memory, persist the base blockhash for use during subsequent
    // initialization.
    if (!in_memory) {
        if (!node::WriteSnapshotBaseBlockhash(*snapshot_chainstate)) {
            return cleanup_bad_snapshot(Untranslated("could not write base blockhash"));
        }
    }

    assert(!m_snapshot_chainstate);
    m_snapshot_chainstate.swap(snapshot_chainstate);
    const bool chaintip_loaded = m_snapshot_chainstate->LoadChainTip();
    assert(chaintip_loaded);

    // Transfer possession of the mempool to the snapshot chainstate.
    // Mempool is empty at this point because we're still in IBD.
    Assert(m_active_chainstate->m_mempool->size() == 0);
    Assert(!m_snapshot_chainstate->m_mempool);
    m_snapshot_chainstate->m_mempool = m_active_chainstate->m_mempool;
    m_active_chainstate->m_mempool = nullptr;
    m_active_chainstate = m_snapshot_chainstate.get();
    m_blockman.m_snapshot_height = this->GetSnapshotBaseHeight();

    LogPrintf("[snapshot] successfully activated snapshot %s\n", base_blockhash.ToString());
    LogPrintf("[snapshot] (%.2f MB)\n",
        m_snapshot_chainstate->CoinsTip().DynamicMemoryUsage() / (1000 * 1000));

    this->MaybeRebalanceCaches();
    return snapshot_start_block;
}

static void FlushSnapshotToDisk(CCoinsViewCache& coins_cache, bool snapshot_loaded)
{
    LOG_TIME_MILLIS_WITH_CATEGORY_MSG_ONCE(
        strprintf("%s (%.2f MB)",
                  snapshot_loaded ? "saving snapshot chainstate" : "flushing coins cache",
                  coins_cache.DynamicMemoryUsage() / (1000 * 1000)),
        BCLog::LogFlags::ALL);

    coins_cache.Flush();
}

struct StopHashingException : public std::exception
{
    const char* what() const noexcept override
    {
        return "ComputeUTXOStats interrupted.";
    }
};

static void SnapshotUTXOHashBreakpoint(const util::SignalInterrupt& interrupt)
{
    if (interrupt) throw StopHashingException();
}

util::Result<void> ChainstateManager::PopulateAndValidateSnapshot(
    Chainstate& snapshot_chainstate,
    AutoFile& coins_file,
    const SnapshotMetadata& metadata)
{
    // It's okay to release cs_main before we're done using `coins_cache` because we know
    // that nothing else will be referencing the newly created snapshot_chainstate yet.
    CCoinsViewCache& coins_cache = *WITH_LOCK(::cs_main, return &snapshot_chainstate.CoinsTip());

    uint256 base_blockhash = metadata.m_base_blockhash;

    CBlockIndex* snapshot_start_block = WITH_LOCK(::cs_main, return m_blockman.LookupBlockIndex(base_blockhash));

    if (!snapshot_start_block) {
        // Needed for ComputeUTXOStats to determine the
        // height and to avoid a crash when base_blockhash.IsNull()
        return util::Error{Untranslated(strprintf("Did not find snapshot start blockheader %s",
                  base_blockhash.ToString()))};
    }

    int base_height = snapshot_start_block->nHeight;
    const auto& maybe_au_data = GetParams().AssumeutxoForHeight(base_height);

    if (!maybe_au_data) {
        return util::Error{Untranslated(strprintf("Assumeutxo height in snapshot metadata not recognized "
                  "(%d) - refusing to load snapshot", base_height))};
    }

    const AssumeutxoData& au_data = *maybe_au_data;

    // This work comparison is a duplicate check with the one performed later in
    // ActivateSnapshot(), but is done so that we avoid doing the long work of staging
    // a snapshot that isn't actually usable.
    if (WITH_LOCK(::cs_main, return !CBlockIndexWorkComparator()(ActiveTip(), snapshot_start_block))) {
        return util::Error{Untranslated("Work does not exceed active chainstate")};
    }

    const uint64_t coins_count = metadata.m_coins_count;
    uint64_t coins_left = metadata.m_coins_count;

    LogPrintf("[snapshot] loading %d coins from snapshot %s\n", coins_left, base_blockhash.ToString());
    int64_t coins_processed{0};

    while (coins_left > 0) {
        try {
            Txid txid;
            coins_file >> txid;
            size_t coins_per_txid{0};
            coins_per_txid = ReadCompactSize(coins_file);

            if (coins_per_txid > coins_left) {
                return util::Error{Untranslated("Mismatch in coins count in snapshot metadata and actual snapshot data")};
            }

            for (size_t i = 0; i < coins_per_txid; i++) {
                COutPoint outpoint;
                Coin coin;
                outpoint.n = static_cast<uint32_t>(ReadCompactSize(coins_file));
                outpoint.hash = txid;
                coins_file >> coin;
                if (coin.nHeight > base_height ||
                    outpoint.n >= std::numeric_limits<decltype(outpoint.n)>::max() // Avoid integer wrap-around in coinstats.cpp:ApplyHash
                ) {
                    return util::Error{Untranslated(strprintf("Bad snapshot data after deserializing %d coins",
                              coins_count - coins_left))};
                }
                if (!MoneyRange(coin.out.nValue)) {
                    return util::Error{Untranslated(strprintf("Bad snapshot data after deserializing %d coins - bad tx out value",
                              coins_count - coins_left))};
                }
                coins_cache.EmplaceCoinInternalDANGER(std::move(outpoint), std::move(coin));

                --coins_left;
                ++coins_processed;

                if (coins_processed % 1000000 == 0) {
                    LogPrintf("[snapshot] %d coins loaded (%.2f%%, %.2f MB)\n",
                        coins_processed,
                        static_cast<float>(coins_processed) * 100 / static_cast<float>(coins_count),
                        coins_cache.DynamicMemoryUsage() / (1000 * 1000));
                }

                // Batch write and flush (if we need to) every so often.
                //
                // If our average Coin size is roughly 41 bytes, checking every 120,000 coins
                // means <5MB of memory imprecision.
                if (coins_processed % 120000 == 0) {
                    if (m_interrupt) {
                        return util::Error{Untranslated("Aborting after an interrupt was requested")};
                    }

                    const auto snapshot_cache_state = WITH_LOCK(::cs_main,
                        return snapshot_chainstate.GetCoinsCacheSizeState());

                    if (snapshot_cache_state >= CoinsCacheSizeState::CRITICAL) {
                        // This is a hack - we don't know what the actual best block is, but that
                        // doesn't matter for the purposes of flushing the cache here. We'll set this
                        // to its correct value (`base_blockhash`) below after the coins are loaded.
                        coins_cache.SetBestBlock(GetRandHash());

                        // No need to acquire cs_main since this chainstate isn't being used yet.
                        FlushSnapshotToDisk(coins_cache, /*snapshot_loaded=*/false);
                    }
                }
            }
        } catch (const std::ios_base::failure&) {
            return util::Error{Untranslated(strprintf("Bad snapshot format or truncated snapshot after deserializing %d coins",
                      coins_processed))};
        }
    }

    // Important that we set this. This and the coins_cache accesses above are
    // sort of a layer violation, but either we reach into the innards of
    // CCoinsViewCache here or we have to invert some of the Chainstate to
    // embed them in a snapshot-activation-specific CCoinsViewCache bulk load
    // method.
    coins_cache.SetBestBlock(base_blockhash);

    bool out_of_coins{false};
    try {
        std::byte left_over_byte;
        coins_file >> left_over_byte;
    } catch (const std::ios_base::failure&) {
        // We expect an exception since we should be out of coins.
        out_of_coins = true;
    }
    if (!out_of_coins) {
        return util::Error{Untranslated(strprintf("Bad snapshot - coins left over after deserializing %d coins",
            coins_count))};
    }

    LogPrintf("[snapshot] loaded %d (%.2f MB) coins from snapshot %s\n",
        coins_count,
        coins_cache.DynamicMemoryUsage() / (1000 * 1000),
        base_blockhash.ToString());

    // No need to acquire cs_main since this chainstate isn't being used yet.
    FlushSnapshotToDisk(coins_cache, /*snapshot_loaded=*/true);

    assert(coins_cache.GetBestBlock() == base_blockhash);

    // As above, okay to immediately release cs_main here since no other context knows
    // about the snapshot_chainstate.
    CCoinsViewDB* snapshot_coinsdb = WITH_LOCK(::cs_main, return &snapshot_chainstate.CoinsDB());

    std::optional<CCoinsStats> maybe_stats;

    try {
        maybe_stats = ComputeUTXOStats(
            CoinStatsHashType::HASH_SERIALIZED, snapshot_coinsdb, m_blockman, [&interrupt = m_interrupt] { SnapshotUTXOHashBreakpoint(interrupt); });
    } catch (StopHashingException const&) {
        return util::Error{Untranslated("Aborting after an interrupt was requested")};
    }
    if (!maybe_stats.has_value()) {
        return util::Error{Untranslated("Failed to generate coins stats")};
    }

    // Assert that the deserialized chainstate contents match the expected assumeutxo value.
    if (AssumeutxoHash{maybe_stats->hashSerialized} != au_data.hash_serialized) {
        return util::Error{Untranslated(strprintf("Bad snapshot content hash: expected %s, got %s",
            au_data.hash_serialized.ToString(), maybe_stats->hashSerialized.ToString()))};
    }

    snapshot_chainstate.m_chain.SetTip(*snapshot_start_block);

    // The remainder of this function requires modifying data protected by cs_main.
    LOCK(::cs_main);

    // Fake various pieces of CBlockIndex state:
    CBlockIndex* index = nullptr;

    // Don't make any modifications to the genesis block since it shouldn't be
    // necessary, and since the genesis block doesn't have normal flags like
    // BLOCK_VALID_SCRIPTS set.
    constexpr int AFTER_GENESIS_START{1};

    for (int i = AFTER_GENESIS_START; i <= snapshot_chainstate.m_chain.Height(); ++i) {
        index = snapshot_chainstate.m_chain[i];

        // Fake BLOCK_OPT_WITNESS so that Chainstate::NeedsRedownload()
        // won't ask for -reindex on startup.
        if (DeploymentActiveAt(*index, *this, Consensus::DEPLOYMENT_SEGWIT)) {
            index->nStatus |= BLOCK_OPT_WITNESS;
        }

        m_blockman.m_dirty_blockindex.insert(index);
        // Changes to the block index will be flushed to disk after this call
        // returns in `ActivateSnapshot()`, when `MaybeRebalanceCaches()` is
        // called, since we've added a snapshot chainstate and therefore will
        // have to downsize the IBD chainstate, which will result in a call to
        // `FlushStateToDisk(ALWAYS)`.
    }

    assert(index);
    assert(index == snapshot_start_block);
    index->m_chain_tx_count = au_data.m_chain_tx_count;
    snapshot_chainstate.setBlockIndexCandidates.insert(snapshot_start_block);

    LogPrintf("[snapshot] validated snapshot (%.2f MB)\n",
        coins_cache.DynamicMemoryUsage() / (1000 * 1000));
    return {};
}

// Currently, this function holds cs_main for its duration, which could be for
// multiple minutes due to the ComputeUTXOStats call. This hold is necessary
// because we need to avoid advancing the background validation chainstate
// farther than the snapshot base block - and this function is also invoked
// from within ConnectTip, i.e. from within ActivateBestChain, so cs_main is
// held anyway.
//
// Eventually (TODO), we could somehow separate this function's runtime from
// maintenance of the active chain, but that will either require
//
//  (i) setting `m_disabled` immediately and ensuring all chainstate accesses go
//      through IsUsable() checks, or
//
//  (ii) giving each chainstate its own lock instead of using cs_main for everything.
SnapshotCompletionResult ChainstateManager::MaybeCompleteSnapshotValidation()
{
    AssertLockHeld(cs_main);
    if (m_ibd_chainstate.get() == &this->ActiveChainstate() ||
            !this->IsUsable(m_snapshot_chainstate.get()) ||
            !this->IsUsable(m_ibd_chainstate.get()) ||
            !m_ibd_chainstate->m_chain.Tip()) {
       // Nothing to do - this function only applies to the background
       // validation chainstate.
       return SnapshotCompletionResult::SKIPPED;
    }
    const int snapshot_tip_height = this->ActiveHeight();
    const int snapshot_base_height = *Assert(this->GetSnapshotBaseHeight());
    const CBlockIndex& index_new = *Assert(m_ibd_chainstate->m_chain.Tip());

    if (index_new.nHeight < snapshot_base_height) {
        // Background IBD not complete yet.
        return SnapshotCompletionResult::SKIPPED;
    }

    assert(SnapshotBlockhash());
    uint256 snapshot_blockhash = *Assert(SnapshotBlockhash());

    auto handle_invalid_snapshot = [&]() EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        bilingual_str user_error = strprintf(_(
            "%s failed to validate the -assumeutxo snapshot state. "
            "This indicates a hardware problem, or a bug in the software, or a "
            "bad software modification that allowed an invalid snapshot to be "
            "loaded. As a result of this, the node will shut down and stop using any "
            "state that was built on the snapshot, resetting the chain height "
            "from %d to %d. On the next "
            "restart, the node will resume syncing from %d "
            "without using any snapshot data. "
            "Please report this incident to %s, including how you obtained the snapshot. "
            "The invalid snapshot chainstate will be left on disk in case it is "
            "helpful in diagnosing the issue that caused this error."),
            CLIENT_NAME, snapshot_tip_height, snapshot_base_height, snapshot_base_height, CLIENT_BUGREPORT
        );

        LogError("[snapshot] !!! %s\n", user_error.original);
        LogError("[snapshot] deleting snapshot, reverting to validated chain, and stopping node\n");

        m_active_chainstate = m_ibd_chainstate.get();
        m_snapshot_chainstate->m_disabled = true;
        assert(!this->IsUsable(m_snapshot_chainstate.get()));
        assert(this->IsUsable(m_ibd_chainstate.get()));

        auto rename_result = m_snapshot_chainstate->InvalidateCoinsDBOnDisk();
        if (!rename_result) {
            user_error += Untranslated("\n") + util::ErrorString(rename_result);
        }

        GetNotifications().fatalError(user_error);
    };

    if (index_new.GetBlockHash() != snapshot_blockhash) {
        LogPrintf("[snapshot] supposed base block %s does not match the "
          "snapshot base block %s (height %d). Snapshot is not valid.\n",
          index_new.ToString(), snapshot_blockhash.ToString(), snapshot_base_height);
        handle_invalid_snapshot();
        return SnapshotCompletionResult::BASE_BLOCKHASH_MISMATCH;
    }

    assert(index_new.nHeight == snapshot_base_height);

    int curr_height = m_ibd_chainstate->m_chain.Height();

    assert(snapshot_base_height == curr_height);
    assert(snapshot_base_height == index_new.nHeight);
    assert(this->IsUsable(m_snapshot_chainstate.get()));
    assert(this->GetAll().size() == 2);

    CCoinsViewDB& ibd_coins_db = m_ibd_chainstate->CoinsDB();
    m_ibd_chainstate->ForceFlushStateToDisk();

    const auto& maybe_au_data = m_options.chainparams.AssumeutxoForHeight(curr_height);
    if (!maybe_au_data) {
        LogPrintf("[snapshot] assumeutxo data not found for height "
            "(%d) - refusing to validate snapshot\n", curr_height);
        handle_invalid_snapshot();
        return SnapshotCompletionResult::MISSING_CHAINPARAMS;
    }

    const AssumeutxoData& au_data = *maybe_au_data;
    std::optional<CCoinsStats> maybe_ibd_stats;
    LogPrintf("[snapshot] computing UTXO stats for background chainstate to validate "
        "snapshot - this could take a few minutes\n");
    try {
        maybe_ibd_stats = ComputeUTXOStats(
            CoinStatsHashType::HASH_SERIALIZED,
            &ibd_coins_db,
            m_blockman,
            [&interrupt = m_interrupt] { SnapshotUTXOHashBreakpoint(interrupt); });
    } catch (StopHashingException const&) {
        return SnapshotCompletionResult::STATS_FAILED;
    }

    // XXX note that this function is slow and will hold cs_main for potentially minutes.
    if (!maybe_ibd_stats) {
        LogPrintf("[snapshot] failed to generate stats for validation coins db\n");
        // While this isn't a problem with the snapshot per se, this condition
        // prevents us from validating the snapshot, so we should shut down and let the
        // user handle the issue manually.
        handle_invalid_snapshot();
        return SnapshotCompletionResult::STATS_FAILED;
    }
    const auto& ibd_stats = *maybe_ibd_stats;

    // Compare the background validation chainstate's UTXO set hash against the hard-coded
    // assumeutxo hash we expect.
    //
    // TODO: For belt-and-suspenders, we could cache the UTXO set
    // hash for the snapshot when it's loaded in its chainstate's leveldb. We could then
    // reference that here for an additional check.
    if (AssumeutxoHash{ibd_stats.hashSerialized} != au_data.hash_serialized) {
        LogPrintf("[snapshot] hash mismatch: actual=%s, expected=%s\n",
            ibd_stats.hashSerialized.ToString(),
            au_data.hash_serialized.ToString());
        handle_invalid_snapshot();
        return SnapshotCompletionResult::HASH_MISMATCH;
    }

    LogPrintf("[snapshot] snapshot beginning at %s has been fully validated\n",
        snapshot_blockhash.ToString());

    m_ibd_chainstate->m_disabled = true;
    this->MaybeRebalanceCaches();

    return SnapshotCompletionResult::SUCCESS;
}

Chainstate& ChainstateManager::ActiveChainstate() const
{
    LOCK(::cs_main);
    assert(m_active_chainstate);
    return *m_active_chainstate;
}

bool ChainstateManager::IsSnapshotActive() const
{
    LOCK(::cs_main);
    return m_snapshot_chainstate && m_active_chainstate == m_snapshot_chainstate.get();
}

void ChainstateManager::MaybeRebalanceCaches()
{
    AssertLockHeld(::cs_main);
    bool ibd_usable = this->IsUsable(m_ibd_chainstate.get());
    bool snapshot_usable = this->IsUsable(m_snapshot_chainstate.get());
    assert(ibd_usable || snapshot_usable);

    if (ibd_usable && !snapshot_usable) {
        // Allocate everything to the IBD chainstate. This will always happen
        // when we are not using a snapshot.
        m_ibd_chainstate->ResizeCoinsCaches(m_total_coinstip_cache, m_total_coinsdb_cache);
    }
    else if (snapshot_usable && !ibd_usable) {
        // If background validation has completed and snapshot is our active chain...
        LogPrintf("[snapshot] allocating all cache to the snapshot chainstate\n");
        // Allocate everything to the snapshot chainstate.
        m_snapshot_chainstate->ResizeCoinsCaches(m_total_coinstip_cache, m_total_coinsdb_cache);
    }
    else if (ibd_usable && snapshot_usable) {
        // If both chainstates exist, determine who needs more cache based on IBD status.
        //
        // Note: shrink caches first so that we don't inadvertently overwhelm available memory.
        if (IsInitialBlockDownload()) {
            m_ibd_chainstate->ResizeCoinsCaches(
                m_total_coinstip_cache * 0.05, m_total_coinsdb_cache * 0.05);
            m_snapshot_chainstate->ResizeCoinsCaches(
                m_total_coinstip_cache * 0.95, m_total_coinsdb_cache * 0.95);
        } else {
            m_snapshot_chainstate->ResizeCoinsCaches(
                m_total_coinstip_cache * 0.05, m_total_coinsdb_cache * 0.05);
            m_ibd_chainstate->ResizeCoinsCaches(
                m_total_coinstip_cache * 0.95, m_total_coinsdb_cache * 0.95);
        }
    }
}

void ChainstateManager::ResetChainstates()
{
    m_ibd_chainstate.reset();
    m_snapshot_chainstate.reset();
    m_active_chainstate = nullptr;
}

/**
 * Apply default chain params to nullopt members.
 * This helps to avoid coding errors around the accidental use of the compare
 * operators that accept nullopt, thus ignoring the intended default value.
 */
static ChainstateManager::Options&& Flatten(ChainstateManager::Options&& opts)
{
    if (!opts.check_block_index.has_value()) opts.check_block_index = opts.chainparams.DefaultConsistencyChecks();
    if (!opts.minimum_chain_work.has_value()) opts.minimum_chain_work = UintToArith256(opts.chainparams.GetConsensus().nMinimumChainWork);
    if (!opts.assumed_valid_block.has_value()) opts.assumed_valid_block = opts.chainparams.GetConsensus().defaultAssumeValid;
    return std::move(opts);
}

ChainstateManager::ChainstateManager(const util::SignalInterrupt& interrupt, Options options, node::BlockManager::Options blockman_options)
    : m_script_check_queue{/*batch_size=*/128, std::clamp(options.worker_threads_num, 0, MAX_SCRIPTCHECK_THREADS)},
      m_interrupt{interrupt},
      m_options{Flatten(std::move(options))},
      m_blockman{interrupt, std::move(blockman_options)},
      m_validation_cache{m_options.script_execution_cache_bytes, m_options.signature_cache_bytes}
{
}

ChainstateManager::~ChainstateManager()
{
    LOCK(::cs_main);

    m_versionbitscache.Clear();
}

bool ChainstateManager::DetectSnapshotChainstate()
{
    assert(!m_snapshot_chainstate);
    std::optional<fs::path> path = node::FindSnapshotChainstateDir(m_options.datadir);
    if (!path) {
        return false;
    }
    std::optional<uint256> base_blockhash = node::ReadSnapshotBaseBlockhash(*path);
    if (!base_blockhash) {
        return false;
    }
    LogPrintf("[snapshot] detected active snapshot chainstate (%s) - loading\n",
        fs::PathToString(*path));

    this->ActivateExistingSnapshot(*base_blockhash);
    return true;
}

Chainstate& ChainstateManager::ActivateExistingSnapshot(uint256 base_blockhash)
{
    assert(!m_snapshot_chainstate);
    m_snapshot_chainstate =
        std::make_unique<Chainstate>(nullptr, m_blockman, *this, base_blockhash);
    LogPrintf("[snapshot] switching active chainstate to %s\n", m_snapshot_chainstate->ToString());

    // Mempool is empty at this point because we're still in IBD.
    Assert(m_active_chainstate->m_mempool->size() == 0);
    Assert(!m_snapshot_chainstate->m_mempool);
    m_snapshot_chainstate->m_mempool = m_active_chainstate->m_mempool;
    m_active_chainstate->m_mempool = nullptr;
    m_active_chainstate = m_snapshot_chainstate.get();
    return *m_snapshot_chainstate;
}

bool IsBIP30Repeat(const CBlockIndex& block_index)
{
    return (block_index.nHeight==91842 && block_index.GetBlockHash() == uint256{"00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec"}) ||
           (block_index.nHeight==91880 && block_index.GetBlockHash() == uint256{"00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721"});
}

bool IsBIP30Unspendable(const CBlockIndex& block_index)
{
    return (block_index.nHeight==91722 && block_index.GetBlockHash() == uint256{"00000000000271a2dc26e7667f8419f2e15416dc6955e5a6c6cdf3f2574dd08e"}) ||
           (block_index.nHeight==91812 && block_index.GetBlockHash() == uint256{"00000000000af0aed4792b1acee3d966af36cf5def14935db8de83d6f9306f2f"});
}

static fs::path GetSnapshotCoinsDBPath(Chainstate& cs) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    AssertLockHeld(::cs_main);
    // Should never be called on a non-snapshot chainstate.
    assert(cs.m_from_snapshot_blockhash);
    auto storage_path_maybe = cs.CoinsDB().StoragePath();
    // Should never be called with a non-existent storage path.
    assert(storage_path_maybe);
    return *storage_path_maybe;
}

util::Result<void> Chainstate::InvalidateCoinsDBOnDisk()
{
    fs::path snapshot_datadir = GetSnapshotCoinsDBPath(*this);

    // Coins views no longer usable.
    m_coins_views.reset();

    auto invalid_path = snapshot_datadir + "_INVALID";
    std::string dbpath = fs::PathToString(snapshot_datadir);
    std::string target = fs::PathToString(invalid_path);
    LogPrintf("[snapshot] renaming snapshot datadir %s to %s\n", dbpath, target);

    // The invalid snapshot datadir is simply moved and not deleted because we may
    // want to do forensics later during issue investigation. The user is instructed
    // accordingly in MaybeCompleteSnapshotValidation().
    try {
        fs::rename(snapshot_datadir, invalid_path);
    } catch (const fs::filesystem_error& e) {
        auto src_str = fs::PathToString(snapshot_datadir);
        auto dest_str = fs::PathToString(invalid_path);

        LogPrintf("%s: error renaming file '%s' -> '%s': %s\n",
                __func__, src_str, dest_str, e.what());
        return util::Error{strprintf(_(
            "Rename of '%s' -> '%s' failed. "
            "You should resolve this by manually moving or deleting the invalid "
            "snapshot directory %s, otherwise you will encounter the same error again "
            "on the next startup."),
            src_str, dest_str, src_str)};
    }
    return {};
}

bool ChainstateManager::DeleteSnapshotChainstate()
{
    AssertLockHeld(::cs_main);
    Assert(m_snapshot_chainstate);
    Assert(m_ibd_chainstate);

    fs::path snapshot_datadir = Assert(node::FindSnapshotChainstateDir(m_options.datadir)).value();
    if (!DeleteCoinsDBFromDisk(snapshot_datadir, /*is_snapshot=*/ true)) {
        LogPrintf("Deletion of %s failed. Please remove it manually to continue reindexing.\n",
                  fs::PathToString(snapshot_datadir));
        return false;
    }
    m_active_chainstate = m_ibd_chainstate.get();
    m_active_chainstate->m_mempool = m_snapshot_chainstate->m_mempool;
    m_snapshot_chainstate.reset();
    return true;
}

ChainstateRole Chainstate::GetRole() const
{
    if (m_chainman.GetAll().size() <= 1) {
        return ChainstateRole::NORMAL;
    }
    return (this != &m_chainman.ActiveChainstate()) ?
               ChainstateRole::BACKGROUND :
               ChainstateRole::ASSUMEDVALID;
}

const CBlockIndex* ChainstateManager::GetSnapshotBaseBlock() const
{
    return m_active_chainstate ? m_active_chainstate->SnapshotBase() : nullptr;
}

std::optional<int> ChainstateManager::GetSnapshotBaseHeight() const
{
    const CBlockIndex* base = this->GetSnapshotBaseBlock();
    return base ? std::make_optional(base->nHeight) : std::nullopt;
}

void ChainstateManager::RecalculateBestHeader()
{
    AssertLockHeld(cs_main);
    m_best_header = ActiveChain().Tip();
    for (auto& entry : m_blockman.m_block_index) {
        if (!(entry.second.nStatus & BLOCK_FAILED_MASK) && m_best_header->nChainWork < entry.second.nChainWork) {
            m_best_header = &entry.second;
        }
    }
}

bool ChainstateManager::ValidatedSnapshotCleanup()
{
    AssertLockHeld(::cs_main);
    auto get_storage_path = [](auto& chainstate) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) -> std::optional<fs::path> {
        if (!(chainstate && chainstate->HasCoinsViews())) {
            return {};
        }
        return chainstate->CoinsDB().StoragePath();
    };
    std::optional<fs::path> ibd_chainstate_path_maybe = get_storage_path(m_ibd_chainstate);
    std::optional<fs::path> snapshot_chainstate_path_maybe = get_storage_path(m_snapshot_chainstate);

    if (!this->IsSnapshotValidated()) {
        // No need to clean up.
        return false;
    }
    // If either path doesn't exist, that means at least one of the chainstates
    // is in-memory, in which case we can't do on-disk cleanup. You'd better be
    // in a unittest!
    if (!ibd_chainstate_path_maybe || !snapshot_chainstate_path_maybe) {
        LogPrintf("[snapshot] snapshot chainstate cleanup cannot happen with "
                  "in-memory chainstates. You are testing, right?\n");
        return false;
    }

    const auto& snapshot_chainstate_path = *snapshot_chainstate_path_maybe;
    const auto& ibd_chainstate_path = *ibd_chainstate_path_maybe;

    // Since we're going to be moving around the underlying leveldb filesystem content
    // for each chainstate, make sure that the chainstates (and their constituent
    // CoinsViews members) have been destructed first.
    //
    // The caller of this method will be responsible for reinitializing chainstates
    // if they want to continue operation.
    this->ResetChainstates();

    // No chainstates should be considered usable.
    assert(this->GetAll().size() == 0);

    LogPrintf("[snapshot] deleting background chainstate directory (now unnecessary) (%s)\n",
              fs::PathToString(ibd_chainstate_path));

    fs::path tmp_old{ibd_chainstate_path + "_todelete"};

    auto rename_failed_abort = [this](
                                   fs::path p_old,
                                   fs::path p_new,
                                   const fs::filesystem_error& err) {
        LogError("[snapshot] Error renaming path (%s) -> (%s): %s\n",
                  fs::PathToString(p_old), fs::PathToString(p_new), err.what());
        GetNotifications().fatalError(strprintf(_(
            "Rename of '%s' -> '%s' failed. "
            "Cannot clean up the background chainstate leveldb directory."),
            fs::PathToString(p_old), fs::PathToString(p_new)));
    };

    try {
        fs::rename(ibd_chainstate_path, tmp_old);
    } catch (const fs::filesystem_error& e) {
        rename_failed_abort(ibd_chainstate_path, tmp_old, e);
        throw;
    }

    LogPrintf("[snapshot] moving snapshot chainstate (%s) to "
              "default chainstate directory (%s)\n",
              fs::PathToString(snapshot_chainstate_path), fs::PathToString(ibd_chainstate_path));

    try {
        fs::rename(snapshot_chainstate_path, ibd_chainstate_path);
    } catch (const fs::filesystem_error& e) {
        rename_failed_abort(snapshot_chainstate_path, ibd_chainstate_path, e);
        throw;
    }

    if (!DeleteCoinsDBFromDisk(tmp_old, /*is_snapshot=*/false)) {
        // No need to FatalError because once the unneeded bg chainstate data is
        // moved, it will not interfere with subsequent initialization.
        LogPrintf("Deletion of %s failed. Please remove it manually, as the "
                  "directory is now unnecessary.\n",
                  fs::PathToString(tmp_old));
    } else {
        LogPrintf("[snapshot] deleted background chainstate directory (%s)\n",
                  fs::PathToString(ibd_chainstate_path));
    }
    return true;
}

Chainstate& ChainstateManager::GetChainstateForIndexing()
{
    // We can't always return `m_ibd_chainstate` because after background validation
    // has completed, `m_snapshot_chainstate == m_active_chainstate`, but it can be
    // indexed.
    return (this->GetAll().size() > 1) ? *m_ibd_chainstate : *m_active_chainstate;
}

std::pair<int, int> ChainstateManager::GetPruneRange(const Chainstate& chainstate, int last_height_can_prune)
{
    if (chainstate.m_chain.Height() <= 0) {
        return {0, 0};
    }
    int prune_start{0};

    if (this->GetAll().size() > 1 && m_snapshot_chainstate.get() == &chainstate) {
        // Leave the blocks in the background IBD chain alone if we're pruning
        // the snapshot chain.
        prune_start = *Assert(GetSnapshotBaseHeight()) + 1;
    }

    int max_prune = std::max<int>(
        0, chainstate.m_chain.Height() - static_cast<int>(MIN_BLOCKS_TO_KEEP));

    // last block to prune is the lesser of (caller-specified height, MIN_BLOCKS_TO_KEEP from the tip)
    //
    // While you might be tempted to prune the background chainstate more
    // aggressively (i.e. fewer MIN_BLOCKS_TO_KEEP), this won't work with index
    // building - specifically blockfilterindex requires undo data, and if
    // we don't maintain this trailing window, we hit indexing failures.
    int prune_end = std::min(last_height_can_prune, max_prune);

    return {prune_start, prune_end};
}
