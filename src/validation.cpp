// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <pos/stake.h>
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
#include <kernel/mempool_priority.h>
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
#include <set>
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
/** Time window to wait between writing blocks/block index and chainstate to disk. */
static constexpr auto DATABASE_WRITE_INTERVAL_MIN{50min};
static constexpr auto DATABASE_WRITE_INTERVAL_MAX{70min};
/** Maximum age of our tip for us to be considered current for fee estimation */
static constexpr std::chrono::hours MAX_FEE_ESTIMATION_TIP_AGE{3};
const std::vector<std::string> CHECKLEVEL_DOC{
    "level 0 reads the blocks from disk",
    "level 1 verifies block validity",
    "level 2 verifies undo data",
    "level 3 checks disconnection of tip blocks",
    "level 4 tries to reconnect the blocks",
    "each level includes the checks of the previous levels",
};
/** The number of blocks to keep below the deepest prune lock. */
static constexpr int PRUNE_LOCK_BUFFER{10};

TRACEPOINT_SEMAPHORE(validation, block_connected);
TRACEPOINT_SEMAPHORE(utxocache, flush);
TRACEPOINT_SEMAPHORE(mempool, replaced);
TRACEPOINT_SEMAPHORE(mempool, rejected);

static bool CheckProofOfWork(const uint256& hash, unsigned int nBits, const Consensus::Params& params)
{
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits);
    if (bnTarget <= 0 || bnTarget > UintToArith256(params.powLimit)) return false;
    return UintToArith256(hash) <= bnTarget;
}

bool CheckBlock(const CBlock& block, BlockValidationState& state, const Consensus::Params& params, bool fCheckPOW, bool fCheckMerkleRoot)
{
    // Check block weight
    if (GetBlockWeight(block) > MAX_BLOCK_WEIGHT) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-blk-weight", "block weight limits exceeded");
    }

    if (block.vtx.empty()) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-missing", "no transactions");
    }

    // First transaction must be coinbase, the rest must not be
    if (!block.vtx[0]->IsCoinBase()) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-missing", "first tx is not coinbase");
    }
    for (size_t i = 1; i < block.vtx.size(); ++i) {
        if (block.vtx[i]->IsCoinBase()) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-multiple", "multiple coinbase");
        }
    }

    // Check for duplicate txids
    std::set<Txid> unique_txids;
    for (const auto& tx : block.vtx) {
        if (!unique_txids.insert(tx->GetHash()).second) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-txns-duplicate", "duplicate transaction");
        }
    }

    // Verify merkle root
    if (fCheckMerkleRoot) {
        if (block.hashMerkleRoot != BlockMerkleRoot(block)) {
            return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "bad-txnmrklroot", "hashMerkleRoot mismatch");
        }
    }

    // Proof checks
    if (IsProofOfStake(block)) {
        const CBlockIndex* pindexPrev{nullptr};
        CCoinsView dummy_view;
        CCoinsViewCache view(&dummy_view);
        CChain chain;
        if (pindexPrev && !ContextualCheckProofOfStake(block, pindexPrev, view, chain, params)) {
            return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "bad-pos", "proof of stake check failed");
        }
    } else if (fCheckPOW) {
        if (!CheckProofOfWork(block.GetHash(), block.nBits, params)) {
            return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "high-hash", "proof of work failed");
        }
    }

    return true;
}

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
            prev_heights[i] = coin->nHeight == MEMPOOL_HEIGHT ? tip.nHeight + 1 // Assume all mempool transaction confirm in the next block.
                                                                :
                                                                coin->nHeight;
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
                                   /*bypass_limits=*/true, /*test_accept=*/false)
                        .m_result_type !=
                    MempoolAcceptResult::ResultType::VALID) {
                // If the transaction doesn't make it in to the mempool, remove any
                // transactions that depend on it (which would now be orphans).
                m_mempool->removeRecursive(**it, MemPoolRemovalReason::REORG);
            } else if (m_mempool->exists((*it)->GetHash())) {
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
                                                         if (m_mempool->exists(txin.prevout.hash)) continue;
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
    explicit MemPoolAccept(CTxMemPool& mempool, Chainstate& active_chainstate) : m_pool(mempool),
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
                                     bool test_accept)
        {
            return ATMPArgs{
                /* m_chainparams */ chainparams,
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
                                          std::vector<COutPoint>& coins_to_uncache)
        {
            return ATMPArgs{
                /* m_chainparams */ chainparams,
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
                                                std::vector<COutPoint>& coins_to_uncache, const std::optional<CFeeRate>& client_maxfeerate)
        {
            return ATMPArgs{
                /* m_chainparams */ chainparams,
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
        static ATMPArgs SingleInPackageAccept(const ATMPArgs& package_args)
        {
            return ATMPArgs{
                /* m_chainparams */ package_args.m_chainparams,
                /* m_accept_time */ package_args.m_accept_time,
                /* m_bypass_limits */ false,
                /* m_coins_to_uncache */ package_args.m_coins_to_uncache,
                /* m_test_accept */ package_args.m_test_accept,
                /* m_allow_replacement */ true,
                /* m_allow_sibling_eviction */ true,
                /* m_package_submission */ true, // do not LimitMempoolSize in Finalize()
                /* m_package_feerates */ false,  // only 1 transaction
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

    if (m_pool.exists(tx.GetWitnessHash())) {
        // Exact transaction already exists in the mempool.
        return state.Invalid(TxValidationResult::TX_CONFLICT, "txn-already-in-mempool");
    } else if (m_pool.exists(tx.GetHash())) {
        // Transaction with the same non-witness data but different witness (same txid, different
        // wtxid) already exists in the mempool.
        return state.Invalid(TxValidationResult::TX_CONFLICT, "txn-same-nonwitness-data-in-mempool");
    }

    // Check for conflicts with in-memory transactions
    for (const CTxIn& txin : tx.vin) {
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
    for (const CTxIn& txin : tx.vin) {
        const Coin& coin = m_view.AccessCoin(txin.prevout);
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
    assert(std::all_of(txns.cbegin(), txns.cend(), [this](const auto& tx) { return !m_pool.exists(tx->GetHash()); }));

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
    for (CTxMemPool::txiter it : m_subpackage.m_changeset->GetRemovals()) {
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
                   replaced_with_tx);
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
    assert(std::all_of(workspaces.cbegin(), workspaces.cend(), [this](const auto& ws) { return !m_pool.exists(ws.m_ptx->GetHash()); }));

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

    // Calculate priority
    int64_t priority = 0;
    priority += CalculateStakePriority(ws.m_ptx->GetValueOut());
    priority += CalculateFeePriority(ws.m_base_fees);
    int64_t stake_duration = 0;
    for (const CTxIn& txin : ws.m_ptx->vin) {
        const Coin& coin = m_view.AccessCoin(txin.prevout);
        if (!coin.IsSpent()) {
            int n_blocks = m_active_chainstate.m_chain.Height() - coin.nHeight;
            if (n_blocks > 0) {
                stake_duration = std::max<int64_t>(stake_duration, n_blocks * args.m_chainparams.GetConsensus().nPowTargetSpacing);
            }
        }
    }
    priority += CalculateStakeDurationPriority(stake_duration);
    if (m_pool.DynamicMemoryUsage() > m_pool.m_opts.max_size_bytes * 9 / 10) {
        priority += CONGESTION_PENALTY;
    }
    const_cast<CTxMemPoolEntry&>(*ws.m_tx_handle).SetPriority(priority);

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
        if (!m_pool.exists(ws.m_hash)) {
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
        if (m_pool.exists(wtxid)) {
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
        } else if (m_pool.exists(txid)) {
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
                assert(m_pool.exists(wtxid));
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
            if (txresult.m_result_type == MempoolAcceptResult::ResultType::VALID && !m_pool.exists(wtxid)) {
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
            if (!m_pool.exists(tx->GetHash())) {
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

} // namespace

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
                   result.m_state.GetRejectReason().c_str());
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
    assert(std::all_of(package.cbegin(), package.cend(), [](const auto& tx) { return tx != nullptr; }));

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
    // Force block reward to zero when right shift is undefined or too large.
    if (halvings >= 64)
        return 0;

    CAmount nSubsidy = 50 * COIN; // BitGold: Start with 50 coins per block
    // Subsidy is cut in half every nSubsidyHalvingInterval blocks.
    nSubsidy >>= halvings;
    return nSubsidy;
}

/**
 * Iterate over the given headers and ensure each one satisfies the proof of
 * work or proof of stake target specified by its nBits field.
 */
bool HasValidProofOfWork(const std::vector<CBlockHeader>& headers, const Consensus::Params& consensusParams)
{
    for (const CBlockHeader& header : headers) {
        bool fNegative{false};
        bool fOverflow{false};
        arith_uint256 bnTarget;
        bnTarget.SetCompact(header.nBits, &fNegative, &fOverflow);

        // Range checks
        if (fNegative || fOverflow || bnTarget == 0 || bnTarget > UintToArith256(consensusParams.powLimit)) {
            return false;
        }

        // Check whether the block hash satisfies the claimed target. This is
        // valid for both proof-of-work and proof-of-stake blocks as both must
        // have hashes below the target encoded in nBits.
        if (UintToArith256(header.GetHash()) > bnTarget) {
            return false;
        }
    }
    return true;
}

arith_uint256 CalculateClaimedHeadersWork(std::span<const CBlockHeader> headers)
{
    arith_uint256 total_work{0};
    for (const CBlockHeader& header : headers) {
        total_work += GetBlockProof(CBlockIndex(header));
    }
    return total_work;
}

CoinsViews::CoinsViews(DBParams db_params, CoinsViewOptions options)
    : m_dbview{std::move(db_params), std::move(options)},
      m_catcherview(&m_dbview) {}

void CoinsViews::InitCache()
{
    AssertLockHeld(::cs_main);
    m_cacheview = std::make_unique<CCoinsViewCache>(&m_catcherview);
}

Chainstate& ChainstateManager::ActiveChainstate() const
{
    Assert(m_active_chainstate);
    return *m_active_chainstate;
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
        },
        CoinsViewOptions{});
}

void Chainstate::LoadDividendPool()
{
    m_dividend_pool = CoinsDB().GetDividendPool();
}

void Chainstate::AddToDividendPool(CAmount amount)
{
    m_dividend_pool += amount;
    CoinsDB().WriteDividendPool(m_dividend_pool);
    // TODO: Integrate quarterly payout distribution when available.
}

bool IsBlockMutated(const CBlock& block, bool check_witness_root)
{
    bool mutated{false};

    bool merkle_mutated{false};
    const uint256 merkle_root{BlockMerkleRoot(block, &merkle_mutated)};
    block.m_checked_merkle_root = true;
    if (merkle_mutated || merkle_root != block.hashMerkleRoot) {
        mutated = true;
    }

    const bool has_witness_tx = std::any_of(block.vtx.begin(), block.vtx.end(),
        [](const CTransactionRef& tx) { return tx->HasWitness(); });

    if (check_witness_root) {
        block.m_checked_witness_commitment = true;
        bool witness_mutated{false};
        const uint256 witness_root{BlockWitnessMerkleRoot(block, &witness_mutated)};
        if (witness_mutated) {
            mutated = true;
        } else if (has_witness_tx) {
            const int commitpos = GetWitnessCommitmentIndex(block);
            if (commitpos == NO_WITNESS_COMMITMENT) {
                mutated = true;
            } else {
                const CScript& script = block.vtx[0]->vout[commitpos].scriptPubKey;
                if (script.size() < 38) {
                    mutated = true;
                } else if (block.vtx[0]->vin.empty() ||
                           block.vtx[0]->vin[0].scriptWitness.stack.size() != 1 ||
                           block.vtx[0]->vin[0].scriptWitness.stack[0].size() != 32) {
                    mutated = true;
                } else {
                    unsigned char hash[32];
                    CSHA256()
                        .Write(witness_root.begin(), 32)
                        .Write(block.vtx[0]->vin[0].scriptWitness.stack[0].data(), 32)
                        .Finalize(hash);
                    if (!std::equal(hash, hash + 32, script.data() + 6)) {
                        mutated = true;
                    }
                }
            }
        }
    } else if (has_witness_tx) {
        mutated = true;
    }

    return mutated;
}
