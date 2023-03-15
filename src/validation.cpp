// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>

#include <kernel/coinstats.h>
#include <kernel/mempool_persist.h>

#include <arith_uint256.h>
#include <auxpow.h>
#include <chain.h>
#include <chainparams.h>
#include <checkqueue.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_check.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <cuckoocache.h>
#include <flatfile.h>
#include <fs.h>
#include <hash.h>
#include <kernel/mempool_entry.h>
#include <logging.h>
#include <logging/timer.h>
#include <node/blockstorage.h>
#include <node/interface_ui.h>
#include <node/utxo_snapshot.h>
#include <policy/policy.h>
#include <policy/rbf.h>
#include <policy/settings.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <reverse_iterator.h>
#include <script/script.h>
#include <script/sigcache.h>
#include <shutdown.h>
#include <signet.h>
#include <tinyformat.h>
#include <txdb.h>
#include <txmempool.h>
#include <uint256.h>
#include <undo.h>
#include <util/check.h> // For NDEBUG compile time check
#include <util/hasher.h>
#include <util/moneystr.h>
#include <util/rbf.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/time.h>
#include <util/trace.h>
#include <util/translation.h>
#include <validationinterface.h>
#include <warnings.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <deque>
#include <numeric>
#include <optional>
#include <string>
#include <utility>
// SYSCOIN
#include <masternode/masternodepayments.h>
#include <evo/specialtx.h>
#include <llmq/quorums_chainlocks.h>
#include <services/assetconsensus.h>
#include <fstream>
#include <cachemultimap.h>
#include <nevm/sha3.h>
#ifndef WIN32
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#endif
// SYSCOIN
#if ENABLE_ZMQ
#include <zmq/zmqabstractnotifier.h>
#include <zmq/zmqnotificationinterface.h>
#include <zmq/zmqrpc.h>
#endif
#ifdef MAC_OSX
    #include <mach-o/dyld.h>
    #include <limits.h>
#endif
pid_t gethpid = -1;
#ifdef WIN32
HANDLE hProcessGeth = NULL;
#endif
RecursiveMutex cs_geth;
NEVMMintTxMap mapMintKeysMempool;
std::unordered_map<COutPoint, std::pair<CTransactionRef, CTransactionRef>, SaltedOutpointHasher> mapAssetAllocationConflicts;
std::vector<CInv> vInvToSend;
std::map<uint256, int64_t> mapRejectedBlocks GUARDED_BY(cs_main);
bool fTestSetting{false};
using kernel::CCoinsStats;
using kernel::CoinStatsHashType;
using kernel::ComputeUTXOStats;
using kernel::LoadMempool;

using fsbridge::FopenFn;
using node::BlockManager;
using node::BlockMap;
using node::CBlockIndexHeightOnlyComparator;
using node::CBlockIndexWorkComparator;
using node::fReindex;
using node::ReadBlockFromDisk;
using node::SnapshotMetadata;
using node::UndoReadFromDisk;
using node::UnlinkPrunedFiles;

/** Maximum kilobytes for transactions to store for processing during reorg */
static const unsigned int MAX_DISCONNECTED_TX_POOL_SIZE = 20000;
/** Time to wait between writing blocks/block index to disk. */
static constexpr std::chrono::hours DATABASE_WRITE_INTERVAL{1};
/** Time to wait between flushing chainstate to disk. */
static constexpr std::chrono::hours DATABASE_FLUSH_INTERVAL{24};
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

GlobalMutex g_best_block_mutex;
std::condition_variable g_best_block_cv;
uint256 g_best_block;
// SYSCOIN
std::atomic_bool fReindexGeth(false);
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
        const CTxIn& txin = tx.vin[i];
        Coin coin;
        if (!coins.GetCoin(txin.prevout, coin)) {
            LogPrintf("ERROR: %s: Missing input %d in transaction \'%s\'\n", __func__, i, tx.GetHash().GetHex());
            return std::nullopt;
        }
        if (coin.nHeight == MEMPOOL_HEIGHT) {
            // Assume all mempool transaction confirm in the next block.
            prev_heights[i] = tip.nHeight + 1;
        } else {
            prev_heights[i] = coin.nHeight;
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
    int expired = pool.Expire(GetTime<std::chrono::seconds>() - pool.m_expiry);
    if (expired != 0) {
        LogPrint(BCLog::MEMPOOL, "Expired %i transactions from the memory pool\n", expired);
    }

    std::vector<COutPoint> vNoSpendsRemaining;
    pool.TrimToSize(pool.m_max_size_bytes, &vNoSpendsRemaining);
    for (const COutPoint& removed : vNoSpendsRemaining)
        coins_cache.Uncache(removed);
}

static bool IsCurrentForFeeEstimation(Chainstate& active_chainstate) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    if (active_chainstate.IsInitialBlockDownload())
        return false;
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
    // disconnectpool's insertion_order index sorts the entries from
    // oldest to newest, but the oldest entry will be the last tx from the
    // latest mined block that was disconnected.
    // Iterate disconnectpool in reverse, so that we add transactions
    // back to the mempool starting with the earliest transaction that had
    // been previously seen in a block.
    auto it = disconnectpool.queuedTx.get<insertion_order>().rbegin();
    while (it != disconnectpool.queuedTx.get<insertion_order>().rend()) {
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
    disconnectpool.queuedTx.clear();
    // AcceptToMemoryPool/addUnchecked all assume that new mempool entries have
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
    const auto filter_final_and_mature = [this](CTxMemPool::txiter it)
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
                m_mempool->mapTx.modify(it, [&new_lock_points](CTxMemPoolEntry& e) { e.UpdateLockPoints(*new_lock_points); });
            } else {
                return true;
            }
        }

        // If the transaction spends any coinbase outputs, it must be mature.
        if (it->GetSpendsCoinbase()) {
            for (const CTxIn& txin : tx.vin) {
                auto it2 = m_mempool->mapTx.find(txin.prevout.hash);
                if (it2 != m_mempool->mapTx.end())
                    continue;
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
                unsigned int flags, PrecomputedTransactionData& txdata, CCoinsViewCache& coins_tip)
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
    return CheckInputScripts(tx, state, view, flags, /* cacheSigStore= */ true, /* cacheFullScriptStore= */ true, txdata);
}

namespace {

class MemPoolAccept
{
public:
    explicit MemPoolAccept(CTxMemPool& mempool, Chainstate& active_chainstate) :
        m_pool(mempool),
        m_view(&m_dummy),
        m_viewmempool(&active_chainstate.CoinsTip(), m_pool),
        m_active_chainstate(active_chainstate),
        m_limits{m_pool.m_limits}
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
        const bool m_test_accept;
        /** Whether we allow transactions to replace mempool transactions by BIP125 rules. If false,
         * any transaction spending the same inputs as a transaction in the mempool is considered
         * a conflict. */
        const bool m_allow_replacement;
        /** When true, the mempool will not be trimmed when individual transactions are submitted in
         * Finalize(). Instead, limits should be enforced at the end to ensure the package is not
         * partially submitted.
         */
        const bool m_package_submission;
        /** When true, use package feerates instead of individual transaction feerates for fee-based
         * policies such as mempool min fee and min relay fee.
         */
        const bool m_package_feerates;

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
                            /* m_package_submission */ false,
                            /* m_package_feerates */ false,
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
                            /* m_package_submission */ false, // not submitting to mempool
                            /* m_package_feerates */ false,
            };
        }

        /** Parameters for child-with-unconfirmed-parents package validation. */
        static ATMPArgs PackageChildWithParents(const CChainParams& chainparams, int64_t accept_time,
                                                std::vector<COutPoint>& coins_to_uncache) {
            return ATMPArgs{/* m_chainparams */ chainparams,
                            /* m_accept_time */ accept_time,
                            /* m_bypass_limits */ false,
                            /* m_coins_to_uncache */ coins_to_uncache,
                            /* m_test_accept */ false,
                            /* m_allow_replacement */ false,
                            /* m_package_submission */ true,
                            /* m_package_feerates */ true,
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
                            /* m_package_submission */ false,
                            /* m_package_feerates */ false, // only 1 transaction
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
                 bool package_submission,
                 bool package_feerates)
            : m_chainparams{chainparams},
              m_accept_time{accept_time},
              m_bypass_limits{bypass_limits},
              m_coins_to_uncache{coins_to_uncache},
              m_test_accept{test_accept},
              m_allow_replacement{allow_replacement},
              m_package_submission{package_submission},
              m_package_feerates{package_feerates}
        {
        }
    };
    
    // Single transaction acceptance
    MempoolAcceptResult AcceptSingleTransaction(const CTransactionRef& ptx, ATMPArgs& args) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /**
    * Multiple transaction acceptance. Transactions may or may not be interdependent, but must not
    * conflict with each other, and the transactions cannot already be in the mempool. Parents must
    * come before children if any dependencies exist.
    */
    PackageMempoolAcceptResult AcceptMultipleTransactions(const std::vector<CTransactionRef>& txns, ATMPArgs& args) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

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
        /** Txids of mempool transactions that this transaction directly conflicts with. */
        std::set<uint256> m_conflicts;
        /** Txids of mempool asset transactions that this transaction directly conflicts with. */
        std::set<uint256> m_conflictsAsset;
        /** Iterators to mempool entries that this transaction directly conflicts with. */
        CTxMemPool::setEntries m_iters_conflicting;
        /** Iterators to mempool asset entries that this transaction directly conflicts with. */
        CTxMemPool::setEntries m_iters_conflictingAsset;
        /** Iterators to all mempool entries that would be replaced by this transaction, including
         * those it directly conflicts with and their descendants. */
        CTxMemPool::setEntries m_all_conflicting;
        /** All mempool ancestors of this transaction. */
        CTxMemPool::setEntries m_ancestors;
        /** Mempool entry constructed for this transaction. Constructed in PreChecks() but not
         * inserted into the mempool until Finalize(). */
        std::unique_ptr<CTxMemPoolEntry> m_entry;
        /** Pointers to the transactions that have been removed from the mempool and replaced by
         * this transaction, used to return to the MemPoolAccept caller. Only populated if
         * validation is successful and the original transactions are removed. */
        std::list<CTransactionRef> m_replaced_transactions;

        /** Virtual size of the transaction as used by the mempool, calculated using serialized size
         * of the transaction and sigops. */
        int64_t m_vsize;
        /** Fees paid by this transaction: total input amounts subtracted by total output amounts. */
        CAmount m_base_fees;
        /** Base fees + any fee delta set by the user with prioritisetransaction. */
        CAmount m_modified_fees;
        /** Total modified fees of all transactions being replaced. */
        CAmount m_conflicting_fees{0};
        /** Total virtual size of all transactions being replaced. */
        size_t m_conflicting_size{0};

        /** If we're doing package validation (i.e. m_package_feerates=true), the "effective"
         * package feerate of this transaction is the total fees divided by the total size of
         * transactions (which may include its ancestors and/or descendants). */
        CFeeRate m_package_feerate{0};

        const CTransactionRef& m_ptx;
        /** Txid. */
        const uint256& m_hash;
        TxValidationState m_state;
        /** A temporary cache containing serialized transaction data for signature verification.
         * Reused across PolicyScriptChecks and ConsensusScriptChecks. */
        PrecomputedTransactionData m_precomputed_txdata;
        // SYSCOIN
        PoDAMAPMemory mapPoDA;
    };

    // Run the policy checks on a given transaction, excluding any script checks.
    // Looks up inputs, calculates feerate, considers replacement, evaluates
    // package limits, etc. As this function can be invoked for "free" by a peer,
    // only tests that are fast should be done here (to avoid CPU DoS).
    bool PreChecks(ATMPArgs& args, Workspace& ws) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    // Run checks for mempool replace-by-fee.
    bool ReplacementChecks(Workspace& ws) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    // Enforce package mempool ancestor/descendant limits (distinct from individual
    // ancestor/descendant limits done in PreChecks).
    bool PackageMempoolChecks(const std::vector<CTransactionRef>& txns,
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
    // Returns true if the transaction is in the mempool after any size
    // limiting is performed, false otherwise.
    bool Finalize(const ATMPArgs& args, Workspace& ws) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    // Submit all transactions to the mempool and call ConsensusScriptChecks to add to the script
    // cache - should only be called after successful validation of all transactions in the package.
    // The package may end up partially-submitted after size limiting; returns true if all
    // transactions are successfully added to the mempool, false otherwise.
    bool SubmitPackage(const ATMPArgs& args, std::vector<Workspace>& workspaces, PackageValidationState& package_state,
                       std::map<const uint256, const MempoolAcceptResult>& results)
         EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

     // Compare a package's feerate against minimum allowed.
    bool CheckFeeRate(size_t package_size, CAmount package_fee, TxValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, m_pool.cs)
    {
        AssertLockHeld(::cs_main);
        AssertLockHeld(m_pool.cs);
        CAmount mempoolRejectFee = m_pool.GetMinFee().GetFee(package_size);
        if (mempoolRejectFee > 0 && package_fee < mempoolRejectFee) {
            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "mempool min fee not met", strprintf("%d < %d", package_fee, mempoolRejectFee));
        }

        if (package_fee < m_pool.m_min_relay_feerate.GetFee(package_size)) {
            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "min relay fee not met",
                                 strprintf("%d < %d", package_fee, m_pool.m_min_relay_feerate.GetFee(package_size)));
        }
        return true;
    }

private:
    CTxMemPool& m_pool;
    CCoinsViewCache m_view;
    CCoinsViewMemPool m_viewmempool;
    CCoinsView m_dummy;

    Chainstate& m_active_chainstate;

    CTxMemPool::Limits m_limits;

    /** Whether the transaction(s) would replace any mempool transactions. If so, RBF rules apply. */
    bool m_rbf{false};
};

bool MemPoolAccept::PreChecks(ATMPArgs& args, Workspace& ws)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(m_pool.cs);
    const CTransactionRef& ptx = ws.m_ptx;
    const CTransaction& tx = *ws.m_ptx;
    const uint256& hash = ws.m_hash;

    // Copy/alias what we need out of args
    const int64_t nAcceptTime = args.m_accept_time;
    const bool bypass_limits = args.m_bypass_limits;
    std::vector<COutPoint>& coins_to_uncache = args.m_coins_to_uncache;

    // Alias what we need out of ws
    TxValidationState& state = ws.m_state;
    std::unique_ptr<CTxMemPoolEntry>& entry = ws.m_entry;

    if (!CheckTransaction(tx, state)) {
        return false; // state filled in by CheckTransaction
    }

    // Coinbase is only valid in a block, not as a loose transaction
    if (tx.IsCoinBase())
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "coinbase");

    // Rather not work on nonstandard transactions (unless -testnet/-regtest)
    std::string reason;
    if (m_pool.m_require_standard && !IsStandardTx(tx, m_pool.m_max_datacarrier_bytes, m_pool.m_permit_bare_multisig, m_pool.m_dust_relay_feerate, reason)) {
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, reason);
    }

    // Transactions smaller than 65 non-witness bytes are not relayed to mitigate CVE-2017-12842.
    if (::GetSerializeSize(tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) < MIN_STANDARD_TX_NONWITNESS_SIZE)
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
    
    // SYSCOIN
    // Check for conflicts with in-memory transactions
    const bool& IsZTx = IsZdagTx(tx.nVersion);
    for (const CTxIn &txin : tx.vin)
    {
        const CTransaction* ptxConflicting = m_pool.GetConflictTx(txin.prevout);
        if (ptxConflicting) {
            if (!args.m_allow_replacement) {
                // Transaction conflicts with a mempool tx, but we're not allowing replacements.
                return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "bip125-replacement-disallowed");
            }
            // SYSCOIN
            if (!ws.m_conflicts.count(ptxConflicting->GetHash()) && !ws.m_conflictsAsset.count(ptxConflicting->GetHash()))
            {
                // Transactions that don't explicitly signal replaceability are
                // *not* replaceable with the current logic, even if one of their
                // unconfirmed ancestors signals replaceability. This diverges
                // from BIP125's inherited signaling description (see CVE-2021-31876).
                // Applications relying on first-seen mempool behavior should
                // check all unconfirmed ancestors; otherwise an opt-in ancestor
                // might be replaced, causing removal of this descendant.
                //
                // If replaceability signaling is ignored due to node setting,
                // replacement is always allowed.
                if (!m_pool.m_full_rbf && !SignalsOptInRBF(*ptxConflicting)) {
                    // if not RBF then allow first double spend to be relayed, ZDAG by default isn't RBF enabled because it shouldn't be replaceable and because of checks below
                    // neither are its ancestors, they will be locked in as soon as you have a ZDAG tx because ZDAG isn't compliant with RBF.
                    if(IsZTx){
                        // allow the first time this outpoint was found in conflict
                        auto it = mapAssetAllocationConflicts.try_emplace(txin.prevout, ptx, MakeTransactionRef(*ptxConflicting));
                        // if was inserted (not found)
                        if(it.second) {
                            // if just testing, and this is the first conflict for this prevout then let it go through but just don't add it to the global mapAssetAllocationConflicts
                            if(args.m_test_accept) {
                                mapAssetAllocationConflicts.erase(txin.prevout);
                            }
                            ws.m_conflictsAsset.insert(ptxConflicting->GetHash());
                            break;
                        }
                        else
                            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "txn-mempool-conflict-zdag");
                    }
                    else
                        return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "txn-mempool-conflict");
                }
                // regardless of RBF flag this tx as per BTC policy but JFYI we don't want to limit asset allocation txs to use NON-RBF,
                // its still valid to send asset allocations with RBF for normal use-case, just that verifyzdag will flag it as a warning and shouldn't be accepted at point-of-sale
                ws.m_conflicts.insert(ptxConflicting->GetHash());
            }
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

    // SYSCOIN
    CAssetsMap mapAssetIn;
    CAssetsMap mapAssetOut;
    if (!Consensus::CheckTxInputs(tx, state, m_view, m_active_chainstate.m_chain.Height() + 1, ws.m_base_fees, mapAssetIn, mapAssetOut)) {
        return false; // state filled in by CheckTxInputs
    }

    // SYSCOIN
    bool isAssetTx = false;
    const auto& params = args.m_chainparams.GetConsensus();
    if(tx.HasAssets()) {
        isAssetTx = IsAssetTx(tx.nVersion);
        if (!CheckSyscoinInputs(tx, params, hash, state, (uint32_t)m_active_chainstate.m_chain.Tip()->nHeight + 1, m_active_chainstate.m_chain.Tip()->GetMedianTimePast(), mapMintKeysMempool, args.m_test_accept, mapAssetIn, mapAssetOut)) {
            return false; // state filled in by CheckSyscoinInputs
        } 
    }  
    // SYSCOIN
    if(!ProcessNEVMData(m_active_chainstate.m_blockman, tx, m_active_chainstate.m_chain.Tip()->GetMedianTimePast(), TicksSinceEpoch<std::chrono::seconds>(m_active_chainstate.m_chainman.m_options.adjusted_time_callback()), ws.mapPoDA)) {
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "bad-txns-poda-invalid");
    }
     if (m_pool.m_require_standard && !AreInputsStandard(tx, m_view)) {
        return state.Invalid(TxValidationResult::TX_INPUTS_NOT_STANDARD, "bad-txns-nonstandard-inputs");
    }


     // Check for non-standard witnesses.
    if (tx.HasWitness() && m_pool.m_require_standard && !IsWitnessStandard(tx, m_view)) {
        return state.Invalid(TxValidationResult::TX_WITNESS_MUTATED, "bad-witness-nonstandard");
    }

    int64_t nSigOpsCost = GetTransactionSigOpCost(tx, m_view, STANDARD_SCRIPT_VERIFY_FLAGS);

    // ws.m_modified_fees includes any fee deltas from PrioritiseTransaction
    ws.m_modified_fees = ws.m_base_fees;
    m_pool.ApplyDelta(hash, ws.m_modified_fees);

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

    entry.reset(new CTxMemPoolEntry(ptx, ws.m_base_fees, nAcceptTime, m_active_chainstate.m_chain.Height(),
            fSpendsCoinbase, nSigOpsCost, lock_points.value()));
    // SYSCOIN only double fee-rate requirement for allocation spends if not RBF
    const bool& isZDAGNoRBF = IsZTx && !SignalsOptInRBF(tx);
    if (isZDAGNoRBF) {
        const size_t& txTotalSize = tx.GetTotalSize();
        if(txTotalSize > MAX_STANDARD_ZDAG_TX_SIZE) {
            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "mempool-zdag-too-large", strprintf("%d > %d", txTotalSize, MAX_STANDARD_ZDAG_TX_SIZE));
        }
    }
    // ensure we apply double the bandwidth costs for zdag to account for 1 double-spend allowance per zdag tx
    ws.m_vsize = isZDAGNoRBF? entry->GetTxSize()*2: entry->GetTxSize();

    if (nSigOpsCost > MAX_STANDARD_TX_SIGOPS_COST)
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "bad-txns-too-many-sigops",
                strprintf("%d", nSigOpsCost));

    // No individual transactions are allowed below the min relay feerate and mempool min feerate except from
    // disconnected blocks and transactions in a package. Package transactions will be checked using
    // package feerate later.
    if (!bypass_limits && !args.m_package_feerates && !CheckFeeRate(ws.m_vsize, ws.m_modified_fees, state)) return false;

    ws.m_iters_conflicting = m_pool.GetIterSet(ws.m_conflicts);
    // Calculate in-mempool ancestors, up to a limit.
    if (ws.m_conflicts.size() == 1) {
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

        m_limits.descendant_count += 1;
        m_limits.descendant_size_vbytes += conflict->GetSizeWithDescendants();
    }
    // SYSCOIN
    ws.m_iters_conflictingAsset = m_pool.GetIterSet(ws.m_conflictsAsset);
    // Calculate in-mempool ancestors, up to a limit.
    if (ws.m_conflictsAsset.size() == 1) {
        assert(ws.m_iters_conflictingAsset.size() == 1);
        CTxMemPool::txiter conflict = *ws.m_iters_conflictingAsset.begin();

        m_limits.descendant_count += 1;
        m_limits.descendant_size_vbytes += conflict->GetSizeWithDescendants();
    }
    auto ancestors{m_pool.CalculateMemPoolAncestors(*entry, m_limits)};
    if (!ancestors) {
        // If CalculateMemPoolAncestors fails second time, we want the original error string.
        // Contracting/payment channels CPFP carve-out:
        // If the new transaction is relatively small (up to 40k weight)
        // and has at most one ancestor (ie ancestor limit of 2, including
        // the new transaction), allow it if its parent has exactly the
        // descendant limit descendants.
        //
        // This allows protocols which rely on distrusting counterparties
        // being able to broadcast descendants of an unconfirmed transaction
        // to be secure by simply only having two immediately-spendable
        // outputs - one for each counterparty. For more info on the uses for
        // this, see https://lists.linuxfoundation.org/pipermail/bitcoin-dev/2018-November/016518.html
        CTxMemPool::Limits cpfp_carve_out_limits{
            .ancestor_count = 2,
            .ancestor_size_vbytes = m_limits.ancestor_size_vbytes,
            .descendant_count = m_limits.descendant_count + 1,
            .descendant_size_vbytes = m_limits.descendant_size_vbytes + EXTRA_DESCENDANT_TX_SIZE_LIMIT,
        };
        const auto error_message{util::ErrorString(ancestors).original};
        if (ws.m_vsize > EXTRA_DESCENDANT_TX_SIZE_LIMIT) {
            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "too-long-mempool-chain", error_message);
        }
        ancestors = m_pool.CalculateMemPoolAncestors(*entry, cpfp_carve_out_limits);
        if (!ancestors) return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "too-long-mempool-chain", error_message);
    }
    // SYSCOIN asset tx must use confirmed inputs because non utxo information changes
    // which may invalidate transactions even though they are entered into mempool
    if(isAssetTx && !ws.m_ancestors.empty()) {
        return state.Invalid(TxValidationResult::TX_CONFLICT, "bad-txns-asset-inputs-missingorspent");
    }

    // check special TXs after all the other checks. If we'd do this before the other checks, we might end up
    // DoS scoring a node for non-critical errors, e.g. duplicate keys because a TX is received that was already
    // mined
    if (!CheckSpecialTx(m_active_chainstate.m_blockman, tx, m_active_chainstate.m_chain.Tip(), state, m_active_chainstate.CoinsTip(), true))
        return false;

    if (m_pool.existsProviderTxConflict(tx)) {
        return state.Invalid(TxValidationResult::TX_CONFLICT, "protx-dup");
    }
    ws.m_ancestors = *ancestors;

    // A transaction that spends outputs that would be replaced by it is invalid. Now
    // that we have the set of all ancestors we can detect this
    // pathological case by making sure setConflicts/setConflictsAsset and setAncestors don't
    // intersect.
    // SYSCOIN
    if (const auto err_string{EntriesAndTxidsDisjoint(ws.m_ancestors, ws.m_conflicts, ws.m_conflictsAsset, hash)}) {
        // We classify this as a consensus error because a transaction depending on something it
        // conflicts with would be inconsistent.
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-spends-conflicting-tx", *err_string);
    }

    m_rbf = !ws.m_conflicts.empty();
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
        return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "insufficient fee", *err_string);
    }

    // Calculate all conflicting entries and enforce Rule #5.
    if (const auto err_string{GetEntriesForConflicts(tx, m_pool, ws.m_iters_conflicting, ws.m_all_conflicting)}) {
        return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY,
                             "too many potential replacements", *err_string);
    }
    // Enforce Rule #2.
    if (const auto err_string{HasNoNewUnconfirmed(tx, m_pool, ws.m_iters_conflicting)}) {
        return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY,
                             "replacement-adds-unconfirmed", *err_string);
    }
    // Check if it's economically rational to mine this transaction rather than the ones it
    // replaces and pays for its own relay fees. Enforce Rules #3 and #4.
    for (CTxMemPool::txiter it : ws.m_all_conflicting) {
        ws.m_conflicting_fees += it->GetModifiedFee();
        ws.m_conflicting_size += it->GetTxSize();
    }
    if (const auto err_string{PaysForRBF(ws.m_conflicting_fees, ws.m_modified_fees, ws.m_vsize,
                                         m_pool.m_incremental_relay_feerate, hash)}) {
        return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "insufficient fee", *err_string);
    }
    return true;
}

bool MemPoolAccept::PackageMempoolChecks(const std::vector<CTransactionRef>& txns,
                                         PackageValidationState& package_state)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(m_pool.cs);

    // CheckPackageLimits expects the package transactions to not already be in the mempool.
    assert(std::all_of(txns.cbegin(), txns.cend(), [this](const auto& tx)
                       { return !m_pool.exists(GenTxid::Txid(tx->GetHash()));}));

    std::string err_string;
    if (!m_pool.CheckPackageLimits(txns, m_limits, err_string)) {
        // This is a package-wide error, separate from an individual transaction error.
        return package_state.Invalid(PackageValidationResult::PCKG_POLICY, "package-mempool-limits", err_string);
    }
   return true;
}

bool MemPoolAccept::PolicyScriptChecks(const ATMPArgs& args, Workspace& ws)
{
    const CTransaction& tx = *ws.m_ptx;
    TxValidationState& state = ws.m_state;

    constexpr unsigned int scriptVerifyFlags = STANDARD_SCRIPT_VERIFY_FLAGS;

    // Check input scripts and signatures.
    // This is done last to help prevent CPU exhaustion denial-of-service attacks.
    if (!CheckInputScripts(tx, state, m_view, scriptVerifyFlags, true, false, ws.m_precomputed_txdata)) {
        // SCRIPT_VERIFY_CLEANSTACK requires SCRIPT_VERIFY_WITNESS, so we
        // need to turn both off, and compare against just turning off CLEANSTACK
        // to see if the failure is specifically due to witness validation.
        TxValidationState state_dummy; // Want reported failures to be from first CheckInputScripts
        if (!tx.HasWitness() && CheckInputScripts(tx, state_dummy, m_view, scriptVerifyFlags & ~(SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_CLEANSTACK), true, false, ws.m_precomputed_txdata) &&
                !CheckInputScripts(tx, state_dummy, m_view, scriptVerifyFlags & ~SCRIPT_VERIFY_CLEANSTACK, true, false, ws.m_precomputed_txdata)) {
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
                                        ws.m_precomputed_txdata, m_active_chainstate.CoinsTip())) {
        LogPrintf("BUG! PLEASE REPORT THIS! CheckInputScripts failed against latest-block but not STANDARD flags %s, %s\n", hash.ToString(), state.ToString());
        return Assume(false);
    }
    return true;
}

bool MemPoolAccept::Finalize(const ATMPArgs& args, Workspace& ws)
{
    const CTransaction& tx = *ws.m_ptx;
    const uint256& hash = ws.m_hash;
    TxValidationState& state = ws.m_state;
    const bool bypass_limits = args.m_bypass_limits;

    std::unique_ptr<CTxMemPoolEntry>& entry = ws.m_entry;
    // Remove conflicting transactions from the mempool
    for (CTxMemPool::txiter it : ws.m_all_conflicting)
    {
        LogPrint(BCLog::MEMPOOL, "replacing tx %s with %s for %s additional fees, %d delta bytes\n",
                it->GetTx().GetHash().ToString(),
                hash.ToString(),
                FormatMoney(ws.m_modified_fees - ws.m_conflicting_fees),
                (int)entry->GetTxSize() - (int)ws.m_conflicting_size);
        ws.m_replaced_transactions.push_back(it->GetSharedTx());
    }
    m_pool.RemoveStaged(ws.m_all_conflicting, false, MemPoolRemovalReason::REPLACED);

    // This transaction should only count for fee estimation if:
    // - it's not being re-added during a reorg which bypasses typical mempool fee limits
    // - the node is not behind
    // - the transaction is not dependent on any other transactions in the mempool
    // - it's not part of a package. Since package relay is not currently supported, this
    // transaction has not necessarily been accepted to miners' mempools.
    bool validForFeeEstimation = !bypass_limits && !args.m_package_submission && IsCurrentForFeeEstimation(m_active_chainstate) && m_pool.HasNoInputsOf(tx);

    // Store transaction in memory
    m_pool.addUnchecked(*entry, ws.m_ancestors, validForFeeEstimation);

    // SYSCOIN
    if(pnevmdatadb)
        pnevmdatadb->FlushDataToCache(ws.mapPoDA, 0);
    // trim mempool and check if tx was trimmed
    // If we are validating a package, don't trim here because we could evict a previous transaction
    // in the package. LimitMempoolSize() should be called at the very end to make sure the mempool
    // is still within limits and package submission happens atomically.
    if (!args.m_package_submission && !bypass_limits) {
        LimitMempoolSize(m_pool, m_active_chainstate.CoinsTip());
        if (!m_pool.exists(GenTxid::Txid(hash)))
            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "mempool full");
    }
    return true;
}

bool MemPoolAccept::SubmitPackage(const ATMPArgs& args, std::vector<Workspace>& workspaces,
                                  PackageValidationState& package_state,
                                  std::map<const uint256, const MempoolAcceptResult>& results)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(m_pool.cs);
    // Sanity check: none of the transactions should be in the mempool, and none of the transactions
    // should have a same-txid-different-witness equivalent in the mempool.
    assert(std::all_of(workspaces.cbegin(), workspaces.cend(), [this](const auto& ws){
        return !m_pool.exists(GenTxid::Txid(ws.m_ptx->GetHash())); }));

    bool all_submitted = true;
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
        }

        // Re-calculate mempool ancestors to call addUnchecked(). They may have changed since the
        // last calculation done in PreChecks, since package ancestors have already been submitted.
        {
            auto ancestors{m_pool.CalculateMemPoolAncestors(*ws.m_entry, m_limits)};
            if(!ancestors) {
                results.emplace(ws.m_ptx->GetWitnessHash(), MempoolAcceptResult::Failure(ws.m_state));
                // Since PreChecks() and PackageMempoolChecks() both enforce limits, this should never fail.
                Assume(false);
                all_submitted = false;
                package_state.Invalid(PackageValidationResult::PCKG_MEMPOOL_ERROR,
                                    strprintf("BUG! Mempool ancestors or descendants were underestimated: %s",
                                                ws.m_ptx->GetHash().ToString()));
            }
            ws.m_ancestors = std::move(ancestors).value_or(ws.m_ancestors);
        }
        // If we call LimitMempoolSize() for each individual Finalize(), the mempool will not take
        // the transaction's descendant feerate into account because it hasn't seen them yet. Also,
        // we risk evicting a transaction that a subsequent package transaction depends on. Instead,
        // allow the mempool to temporarily bypass limits, the maximum package size) while
        // submitting transactions individually and then trim at the very end.
        if (!Finalize(args, ws)) {
            results.emplace(ws.m_ptx->GetWitnessHash(), MempoolAcceptResult::Failure(ws.m_state));
            // Since LimitMempoolSize() won't be called, this should never fail.
            Assume(false);
            all_submitted = false;
            package_state.Invalid(PackageValidationResult::PCKG_MEMPOOL_ERROR,
                                  strprintf("BUG! Adding to mempool failed: %s", ws.m_ptx->GetHash().ToString()));
        }
    }

    // It may or may not be the case that all the transactions made it into the mempool. Regardless,
    // make sure we haven't exceeded max mempool size.
    LimitMempoolSize(m_pool, m_active_chainstate.CoinsTip());

    std::vector<uint256> all_package_wtxids;
    all_package_wtxids.reserve(workspaces.size());
    std::transform(workspaces.cbegin(), workspaces.cend(), std::back_inserter(all_package_wtxids),
                   [](const auto& ws) { return ws.m_ptx->GetWitnessHash(); });
    // Find the wtxids of the transactions that made it into the mempool. Allow partial submission,
    // but don't report success unless they all made it into the mempool.
    for (Workspace& ws : workspaces) {
        const auto effective_feerate = args.m_package_feerates ? ws.m_package_feerate :
            CFeeRate{ws.m_modified_fees, static_cast<uint32_t>(ws.m_vsize)};
        const auto effective_feerate_wtxids = args.m_package_feerates ? all_package_wtxids :
            std::vector<uint256>({ws.m_ptx->GetWitnessHash()});
        if (m_pool.exists(GenTxid::Wtxid(ws.m_ptx->GetWitnessHash()))) {
            results.emplace(ws.m_ptx->GetWitnessHash(),
                MempoolAcceptResult::Success(std::move(ws.m_replaced_transactions), ws.m_vsize,
                                             ws.m_base_fees, effective_feerate, effective_feerate_wtxids));
            GetMainSignals().TransactionAddedToMempool(ws.m_ptx, m_pool.GetAndIncrementSequence());
        } else {
            all_submitted = false;
            ws.m_state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "mempool full");
            results.emplace(ws.m_ptx->GetWitnessHash(), MempoolAcceptResult::Failure(ws.m_state));
        }
    }
    return all_submitted;
}

MempoolAcceptResult MemPoolAccept::AcceptSingleTransaction(const CTransactionRef& ptx, ATMPArgs& args)
{
    AssertLockHeld(cs_main);
    LOCK(m_pool.cs); // mempool "read lock" (held through GetMainSignals().TransactionAddedToMempool())

    Workspace ws(ptx);

    if (!PreChecks(args, ws)) return MempoolAcceptResult::Failure(ws.m_state);

    if (m_rbf && !ReplacementChecks(ws)) return MempoolAcceptResult::Failure(ws.m_state);

    // Perform the inexpensive checks first and avoid hashing and signature verification unless
    // those checks pass, to mitigate CPU exhaustion denial-of-service attacks.
    if (!PolicyScriptChecks(args, ws)) return MempoolAcceptResult::Failure(ws.m_state);

    if (!ConsensusScriptChecks(args, ws)) return MempoolAcceptResult::Failure(ws.m_state);

    const CFeeRate effective_feerate{ws.m_modified_fees, static_cast<uint32_t>(ws.m_vsize)};
    const std::vector<uint256> single_wtxid{ws.m_ptx->GetWitnessHash()};
    // Tx was accepted, but not added
    if (args.m_test_accept) {
        return MempoolAcceptResult::Success(std::move(ws.m_replaced_transactions), ws.m_vsize,
                                            ws.m_base_fees, effective_feerate, single_wtxid);
    }

    if (!Finalize(args, ws)) return MempoolAcceptResult::Failure(ws.m_state);

    GetMainSignals().TransactionAddedToMempool(ptx, m_pool.GetAndIncrementSequence());

    return MempoolAcceptResult::Success(std::move(ws.m_replaced_transactions), ws.m_vsize, ws.m_base_fees,
                                        effective_feerate, single_wtxid);
}

PackageMempoolAcceptResult MemPoolAccept::AcceptMultipleTransactions(const std::vector<CTransactionRef>& txns, ATMPArgs& args)
{
    AssertLockHeld(cs_main);

    // These context-free package limits can be done before taking the mempool lock.
    PackageValidationState package_state;
    if (!CheckPackage(txns, package_state)) return PackageMempoolAcceptResult(package_state, {});

    std::vector<Workspace> workspaces{};
    workspaces.reserve(txns.size());
    std::transform(txns.cbegin(), txns.cend(), std::back_inserter(workspaces),
                   [](const auto& tx) { return Workspace(tx); });
    std::map<const uint256, const MempoolAcceptResult> results;

    LOCK(m_pool.cs);

    // Do all PreChecks first and fail fast to avoid running expensive script checks when unnecessary.
    for (Workspace& ws : workspaces) {
        if (!PreChecks(args, ws)) {
            package_state.Invalid(PackageValidationResult::PCKG_TX, "transaction failed");
            // Exit early to avoid doing pointless work. Update the failed tx result; the rest are unfinished.
            results.emplace(ws.m_ptx->GetWitnessHash(), MempoolAcceptResult::Failure(ws.m_state));
            return PackageMempoolAcceptResult(package_state, std::move(results));
        }
        // Make the coins created by this transaction available for subsequent transactions in the
        // package to spend. Since we already checked conflicts in the package and we don't allow
        // replacements, we don't need to track the coins spent. Note that this logic will need to be
        // updated if package replace-by-fee is allowed in the future.
        assert(!args.m_allow_replacement);
        m_viewmempool.PackageAddTransaction(ws.m_ptx);
    }

    // Transactions must meet two minimum feerates: the mempool minimum fee and min relay fee.
    // For transactions consisting of exactly one child and its parents, it suffices to use the
    // package feerate (total modified fees / total virtual size) to check this requirement.
    const auto m_total_vsize = std::accumulate(workspaces.cbegin(), workspaces.cend(), int64_t{0},
        [](int64_t sum, auto& ws) { return sum + ws.m_vsize; });
    const auto m_total_modified_fees = std::accumulate(workspaces.cbegin(), workspaces.cend(), CAmount{0},
        [](CAmount sum, auto& ws) { return sum + ws.m_modified_fees; });
    const CFeeRate package_feerate(m_total_modified_fees, m_total_vsize);
    TxValidationState placeholder_state;
    if (args.m_package_feerates &&
        !CheckFeeRate(m_total_vsize, m_total_modified_fees, placeholder_state)) {
        package_state.Invalid(PackageValidationResult::PCKG_POLICY, "package-fee-too-low");
        return PackageMempoolAcceptResult(package_state, {});
    }

    // Apply package mempool ancestor/descendant limits. Skip if there is only one transaction,
    // because it's unnecessary. Also, CPFP carve out can increase the limit for individual
    // transactions, but this exemption is not extended to packages in CheckPackageLimits().
    std::string err_string;
    if (txns.size() > 1 && !PackageMempoolChecks(txns, package_state)) {
        return PackageMempoolAcceptResult(package_state, std::move(results));
    }

    std::vector<uint256> all_package_wtxids;
    all_package_wtxids.reserve(workspaces.size());
    std::transform(workspaces.cbegin(), workspaces.cend(), std::back_inserter(all_package_wtxids),
                   [](const auto& ws) { return ws.m_ptx->GetWitnessHash(); });
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
                std::vector<uint256>{ws.m_ptx->GetWitnessHash()};
            results.emplace(ws.m_ptx->GetWitnessHash(),
                            MempoolAcceptResult::Success(std::move(ws.m_replaced_transactions),
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

PackageMempoolAcceptResult MemPoolAccept::AcceptPackage(const Package& package, ATMPArgs& args)
{
    AssertLockHeld(cs_main);
    // Used if returning a PackageMempoolAcceptResult directly from this function.
    PackageValidationState package_state_quit_early;

    // Check that the package is well-formed. If it isn't, we won't try to validate any of the
    // transactions and thus won't return any MempoolAcceptResults, just a package-wide error.

    // Context-free package checks.
    if (!CheckPackage(package, package_state_quit_early)) return PackageMempoolAcceptResult(package_state_quit_early, {});

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

    LOCK(m_pool.cs);
    // Stores final results that won't change
    std::map<const uint256, const MempoolAcceptResult> results_final;
    // Node operators are free to set their mempool policies however they please, nodes may receive
    // transactions in different orders, and malicious counterparties may try to take advantage of
    // policy differences to pin or delay propagation of transactions. As such, it's possible for
    // some package transaction(s) to already be in the mempool, and we don't want to reject the
    // entire package in that case (as that could be a censorship vector). De-duplicate the
    // transactions that are already in the mempool, and only call AcceptMultipleTransactions() with
    // the new transactions. This ensures we don't double-count transaction counts and sizes when
    // checking ancestor/descendant limits, or double-count transaction fees for fee-related policy.
    ATMPArgs single_args = ATMPArgs::SingleInPackageAccept(args);
    // Results from individual validation. "Nonfinal" because if a transaction fails by itself but
    // succeeds later (i.e. when evaluated with a fee-bumping child), the result changes (though not
    // reflected in this map). If a transaction fails more than once, we want to return the first
    // result, when it was considered on its own. So changes will only be from invalid -> valid.
    std::map<uint256, MempoolAcceptResult> individual_results_nonfinal;
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
            auto iter = m_pool.GetIter(txid);
            assert(iter != std::nullopt);
            results_final.emplace(wtxid, MempoolAcceptResult::MempoolTx(iter.value()->GetTxSize(), iter.value()->GetFee()));
        } else if (m_pool.exists(GenTxid::Txid(txid))) {
            // Transaction with the same non-witness data but different witness (same txid,
            // different wtxid) already exists in the mempool.
            //
            // We don't allow replacement transactions right now, so just swap the package
            // transaction for the mempool one. Note that we are ignoring the validity of the
            // package transaction passed in.
            // TODO: allow witness replacement in packages.
            auto iter = m_pool.GetIter(txid);
            assert(iter != std::nullopt);
            // Provide the wtxid of the mempool tx so that the caller can look it up in the mempool.
            results_final.emplace(wtxid, MempoolAcceptResult::MempoolTxDifferentWitness(iter.value()->GetTx().GetWitnessHash()));
        } else {
            // Transaction does not already exist in the mempool.
            // Try submitting the transaction on its own.
            const auto single_res = AcceptSingleTransaction(tx, single_args);
            if (single_res.m_result_type == MempoolAcceptResult::ResultType::VALID) {
                // The transaction succeeded on its own and is now in the mempool. Don't include it
                // in package validation, because its fees should only be "used" once.
                assert(m_pool.exists(GenTxid::Wtxid(wtxid)));
                results_final.emplace(wtxid, single_res);
            } else if (single_res.m_state.GetResult() != TxValidationResult::TX_MEMPOOL_POLICY &&
                       single_res.m_state.GetResult() != TxValidationResult::TX_MISSING_INPUTS) {
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

    // Quit early because package validation won't change the result or the entire package has
    // already been submitted.
    if (quit_early || txns_package_eval.empty()) {
        for (const auto& [wtxid, mempoolaccept_res] : individual_results_nonfinal) {
            Assume(results_final.emplace(wtxid, mempoolaccept_res).second);
            Assume(mempoolaccept_res.m_result_type == MempoolAcceptResult::ResultType::INVALID);
        }
        return PackageMempoolAcceptResult(package_state_quit_early, std::move(results_final));
    }
    // Validate the (deduplicated) transactions as a package. Note that submission_result has its
    // own PackageValidationState; package_state_quit_early is unused past this point.
    auto submission_result = AcceptMultipleTransactions(txns_package_eval, args);
    // Include already-in-mempool transaction results in the final result.
    for (const auto& [wtxid, mempoolaccept_res] : results_final) {
        Assume(submission_result.m_tx_results.emplace(wtxid, mempoolaccept_res).second);
        Assume(mempoolaccept_res.m_result_type != MempoolAcceptResult::ResultType::INVALID);
    }
    if (submission_result.m_state.GetResult() == PackageValidationResult::PCKG_TX) {
        // Package validation failed because one or more transactions failed. Provide a result for
        // each transaction; if AcceptMultipleTransactions() didn't return a result for a tx,
        // include the previous individual failure reason.
        submission_result.m_tx_results.insert(individual_results_nonfinal.cbegin(),
                                              individual_results_nonfinal.cend());
        Assume(submission_result.m_tx_results.size() == package.size());
    }
    return submission_result;
}

} // anon namespace

MempoolAcceptResult AcceptToMemoryPool(Chainstate& active_chainstate, const CTransactionRef& tx,
                                       int64_t accept_time, bool bypass_limits, bool test_accept)
    EXCLUSIVE_LOCKS_REQUIRED(cs_main)
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

        for (const COutPoint& hashTx : coins_to_uncache) {
            active_chainstate.CoinsTip().Uncache(hashTx);
            // SYSCOIN
            mapAssetAllocationConflicts.erase(hashTx);
        }
        // if we had duplicate mint's we don't want to remove the mint tx hash, but only if we had some other error not related to TX_MINT_DUPLICATE
        if(result.m_state.GetResult() != TxValidationResult::TX_MINT_DUPLICATE) {
            // remove nevm tx from mempool structure
            if(IsSyscoinMintTx(tx->nVersion)) {
                CMintSyscoin mintSyscoin(*tx);
                if(!mintSyscoin.IsNull()) {
                    mapMintKeysMempool.erase(mintSyscoin.nTxHash);
                }
            }
        }
    }
    // After we've (potentially) uncached entries, ensure our coins cache is still within its size limits
    BlockValidationState state_dummy;
    active_chainstate.FlushStateToDisk(state_dummy, FlushStateMode::PERIODIC);
    return result;
}

PackageMempoolAcceptResult ProcessNewPackage(Chainstate& active_chainstate, CTxMemPool& pool,
                                                   const Package& package, bool test_accept)
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
            auto args = MemPoolAccept::ATMPArgs::PackageChildWithParents(chainparams, GetTime(), coins_to_uncache);
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

//////////////////////////////////////////////////////////////////////////////
//
// CBlock and CBlockIndex
//
bool CheckProofOfWork(const CBlockHeader& block, const Consensus::Params& params)
{
    /* Except for legacy blocks with full version 1, ensure that
       the chain ID is correct.  Legacy blocks are not allowed since
       the merge-mining start, which is checked in AcceptBlockHeader
       where the height is known.  */
    const int32_t &nChainID = block.GetChainId();
    if (!block.IsLegacy() && params.fStrictChainId) {
        if(nChainID > 0) {
            if(nChainID != params.nAuxpowChainId)
                return error("%s : block does not have our chain ID"
                        " (got %d, expected %d, full nVersion %d)",
                        __func__, nChainID,
                        params.nAuxpowChainId, block.nVersion);
        } else if(block.auxpow) {
            const int32_t &nOldChainID = block.GetOldChainId();
            if(nOldChainID != params.nAuxpowOldChainId)
                return error("%s : block does not have our old chain ID"
                        " (got %d, expected %d, full nVersion %d)",
                        __func__, nOldChainID,
                        params.nAuxpowOldChainId, block.nVersion);
        }
    }
        

    /* If there is no auxpow, just check the block hash.  */
    if (!block.auxpow)
    {
        if (block.IsAuxpow())
            return error("%s : no auxpow on block with auxpow version",
                         __func__);

        if (!CheckProofOfWork(block.GetHash(), block.nBits, params))
            return error("%s : non-AUX proof of work failed", __func__);

        return true;
    }

    /* We have auxpow.  Check it.  */
    if (!block.IsAuxpow())
        return error("%s : auxpow on block with non-auxpow version", __func__);
  
    if (!CheckProofOfWork(block.auxpow->getParentBlockHash(), block.nBits, params))
        return error("%s : AUX proof of work failed", __func__);
    if (!block.auxpow->check(block.GetHash(), block.GetChainId(), params))
        return error("%s : AUX POW is not valid", __func__);
    

    return true;
}

CAmount GetBlockSubsidyRegtest(int nHeight, const Consensus::Params& consensusParams)
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
CAmount GetBlockSubsidy(unsigned int nHeight, const Consensus::Params& consensusParams, bool fSuperblockPartOnly)
{
    if(fRegTest) {
        return GetBlockSubsidyRegtest(nHeight, consensusParams);
    }
    if (nHeight == 0)
        return 50*COIN;
    if (nHeight == 1)
    {
        // SYSCOIN 4 snapshot
        return 554200000 * COIN;
    }

    CAmount nSubsidy = 38.5 * COIN;
    // account for NEVM adjustment to 2.5 blocks
    if(nHeight >= (unsigned int)consensusParams.nNEVMStartBlock)
        nSubsidy *= 2.5;
    const int &reductions = consensusParams.SubsidyHalvingIntervals(nHeight);
    if (reductions >= 50) {
        return 0;
    }
    // Subsidy reduced every year by 5%
    for (int i = 0; i < reductions; i++) {
        nSubsidy -= nSubsidy / 20;
    }
    // Reduce the block reward of miners (allowing budget/superblocks)
    const CAmount &nSuperblockPart = (nSubsidy*0.1);

    if (fSuperblockPartOnly)
        return nSuperblockPart;
    nSubsidy -= nSuperblockPart;
    return nSubsidy;

}
CoinsViews::CoinsViews(DBParams db_params, CoinsViewOptions options)
    : m_dbview{std::move(db_params), std::move(options)},
      m_catcherview(&m_dbview) {}

void CoinsViews::InitCache()
{
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
}

void Chainstate::InitCoinsCache(size_t cache_size_bytes)
{
    assert(m_coins_views != nullptr);
    m_coinstip_cache_size_bytes = cache_size_bytes;
    m_coins_views->InitCache();
}

// Note that though this is marked const, we may end up modifying `m_cached_finished_ibd`, which
// is a performance-related implementation detail. This function must be marked
// `const` so that `CValidationInterface` clients (which are given a `const Chainstate*`)
// can call it.
//
bool Chainstate::IsInitialBlockDownload() const
{
    // Optimization: pre-test latch before taking the lock.
    if (m_cached_finished_ibd.load(std::memory_order_relaxed))
        return false;

    LOCK(cs_main);
    if (m_cached_finished_ibd.load(std::memory_order_relaxed))
        return false;
    if (m_chainman.m_blockman.LoadingBlocks()) {
        return true;
    }
    if (m_chain.Tip() == nullptr)
        return true;
    if (m_chain.Tip()->nChainWork < m_chainman.MinimumChainWork()) {
        return true;
    }
    if (m_chain.Tip()->Time() < Now<NodeSeconds>() - m_chainman.m_options.max_tip_age) {
        return true;
    }
    LogPrintf("Leaving InitialBlockDownload (latching to false)\n");
    if(fNEVMConnection && !fRegTest) {
        bool bResponse = false;
        GetMainSignals().NotifyNEVMComms("startnetwork", bResponse);
    }
    m_cached_finished_ibd.store(true, std::memory_order_relaxed);
    return false;
}

static void AlertNotify(const std::string& strMessage)
{
    uiInterface.NotifyAlertChanged();
#if HAVE_SYSTEM
    std::string strCmd = gArgs.GetArg("-alertnotify", "");
    if (strCmd.empty()) return;

    // Alert text should be plain ascii coming from a trusted source, but to
    // be safe we first strip anything not in safeChars, then add single quotes around
    // the whole string before passing it to the shell:
    std::string singleQuote("'");
    std::string safeStatus = SanitizeString(strMessage);
    safeStatus = singleQuote+safeStatus+singleQuote;
    ReplaceAll(strCmd, "%s", safeStatus);

    std::thread t(runCommand, strCmd);
    t.detach(); // thread runs free
#endif
}

void Chainstate::CheckForkWarningConditions()
{
    AssertLockHeld(cs_main);

    // Before we get past initial download, we cannot reliably alert about forks
    // (we assume we don't get stuck on a fork before finishing our initial sync)
    if (IsInitialBlockDownload()) {
        return;
    }

    if (m_chainman.m_best_invalid && m_chainman.m_best_invalid->nChainWork > m_chain.Tip()->nChainWork + (GetBlockProof(*m_chain.Tip()) * 6)) {
        LogPrintf("%s: Warning: Found invalid chain at least ~6 blocks longer than our best chain.\nChain state database corruption likely.\n", __func__);
        SetfLargeWorkInvalidChainFound(true);
    } else {
        SetfLargeWorkInvalidChainFound(false);
    }
}

// Called both upon regular invalid block discovery *and* InvalidateBlock
void Chainstate::InvalidChainFound(CBlockIndex* pindexNew)
{
    if (!m_chainman.m_best_invalid || pindexNew->nChainWork > m_chainman.m_best_invalid->nChainWork) {
        m_chainman.m_best_invalid = pindexNew;
    }
    if (m_chainman.m_best_header != nullptr && m_chainman.m_best_header->GetAncestor(pindexNew->nHeight) == pindexNew) {
        m_chainman.m_best_header = m_chain.Tip();
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
// SYSCOIN
void Chainstate::ConflictingChainFound(CBlockIndex* pindexNew)
{
    LogPrintf("%s: conflicting block=%s  height=%d  log2_work=%.8f  date=%s\n", __func__,
      pindexNew->GetBlockHash().ToString(), pindexNew->nHeight,
      log(pindexNew->nChainWork.getdouble())/log(2.0), FormatISO8601DateTime(pindexNew->GetBlockTime()));
    CBlockIndex *tip = m_chain.Tip();
    assert (tip);
    LogPrintf("%s:  current best=%s  height=%d  log2_work=%.8f  date=%s\n", __func__,
      tip->GetBlockHash().ToString(), m_chain.Height(), log(tip->nChainWork.getdouble())/log(2.0),
      FormatISO8601DateTime(tip->GetBlockTime()));
    CheckForkWarningConditions();
}
// Same as InvalidChainFound, above, except not called directly from InvalidateBlock,
// which does its own setBlockIndexCandidates management.
void Chainstate::InvalidBlockFound(CBlockIndex* pindex, const BlockValidationState& state)
{
    // because chainlock is the last thing we check in CheckBlock() if we get any other error
    // prevent situation where an invalid header or block can be locked and force clients to be stuck on that chain
    // which will never be accepted
    if (state.GetResult() != BlockValidationResult::BLOCK_CHAINLOCK) {
        // if its any other error other than chainlock and we have a conflict then move back to previous known good chainlock
        if (llmq::chainLocksHandler->HasChainLock(pindex->nHeight, pindex->GetBlockHash())) {
            llmq::chainLocksHandler->SetToPreviousChainLock(); 
        }
    }
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

bool CScriptCheck::operator()() {
    const CScript &scriptSig = ptxTo->vin[nIn].scriptSig;
    const CScriptWitness *witness = &ptxTo->vin[nIn].scriptWitness;
    return VerifyScript(scriptSig, m_tx_out.scriptPubKey, witness, nFlags, CachingTransactionSignatureChecker(ptxTo, nIn, m_tx_out.nValue, cacheStore, *txdata), &error);
}

static CuckooCache::cache<uint256, SignatureCacheHasher> g_scriptExecutionCache;
static CSHA256 g_scriptExecutionCacheHasher;

bool InitScriptExecutionCache(size_t max_size_bytes)
{
    // Setup the salted hasher
    uint256 nonce = GetRandHash();
    // We want the nonce to be 64 bytes long to force the hasher to process
    // this chunk, which makes later hash computations more efficient. We
    // just write our 32-byte entropy twice to fill the 64 bytes.
    g_scriptExecutionCacheHasher.Write(nonce.begin(), 32);
    g_scriptExecutionCacheHasher.Write(nonce.begin(), 32);

    auto setup_results = g_scriptExecutionCache.setup_bytes(max_size_bytes);
    if (!setup_results) return false;

    const auto [num_elems, approx_size_bytes] = *setup_results;
    LogPrintf("Using %zu MiB out of %zu MiB requested for script execution cache, able to store %zu elements\n",
              approx_size_bytes >> 20, max_size_bytes >> 20, num_elems);
    return true;
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
 * Non-static (and re-declared) in src/test/txvalidationcache_tests.cpp
 */
bool CheckInputScripts(const CTransaction& tx, TxValidationState& state,
                       const CCoinsViewCache& inputs, unsigned int flags, bool cacheSigStore,
                       bool cacheFullScriptStore, PrecomputedTransactionData& txdata,
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
    CSHA256 hasher = g_scriptExecutionCacheHasher;
    hasher.Write(tx.GetWitnessHash().begin(), 32).Write((unsigned char*)&flags, sizeof(flags)).Finalize(hashCacheEntry.begin());
    AssertLockHeld(cs_main); //TODO: Remove this requirement by making CuckooCache not require external locks
    if (g_scriptExecutionCache.contains(hashCacheEntry, !cacheFullScriptStore)) {
        return true;
    }

    if (!txdata.m_spent_outputs_ready) {
        // SYSCOIN
        std::vector<CTxOutCoin> spent_outputs;
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
        CScriptCheck check(txdata.m_spent_outputs[i], tx, i, flags, cacheSigStore, &txdata);
        if (pvChecks) {
            pvChecks->push_back(CScriptCheck());
            check.swap(pvChecks->back());
        } else if (!check()) {
            if (flags & STANDARD_NOT_MANDATORY_VERIFY_FLAGS) {
                // Check whether the failure was caused by a
                // non-mandatory script verification check, such as
                // non-standard DER encodings or non-null dummy
                // arguments; if so, ensure we return NOT_STANDARD
                // instead of CONSENSUS to avoid downstream users
                // splitting the network between upgraded and
                // non-upgraded nodes by banning CONSENSUS-failing
                // data providers.
                CScriptCheck check2(txdata.m_spent_outputs[i], tx, i,
                        flags & ~STANDARD_NOT_MANDATORY_VERIFY_FLAGS, cacheSigStore, &txdata);
                if (check2())
                    return state.Invalid(TxValidationResult::TX_NOT_STANDARD, strprintf("non-mandatory-script-verify-flag (%s)", ScriptErrorString(check.GetScriptError())));
            }
            // MANDATORY flag failures correspond to
            // TxValidationResult::TX_CONSENSUS. Because CONSENSUS
            // failures are the most serious case of validation
            // failures, we may need to consider using
            // RECENT_CONSENSUS_CHANGE for any script failure that
            // could be due to non-upgraded nodes which we may want to
            // support, to avoid splitting the network (but this
            // depends on the details of how net_processing handles
            // such errors).
            return state.Invalid(TxValidationResult::TX_CONSENSUS, strprintf("mandatory-script-verify-flag-failed (%s)", ScriptErrorString(check.GetScriptError())));
        }
    }

    if (cacheFullScriptStore && !pvChecks) {
        // We executed all of the provided scripts, and were told to
        // cache the result. Do so now.
        g_scriptExecutionCache.insert(hashCacheEntry);
    }

    return true;
}

bool AbortNode(BlockValidationState& state, const std::string& strMessage, const bilingual_str& userMessage)
{
    AbortNode(strMessage, userMessage);
    return state.Error(strMessage);
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
bool GetNEVMData(BlockValidationState& state, const CBlock& block, CNEVMHeader &evmBlockHeader) {
    std::vector<unsigned char> vchData;
	int nOut;
	if (!GetSyscoinData(*block.vtx[0], vchData, nOut))
		return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "nevm-block-data-output");
    auto pos = std::search(vchData.begin(), vchData.end(), std::begin(NEVM_MAGIC_BYTES), std::end(NEVM_MAGIC_BYTES));
    if(pos == vchData.end() )
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "nevm-block-tag");
    pos = std::next(pos, sizeof(NEVM_MAGIC_BYTES));
    vchData = std::vector<unsigned char>(pos, pos+sizeof(CNEVMHeader));
    CDataStream ds(vchData, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ds >> evmBlockHeader;
    } catch (std::exception& e) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "nevm-block-unserialize");
    }
    return true;
}
bool Chainstate::ConnectNEVMCommitment(BlockValidationState& state, NEVMTxRootMap &mapNEVMTxRoots, const CBlock& block, const uint256& nBlockHash, const uint32_t& nHeight, const bool fJustCheck, PoDAMAPMemory &mapPoDA) {
    CNEVMHeader nevmBlockHeader;
    if(!GetNEVMData(state, block, nevmBlockHeader)) {
        return false; //state filled by GetNEVMData 
    }
    if(block.vchNEVMBlockData.empty()) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "nevm-block-empty");
    }
    // convert into vector of VH's because it directly gets serialized to buffer for NEVM processing
    NEVMDataVec NEVMDataVecOut;
    for (auto const& [key, val] : mapPoDA) {
        NEVMDataVecOut.emplace_back(key);
    }
    bool bSkipValidation = nHeight <= nLastKnownHeightOnStart;
    if(bSkipValidation) {
        LogPrintf("ConnectNEVMCommitment: skipping validation result...\n");
    }
    std::string stateStr;
    if(fNEVMConnection) {
        GetMainSignals().NotifyNEVMBlockConnect(nevmBlockHeader, block, stateStr, fJustCheck? uint256(): nBlockHash, NEVMDataVecOut, nHeight, bSkipValidation);
        if(!stateStr.empty()) {
            state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, stateStr);
            if(stateStr == "nevm-connect-response-invalid-data" || stateStr == "nevm-response-not-found") {
                // if exitwhensynced is set on geth we likely have shutdown the geth node so we should also shut syscoin down here
                const std::vector<std::string> &cmdLine = gArgs.GetArgs("-gethcommandline");
                if(std::find(cmdLine.begin(), cmdLine.end(), "--exitwhensynced") != cmdLine.end()) {
                    StartShutdown();
                    return true;
                }
            }
        }
    }
    bool res = state.IsValid();
    // try to bring connection back alive if its not connected for some reason
    if(!res) {
        if(state.GetRejectReason() == "nevm-connect-not-sent") {
            bool bResponse = false;
            GetMainSignals().NotifyNEVMComms("status", bResponse);
            if(!bResponse) {
                if(RestartGethNode()) {
                    // try again after resetting connection
                    GetMainSignals().NotifyNEVMBlockConnect(nevmBlockHeader, block, stateStr, fJustCheck? uint256(): nBlockHash, NEVMDataVecOut, nHeight, bSkipValidation);
                    if(!stateStr.empty()) {
                        state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, stateStr);
                        if(stateStr == "nevm-connect-response-invalid-data" || stateStr == "nevm-response-not-found") {
                            // if exitwhensynced is set on geth we likely have shutdown the geth node so we should also shut syscoin down here
                            const std::vector<std::string> &cmdLine = gArgs.GetArgs("-gethcommandline");
                            if(std::find(cmdLine.begin(), cmdLine.end(), "--exitwhensynced") != cmdLine.end()) {
                                StartShutdown();
                                return true;
                            }
                        }
                    }
                    res = state.IsValid();
                }
            }
        }
    }
    if(res && !fJustCheck) {
        NEVMTxRoot txRootDB;
        txRootDB.nTxRoot = nevmBlockHeader.nTxRoot;
        txRootDB.nReceiptRoot = nevmBlockHeader.nReceiptRoot;
        mapNEVMTxRoots.try_emplace(nevmBlockHeader.nBlockHash, txRootDB);
    }


    return res;
}
bool DisconnectNEVMCommitment(BlockValidationState& state, std::vector<uint256> &vecNEVMBlocks, const CBlock& block, const uint256& nBlockHash) {
    CNEVMHeader evmBlock;
    if(!GetNEVMData(state, block, evmBlock)) {
        return false; // state filled by GetNEVMData
    }
    if(fNEVMConnection) {
        std::string stateStr;
        GetMainSignals().NotifyNEVMBlockDisconnect(stateStr, nBlockHash);
        if(!stateStr.empty()) {
            state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, stateStr);
        }
    }
    bool res = state.IsValid() || !fNEVMConnection;
    if(res) {
        vecNEVMBlocks.emplace_back(evmBlock.nBlockHash);
    }
    return res;
}
// before propagating blocks/txs out to peers we need to fill the OPRETURN with the NEVM DA payload from separate store
bool FillNEVMData(const CTransactionRef &tx) {
    if(!tx->IsNEVMData()) {
        return true;
    }
    const auto &nOut = GetSyscoinDataOutput(*tx);
    if (nOut == -1) {
        return false;
    }
    // uncast nevmdata, its safe because its not used anywhere else
    std::vector<uint8_t> &vchNEVMData = const_cast<std::vector<uint8_t>&>(tx->vout[nOut].vchNEVMData);
    CNEVMData nevmData(tx->vout[nOut].scriptPubKey);
    if(nevmData.IsNull()) {
        return false;
    }
    // vout already has payload
    if(!tx->vout[nOut].vchNEVMData.empty()) {
        return true;
    }
    // data doesn't have to exist but if it does fill it
    pnevmdatadb->ReadData(nevmData.vchVersionHash, vchNEVMData);
    return true; 
}
bool EraseNEVMData(const NEVMDataVec &NEVMDataVecOut) {
    if(!NEVMDataVecOut.empty()) {
        return pnevmdatadb->FlushErase(NEVMDataVecOut);
    }
    return true;
}
class CBlobCheck
{
private:
    const CNEVMData* nevmData;
public:
    CBlobCheck() {}
    CBlobCheck(const CNEVMData* nevmDataIn) :
        nevmData(nevmDataIn)  { };

    bool operator()() noexcept {
        return nevmData->vchVersionHash == dev::sha3(*nevmData->vchNEVMData).asBytes();
    }

    void swap(CBlobCheck& check) noexcept
    {
        std::swap(nevmData, check.nevmData);
    }
};
static CCheckQueue<CBlobCheck> blobcheckqueue(MAX_DATA_BLOBS);
bool ProcessNEVMDataHelper(const BlockManager& blockman, const std::vector<CNEVMData> &vecNevmDataPayload, const int64_t &nMedianTime, const int64_t &nTimeNow, PoDAMAPMemory &mapPoDA) { 
    int64_t nMedianTimeCL = 0;
    if(llmq::chainLocksHandler) {
        // use previous chainlock because chain can technically rollback up to previous lock
        const CBlockIndex* prevCLIndex = llmq::chainLocksHandler->GetPreviousChainLock();
        if (prevCLIndex) {
            nMedianTimeCL = prevCLIndex->GetMedianTimePast();
        }
    }
    // first sanity test times to ensure data should or shouldn't exist and save to another vector
    CCheckQueueControl<CBlobCheck> control(&blobcheckqueue);
    std::vector<CBlobCheck> vChecks;
    for (const auto &nevmDataPayload : vecNevmDataPayload) {
        // if connecting block is over NEVM_DATA_ENFORCE_TIME_NOT_HAVE_DATA seconds old (median) and we have a chainlock less than NEVM_DATA_ENFORCE_TIME_HAVE_DATA seconds old (median)
        const bool enforceNotHaveData = nMedianTimeCL > 0 && nMedianTime < (nTimeNow - NEVM_DATA_ENFORCE_TIME_NOT_HAVE_DATA) && nMedianTimeCL >= (nTimeNow - NEVM_DATA_ENFORCE_TIME_HAVE_DATA);
        const bool enforceHaveData = nMedianTime >= (nTimeNow - NEVM_DATA_ENFORCE_TIME_HAVE_DATA);
        if(enforceHaveData && (!nevmDataPayload.vchNEVMData || nevmDataPayload.vchNEVMData->empty())) {
            LogPrint(BCLog::SYS, "ProcessNEVMDataHelper: Enforcing data but NEVM Data is empty nMedianTime %ld nTimeNow %ld NEVM_DATA_ENFORCE_TIME_NOT_HAVE_DATA %d\n", nMedianTime, nTimeNow, NEVM_DATA_ENFORCE_TIME_NOT_HAVE_DATA);
            return false;
        } else if(enforceNotHaveData && nevmDataPayload.vchNEVMData && !nevmDataPayload.vchNEVMData->empty()) {
            LogPrint(BCLog::SYS, "ProcessNEVMDataHelper: Enforcing no data but NEVM Data is not empty nMedianTime %ld nTimeNow %ld NEVM_DATA_ENFORCE_TIME_HAVE_DATA %d\n", nMedianTime, nTimeNow, NEVM_DATA_ENFORCE_TIME_HAVE_DATA);
            return false;
        }
        if(nevmDataPayload.vchNEVMData && !nevmDataPayload.vchNEVMData->empty() && !pnevmdatadb->BlobExists(nevmDataPayload.vchVersionHash)){
            vChecks.emplace_back(CBlobCheck(&nevmDataPayload));
        }
    }
    if(!vChecks.empty()) {
        // process new vector in batch checking the blobs
        BlockValidationState state;
        const auto time_1{SteadyClock::now()};
        control.Add(vChecks);
        if (!control.Wait()){
            LogPrint(BCLog::SYS, "ProcessNEVMDataHelper: Invalid blob(s)\n");
            return false;
        }
        const auto time_2{SteadyClock::now()};
        LogPrint(BCLog::BENCHMARK, "ProcessNEVMDataHelper: verified %d blobs in %.2fms (%.2fms/blob)\n", vChecks.size(), Ticks<MillisecondsDouble>(time_2 - time_1), Ticks<MillisecondsDouble>(time_2 - time_1) / vChecks.size());
    }
    for (const auto &nevmDataPayload : vecNevmDataPayload) {
        mapPoDA.try_emplace(nevmDataPayload.vchVersionHash, nevmDataPayload.vchNEVMData);
    }
    return true;
}
// when we receive blocks/txs from peers we need to strip the OPRETURN NEVM DA payload and store separately
bool ProcessNEVMData(const BlockManager& blockman, const CBlock &block, const int64_t &nMedianTime, const int64_t& nTimeNow, PoDAMAPMemory &mapPoDA) {
    std::vector<CNEVMData> vecNevmDataPayload;
    int nCountBlobs = 0;
    for (auto &tx : block.vtx) {
        if(tx->IsNEVMData()) {
            nCountBlobs++;
            if(nCountBlobs > MAX_DATA_BLOBS) {
                LogPrintf("ProcessNEVMData nCountBlobs > MAX_DATA_BLOBS, nCountBlobs: %d\n", nCountBlobs);
                return false;
            }
            const CNEVMData nevmDataPayload(*tx);
            if(nevmDataPayload.IsNull()) {
                return false;
            }
            vecNevmDataPayload.emplace_back(nevmDataPayload);
        }
    }
    if(!vecNevmDataPayload.empty() && !ProcessNEVMDataHelper(blockman, vecNevmDataPayload, nMedianTime, nTimeNow, mapPoDA)) {
        return false;
    }
    return true;
}
bool ProcessNEVMData(const BlockManager& blockman, const CTransaction& tx, const int64_t &nMedianTime, const int64_t& nTimeNow, PoDAMAPMemory &mapPoDA) {
    if(!tx.IsNEVMData()) {
        return true;
    }
    const CNEVMData nevmDataPayload(tx);
    if(nevmDataPayload.IsNull()) {
        return false;
    }
    std::vector<CNEVMData> vecPayload{nevmDataPayload};
    if(!ProcessNEVMDataHelper(blockman, vecPayload, nMedianTime, nTimeNow, mapPoDA)) {
        return false;
    }
    return true;
}
/** Undo the effects of this block (with given index) on the UTXO set represented by coins.
 *  When FAILED is returned, view is left in an indeterminate state. */
// SYSCOIN
DisconnectResult Chainstate::DisconnectBlock(const CBlock& block, const CBlockIndex* pindex, CCoinsViewCache& view, AssetMap &mapAssets, NEVMMintTxMap &mapMintKeys, NEVMDataVec &NEVMDataVecOut, std::vector<uint256> &vecNEVMBlocks, std::vector<std::pair<uint256, uint32_t> > &vecTXIDPairs, bool bReverify)
{
    AssertLockHeld(::cs_main);
    // SYSCOIN
    const auto& params = Params().GetConsensus();
    bool fDIP0003Active = pindex->nHeight >= params.DIP0003Height;
    if (fDIP0003Active && !evoDb->VerifyBestBlock(pindex->GetBlockHash())) {
        // Nodes that upgraded after DIP3 activation will have to reindex to ensure evodb consistency
        AbortNode("Found EvoDB inconsistency, you must reindex to continue");
        return DISCONNECT_FAILED;
    }
    bool fClean = true;

    CBlockUndo blockUndo;
    if (!UndoReadFromDisk(blockUndo, pindex)) {
        error("DisconnectBlock(): failure reading undo data");
        return DISCONNECT_FAILED;
    }

    if (blockUndo.vtxundo.size() + 1 != block.vtx.size()) {
        error("DisconnectBlock(): block and undo data inconsistent");
        return DISCONNECT_FAILED;
    }
    // SYSCOIN
    if (!UndoSpecialTxsInBlock(block, pindex)) {
        return DISCONNECT_FAILED;
    }

    // Ignore blocks that contain transactions which are 'overwritten' by later transactions,
    // unless those are already completely spent.
    // See https://github.com/bitcoin/bitcoin/issues/22596 for additional information.
    // Note: the blocks specified here are different than the ones used in ConnectBlock because DisconnectBlock
    // unwinds the blocks in reverse. As a result, the inconsistency is not discovered until the earlier
    // blocks with the duplicate coinbase transactions are disconnected.
    /*bool fEnforceBIP30 = !((pindex->nHeight==91722 && pindex->GetBlockHash() == uint256S("0x00000000000271a2dc26e7667f8419f2e15416dc6955e5a6c6cdf3f2574dd08e")) ||
                           (pindex->nHeight==91812 && pindex->GetBlockHash() == uint256S("0x00000000000af0aed4792b1acee3d966af36cf5def14935db8de83d6f9306f2f")));*/

    // undo transactions in reverse order
    for (int i = block.vtx.size() - 1; i >= 0; i--) {
        const CTransaction &tx = *(block.vtx[i]);
        // SYSCOIN
        const uint256 &hash = tx.GetHash();
        const bool &is_coinbase = tx.IsCoinBase();

        // Check that all outputs are available and match the outputs in the block itself
        // exactly.
        for (size_t o = 0; o < tx.vout.size(); o++) {
            if (!tx.vout[o].scriptPubKey.IsUnspendable()) {
                COutPoint out(hash, o);
                Coin coin;
                bool is_spent = view.SpendCoin(out, &coin);
                if (!is_spent || tx.vout[o] != coin.out || pindex->nHeight != coin.nHeight || is_coinbase != coin.fCoinBase) {
                    fClean = false; // transaction output mismatch
                }
            }
        }
        // SYSCOIN	
		vecTXIDPairs.emplace_back(std::make_pair(hash, pindex->nHeight));
        // restore inputs
        if (i > 0) { // not coinbases
            CTxUndo &txundo = blockUndo.vtxundo[i-1];
            if (txundo.vprevout.size() != tx.vin.size()) {
                error("DisconnectBlock(): transaction and undo data inconsistent");
                return DISCONNECT_FAILED;
            }
            // SYSCOIN
            if(passetdb != nullptr && !DisconnectSyscoinTransaction(tx, hash, txundo, view, mapAssets, mapMintKeys, NEVMDataVecOut))
                fClean = false;
                
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
    BlockValidationState state;
    bool bRegTestContext = !fRegTest || (fRegTest && fNEVMConnection);
    if(bRegTestContext && bReverify && pindex->nHeight >= params.nNEVMStartBlock && !DisconnectNEVMCommitment(state, vecNEVMBlocks, block, block.GetHash())) {
        const std::string &errStr = strprintf("DisconnectBlock(): NEVM block failed to disconnect: %s\n", state.ToString().c_str());
        error(errStr.c_str());
        return DISCONNECT_FAILED; 
    }
    // move best block pointer to prevout block
    view.SetBestBlock(pindex->pprev->GetBlockHash());
    // SYSCOIN
    evoDb->WriteBestBlock(pindex->pprev->GetBlockHash());
    return fClean ? DISCONNECT_OK : DISCONNECT_UNCLEAN;
}

static CCheckQueue<CScriptCheck> scriptcheckqueue(128);

void StartScriptCheckWorkerThreads(int threads_num)
{
    scriptcheckqueue.StartWorkerThreads(threads_num);
    blobcheckqueue.StartWorkerThreads(threads_num);
}

void StopScriptCheckWorkerThreads()
{
    scriptcheckqueue.StopWorkerThreads();
    blobcheckqueue.StopWorkerThreads();
}

// SYSCOIN
bool GetBlockHash(ChainstateManager& chainman, uint256& hashRet, int nBlockHeight)
{
    LOCK(cs_main);
    if(chainman.ActiveTip() == nullptr) return false;
    if(nBlockHeight < -1 || nBlockHeight > chainman.ActiveHeight()) return false;
    if(nBlockHeight == -1) nBlockHeight = chainman.ActiveHeight();
    hashRet = chainman.ActiveChain()[nBlockHeight]->GetBlockHash();
    return true;
}
/**
 * Threshold condition checker that triggers when unknown versionbits are seen on the network.
 */
class WarningBitsConditionChecker : public AbstractThresholdConditionChecker
{
private:
    const ChainstateManager& m_chainman;
    int m_bit;

public:
    explicit WarningBitsConditionChecker(const ChainstateManager& chainman, int bit) : m_chainman{chainman}, m_bit(bit) {}

    int64_t BeginTime(const Consensus::Params& params) const override { return 0; }
    int64_t EndTime(const Consensus::Params& params) const override { return std::numeric_limits<int64_t>::max(); }
    int Period(const Consensus::Params& params) const override { return params.nMinerConfirmationWindow; }
    int Threshold(const Consensus::Params& params) const override { return params.nRuleChangeActivationThreshold; }

    bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const override
    {
        // SYSCOIN
        return m_bit != 19 && m_bit != 28 && m_bit != 7 && pindex->nHeight >= params.MinBIP9WarningHeight &&
               ((pindex->nVersion & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS) &&
               ((pindex->GetBaseVersion() >> m_bit) & 1) != 0 &&
               ((m_chainman.m_versionbitscache.ComputeBlockVersion(pindex->pprev, params) >> m_bit) & 1) == 0;
    }
};

static std::array<ThresholdConditionCache, VERSIONBITS_NUM_BITS> warningcache GUARDED_BY(cs_main);

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


static SteadyClock::duration time_check{};
static SteadyClock::duration time_forks{};
static SteadyClock::duration time_connect{};
static SteadyClock::duration time_verify{};
static SteadyClock::duration time_undo{};
static SteadyClock::duration time_index{};
static SteadyClock::duration time_total{};
static int64_t num_blocks_total = 0;
// SYSCOIN
bool Chainstate::ConnectBlock(const CBlock& block, BlockValidationState& state, CBlockIndex* pindex,
                  CCoinsViewCache& view, bool fJustCheck, bool bReverify) {
    AssetMap mapAssets;
    NEVMMintTxMap mapMintKeys;
    NEVMTxRootMap mapNEVMTxRoots;
    PoDAMAPMemory mapPoDA;
    std::vector<std::pair<uint256, uint32_t> > vecTXIDPairs;
    return ConnectBlock(block, state, pindex, view, fJustCheck, mapAssets, mapMintKeys, mapNEVMTxRoots, mapPoDA, vecTXIDPairs, bReverify);       
}
/** Apply the effects of this block (with given index) on the UTXO set represented by coins.
 *  Validity checks that depend on the UTXO set are also done; ConnectBlock()
 *  can fail if those validity checks fail (among other reasons). */
bool Chainstate::ConnectBlock(const CBlock& block, BlockValidationState& state, CBlockIndex* pindex,
                  CCoinsViewCache& view, bool fJustCheck, 
                  AssetMap &mapAssets, NEVMMintTxMap &mapMintKeys, NEVMTxRootMap &mapNEVMTxRoots, PoDAMAPMemory &mapPoDA, std::vector<std::pair<uint256, uint32_t> > &vecTXIDPairs, bool bReverify)
{
    AssertLockHeld(cs_main);
    assert(pindex);

    uint256 block_hash{block.GetHash()};
    assert(*pindex->phashBlock == block_hash);
    const bool parallel_script_checks{scriptcheckqueue.HasThreads()};

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
    // m_adjusted_time_callback() to go backward).
    if (!CheckBlock(block, state, params.GetConsensus(), !fJustCheck, !fJustCheck)) {
        if (state.GetResult() == BlockValidationResult::BLOCK_MUTATED) {
            // We don't write down blocks to disk if they may have been
            // corrupted, so this should be impossible unless we're having hardware
            // problems.
            return AbortNode(state, "Corrupt block found indicating potential hardware failure; shutting down");
        }
        return error("%s: Consensus::CheckBlock: %s", __func__, state.ToString());
    }

    // verify that the view's current state corresponds to the previous block
    uint256 hashPrevBlock = pindex->pprev == nullptr ? uint256() : pindex->pprev->GetBlockHash();
    assert(hashPrevBlock == view.GetBestBlock());
    // SYSCOIN
    if (pindex->pprev) {
        bool fDIP0003Active = pindex->nHeight >= Params().GetConsensus().DIP0003Height;
        if (fDIP0003Active && !evoDb->VerifyBestBlock(pindex->pprev->GetBlockHash())) {
            // Nodes that upgraded after DIP3 activation will have to reindex to ensure evodb consistency
            return AbortNode(state, "Found EvoDB inconsistency, you must reindex to continue");
        }
    }
    num_blocks_total++;

    // Special case for the genesis block, skipping connection of its transactions
    // (its coinbase is unspendable)
    if (block_hash == params.GetConsensus().hashGenesisBlock) {
        if (!fJustCheck) {
            view.SetBestBlock(pindex->GetBlockHash());
        }
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
                // SYSCOIN
                int64_t timeForWork = 1209600;
                if(fRegTest)
                    timeForWork /= 10;
                fScriptChecks = (GetBlockProofEquivalentTime(*m_chainman.m_best_header, *pindex, *m_chainman.m_best_header, params.GetConsensus()) <= timeForWork);
            }
        }
    }

    const auto time_1{SteadyClock::now()};
    time_check += time_1 - time_start;
    LogPrint(BCLog::BENCHMARK, "    - Sanity checks: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_1 - time_start),
             Ticks<SecondsDouble>(time_check),
             Ticks<MillisecondsDouble>(time_check) / num_blocks_total);

    // Do not allow blocks that contain transactions which 'overwrite' older transactions,
    // unless those are already completely spent.
    // If such overwrites are allowed, coinbases and transactions depending upon those
    // can be duplicated to remove the ability to spend the first instance -- even after
    // being sent to another address.
    // See BIP30, CVE-2012-1909, and http://r6.ca/blog/20120206T005236Z.html for more information.
    // This rule was originally applied to all blocks with a timestamp after March 15, 2012, 0:00 UTC.
    // Now that the whole chain is irreversibly beyond that time it is applied to all blocks except the
    // two in the chain that violate it. This prevents exploiting the issue against nodes during their
    // initial block download.
    // bool fEnforceBIP30 = !((pindex->nHeight==91842 && pindex->GetBlockHash() == uint256S("0x00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec")) ||
                           // (pindex->nHeight==91880 && pindex->GetBlockHash() == uint256S("0x00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721")));

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
    // static constexpr int BIP34_IMPLIES_BIP30_LIMIT = 1983702;

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
    // CBlockIndex* pindexBIP34height = pindex->pprev->GetAncestor(m_params.GetConsensus().BIP34Height);
    //Only continue to enforce if we're below BIP34 activation height or the block hash at that height doesn't correspond.
    // fEnforceBIP30 = fEnforceBIP30 && (!pindexBIP34height || !(pindexBIP34height->GetBlockHash() == m_params.GetConsensus().BIP34Hash));

    // TODO: Remove BIP30 checking from block height 1,983,702 on, once we have a
    // consensus change that ensures coinbases at those heights cannot
    // duplicate earlier coinbases.
    // if (fEnforceBIP30 || pindex->nHeight >= BIP34_IMPLIES_BIP30_LIMIT) {
        for (const auto& tx : block.vtx) {
            for (size_t o = 0; o < tx->vout.size(); o++) {
                if (view.HaveCoin(COutPoint(tx->GetHash(), o))) {
                    LogPrintf("ERROR: ConnectBlock(): tried to overwrite transaction\n");
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-txns-BIP30");
                }
            }
        }
    //}

    // Enforce BIP68 (sequence locks)
    int nLockTimeFlags = 0;
    if (DeploymentActiveAt(*pindex, m_chainman, Consensus::DEPLOYMENT_CSV)) {
        nLockTimeFlags |= LOCKTIME_VERIFY_SEQUENCE;
    }

    // Get the script flags for this block
    unsigned int flags{GetBlockScriptFlags(*pindex, m_chainman)};

    const auto time_2{SteadyClock::now()};
    time_forks += time_2 - time_1;
    LogPrint(BCLog::BENCHMARK, "    - Fork checks: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_2 - time_1),
             Ticks<SecondsDouble>(time_forks),
             Ticks<MillisecondsDouble>(time_forks) / num_blocks_total);

    CBlockUndo blockundo;

    // Precomputed transaction data pointers must not be invalidated
    // until after `control` has run the script checks (potentially
    // in multiple threads). Preallocate the vector size so a new allocation
    // doesn't invalidate pointers into the vector, and keep txsdata in scope
    // for as long as `control`.
    CCheckQueueControl<CScriptCheck> control(fScriptChecks && parallel_script_checks ? &scriptcheckqueue : nullptr);
    std::vector<PrecomputedTransactionData> txsdata(block.vtx.size());

    std::vector<int> prevheights;
    CAmount nFees = 0;
    int nInputs = 0;
    int64_t nSigOpsCost = 0;
    blockundo.vtxundo.reserve(block.vtx.size() - 1);
    // SYSCOIN
    const bool ibd = IsInitialBlockDownload();
    const uint256& blockHash = block.GetHash();
    // MUST process special txes before updating UTXO to ensure consistency between mempool and block processing
    if (!ProcessSpecialTxsInBlock(m_blockman, block, pindex, state, view, fJustCheck, fScriptChecks)) {
        LogPrintf("ERROR: ConnectBlock(): ProcessSpecialTxsInBlock for block %s failed with %s\n",
                     pindex->GetBlockHash().ToString(), state.ToString());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-process-mn");
    }
    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction &tx = *(block.vtx[i]);

        nInputs += tx.vin.size();
        // SYSCOIN
        const uint256& txHash = tx.GetHash(); 
        const bool &isCoinBase = tx.IsCoinBase();
        const bool &hasAssets = tx.HasAssets();
        vecTXIDPairs.emplace_back(txHash, pindex->nHeight);
        if (!isCoinBase)
        {
            TxValidationState tx_state;
            CAmount txfee = 0;
            // SYSCOIN
            CAssetsMap mapAssetIn;
            CAssetsMap mapAssetOut;
            if (!Consensus::CheckTxInputs(tx, tx_state, view, pindex->nHeight, txfee, mapAssetIn, mapAssetOut)) {
                // Any transaction validation failure in ConnectBlock is a block consensus failure
                state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                            tx_state.GetRejectReason(), tx_state.GetDebugMessage());
                return error("%s: Consensus::CheckTxInputs: %s, %s", __func__, tx.GetHash().ToString(), state.ToString());
            }
            // SYSCOIN
            if(hasAssets){
                TxValidationState tx_statesys;
                // just temp var not used in !fJustCheck mode
                if (!CheckSyscoinInputs(ibd, params.GetConsensus(), tx, txHash, tx_statesys, false, (uint32_t)pindex->nHeight, m_chain.Tip()->GetMedianTimePast(), blockHash, fJustCheck, mapAssets, mapMintKeys, mapAssetIn, mapAssetOut)){
                    // Any transaction validation failure in ConnectBlock is a block consensus failure
                    state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                                tx_statesys.GetRejectReason(), tx_statesys.GetDebugMessage());
                    return error("%s: Consensus::CheckSyscoinInputs: %s, %s", __func__, tx.GetHash().ToString(), state.ToString());
                }
            }
            nFees += txfee;
            if (!MoneyRange(nFees)) {
                LogPrintf("ERROR: %s: accumulated fee in the block out of range.\n", __func__);
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-txns-accumulated-fee-outofrange");
            }

            // Check that transaction is BIP68 final
            // BIP68 lock checks (as opposed to nLockTime checks) must
            // be in ConnectBlock because they require the UTXO set
            prevheights.resize(tx.vin.size());
            for (size_t j = 0; j < tx.vin.size(); j++) {
                prevheights[j] = view.AccessCoin(tx.vin[j].prevout).nHeight;
            }

            if (!SequenceLocks(tx, nLockTimeFlags, prevheights, *pindex)) {
                LogPrintf("ERROR: %s: contains a non-BIP68-final transaction\n", __func__);
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-txns-nonfinal");
            }
        }

        // GetTransactionSigOpCost counts 3 types of sigops:
        // * legacy (always)
        // * p2sh (when P2SH enabled in flags and excludes coinbase)
        // * witness (when witness enabled in flags and excludes coinbase)
        // SYSCOIN
        nSigOpsCost += GetTransactionSigOpCost(tx, view, flags);
        if (nSigOpsCost > MAX_BLOCK_SIGOPS_COST) {
            LogPrintf("ERROR: ConnectBlock(): too many sigops\n");
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-blk-sigops");
        }

        if (!tx.IsCoinBase())
        {
            std::vector<CScriptCheck> vChecks;
            bool fCacheResults = fJustCheck; /* Don't cache results if we're actually connecting blocks (still consult the cache, though) */
            TxValidationState tx_state;
            if (fScriptChecks && !CheckInputScripts(tx, tx_state, view, flags, fCacheResults, fCacheResults, txsdata[i], parallel_script_checks ? &vChecks : nullptr)) {
                // Any transaction validation failure in ConnectBlock is a block consensus failure
                state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                              tx_state.GetRejectReason(), tx_state.GetDebugMessage());
                return error("ConnectBlock(): CheckInputScripts on %s failed with %s",
                    tx.GetHash().ToString(), state.ToString());
            }
            control.Add(vChecks);
        }
        CTxUndo undoDummy;
        if (i > 0) {
            blockundo.vtxundo.push_back(CTxUndo());
        }
        UpdateCoins(tx, view, i == 0 ? undoDummy : blockundo.vtxundo.back(), pindex->nHeight);
    }
    bool PODAContext = pindex->nHeight >= params.GetConsensus().nPODAStartBlock;
    if(PODAContext && !ProcessNEVMData(m_chainman.m_blockman, block, m_chain.Tip()->GetMedianTimePast(), TicksSinceEpoch<std::chrono::seconds>(m_chainman.m_options.adjusted_time_callback()), mapPoDA)) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "poda-validation-failed");
    }
    bool bRegTestContext = !fRegTest || (fRegTest && fNEVMConnection);
    if (bRegTestContext && bReverify && pindex->nHeight >= params.GetConsensus().nNEVMStartBlock && !ConnectNEVMCommitment(state, mapNEVMTxRoots, block, blockHash, (uint32_t)pindex->nHeight, fJustCheck, mapPoDA)) {
        return false; // state filled by ConnectNEVMCommitment
    }
    const auto time_3{SteadyClock::now()};
    time_connect += time_3 - time_2;
    LogPrint(BCLog::BENCHMARK, "      - Connect %u transactions: %.2fms (%.3fms/tx, %.3fms/txin) [%.2fs (%.2fms/blk)]\n", (unsigned)block.vtx.size(),
             Ticks<MillisecondsDouble>(time_3 - time_2), Ticks<MillisecondsDouble>(time_3 - time_2) / block.vtx.size(),
             nInputs <= 1 ? 0 : Ticks<MillisecondsDouble>(time_3 - time_2) / (nInputs - 1),
             Ticks<SecondsDouble>(time_connect),
             Ticks<MillisecondsDouble>(time_connect) / num_blocks_total);


    if (!control.Wait()){
        LogPrintf("ERROR: %s: CheckQueue failed\n", __func__);
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "block-validation-failed");
    }
    // SYSCOIN : MODIFIED TO CHECK MASTERNODE PAYMENTS AND SUPERBLOCKS
    const CAmount &blockReward = GetBlockSubsidy(pindex->nHeight, params.GetConsensus());
    CAmount nMNSeniorityRet = 0;
    CAmount nMNFloorDiffRet = 0;
    // detect MN was paid properly, accounting for seniority which is added to subsidy
    if (!IsBlockPayeeValid(m_chain, *block.vtx[0], pindex->nHeight, blockReward, nFees, nMNSeniorityRet, nMNFloorDiffRet)) {
        LogPrintf("ERROR: ConnectBlock(): couldn't find masternode or superblock payments\n");
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-payee");
    }

    std::string strError;
    // add seniority to reward when checking for limit
    if (!fTestSetting && !IsBlockValueValid(block, pindex->nHeight, blockReward+nFees+nMNSeniorityRet+nMNFloorDiffRet, strError) && (fRegTest || pindex->nHeight >= params.GetConsensus().DIP0003EnforcementHeight)) {
        LogPrintf("ERROR: ConnectBlock(): coinbase pays too much (actual=%lld vs limit=%lld)\n", block.vtx[0]->GetValueOut(), blockReward+nFees+nMNSeniorityRet+nMNFloorDiffRet);
        // hack for feature_signet.py to pass which uses bitcoin blocks signed by the signet witness
        if(!fSigNet || pindex->nHeight > 100) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-amount");
        }
    }
    if (pindex->pprev && pindex->phashBlock && llmq::chainLocksHandler->HasConflictingChainLock(pindex->nHeight, pindex->GetBlockHash())) {
        return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "bad-chainlock");
    }
    // END SYSCOIN
    const auto time_4{SteadyClock::now()};
    time_verify += time_4 - time_2;
    LogPrint(BCLog::BENCHMARK, "    - Verify %u txins: %.2fms (%.3fms/txin) [%.2fs (%.2fms/blk)]\n", nInputs - 1,
             Ticks<MillisecondsDouble>(time_4 - time_2),
             nInputs <= 1 ? 0 : Ticks<MillisecondsDouble>(time_4 - time_2) / (nInputs - 1),
             Ticks<SecondsDouble>(time_verify),
             Ticks<MillisecondsDouble>(time_verify) / num_blocks_total);
    if (fJustCheck)
        return true;

    if (!m_blockman.WriteUndoDataForBlock(blockundo, state, pindex, params)) {
        return false;
    }

    const auto time_5{SteadyClock::now()};
    time_undo += time_5 - time_4;
    LogPrint(BCLog::BENCHMARK, "    - Write undo data: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_5 - time_4),
             Ticks<SecondsDouble>(time_undo),
             Ticks<MillisecondsDouble>(time_undo) / num_blocks_total);

    if (!pindex->IsValid(BLOCK_VALID_SCRIPTS)) {
        pindex->RaiseValidity(BLOCK_VALID_SCRIPTS);
        m_blockman.m_dirty_blockindex.insert(pindex);
    }

    // add this block to the view's block chain
    view.SetBestBlock(pindex->GetBlockHash());
    // SYSCOIN
    evoDb->WriteBestBlock(pindex->GetBlockHash());
    const auto time_6{SteadyClock::now()};
    time_index += time_6 - time_5;
    LogPrint(BCLog::BENCHMARK, "    - Index writing: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_6 - time_5),
             Ticks<SecondsDouble>(time_index),
             Ticks<MillisecondsDouble>(time_index) / num_blocks_total);
    TRACE6(validation, block_connected,
        block_hash.data(),
        pindex->nHeight,
        block.vtx.size(),
        nInputs,
        nSigOpsCost,
        time_5 - time_start // in microseconds (µs)
    );

    return true;
}

CoinsCacheSizeState Chainstate::GetCoinsCacheSizeState()
{
    return this->GetCoinsCacheSizeState(
        m_coinstip_cache_size_bytes,
        m_mempool ? m_mempool->m_max_size_bytes : 0);
}

CoinsCacheSizeState Chainstate::GetCoinsCacheSizeState(
    size_t max_coins_cache_size_bytes,
    size_t max_mempool_size_bytes)
{
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
        bool fDoFullFlush = false;

        CoinsCacheSizeState cache_state = GetCoinsCacheSizeState();
        LOCK(m_blockman.cs_LastBlockFile);
        if (m_blockman.IsPruneMode() && (m_blockman.m_check_for_pruning || nManualPruneHeight > 0) && !fReindex) {
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
                LogPrint(BCLog::PRUNE, "%s limited pruning to height %d\n", limiting_lock.value(), last_prune);
            }

            if (nManualPruneHeight > 0) {
                LOG_TIME_MILLIS_WITH_CATEGORY("find files to prune (manual)", BCLog::BENCHMARK);

                m_blockman.FindFilesToPruneManual(setFilesToPrune, std::min(last_prune, nManualPruneHeight), m_chain.Height());
            } else {
                LOG_TIME_MILLIS_WITH_CATEGORY("find files to prune", BCLog::BENCHMARK);

                m_blockman.FindFilesToPrune(setFilesToPrune, m_chainman.GetParams().PruneAfterHeight(), m_chain.Height(), last_prune, IsInitialBlockDownload());
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
        const auto nNow{SteadyClock::now()};
        // Avoid writing/flushing immediately after startup.
        if (m_last_write == decltype(m_last_write){}) {
            m_last_write = nNow;
        }
        if (m_last_flush == decltype(m_last_flush){}) {
            m_last_flush = nNow;
        }
        // The cache is large and we're within 10% and 10 MiB of the limit, but we have time now (not in the middle of a block processing).
        bool fCacheLarge = mode == FlushStateMode::PERIODIC && cache_state >= CoinsCacheSizeState::LARGE;
        // The cache is over the limit, we have to write now.
        bool fCacheCritical = mode == FlushStateMode::IF_NEEDED && cache_state >= CoinsCacheSizeState::CRITICAL;
        // It's been a while since we wrote the block index to disk. Do this frequently, so we don't need to redownload after a crash.
        bool fPeriodicWrite = mode == FlushStateMode::PERIODIC && nNow > m_last_write + DATABASE_WRITE_INTERVAL;
        // It's been very long since we flushed the cache. Do this infrequently, to optimize cache usage.
        bool fPeriodicFlush = mode == FlushStateMode::PERIODIC && nNow > m_last_flush + DATABASE_FLUSH_INTERVAL;
        // SYSCOIN The evodb cache is too large
        bool fEvoDbCacheCritical = mode == FlushStateMode::IF_NEEDED && evoDb != nullptr && evoDb->GetMemoryUsage() >= (64 << 20);
        // Combine all conditions that result in a full cache flush.
        fDoFullFlush = (mode == FlushStateMode::ALWAYS) || fCacheLarge || fCacheCritical || fEvoDbCacheCritical || fPeriodicFlush || fFlushForPrune;
        // Write blocks and block index to disk.
        if (fDoFullFlush || fPeriodicWrite) {
            // Ensure we can write block index
            if (!CheckDiskSpace(gArgs.GetBlocksDirPath())) {
                return AbortNode(state, "Disk space is too low!", _("Disk space is too low!"));
            }
            {
                LOG_TIME_MILLIS_WITH_CATEGORY("write block and undo data to disk", BCLog::BENCHMARK);

                // First make sure all block and undo data is flushed to disk.
                m_blockman.FlushBlockFile();
            }

            // Then update all block file information (which may refer to block and undo files).
            {
                LOG_TIME_MILLIS_WITH_CATEGORY("write block index to disk", BCLog::BENCHMARK);

                if (!m_blockman.WriteBlockIndexDB()) {
                    return AbortNode(state, "Failed to write to block index database");
                }
            }
            // Finally remove any pruned files
            if (fFlushForPrune) {
                LOG_TIME_MILLIS_WITH_CATEGORY("unlink pruned files", BCLog::BENCHMARK);

                UnlinkPrunedFiles(setFilesToPrune);
            }
            // SYSCOIN
            if (pnevmdatadb && !pnevmdatadb->FlushCacheToDisk()) {
                return AbortNode(state, "Failed to commit PoDA");
            }
            if (pblockindexdb && !pblockindexdb->FlushCacheToDisk((uint32_t)m_chainman.ActiveHeight())) {
                return AbortNode(state, "Failed to commit to block index db");
            }
            if (pnevmtxrootsdb && !pnevmtxrootsdb->FlushCacheToDisk()) {
                return AbortNode(state, "Failed to commit to nevm tx roots db");
            }
            m_last_write = nNow;
        }
        // Flush best chain related state. This can only be done if the blocks / block index write was also done.
        if (fDoFullFlush && !CoinsTip().GetBestBlock().IsNull()) {
            LOG_TIME_MILLIS_WITH_CATEGORY(strprintf("write coins cache to disk (%d coins, %.2fkB)",
                coins_count, coins_mem_usage / 1000), BCLog::BENCHMARK);

            // Typical Coin structures on disk are around 48 bytes in size.
            // Pushing a new one to the database can cause it to be written
            // twice (once in the log, and once in the tables). This is already
            // an overestimation, as most will delete an existing entry or
            // overwrite one. Still, use a conservative safety factor of 2.
            if (!CheckDiskSpace(gArgs.GetDataDirNet(), 48 * 2 * 2 * CoinsTip().GetCacheSize())) {
                return AbortNode(state, "Disk space is too low!", _("Disk space is too low!"));
            }
            // Flush the chainstate (which may refer to block index entries).
            if (!CoinsTip().Flush())
                return AbortNode(state, "Failed to write to coin database");
            // SYSCOIN
            if (!evoDb->CommitRootTransaction()) {
                return AbortNode(state, "Failed to commit EvoDB");
            }
            m_last_flush = nNow;
            full_flush_completed = true;
            TRACE5(utxocache, flush,
                   int64_t{Ticks<std::chrono::microseconds>(SteadyClock::now() - nNow)},
                   (uint32_t)mode,
                   (uint64_t)coins_count,
                   (uint64_t)coins_mem_usage,
                   (bool)fFlushForPrune);
        }
    }
    if (full_flush_completed) {
        // Update best block in wallet (so we can detect restored wallets).
        GetMainSignals().ChainStateFlushed(m_chain.GetLocator());
    }
    } catch (const std::runtime_error& e) {
        return AbortNode(state, std::string("System error while flushing: ") + e.what());
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

static void DoWarning(const bilingual_str& warning)
{
    static bool fWarned = false;
    SetMiscWarning(warning);
    if (!fWarned) {
        AlertNotify(warning.original);
        fWarned = true;
    }
}

/** Private helper function that concatenates warning messages. */
static void AppendWarning(bilingual_str& res, const bilingual_str& warn)
{
    if (!res.empty()) res += Untranslated(", ");
    res += warn;
}

static void UpdateTipLog(
    const CCoinsViewCache& coins_tip,
    const CBlockIndex* tip,
    const CChainParams& params,
    const std::string& func_name,
    const std::string& prefix,
    const std::string& warning_messages) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{

    AssertLockHeld(::cs_main);
    LogPrintf("%s%s: new best=%s height=%d version=0x%08x log2_work=%f tx=%lu date='%s' progress=%f cache=%.1fMiB(%utxo)%s\n",
        prefix, func_name,
        tip->GetBlockHash().ToString(), tip->nHeight, tip->nVersion,
        log(tip->nChainWork.getdouble()) / log(2.0), (unsigned long)tip->nChainTx,
        FormatISO8601DateTime(tip->GetBlockTime()),
        GuessVerificationProgress(params.TxData(), tip),
        coins_tip.DynamicMemoryUsage() * (1.0 / (1 << 20)),
        coins_tip.GetCacheSize(),
        !warning_messages.empty() ? strprintf(" warning='%s'", warning_messages) : "");
}

void Chainstate::UpdateTip(const CBlockIndex* pindexNew)
{
    const auto& coins_tip = this->CoinsTip();

    const CChainParams& params{m_chainman.GetParams()};

    // The remainder of the function isn't relevant if we are not acting on
    // the active chainstate, so return if need be.
    if (this != &m_chainman.ActiveChainstate()) {
        // Only log every so often so that we don't bury log messages at the tip.
        constexpr int BACKGROUND_LOG_INTERVAL = 2000;
        if (pindexNew->nHeight % BACKGROUND_LOG_INTERVAL == 0) {
            UpdateTipLog(coins_tip, pindexNew, params, __func__, "[background validation] ", "");
        }
        return;
    }

    // New best block
    if (m_mempool) {
        m_mempool->AddTransactionsUpdated(1);
    }

    {
        LOCK(g_best_block_mutex);
        g_best_block = pindexNew->GetBlockHash();
        g_best_block_cv.notify_all();
    }

    bilingual_str warning_messages;
    if (!this->IsInitialBlockDownload()) {
        const CBlockIndex* pindex = pindexNew;
        for (int bit = 0; bit < VERSIONBITS_NUM_BITS; bit++) {
            WarningBitsConditionChecker checker(m_chainman, bit);
            ThresholdState state = checker.GetStateFor(pindex, params.GetConsensus(), warningcache.at(bit));
            if (state == ThresholdState::ACTIVE || state == ThresholdState::LOCKED_IN) {
                const bilingual_str warning = strprintf(_("Unknown new rules activated (versionbit %i)"), bit);
                if (state == ThresholdState::ACTIVE) {
                    DoWarning(warning);
                } else {
                    AppendWarning(warning_messages, warning);
                }
            }
        }
    }
    UpdateTipLog(coins_tip, pindexNew, params, __func__, "", warning_messages.original);
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
 // SYSCOIN
bool Chainstate::DisconnectTip(BlockValidationState& state, DisconnectedBlockTransactions* disconnectpool, bool bReverify)
{
    AssertLockHeld(cs_main);
    if (m_mempool) AssertLockHeld(m_mempool->cs);

    CBlockIndex *pindexDelete = m_chain.Tip();
    assert(pindexDelete);
    assert(pindexDelete->pprev);
    // Read block from disk.
    std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
    CBlock& block = *pblock;
    if (!ReadBlockFromDisk(block, pindexDelete, m_chainman.GetConsensus())) {
        return error("DisconnectTip(): Failed to read block");
    }
    // Apply the block atomically to the chain state.
    // SYSCOIN
    AssetMap mapAssets;
    NEVMMintTxMap mapMintKeys;
    NEVMDataVec NEVMDataVecOut;
    std::vector<uint256> vecNEVMBlocks;
    std::vector<std::pair<uint256,uint32_t> > vecTXIDPairs;
    const auto time_start{SteadyClock::now()};
    {
        // SYSCOIN
        auto dbTx = evoDb->BeginTransaction();
        CCoinsViewCache view(&CoinsTip());
        assert(view.GetBestBlock() == pindexDelete->GetBlockHash());
        if (DisconnectBlock(block, pindexDelete, view, mapAssets, mapMintKeys, NEVMDataVecOut, vecNEVMBlocks, vecTXIDPairs, bReverify) != DISCONNECT_OK)
            return error("DisconnectTip(): DisconnectBlock %s failed", pindexDelete->GetBlockHash().ToString());
        bool flushed = view.Flush();
        assert(flushed);
        // SYSCOIN
        dbTx->Commit();
    }
    // SYSCOIN 
    if(passetdb != nullptr){
        if(!passetdb->Flush(mapAssets) || !passetnftdb->Flush(mapAssets) || !pnevmtxmintdb->FlushErase(mapMintKeys) || !pnevmtxrootsdb->FlushErase(vecNEVMBlocks) || !pblockindexdb->FlushErase(vecTXIDPairs) || !pnevmdatadb->FlushEraseMTPs(NEVMDataVecOut)){
            return error("DisconnectTip(): Error flushing to asset dbs on disconnect %s", pindexDelete->GetBlockHash().ToString());
        }
    }
    LogPrint(BCLog::BENCHMARK, "- Disconnect block: %.2fms\n",
             Ticks<MillisecondsDouble>(SteadyClock::now() - time_start));

    {
        // Prune locks that began at or after the tip should be moved backward so they get a chance to reorg
        const int max_height_first{pindexDelete->nHeight - 1};
        for (auto& prune_lock : m_blockman.m_prune_locks) {
            if (prune_lock.second.height_first <= max_height_first) continue;

            prune_lock.second.height_first = max_height_first;
            LogPrint(BCLog::PRUNE, "%s prune lock moved back to %d\n", prune_lock.first, max_height_first);
        }
    }

    // Write the chain state to disk, if necessary.
    if (!FlushStateToDisk(state, FlushStateMode::IF_NEEDED)) {
        return false;
    }

    if (disconnectpool && m_mempool) {
        // Save transactions to re-add to mempool at end of reorg
        for (auto it = block.vtx.rbegin(); it != block.vtx.rend(); ++it) {
            disconnectpool->addTransaction(*it);
        }
        while (disconnectpool->DynamicMemoryUsage() > MAX_DISCONNECTED_TX_POOL_SIZE * 1000) {
            // Drop the earliest entry, and remove its children from the mempool.
            auto it = disconnectpool->queuedTx.get<insertion_order>().begin();
            m_mempool->removeRecursive(**it, MemPoolRemovalReason::REORG);
            disconnectpool->removeEntry(it);
        }
    }

    m_chain.SetTip(*pindexDelete->pprev);

    UpdateTip(pindexDelete->pprev);
    // Let wallets know transactions went from 1-confirmed to
    // 0-confirmed or conflicted:
    GetMainSignals().BlockDisconnected(pblock, pindexDelete);
    return true;
}

static SteadyClock::duration time_read_from_disk_total{};
static SteadyClock::duration time_connect_total{};
static SteadyClock::duration time_flush{};
static SteadyClock::duration time_chainstate{};
static SteadyClock::duration time_post_connect{};

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
// SYSCOIN
bool Chainstate::ConnectTip(BlockValidationState& state, CBlockIndex* pindexNew, const std::shared_ptr<const CBlock>& pblock, ConnectTrace& connectTrace, DisconnectedBlockTransactions &disconnectpool)
{
    AssertLockHeld(cs_main);
    if (m_mempool) AssertLockHeld(m_mempool->cs);

    assert(pindexNew->pprev == m_chain.Tip());
    // Read block from disk.
    const auto time_1{SteadyClock::now()};
    std::shared_ptr<const CBlock> pthisBlock;
    
    if (!pblock) {
        std::shared_ptr<CBlock> pblockNew = std::make_shared<CBlock>();
        if (!ReadBlockFromDisk(*pblockNew, pindexNew, m_chainman.GetConsensus())) {
            return AbortNode(state, "Failed to read block");
        }
        pthisBlock = pblockNew;
    } else {
        LogPrint(BCLog::BENCHMARK, "  - Using cached block\n");
        pthisBlock = pblock;
    }
    const CBlock& blockConnecting = *pthisBlock;
    // Apply the block atomically to the chain state.
    const auto time_2{SteadyClock::now()};
    time_read_from_disk_total += time_2 - time_1;
    SteadyClock::time_point time_3;
    LogPrint(BCLog::BENCHMARK, "  - Load block from disk: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_2 - time_1),
             Ticks<SecondsDouble>(time_read_from_disk_total),
             Ticks<MillisecondsDouble>(time_read_from_disk_total) / num_blocks_total);
    // SYSCOIN
    AssetMap mapAssets;
    NEVMMintTxMap mapMintKeys;
    NEVMTxRootMap mapNEVMTxRoots;
    PoDAMAPMemory mapPoDA;
    std::vector<std::pair<uint256, uint32_t> > vecTXIDPairs;
    {
        // SYSCOIN
        auto dbTx = evoDb->BeginTransaction();

        CCoinsViewCache view(&CoinsTip());
        bool rv = ConnectBlock(blockConnecting, state, pindexNew, view, false, mapAssets, mapMintKeys, mapNEVMTxRoots, mapPoDA, vecTXIDPairs);
        GetMainSignals().BlockChecked(blockConnecting, state);
        if (!rv) {
            if (state.IsInvalid())
                InvalidBlockFound(pindexNew, state);
            return error("%s: ConnectBlock %s failed, %s", __func__, pindexNew->GetBlockHash().ToString(), state.ToString());
        }
        time_3 = SteadyClock::now();
        time_connect_total += time_3 - time_2;
        assert(num_blocks_total > 0);
        LogPrint(BCLog::BENCHMARK, "  - Connect total: %.2fms [%.2fs (%.2fms/blk)]\n",
                 Ticks<MillisecondsDouble>(time_3 - time_2),
                 Ticks<SecondsDouble>(time_connect_total),
                 Ticks<MillisecondsDouble>(time_connect_total) / num_blocks_total);
        bool flushed = view.Flush();
        assert(flushed); 
        // SYSCOIN
        dbTx->Commit();      
    }
    // SYSCOIN
    if(passetdb){
        if(!passetdb->Flush(mapAssets) || !passetnftdb->Flush(mapAssets) || !pnevmtxmintdb->FlushWrite(mapMintKeys)) {
            return error("Error flushing to Syscoin DBs: %s", pindexNew->GetBlockHash().ToString());
        }
    }
    if(pnevmdatadb)
        pnevmdatadb->FlushDataToCache(mapPoDA, pindexNew->GetMedianTimePast());
    if(pblockindexdb)
        pblockindexdb->FlushDataToCache(vecTXIDPairs);
    if(pnevmtxrootsdb)
        pnevmtxrootsdb->FlushDataToCache(mapNEVMTxRoots);
    const auto time_4{SteadyClock::now()};
    time_flush += time_4 - time_3;
    LogPrint(BCLog::BENCHMARK, "  - Flush: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_4 - time_3),
             Ticks<SecondsDouble>(time_flush),
             Ticks<MillisecondsDouble>(time_flush) / num_blocks_total);
    // Write the chain state to disk, if necessary.
    if (!FlushStateToDisk(state, FlushStateMode::IF_NEEDED)) {
        return false;
    }
    const auto time_5{SteadyClock::now()};
    time_chainstate += time_5 - time_4;
    LogPrint(BCLog::BENCHMARK, "  - Writing chainstate: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_5 - time_4),
             Ticks<SecondsDouble>(time_chainstate),
             Ticks<MillisecondsDouble>(time_chainstate) / num_blocks_total);
    // Remove conflicting transactions from the mempool.;
    if (m_mempool) {
        m_mempool->removeForBlock(blockConnecting.vtx, pindexNew->nHeight);
        disconnectpool.removeForBlock(blockConnecting.vtx);
    }
    // Update m_chain & related variables.
    m_chain.SetTip(*pindexNew);
    UpdateTip(pindexNew);

    const auto time_6{SteadyClock::now()};
    time_post_connect += time_6 - time_5;
    time_total += time_6 - time_1;
    LogPrint(BCLog::BENCHMARK, "  - Connect postprocess: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_6 - time_5),
             Ticks<SecondsDouble>(time_post_connect),
             Ticks<MillisecondsDouble>(time_post_connect) / num_blocks_total);
    LogPrint(BCLog::BENCHMARK, "- Connect block: %.2fms [%.2fs (%.2fms/blk)]\n",
             Ticks<MillisecondsDouble>(time_6 - time_1),
             Ticks<SecondsDouble>(time_total),
             Ticks<MillisecondsDouble>(time_total) / num_blocks_total);

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
            assert(pindexTest->HaveTxsDownloaded() || pindexTest->nHeight == 0);

            // Pruned nodes may have entries in setBlockIndexCandidates for
            // which block files have been deleted.  Remove those as candidates
            // for the most work chain if we come across them; we can't switch
            // to a chain unless we have all the non-active-chain parent blocks.
            bool fFailedChain = pindexTest->nStatus & BLOCK_FAILED_MASK;
            // SYSCOIN
            bool fConflictingChain = pindexTest->nStatus & BLOCK_CONFLICT_CHAINLOCK;
            bool fMissingData = !(pindexTest->nStatus & BLOCK_HAVE_DATA);
            // SYSCOIN
            if (fFailedChain || fMissingData || fConflictingChain) {
                // Candidate chain is not usable (either invalid or conflicting or missing data)
                if (fFailedChain && (m_chainman.m_best_invalid == nullptr || pindexNew->nChainWork > m_chainman.m_best_invalid->nChainWork)) {
                    m_chainman.m_best_invalid = pindexNew;
                }
                CBlockIndex *pindexFailed = pindexNew;
                // Remove the entire chain from the set.
                while (pindexTest != pindexFailed) {
                    if (fFailedChain) {
                        pindexFailed->nStatus |= BLOCK_FAILED_CHILD;
                    // SYSCOIN
                    }   else if (fConflictingChain) {
                        // We don't need data for conflicting blocks
                        pindexFailed->nStatus |= BLOCK_CONFLICT_CHAINLOCK;
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
    DisconnectedBlockTransactions disconnectpool;
    while (m_chain.Tip() && m_chain.Tip() != pindexFork) {
        if (!DisconnectTip(state, &disconnectpool)) {
            // This is likely a fatal error, but keep the mempool consistent,
            // just in case. Only remove from the mempool in this case.
            MaybeUpdateMempoolForReorg(disconnectpool, false);

            // If we're unable to disconnect a block during normal operation,
            // then that is a failure of our local system -- we should abort
            // rather than stay on a less work chain.
            AbortNode(state, "Failed to disconnect block; see debug.log for details");
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
        for (CBlockIndex* pindexConnect : reverse_iterate(vpindexToConnect)) {
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

static SynchronizationState GetSynchronizationState(bool init)
{
    if (!init) return SynchronizationState::POST_INIT;
    if (::fReindex) return SynchronizationState::INIT_REINDEX;
    return SynchronizationState::INIT_DOWNLOAD;
}

static bool NotifyHeaderTip(Chainstate& chainstate) LOCKS_EXCLUDED(cs_main) {
    bool fNotify = false;
    bool fInitialBlockDownload = false;
    static CBlockIndex* pindexHeaderOld = nullptr;
    CBlockIndex* pindexHeader = nullptr;
    {
        LOCK(cs_main);
        pindexHeader = chainstate.m_chainman.m_best_header;

        if (pindexHeader != pindexHeaderOld) {
            fNotify = true;
            fInitialBlockDownload = chainstate.IsInitialBlockDownload();
            pindexHeaderOld = pindexHeader;
        }
    }
    // Send block tip changed notifications without cs_main
    if (fNotify) {
        uiInterface.NotifyHeaderTip(GetSynchronizationState(fInitialBlockDownload), pindexHeader->nHeight, pindexHeader->nTime, false);
        // SYSCOIN
        GetMainSignals().NotifyHeaderTip(pindexHeader);
    }
    return fNotify;
}

static void LimitValidationInterfaceQueue() LOCKS_EXCLUDED(cs_main) {
    AssertLockNotHeld(cs_main);
    if (GetMainSignals().CallbacksPending() > 10) {
        SyncWithValidationInterfaceQueue();
    }
}

bool Chainstate::ActivateBestChain(BlockValidationState& state, std::shared_ptr<const CBlock> pblock)
{
    AssertLockNotHeld(m_chainstate_mutex);

    // Note that while we're often called here from ProcessNewBlock, this is
    // far from a guarantee. Things in the P2P/RPC will often end up calling
    // us in the middle of ProcessNewBlock - do not assume pblock is set
    // sanely for performance or correctness!
    AssertLockNotHeld(cs_main);
    // ABC maintains a fair degree of expensive-to-calculate internal state
    // because this function periodically releases cs_main so that it does not lock up other threads for too long
    // during large connects - and to allow for e.g. the callback queue to drain
    // we use m_chainstate_mutex to enforce mutual exclusion so that only one caller may execute this function at a time
    LOCK(m_chainstate_mutex);

    // Belt-and-suspenders check that we aren't attempting to advance the background
    // chainstate past the snapshot base block.
    if (WITH_LOCK(::cs_main, return m_disabled)) {
        LogPrintf("m_disabled is set - this chainstate should not be in operation. " /* Continued */
            "Please report this as a bug. %s\n", PACKAGE_BUGREPORT);
        return false;
    }

    CBlockIndex *pindexMostWork = nullptr;
    CBlockIndex *pindexNewTip = nullptr;
    int nStopAtHeight = gArgs.GetIntArg("-stopatheight", DEFAULT_STOPATHEIGHT);
    do {
        // Block until the validation queue drains. This should largely
        // never happen in normal operation, however may happen during
        // reindex, causing memory blowup if we run too far ahead.
        // Note that if a validationinterface callback ends up calling
        // ActivateBestChain this may lead to a deadlock! We should
        // probably have a DEBUG_LOCKORDER test for this in the future.
        LimitValidationInterfaceQueue();
        {
            LOCK(cs_main);
            // Lock transaction pool for at least as long as it takes for connectTrace to be consumed
            LOCK(MempoolMutex());
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
                // SYSCOIN
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
                    GetMainSignals().BlockConnected(trace.pblock, trace.pindex);
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
            bool fInitialDownload = IsInitialBlockDownload();

            // Notify external listeners about the new tip.
            // Enqueue while holding cs_main to ensure that UpdatedBlockTip is called in the order in which blocks are connected
            if (pindexFork != pindexNewTip) {
                // Notify ValidationInterface subscribers
                // SYSCOIN
                GetMainSignals().SynchronousUpdatedBlockTip(pindexNewTip, pindexFork);
                GetMainSignals().UpdatedBlockTip(pindexNewTip, pindexFork, m_chainman, fInitialDownload);
                // Always notify the UI if a new block tip was connected
                uiInterface.NotifyBlockTip(GetSynchronizationState(fInitialDownload), pindexNewTip);
            }
        }
        // When we reach this point, we switched to a new tip (stored in pindexNewTip).

        if (nStopAtHeight && pindexNewTip && pindexNewTip->nHeight >= nStopAtHeight) StartShutdown();

        if (WITH_LOCK(::cs_main, return m_disabled)) {
            // Background chainstate has reached the snapshot base block, so exit.
            break;
        }

        // We check shutdown only after giving ActivateBestChainStep a chance to run once so that we
        // never shutdown before connecting the genesis block during LoadChainTip(). Previously this
        // caused an assert() failure during shutdown in such cases as the UTXO DB flushing checks
        // that the best block hash is non-null.
        if (ShutdownRequested()) break;
    } while (pindexNewTip != pindexMostWork);
    CheckBlockIndex();
    // Write changes periodically to disk, after relay.
    if (!FlushStateToDisk(state, FlushStateMode::PERIODIC)) {
        return false;
    }
    return true;
}

bool Chainstate::PreciousBlock(BlockValidationState& state, CBlockIndex* pindex)
{
    {
        LOCK(cs_main);
        if (pindex->nChainWork < m_chain.Tip()->nChainWork) {
            // Nothing to do, this block is not at the tip.
            return true;
        }
        if (m_chain.Tip()->nChainWork > nLastPreciousChainwork) {
            // The chain has been extended since the last call, reset the counter.
            nBlockReverseSequenceId = -1;
        }
        nLastPreciousChainwork = m_chain.Tip()->nChainWork;
        setBlockIndexCandidates.erase(pindex);
        pindex->nSequenceId = nBlockReverseSequenceId;
        if (nBlockReverseSequenceId > std::numeric_limits<int32_t>::min()) {
            // We can't keep reducing the counter if somebody really wants to
            // call preciousblock 2**31-1 times on the same set of tips...
            nBlockReverseSequenceId--;
        }
        // SYSCOIN
        if (pindex->IsValid(BLOCK_VALID_TRANSACTIONS) && !(pindex->nStatus & BLOCK_CONFLICT_CHAINLOCK) && pindex->HaveTxsDownloaded()) {
            setBlockIndexCandidates.insert(pindex);
            PruneBlockIndexCandidates();
        }
    }

    return ActivateBestChain(state, std::shared_ptr<const CBlock>());
}
bool Chainstate::EnforceBestChainLock(const CBlockIndex* bestChainLockBlockIndex)
{
    AssertLockNotHeld(m_chainstate_mutex);
    AssertLockNotHeld(cs_main);
    if (!bestChainLockBlockIndex) {
        // we don't have the header/block, so we can't do anything right now
        return true;
    }
    BlockValidationState state;
    // Go backwards through the chain referenced by clsig until we find a block that is part of the main chain and invalidate the fork block (next block in main chain).
    LogPrint(BCLog::CHAINLOCKS, "Chainstate::%s -- enforcing block %s via CLSIG\n", __func__, bestChainLockBlockIndex->GetBlockHash().ToString());
    EnforceBlock(state, bestChainLockBlockIndex);
    // no cs_main allowed
    bool activateNeeded = WITH_LOCK(::cs_main, return m_chain.Tip()->GetAncestor(bestChainLockBlockIndex->nHeight)) != bestChainLockBlockIndex;
    if (activateNeeded) {
        if(!ActivateBestChain(state, nullptr)) {
            LogPrintf("Chainstate::%s -- ActivateBestChain failed: %s\n", __func__, state.ToString());
            return false;
        }
    }
    return true;
}
void Chainstate::EnforceBlock(BlockValidationState& state, const CBlockIndex *pindex)
{
    AssertLockNotHeld(m_chainstate_mutex);
    AssertLockNotHeld(cs_main);
    // We do not allow ActivateBestChain() to run while InvalidateBlock()/MarkConflictingBlock() is
    // running, as that could cause the tip to change while we disconnect
    // blocks.
    LOCK(m_chainstate_mutex);
    LOCK(cs_main);
    const CBlockIndex* pindex_walk = pindex;

    while (pindex_walk && !m_chain.Contains(pindex_walk)) {
        // Mark all blocks that have the same prevBlockHash but are not equal to blockHash as conflicting
        auto itp = m_blockman.LookupBlockIndexPrev(pindex_walk->pprev->GetBlockHash());
        for (auto jt = itp.first; jt != itp.second; ++jt) {
            if (jt->second == pindex_walk) {
                continue;
            }
            if (!MarkConflictingBlock(state, jt->second)) {
                LogPrintf("Chainstate::%s -- MarkConflictingBlock failed: %s\n", __func__, state.ToString());
                // This should not have happened and we are in a state were it's not safe to continue anymore
                assert(false);
            }
            LogPrintf("Chainstate::%s -- marked block %s as conflicting\n",
                      __func__, jt->second->GetBlockHash().ToString());
        }
        pindex_walk = pindex_walk->pprev;
    }
    // In case blocks from the enforced chain are invalid at the moment, reconsider them.
    if (!pindex->IsValid()) {
        LogPrintf("Chainstate::%s -- ResetBlockFailureFlags: %s\n", __func__, pindex->GetBlockHash().ToString());
        ResetBlockFailureFlags(m_blockman.LookupBlockIndex(pindex->GetBlockHash()));
    }
}
// SYSCOIN
bool Chainstate::InvalidateBlock(BlockValidationState& state, CBlockIndex *pindex, bool bReverify)
{
    AssertLockNotHeld(m_chainstate_mutex);

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
                    candidate->HaveTxsDownloaded()) {
                candidate_blocks_by_work.insert(std::make_pair(candidate->nChainWork, candidate));
            }
        }
    }

    // Disconnect (descendants of) pindex, and mark them invalid.
    while (true) {
        if (ShutdownRequested()) break;

        // Make sure the queue of validation callbacks doesn't grow unboundedly.
        LimitValidationInterfaceQueue();

        LOCK(cs_main);
        // Lock for as long as disconnectpool is in scope to make sure MaybeUpdateMempoolForReorg is
        // called after DisconnectTip without unlocking in between
        LOCK(MempoolMutex());
        if (!m_chain.Contains(pindex)) break;
        pindex_was_in_chain = true;
        CBlockIndex *invalid_walk_tip = m_chain.Tip();

        // ActivateBestChain considers blocks already in m_chain
        // unconditionally valid already, so force disconnect away from it.
        DisconnectedBlockTransactions disconnectpool;
        bool ret = DisconnectTip(state, &disconnectpool, bReverify);
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
        if (invalid_walk_tip->pprev == to_mark_failed && (to_mark_failed->nStatus & BLOCK_FAILED_VALID)) {
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

    CheckBlockIndex();

    {
        LOCK(cs_main);
        if (m_chain.Contains(to_mark_failed)) {
            // If the to-be-marked invalid block is in the active chain, something is interfering and we can't proceed.
            return false;
        }

        // Mark pindex (or the last disconnected block) as invalid, even when it never was in the main chain
        to_mark_failed->nStatus |= BLOCK_FAILED_VALID;
        m_blockman.m_dirty_blockindex.insert(to_mark_failed);
        setBlockIndexCandidates.erase(to_mark_failed);
        m_chainman.m_failed_blocks.insert(to_mark_failed);

        // If any new blocks somehow arrived while we were disconnecting
        // (above), then the pre-calculation of what should go into
        // setBlockIndexCandidates may have missed entries. This would
        // technically be an inconsistency in the block index, but if we clean
        // it up here, this should be an essentially unobservable error.
        // Loop back over all block index entries and add any missing entries
        // to setBlockIndexCandidates.
        for (auto& [_, block_index] : m_blockman.m_block_index) {
            // SYSCOIN
            if (!(block_index.nStatus & BLOCK_CONFLICT_CHAINLOCK) && block_index.IsValid(BLOCK_VALID_TRANSACTIONS) && block_index.HaveTxsDownloaded() && !setBlockIndexCandidates.value_comp()(&block_index, m_chain.Tip())) {
                setBlockIndexCandidates.insert(&block_index);
            }
        }

        InvalidChainFound(to_mark_failed);
    }
    // Only notify about a new block tip if the active chain was modified.
    if (pindex_was_in_chain) {
        // SYSCOIN for MN list to update
        GetMainSignals().SynchronousUpdatedBlockTip(to_mark_failed->pprev, nullptr);
        uiInterface.NotifyBlockTip(GetSynchronizationState(IsInitialBlockDownload()), to_mark_failed->pprev);
    }
    return true;
}
// SYSCOIN
bool Chainstate::MarkConflictingBlock(BlockValidationState& state, CBlockIndex *pindex)
{
    AssertLockHeld(cs_main);
    AssertLockNotHeld(m_mempool->cs);
    bool pindex_was_in_chain = false;
    int disconnected = 0;
    CBlockIndex *conflicting_walk_tip = m_chain.Tip();


    if (pindex == m_chainman.m_best_header) {
        m_chainman.m_best_header = m_chainman.m_best_header->pprev;
    }
    DisconnectedBlockTransactions disconnectpool;
    while (true) {
        if (ShutdownRequested()) break;

        LOCK(MempoolMutex()); // Lock transaction pool for at least as long as it takes for connectTrace to be consumed
        if(!m_chain.Contains(pindex)) break;
        const CBlockIndex* pindexOldTip = m_chain.Tip();
        pindex_was_in_chain = true;
        // ActivateBestChain considers blocks already in m_chain
        // unconditionally valid already, so force disconnect away from it.
        bool ret = DisconnectTip(state, &disconnectpool);
        // DisconnectTip will add transactions to disconnectpool.
        // Adjust the mempool to be consistent with the new tip, adding
        // transactions back to the mempool if disconnecting was successful,
        // and we're not doing a very deep invalidation (in which case
        // keeping the mempool up to date is probably futile anyway).
        MaybeUpdateMempoolForReorg(disconnectpool, /* fAddToMempool = */ (++disconnected <= 10) && ret);
        if (!ret) return false;
        if (pindexOldTip == m_chainman.m_best_header) {
            m_chainman.m_best_header = m_chainman.m_best_header->pprev;
        }
    }

    // Now mark the blocks we just disconnected as descendants conflicting
    // (note this may not be all descendants).
    while (pindex_was_in_chain && conflicting_walk_tip != pindex) {
        conflicting_walk_tip->nStatus |= BLOCK_CONFLICT_CHAINLOCK;
        setBlockIndexCandidates.erase(conflicting_walk_tip);
        conflicting_walk_tip = conflicting_walk_tip->pprev;
    }


    if (m_chain.Contains(pindex)) {
        // If the to-be-marked invalid block is in the active chain, something is interfering and we can't proceed.
        return false;
    }
    // Mark the block itself as conflicting.
    pindex->nStatus |= BLOCK_CONFLICT_CHAINLOCK;
    setBlockIndexCandidates.erase(pindex);

    // DisconnectTip will add transactions to disconnectpool; try to add these
    // back to the mempool.
    {
        LOCK(MempoolMutex());
        MaybeUpdateMempoolForReorg(disconnectpool, true);
    }

    // The resulting new best tip may not be in setBlockIndexCandidates anymore, so
    // add it again.
    for (auto& [_, block_index] : m_blockman.m_block_index) {
        // SYSCOIN
        if (!(block_index.nStatus & BLOCK_CONFLICT_CHAINLOCK) && block_index.IsValid(BLOCK_VALID_TRANSACTIONS) && block_index.HaveTxsDownloaded() && !setBlockIndexCandidates.value_comp()(&block_index, m_chain.Tip())) {
            setBlockIndexCandidates.insert(&block_index);
        }
    }

    ConflictingChainFound(pindex);
    GetMainSignals().SynchronousUpdatedBlockTip(m_chain.Tip(), nullptr);
    GetMainSignals().UpdatedBlockTip(m_chain.Tip(), nullptr, m_chainman, false);

    // Only notify about a new block tip if the active chain was modified.
    if (pindex_was_in_chain) {
        // SYSCOIN for MN list to update
        uiInterface.NotifyBlockTip(GetSynchronizationState(IsInitialBlockDownload()), pindex->pprev);
    }
    return true;
}
bool Chainstate::ResetLastBlock() {
    AssertLockHeld(cs_main);
    if(m_chainman.m_best_invalid && m_chainman.m_best_invalid->GetAncestor(m_chain.Height()) == m_chain.Tip()) {
        LogPrintf("%s: Found invalid best block, resetting %s\n", __func__, m_chainman.m_best_invalid->GetBlockHash().ToString());
        uint256 hash(m_chainman.m_best_invalid->GetBlockHash());
        CBlockIndex* pblockindex = m_blockman.LookupBlockIndex(hash);
        if (!pblockindex) {
            LogPrintf("%s: Block not found\n", __func__);
            return false;
        }
        ResetBlockFailureFlags(pblockindex);
    }
    return true;
}
void Chainstate::ResetBlockFailureFlags(CBlockIndex *pindex) {
    AssertLockHeld(cs_main);
    // SYSCOIN
    if ( !pindex) {
        if (llmq::AreChainLocksEnabled() && m_chainman.m_best_invalid && m_chainman.m_best_invalid->GetAncestor(m_chain.Height()) == m_chain.Tip()) {
            LogPrintf("%s: the best known invalid block (%s) is ahead of our tip, reconsidering\n",
                    __func__, m_chainman.m_best_invalid->GetBlockHash().ToString());
            pindex = m_chainman.m_best_invalid;
        } else {
            return;
        }
    }
    int nHeight = pindex->nHeight;

    // Remove the invalidity flag from this block and all its descendants.
    for (auto& [_, block_index] : m_blockman.m_block_index) {
        if (!block_index.IsValid() && block_index.GetAncestor(nHeight) == pindex) {
            block_index.nStatus &= ~BLOCK_FAILED_MASK;
            m_blockman.m_dirty_blockindex.insert(&block_index);
            // SYSCOIN
            if (!(block_index.nStatus & BLOCK_CONFLICT_CHAINLOCK) && block_index.IsValid(BLOCK_VALID_TRANSACTIONS) && block_index.HaveTxsDownloaded() && setBlockIndexCandidates.value_comp()(m_chain.Tip(), &block_index)) {
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
            // SYSCOIN
            if (pindex->IsValid(BLOCK_VALID_TRANSACTIONS) && !(pindex->nStatus & BLOCK_CONFLICT_CHAINLOCK) && pindex->HaveTxsDownloaded() && setBlockIndexCandidates.value_comp()(m_chain.Tip(), pindex)) {
                setBlockIndexCandidates.insert(pindex);
            }
            if (pindex == m_chainman.m_best_invalid) {
                // Reset invalid block marker if it was pointing to one of those.
                m_chainman.m_best_invalid = nullptr;
            }
            m_chainman.m_failed_blocks.erase(pindex);
            // Mark all nearest BLOCK_FAILED_CHILD descendants (if any) as BLOCK_FAILED_VALID
            auto itp = m_blockman.m_prev_block_index.equal_range(pindex->GetBlockHash());
            for (auto jt = itp.first; jt != itp.second; ++jt) {
                if (jt->second->nStatus & BLOCK_FAILED_CHILD) {
                    jt->second->nStatus |= BLOCK_FAILED_VALID;
                    m_chainman.m_failed_blocks.insert(jt->second);
                    m_blockman.m_dirty_blockindex.insert(jt->second);
                    setBlockIndexCandidates.erase(jt->second);
                }
            }
        }
        pindex = pindex->pprev;
    }
}


/** Mark a block as having its data received and checked (up to BLOCK_VALID_TRANSACTIONS). */
void Chainstate::ReceivedBlockTransactions(const CBlock& block, CBlockIndex* pindexNew, const FlatFilePos& pos)
{
    pindexNew->nTx = block.vtx.size();
    pindexNew->nChainTx = 0;
    pindexNew->nFile = pos.nFile;
    pindexNew->nDataPos = pos.nPos;
    pindexNew->nUndoPos = 0;
    pindexNew->nStatus |= BLOCK_HAVE_DATA;
    if (DeploymentActiveAt(*pindexNew, m_chainman, Consensus::DEPLOYMENT_SEGWIT)) {
        pindexNew->nStatus |= BLOCK_OPT_WITNESS;
    }
    pindexNew->RaiseValidity(BLOCK_VALID_TRANSACTIONS);
    m_blockman.m_dirty_blockindex.insert(pindexNew);

    if (pindexNew->pprev == nullptr || pindexNew->pprev->HaveTxsDownloaded()) {
        // If pindexNew is the genesis block or all parents are BLOCK_VALID_TRANSACTIONS.
        std::deque<CBlockIndex*> queue;
        queue.push_back(pindexNew);

        // Recursively process any descendant blocks that now may be eligible to be connected.
        while (!queue.empty()) {
            CBlockIndex *pindex = queue.front();
            queue.pop_front();
            pindex->nChainTx = (pindex->pprev ? pindex->pprev->nChainTx : 0) + pindex->nTx;
            pindex->nSequenceId = nBlockSequenceId++;
            if (m_chain.Tip() == nullptr || !setBlockIndexCandidates.value_comp()(pindex, m_chain.Tip())) {
                // SYSCOIN
                if (!(pindex->nStatus & BLOCK_CONFLICT_CHAINLOCK)) {
                    setBlockIndexCandidates.insert(pindex);
                }
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
    if (fCheckPOW && !CheckProofOfWork(block, consensusParams))
        return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "high-hash", "proof of work failed");

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
    if (fCheckMerkleRoot) {
        bool mutated;
        uint256 hashMerkleRoot2 = BlockMerkleRoot(block, &mutated);
        if (block.hashMerkleRoot != hashMerkleRoot2)
            return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "bad-txnmrklroot", "hashMerkleRoot mismatch");

        // Check for merkle tree malleability (CVE-2012-2459): repeating sequences
        // of transactions in a block without affecting the merkle root of a block,
        // while still invalidating it.
        if (mutated)
            return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "bad-txns-duplicate", "duplicate transaction");
    }

    // All potential-corruption validation must be done before we do any
    // transaction validation, as otherwise we may mark the header as invalid
    // because we receive the wrong transactions for it.
    // Note that witness malleability is checked in ContextualCheckBlock, so no
    // checks that use witness data may be performed here.

    // Size limits
    if (block.vtx.empty() || block.vtx.size() * WITNESS_SCALE_FACTOR > MAX_BLOCK_WEIGHT || ::GetSerializeSize(block, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * WITNESS_SCALE_FACTOR > MAX_BLOCK_WEIGHT) {
        // SYSCOIN
        if(block.IsNEVM() || fRegTest) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-blk-length", "size limits failed");
        }
    }
    // SYSCOIN
    if(block.IsNEVM() && block.vchNEVMBlockData.empty() && !fRegTest) {
        return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "nevm-data-not-found");
    }
    if(block.vchNEVMBlockData.size() > MAX_NEVM_BLOCK_SIZE) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "nevm-block-size");
    }
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
// SYSCOIN
std::vector<unsigned char> ChainstateManager::GenerateCoinbaseCommitment(CBlock& block, const CBlockIndex* pindexPrev, const std::vector<unsigned char> &vchExtraData) const
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
        // SYSCOIN push extra data to stack
        if(!vchExtraData.empty())
            out.scriptPubKey << vchExtraData;
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
    // SYSCOIN
    return std::all_of(headers.cbegin(), headers.cend(),
            [&](const auto& header) { return CheckProofOfWork(header, consensusParams);});
}

arith_uint256 CalculateHeadersWork(const std::vector<CBlockHeader>& headers)
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
 */
static bool ContextualCheckBlockHeader(const CBlockHeader& block, BlockValidationState& state, BlockManager& blockman, const ChainstateManager& chainman, const CBlockIndex* pindexPrev, NodeClock::time_point now) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    assert(pindexPrev != nullptr);
    const int nHeight = pindexPrev->nHeight + 1;

    // Check proof of work
    const Consensus::Params& consensusParams = chainman.GetConsensus();
    // Disallow legacy blocks after merge-mining start.
    if (!consensusParams.AllowLegacyBlocks(nHeight) && block.IsLegacy()){
        LogPrintf("ERROR: %s: legacy block after auxpow start\n", __func__);
        return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "late-legacy-block");
     }
    if (block.nBits != GetNextWorkRequired(pindexPrev, &block, consensusParams))
        return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "bad-diffbits", "incorrect proof of work");

    // Check against checkpoints
    if (chainman.m_options.checkpoints_enabled) {
        // Don't accept any forks from the main chain prior to last checkpoint.
        // GetLastCheckpoint finds the last checkpoint in MapCheckpoints that's in our
        // BlockIndex().
        const CBlockIndex* pcheckpoint = blockman.GetLastCheckpoint(chainman.GetParams().Checkpoints());
        if (pcheckpoint && nHeight < pcheckpoint->nHeight) {
            LogPrintf("ERROR: %s: forked chain older than last checkpoint (height %d)\n", __func__, nHeight);
            return state.Invalid(BlockValidationResult::BLOCK_CHECKPOINT, "bad-fork-prior-to-checkpoint");
        }
    }

    // Check timestamp against prev
    if (block.GetBlockTime() <= pindexPrev->GetMedianTimePast())
        return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "time-too-old", "block's timestamp is too early");

    // Check timestamp
    if (block.Time() > now + std::chrono::seconds{MAX_FUTURE_BLOCK_TIME}) {
        return state.Invalid(BlockValidationResult::BLOCK_TIME_FUTURE, "time-too-new", "block timestamp too far in the future");
    }
    // SYSCOIN
    if((pindexPrev->nHeight+1) >= consensusParams.nNEVMStartBlock) {
        if(!block.IsNEVM() && !fRegTest) {
            return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "nevm-version-expected");
        }
    }
    // Reject outdated version blocks when 95% (75% on testnet) of the network has upgraded:
    // check for version 2, 3 and 4 upgrades
    // SYSCOIN
    if(nHeight >= consensusParams.nUTXOAssetsBlock) {
        // if valid Chain ID (> 0) then it should always be nAuxpowChainId after nUTXOAssetsBlock block
        const int32_t& nChainID = block.GetChainId();
        const auto& baseVer = block.GetBaseVersion();
        if(!fTestNet && ((baseVer < 2 && nHeight >= consensusParams.BIP34Height) ||
        (baseVer < 3 && nHeight >= consensusParams.BIP66Height) ||
        (baseVer < 4 && nHeight >= consensusParams.BIP65Height) ||
        !CPureBlockHeader::IsValidBaseVersion(baseVer) ||
        (nChainID > 0 && nChainID != consensusParams.nAuxpowChainId)))
                return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, strprintf("bad-version(0x%08x)", baseVer),
                                    strprintf("rejected nVersion=0x%08x block", block.nVersion));
    } else {
        const auto& baseVer = block.GetOldBaseVersion();
        if((baseVer < 2 && nHeight >= consensusParams.BIP34Height) ||
        (baseVer < 3 && nHeight >= consensusParams.BIP66Height) ||
        (baseVer < 4 && nHeight >= consensusParams.BIP65Height))
                return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, strprintf("bad-version-old(0x%08x)", baseVer),
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
    bool fHaveWitness = false;
    if (DeploymentActiveAfter(pindexPrev, chainman, Consensus::DEPLOYMENT_SEGWIT)) {
        int commitpos = GetWitnessCommitmentIndex(block);
        if (commitpos != NO_WITNESS_COMMITMENT) {
            bool malleated = false;
            uint256 hashWitness = BlockWitnessMerkleRoot(block, &malleated);
            // The malleation check is ignored; as the transaction tree itself
            // already does not permit it, it is impossible to trigger in the
            // witness tree.
            if (block.vtx[0]->vin[0].scriptWitness.stack.size() != 1 || block.vtx[0]->vin[0].scriptWitness.stack[0].size() != 32) {
                return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "bad-witness-nonce-size", strprintf("%s : invalid witness reserved value size", __func__));
            }
            CHash256().Write(hashWitness).Write(block.vtx[0]->vin[0].scriptWitness.stack[0]).Finalize(hashWitness);
            if (memcmp(hashWitness.begin(), &block.vtx[0]->vout[commitpos].scriptPubKey[6], 32)) {
                return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "bad-witness-merkle-match", strprintf("%s : witness merkle commitment mismatch", __func__));
            }
            fHaveWitness = true;
        }
    }

    // No witness data is allowed in blocks that don't commit to witness data, as this would otherwise leave room for spam
    if (!fHaveWitness) {
      for (const auto& tx : block.vtx) {
            if (tx->HasWitness()) {
                return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "unexpected-witness", strprintf("%s : unexpected witness data found", __func__));
            }
        }
    }

    // After the coinbase witness reserved value and commitment are verified,
    // we can check if the block weight passes (before we've checked the
    // coinbase witness, it would be possible for the weight to be too
    // large by filling up the coinbase witness, which doesn't change
    // the block hash, so we couldn't mark the block as permanently
    // failed).
    const auto& consensusParams = chainman.GetParams().GetConsensus();
    bool nevmContext = nHeight >= consensusParams.nNEVMStartBlock;
    if ((fRegTest || nevmContext) && GetBlockWeight(block) > MAX_BLOCK_WEIGHT) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-blk-weight", strprintf("%s : weight limit failed", __func__));
    }
    // SYSCOIN
    bool fDIP0003Active_context = nHeight >= consensusParams.DIP0003Height;
    if(fDIP0003Active_context && block.vtx[0]->nVersion != SYSCOIN_TX_VERSION_MN_COINBASE && block.vtx[0]->nVersion != SYSCOIN_TX_VERSION_MN_QUORUM_COMMITMENT) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-txns-cb-type", strprintf("%s : Incorrect version of coinbase transaction", __func__));
    }
    for (const auto& txRef : block.vtx)
    {
        if (!txRef->IsCoinBase() && (txRef->nVersion == SYSCOIN_TX_VERSION_MN_COINBASE || txRef->nVersion == SYSCOIN_TX_VERSION_MN_QUORUM_COMMITMENT)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-mn-version", "Bad version for non-coinbase masternode transaction");
        }
    }
    return true;
}
bool ChainstateManager::AcceptBlockHeader(const CBlockHeader& block, BlockValidationState& state, CBlockIndex** ppindex, bool min_pow_checked, bool bForBlock)
{
    AssertLockHeld(cs_main);

    // Check for duplicate
    uint256 hash = block.GetHash();
    BlockMap::iterator miSelf{m_blockman.m_block_index.find(hash)};
    // SYSCOIN
    BlockStatus nStatus = BLOCK_VALID_TREE;
    CBlockIndex *pindex = nullptr;
    if (hash != GetConsensus().hashGenesisBlock) {
        if (miSelf != m_blockman.m_block_index.end()) {
            // Block header is already known.
            pindex = &(miSelf->second);
            if (ppindex)
                *ppindex = pindex;
            if (pindex->nStatus & BLOCK_FAILED_MASK) {
                LogPrint(BCLog::VALIDATION, "%s: block %s is marked invalid\n", __func__, hash.ToString());
                return state.Invalid(BlockValidationResult::BLOCK_CACHED_INVALID, "duplicate");
            }
            // SYSCOIN
            if (pindex->nStatus & BLOCK_CONFLICT_CHAINLOCK) {
                LogPrintf("ERROR: %s: block %s is marked conflicting\n", __func__, hash.ToString());
                return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "duplicate");
            }
            return true;
        }

        if (!CheckBlockHeader(block, state, GetConsensus())) {
            LogPrint(BCLog::VALIDATION, "%s: Consensus::CheckBlockHeader: %s, %s\n", __func__, hash.ToString(), state.ToString());
            return false;
        }

        // Get prev block index
        CBlockIndex* pindexPrev = nullptr;
        BlockMap::iterator mi{m_blockman.m_block_index.find(block.hashPrevBlock)};
        if (mi == m_blockman.m_block_index.end()) {
            LogPrint(BCLog::VALIDATION, "header %s has prev block not found: %s\n", hash.ToString(), block.hashPrevBlock.ToString());
            return state.Invalid(BlockValidationResult::BLOCK_MISSING_PREV, "prev-blk-not-found");
        }
        pindexPrev = &((*mi).second);
        if (pindexPrev->nStatus & BLOCK_FAILED_MASK) {
            LogPrint(BCLog::VALIDATION, "header %s has prev block invalid: %s\n", hash.ToString(), block.hashPrevBlock.ToString());
            return state.Invalid(BlockValidationResult::BLOCK_INVALID_PREV, "bad-prevblk");
        }
        // SYSCOIN
        if (pindexPrev->nStatus & BLOCK_CONFLICT_CHAINLOCK) {
            // it's ok-ish, the other node is probably missing the latest chainlock
            return state.Invalid(BlockValidationResult::BLOCK_MISSING_PREV, "bad-prevblk-chainlock");
        }
        if (!ContextualCheckBlockHeader(block, state, m_blockman, *this, pindexPrev, m_options.adjusted_time_callback())) {
            LogPrint(BCLog::VALIDATION, "%s: Consensus::ContextualCheckBlockHeader: %s, %s\n", __func__, hash.ToString(), state.ToString());
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
                    LogPrint(BCLog::VALIDATION, "header %s has prev block invalid: %s\n", hash.ToString(), block.hashPrevBlock.ToString());
                    return state.Invalid(BlockValidationResult::BLOCK_INVALID_PREV, "bad-prevblk");
                }
            }
        }
        // SYSCOIN
        if (llmq::chainLocksHandler->HasConflictingChainLock(pindexPrev->nHeight + 1, hash)) {
            // if processing block or if min_pow_checked is true (otherwise we want to reject block as semantically invalid so lock is removed)
            // for block it will check in ConnectBlock() for a conflicting chainlock and return state back to ActivateBestChainStep which will call InvalidBlockFound
            // there if the block is semantically invalid (anything other than BLOCK_CHAINLOCK) we will remove the lock if it exists on the block
            // the nuance here is that we want to check the header here but we want to check for the rest of the block consensus including transactions to know if its semantically valid
            // before deciding to enforce a chainlock in a conflict
            nStatus = BLOCK_CONFLICT_CHAINLOCK;
            if(min_pow_checked && !bForBlock) {
                if(pindex == nullptr)
                    pindex = m_blockman.AddToBlockIndex(block, m_best_header, nStatus);

                if (ppindex)
                    *ppindex = pindex;
                return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "bad-chainlock");
            }
        }
    }
    if (!min_pow_checked) {
        LogPrint(BCLog::VALIDATION, "%s: not adding new block header %s, missing anti-dos proof-of-work validation\n", __func__, hash.ToString());
        return state.Invalid(BlockValidationResult::BLOCK_HEADER_LOW_WORK, "too-little-chainwork");
    }
    if(pindex == nullptr)
        pindex = m_blockman.AddToBlockIndex(block, m_best_header, nStatus);

    if (ppindex)
        *ppindex = pindex;

    return true;
}

// Exposed wrapper for AcceptBlockHeader
bool ChainstateManager::ProcessNewBlockHeaders(const std::vector<CBlockHeader>& headers, bool min_pow_checked, BlockValidationState& state, const CBlockIndex** ppindex)
{
    AssertLockNotHeld(cs_main);
    {
        LOCK(cs_main);
        for (const CBlockHeader& header : headers) {
            CBlockIndex *pindex = nullptr; // Use a temp pindex instead of ppindex to avoid a const_cast
            // SYSCOIN
            bool accepted{AcceptBlockHeader(header, state, &pindex, min_pow_checked, false)};
            ActiveChainstate().CheckBlockIndex();
            if (!accepted) {
                return false;
            }
            if (ppindex) {
                *ppindex = pindex;
            }
        }
    }
    if (NotifyHeaderTip(ActiveChainstate())) {
        if (ActiveChainstate().IsInitialBlockDownload() && ppindex && *ppindex) {
            const CBlockIndex& last_accepted{**ppindex};
            const int64_t blocks_left{(GetTime() - last_accepted.GetBlockTime()) / GetConsensus().nPowTargetSpacing};
            const double progress{100.0 * last_accepted.nHeight / (last_accepted.nHeight + blocks_left)};
            LogPrintf("Synchronizing blockheaders, height: %d (~%.2f%%)\n", last_accepted.nHeight, progress);
        }
    }
    return true;
}

void ChainstateManager::ReportHeadersPresync(const arith_uint256& work, int64_t height, int64_t timestamp)
{
    AssertLockNotHeld(cs_main);
    const auto& chainstate = ActiveChainstate();
    {
        LOCK(cs_main);
        // Don't report headers presync progress if we already have a post-minchainwork header chain.
        // This means we lose reporting for potentially legimate, but unlikely, deep reorgs, but
        // prevent attackers that spam low-work headers from filling our logs.
        if (m_best_header->nChainWork >= UintToArith256(GetConsensus().nMinimumChainWork)) return;
        // Rate limit headers presync updates to 4 per second, as these are not subject to DoS
        // protection.
        auto now = std::chrono::steady_clock::now();
        if (now < m_last_presync_update + std::chrono::milliseconds{250}) return;
        m_last_presync_update = now;
    }
    bool initial_download = chainstate.IsInitialBlockDownload();
    uiInterface.NotifyHeaderTip(GetSynchronizationState(initial_download), height, timestamp, /*presync=*/true);
    // SYSCOIN
    // GetMainSignals().NotifyHeaderTip(m_best_header, m_best_header->nHeight);
    if (initial_download) {
        const int64_t blocks_left{(GetTime() - timestamp) / GetConsensus().nPowTargetSpacing};
        const double progress{100.0 * height / (height + blocks_left)};
        LogPrintf("Pre-synchronizing blockheaders, height: %d (~%.2f%%)\n", height, progress);
    }
}

/** Store block on disk. If dbp is non-nullptr, the file is known to already reside on disk */
bool Chainstate::AcceptBlock(const std::shared_ptr<const CBlock>& pblock, BlockValidationState& state, CBlockIndex** ppindex, bool fRequested, const FlatFilePos* dbp, bool* fNewBlock, bool min_pow_checked)
{
    const CBlock& block = *pblock;

    if (fNewBlock) *fNewBlock = false;
    AssertLockHeld(cs_main);

    CBlockIndex *pindexDummy = nullptr;
    CBlockIndex *&pindex = ppindex ? *ppindex : pindexDummy;
    bool accepted_header{m_chainman.AcceptBlockHeader(block, state, &pindex, min_pow_checked)};
    CheckBlockIndex();

    if (!accepted_header)
        return false;

    // Try to process all requested blocks that we don't have, but only
    // process an unrequested block if it's new and has enough work to
    // advance our tip, and isn't too many blocks ahead.
    bool fAlreadyHave = pindex->nStatus & BLOCK_HAVE_DATA;
    bool fHasMoreOrSameWork = (m_chain.Tip() ? pindex->nChainWork >= m_chain.Tip()->nChainWork : true);
    // Blocks that are too out-of-order needlessly limit the effectiveness of
    // pruning, because pruning will not delete block files that contain any
    // blocks which are too close in height to the tip.  Apply this test
    // regardless of whether pruning is enabled; it should generally be safe to
    // not process unrequested blocks.
    bool fTooFarAhead{pindex->nHeight > m_chain.Height() + int(MIN_BLOCKS_TO_KEEP)};

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
        if (pindex->nChainWork < m_chainman.MinimumChainWork()) return true;
    }

    const CChainParams& params{m_chainman.GetParams()};
    if (!CheckBlock(block, state, params.GetConsensus()) ||
        !ContextualCheckBlock(block, state, m_chainman, pindex->pprev)) {
        if (state.IsInvalid() && state.GetResult() != BlockValidationResult::BLOCK_MUTATED) {
            pindex->nStatus |= BLOCK_FAILED_VALID;
            m_blockman.m_dirty_blockindex.insert(pindex);
        }
        return error("%s: %s", __func__, state.ToString());
    }
    // SYSCOIN ProcessNEVMData/FlushDataToCache so we have the data from processing out-of-order blocks and it reads from disk (recreating PoDA data in block) prior to validation in ConnectTip()
    bool PODAContext = pindex->nHeight >= params.GetConsensus().nPODAStartBlock;
    PoDAMAPMemory mapPoDA;
    if(PODAContext && !ProcessNEVMData(m_chainman.m_blockman, block, m_chain.Tip()? m_chain.Tip()->GetMedianTimePast(): 0, TicksSinceEpoch<std::chrono::seconds>(m_chainman.m_options.adjusted_time_callback()), mapPoDA)) {
        state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "poda-validation-failed");
        pindex->nStatus |= BLOCK_FAILED_VALID;
        m_blockman.m_dirty_blockindex.insert(pindex);
        return error("%s: %s", __func__, state.ToString());
    }
    if(pnevmdatadb)
        pnevmdatadb->FlushDataToCache(mapPoDA, pindex->GetMedianTimePast());

    // Header is valid/has work, merkle tree and segwit merkle tree are good...RELAY NOW
    // (but if it does not build on our best tip, let the SendMessages loop relay it)
    if (!IsInitialBlockDownload() && m_chain.Tip() == pindex->pprev)
        GetMainSignals().NewPoWValidBlock(pindex, pblock);

    // Write block to history file
    if (fNewBlock) *fNewBlock = true;
    try {
        FlatFilePos blockPos{m_blockman.SaveBlockToDisk(block, pindex->nHeight, m_chain, params, dbp)};
        if (blockPos.IsNull()) {
            state.Error(strprintf("%s: Failed to find position to write new block to disk", __func__));
            return false;
        }
        ReceivedBlockTransactions(block, pindex, blockPos);
    } catch (const std::runtime_error& e) {
        return AbortNode(state, std::string("System error: ") + e.what());
    }

    FlushStateToDisk(state, FlushStateMode::NONE);

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
            ret = ActiveChainstate().AcceptBlock(block, state, &pindex, force_processing, nullptr, new_block, min_pow_checked);
        }
        if (!ret) {
            GetMainSignals().BlockChecked(*block, state);
            return error("%s: AcceptBlock FAILED (%s)", __func__, state.ToString());
        }
    }

    NotifyHeaderTip(ActiveChainstate());

    BlockValidationState state; // Only used to report errors, not invalidity - ignore it
    if (!ActiveChainstate().ActivateBestChain(state, block)) {
        return error("%s: ActivateBestChain failed (%s)", __func__, state.ToString());
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
                       const std::function<NodeClock::time_point()>& adjusted_time_callback,
                       bool fCheckPOW,
                       bool fCheckMerkleRoot)
{
    AssertLockHeld(cs_main);
    assert(pindexPrev && pindexPrev == chainstate.m_chain.Tip());
    CCoinsViewCache viewNew(&chainstate.CoinsTip());
    // SYSCOIN
    uint256 block_hash(block.GetHash());
    if (llmq::chainLocksHandler->HasConflictingChainLock(pindexPrev->nHeight + 1, block_hash)) {
        return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "bad-chainlock");
    }
    
    CBlockIndex indexDummy(block);
    indexDummy.pprev = pindexPrev;
    indexDummy.nHeight = pindexPrev->nHeight + 1;
    indexDummy.phashBlock = &block_hash;

    // SYSCOIN begin tx and let it rollback
    auto dbTx = evoDb->BeginTransaction();

    // NOTE: CheckBlockHeader is called by CheckBlock
    if (!ContextualCheckBlockHeader(block, state, chainstate.m_blockman, chainstate.m_chainman, pindexPrev, adjusted_time_callback()))
        return error("%s: Consensus::ContextualCheckBlockHeader: %s", __func__, state.ToString());
    if (!CheckBlock(block, state, chainparams.GetConsensus(), fCheckPOW, fCheckMerkleRoot))
        return error("%s: Consensus::CheckBlock: %s", __func__, state.ToString());
    if (!ContextualCheckBlock(block, state, chainstate.m_chainman, pindexPrev))
        return error("%s: Consensus::ContextualCheckBlock: %s", __func__, state.ToString());
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

void Chainstate::LoadMempool(const fs::path& load_path, FopenFn mockable_fopen_function)
{
    if (!m_mempool) return;
    ::LoadMempool(*m_mempool, load_path, *this, mockable_fopen_function);
    m_mempool->SetLoadTried(!ShutdownRequested());
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
              GuessVerificationProgress(m_chainman.GetParams().TxData(), tip));
    return true;
}

CVerifyDB::CVerifyDB()
{
    uiInterface.ShowProgress(_("Verifying blocks…").translated, 0, false);
}

CVerifyDB::~CVerifyDB()
{
    uiInterface.ShowProgress("", 100, false);
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
    // SYSCOIN begin tx and let it rollback
    auto dbTx = evoDb->BeginTransaction();
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

    const bool is_snapshot_cs{!chainstate.m_from_snapshot_blockhash};
    // SYSCOIN
    AssetMap mapAssets;
    NEVMMintTxMap mapMintKeys;
    std::vector<uint256> vecNEVMBlocks;
    NEVMDataVec NEVMDataVecOut;
    std::vector<std::pair<uint256,uint32_t> > vecTXIDPairs;
    for (pindex = chainstate.m_chain.Tip(); pindex && pindex->pprev; pindex = pindex->pprev) {
        const int percentageDone = std::max(1, std::min(99, (int)(((double)(chainstate.m_chain.Height() - pindex->nHeight)) / (double)nCheckDepth * (nCheckLevel >= 4 ? 50 : 100))));
        if (reportDone < percentageDone / 10) {
            // report every 10% step
            LogPrintf("Verification progress: %d%%\n", percentageDone);
            reportDone = percentageDone / 10;
        }
        uiInterface.ShowProgress(_("Verifying blocks…").translated, percentageDone, false);
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
        if (!ReadBlockFromDisk(block, pindex, consensus_params)) {
            LogPrintf("Verification error: ReadBlockFromDisk failed at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
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
                if (!UndoReadFromDisk(undo, pindex)) {
                    LogPrintf("Verification error: found bad undo data at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                    return VerifyDBResult::CORRUPTED_BLOCK_DB;
                }
            }
        }
        // check level 3: check for inconsistencies during memory-only disconnect of tip blocks
        size_t curr_coins_usage = coins.DynamicMemoryUsage() + chainstate.CoinsTip().DynamicMemoryUsage();

        if (nCheckLevel >= 3) {
            if (curr_coins_usage <= chainstate.m_coinstip_cache_size_bytes) {
                // SYSCOIN
                DisconnectResult res = chainstate.DisconnectBlock(block, pindex, coins, mapAssets, mapMintKeys, NEVMDataVecOut, vecNEVMBlocks, vecTXIDPairs, false);
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
        if (ShutdownRequested()) return VerifyDBResult::INTERRUPTED;
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
            uiInterface.ShowProgress(_("Verifying blocks…").translated, percentageDone, false);
            pindex = chainstate.m_chain.Next(pindex);
            CBlock block;
            if (!ReadBlockFromDisk(block, pindex, consensus_params)) {
                LogPrintf("Verification error: ReadBlockFromDisk failed at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                return VerifyDBResult::CORRUPTED_BLOCK_DB;
            }
            // SYSCOIN
            if (!chainstate.ConnectBlock(block, state, pindex, coins, false, false)) {
                LogPrintf("Verification error: found unconnectable block at %d, hash=%s (%s)\n", pindex->nHeight, pindex->GetBlockHash().ToString(), state.ToString());
                return VerifyDBResult::CORRUPTED_BLOCK_DB;
            }
            if (ShutdownRequested()) return VerifyDBResult::INTERRUPTED;
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
bool Chainstate::RollforwardBlock(const CBlockIndex* pindex, CCoinsViewCache& inputs, AssetMap &mapAssets, NEVMTxRootMap &mapNEVMTxRoots, NEVMMintTxMap &mapMintKeys, PoDAMAPMemory &mapPoDA, std::vector<std::pair<uint256, uint32_t> > &vecTXIDPairs)
{
    // TODO: merge with ConnectBlock
    CBlock block;
    // SYSCOIN 
    const auto& chainParams = m_chainman.GetParams().GetConsensus();
    if (!ReadBlockFromDisk(block, pindex, chainParams)) {
        return error("ReplayBlock(): ReadBlockFromDisk failed at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
    }
    // SYSCOIN
    const uint256& blockHash = pindex->GetBlockHash();
    const bool ibd = IsInitialBlockDownload();
    // MUST process special txes before updating UTXO to ensure consistency between mempool and block processing
    BlockValidationState state;
    if (!ProcessSpecialTxsInBlock(m_blockman, block, pindex, state, inputs, false /*fJustCheck*/, false /*fScriptChecks*/)) {
        return error("%s: ProcessSpecialTxsInBlock for block %s failed with %s", __func__,
            pindex->GetBlockHash().ToString(), state.ToString());
    }
    if(!ProcessNEVMData(m_chainman.m_blockman, block, m_chain.Tip()->GetMedianTimePast(), TicksSinceEpoch<std::chrono::seconds>(m_chainman.m_options.adjusted_time_callback()), mapPoDA)) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "poda-validation-failed");
    }
    for (const CTransactionRef& tx : block.vtx) {
        if (!tx->IsCoinBase()) {
            for (const CTxIn &txin : tx->vin) {
                inputs.SpendCoin(txin.prevout);
            }
            // SYSCOIN
            const uint256& txHash = tx->GetHash();
            if(tx->HasAssets()){
                TxValidationState tx_state;
                CAmount txfee = 0;
                CAssetsMap mapAssetIn;
                CAssetsMap mapAssetOut;
                if (!Consensus::CheckTxInputs(*tx, tx_state, inputs, pindex->nHeight, txfee, mapAssetIn, mapAssetOut)) {
                    return error("%s: Consensus::CheckTxInputs: %s, %s", __func__, txHash.ToString(), tx_state.ToString());
                }
                // just temp var not used in !fJustCheck mode
                if (!CheckSyscoinInputs(ibd, chainParams, *tx, txHash, tx_state, false, (uint32_t)pindex->nHeight, m_chain.Tip()->GetMedianTimePast(), blockHash, false, mapAssets, mapMintKeys, mapAssetIn, mapAssetOut)) {
                    return error("%s: Consensus::CheckSyscoinInputs: %s, %s", __func__, txHash.ToString(), tx_state.ToString());
                }
            }
            vecTXIDPairs.emplace_back(txHash, pindex->nHeight);

        }
        // Pass check = true as every addition may be an overwrite.
        AddCoins(inputs, *tx, pindex->nHeight, true);
    }
    bool bRegTestContext = !fRegTest || (fRegTest && fNEVMConnection);
    if (bRegTestContext && pindex->nHeight >= chainParams.nNEVMStartBlock && !ConnectNEVMCommitment(state, mapNEVMTxRoots, block, pindex->GetBlockHash(), pindex->nHeight, false, mapPoDA)) {
        return error("RollforwardBlock(): ConnectNEVMCommitment() failed at %d, hash=%s state=%s", pindex->nHeight, pindex->GetBlockHash().ToString(), state.ToString());
    }
    return true;
}

bool Chainstate::ReplayBlocks()
{
    LOCK(cs_main);
    CCoinsView& db = this->CoinsDB();
    CCoinsViewCache cache(&db);
    std::vector<uint256> hashHeads = db.GetHeadBlocks();
    // SYSCOIN
    AssetMap mapAssetsDisconnect, mapAssetsConnect;
    PoDAMAPMemory mapPoDAConnect;
    NEVMDataVec vecNEVMDataDisconnect;
    NEVMMintTxMap mapMintKeysDisconnect, mapMintKeysConnect;
    std::vector<uint256> vecNEVMBlocks;
    std::vector<std::pair<uint256,uint32_t> > vecTXIDPairs;
    BlockValidationState state;
    if (hashHeads.empty()) return true; // We're already in a consistent state.
    if (hashHeads.size() != 2) return error("ReplayBlocks(): unknown inconsistent state");

    uiInterface.ShowProgress(_("Replaying blocks…").translated, 0, false);
    LogPrintf("Replaying blocks\n");

    const CBlockIndex* pindexOld = nullptr;  // Old tip during the interrupted flush.
    const CBlockIndex* pindexNew;            // New tip during the interrupted flush.
    const CBlockIndex* pindexFork = nullptr; // Latest block common to both the old and the new tip.

    if (m_blockman.m_block_index.count(hashHeads[0]) == 0) {
        return error("ReplayBlocks(): reorganization to unknown block requested");
    }
    pindexNew = &(m_blockman.m_block_index[hashHeads[0]]);

    if (!hashHeads[1].IsNull()) { // The old tip is allowed to be 0, indicating it's the first flush.
        if (m_blockman.m_block_index.count(hashHeads[1]) == 0) {
            return error("ReplayBlocks(): reorganization from unknown block requested");
        }
        pindexOld = &(m_blockman.m_block_index[hashHeads[1]]);
        pindexFork = LastCommonAncestor(pindexOld, pindexNew);
        assert(pindexFork != nullptr);
    }
    // SYSCOIN
    auto dbTx = evoDb->BeginTransaction();
    // Rollback along the old branch.
    while (pindexOld != pindexFork) {
        if (pindexOld->nHeight > 0) { // Never disconnect the genesis block.
            CBlock block;
            if (!ReadBlockFromDisk(block, pindexOld, m_chainman.GetConsensus())) {
                return error("RollbackBlock(): ReadBlockFromDisk() failed at %d, hash=%s", pindexOld->nHeight, pindexOld->GetBlockHash().ToString());
            }
            LogPrintf("Rolling back %s (%i)\n", pindexOld->GetBlockHash().ToString(), pindexOld->nHeight);
            // SYSCOIN
            DisconnectResult res = DisconnectBlock(block, pindexOld, cache, mapAssetsDisconnect, mapMintKeysDisconnect, vecNEVMDataDisconnect, vecNEVMBlocks, vecTXIDPairs);
            if (res == DISCONNECT_FAILED) {
                return error("RollbackBlock(): DisconnectBlock failed at %d, hash=%s", pindexOld->nHeight, pindexOld->GetBlockHash().ToString());
            }
            // If DISCONNECT_UNCLEAN is returned, it means a non-existing UTXO was deleted, or an existing UTXO was
            // overwritten. It corresponds to cases where the block-to-be-disconnect never had all its operations
            // applied to the UTXO set. However, as both writing a UTXO and deleting a UTXO are idempotent operations,
            // the result is still a version of the UTXO set with the effects of that block undone.
        }
        pindexOld = pindexOld->pprev;
    }
    // SYSCOIN must flush for now because disconnect may remove asset data and rolling forward expects it to be clean from db
    if(passetdb != nullptr){
        if(!passetdb->Flush(mapAssetsDisconnect) || !passetnftdb->Flush(mapAssetsDisconnect) || !pnevmtxmintdb->FlushErase(mapMintKeysDisconnect) || !pnevmtxrootsdb->FlushErase(vecNEVMBlocks) || !pblockindexdb->FlushErase(vecTXIDPairs) || !pnevmdatadb->FlushEraseMTPs(vecNEVMDataDisconnect)){
            return error("RollbackBlock(): Error flushing to asset dbs on disconnect %s", pindexOld->GetBlockHash().ToString());
        }
    }
    NEVMTxRootMap mapNEVMTxRoots;
    vecTXIDPairs.clear();
    // Roll forward from the forking point to the new tip.
    int nForkHeight = pindexFork ? pindexFork->nHeight : 0;
    for (int nHeight = nForkHeight + 1; nHeight <= pindexNew->nHeight; ++nHeight) {
        const CBlockIndex& pindex{*Assert(pindexNew->GetAncestor(nHeight))};
        LogPrintf("Rolling forward %s (%i)\n", pindex.GetBlockHash().ToString(), nHeight);
        // SYSCOIN
        uiInterface.ShowProgress(_("Replaying blocks…").translated, (int) ((nHeight - nForkHeight) * 100.0 / (pindexNew->nHeight - nForkHeight)) , false);
        if (!RollforwardBlock(&pindex, cache, mapAssetsConnect, mapNEVMTxRoots, mapMintKeysConnect, mapPoDAConnect, vecTXIDPairs)) return false;
    }

    cache.SetBestBlock(pindexNew->GetBlockHash());
    // SYSCOIN
    evoDb->WriteBestBlock(pindexNew->GetBlockHash());
    cache.Flush();
    if(passetdb != nullptr){
        if(!passetdb->Flush(mapAssetsConnect) || !passetnftdb->Flush(mapAssetsConnect) || !pnevmtxmintdb->FlushWrite(mapMintKeysConnect)){
            return error("RollbackBlock(): Error flushing to Syscoin dbs on roll forward %s", pindexOld->GetBlockHash().ToString());
        }
    }
    if(pnevmdatadb)
        pnevmdatadb->FlushDataToCache(mapPoDAConnect, pindexNew->GetMedianTimePast());
    if(pblockindexdb)
        pblockindexdb->FlushDataToCache(vecTXIDPairs);
    if(pnevmtxrootsdb)
        pnevmtxrootsdb->FlushDataToCache(mapNEVMTxRoots);
    dbTx->Commit();
    uiInterface.ShowProgress("", 100, false);
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

void Chainstate::UnloadBlockIndex()
{
    AssertLockHeld(::cs_main);
    nBlockSequenceId = 1;
    setBlockIndexCandidates.clear();
}

bool ChainstateManager::LoadBlockIndex()
{
    AssertLockHeld(cs_main);
    // Load block index from databases
    bool needs_init = fReindex;
    if (!fReindex) {
        bool ret = m_blockman.LoadBlockIndexDB(GetConsensus());
        if (!ret) return false;

        m_blockman.ScanAndUnlinkAlreadyPrunedFiles();

        std::vector<CBlockIndex*> vSortedByHeight{m_blockman.GetAllBlockIndices()};
        std::sort(vSortedByHeight.begin(), vSortedByHeight.end(),
                  CBlockIndexHeightOnlyComparator());

        // Find start of assumed-valid region.
        int first_assumed_valid_height = std::numeric_limits<int>::max();

        for (const CBlockIndex* block : vSortedByHeight) {
            if (block->IsAssumedValid()) {
                auto chainstates = GetAll();

                // If we encounter an assumed-valid block index entry, ensure that we have
                // one chainstate that tolerates assumed-valid entries and another that does
                // not (i.e. the background validation chainstate), since assumed-valid
                // entries should always be pending validation by a fully-validated chainstate.
                auto any_chain = [&](auto fnc) { return std::any_of(chainstates.cbegin(), chainstates.cend(), fnc); };
                assert(any_chain([](auto chainstate) { return chainstate->reliesOnAssumedValid(); }));
                assert(any_chain([](auto chainstate) { return !chainstate->reliesOnAssumedValid(); }));

                first_assumed_valid_height = block->nHeight;
                LogPrintf("Saw first assumedvalid block at height %d (%s)\n",
                        first_assumed_valid_height, block->ToString());
                break;
            }
        }

        for (CBlockIndex* pindex : vSortedByHeight) {
            if (ShutdownRequested()) return false;
            if (pindex->IsAssumedValid() ||
                    (pindex->IsValid(BLOCK_VALID_TRANSACTIONS) &&
                    // SYSCOIN
                    !(pindex->nStatus & BLOCK_CONFLICT_CHAINLOCK) && 
                     (pindex->HaveTxsDownloaded() || pindex->pprev == nullptr))) {

                // Fill each chainstate's block candidate set. Only add assumed-valid
                // blocks to the tip candidate set if the chainstate is allowed to rely on
                // assumed-valid blocks.
                //
                // If all setBlockIndexCandidates contained the assumed-valid blocks, the
                // background chainstate's ActivateBestChain() call would add assumed-valid
                // blocks to the chain (based on how FindMostWorkChain() works). Obviously
                // we don't want this since the purpose of the background validation chain
                // is to validate assued-valid blocks.
                //
                // Note: This is considering all blocks whose height is greater or equal to
                // the first assumed-valid block to be assumed-valid blocks, and excluding
                // them from the background chainstate's setBlockIndexCandidates set. This
                // does mean that some blocks which are not technically assumed-valid
                // (later blocks on a fork beginning before the first assumed-valid block)
                // might not get added to the background chainstate, but this is ok,
                // because they will still be attached to the active chainstate if they
                // actually contain more work.
                //
                // Instead of this height-based approach, an earlier attempt was made at
                // detecting "holistically" whether the block index under consideration
                // relied on an assumed-valid ancestor, but this proved to be too slow to
                // be practical.
                for (Chainstate* chainstate : GetAll()) {
                    if (chainstate->reliesOnAssumedValid() ||
                            pindex->nHeight < first_assumed_valid_height) {
                        chainstate->setBlockIndexCandidates.insert(pindex);
                    }
                }
            }
            if (pindex->nStatus & BLOCK_FAILED_MASK && (!m_best_invalid || pindex->nChainWork > m_best_invalid->nChainWork)) {
                m_best_invalid = pindex;
            }
            if (pindex->IsValid(BLOCK_VALID_TREE) && (m_best_header == nullptr || CBlockIndexWorkComparator()(m_best_header, pindex)))
                m_best_header = pindex;
        }

        needs_init = m_blockman.m_block_index.empty();
    }

    if (needs_init) {
        // Everything here is for *new* reindex/DBs. Thus, though
        // LoadBlockIndexDB may have set fReindex if we shut down
        // mid-reindex previously, we don't check fReindex and
        // instead only check it prior to LoadBlockIndexDB to set
        // needs_init.

        LogPrintf("Initializing databases...\n");
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
        FlatFilePos blockPos{m_blockman.SaveBlockToDisk(block, 0, m_chain, params, nullptr)};
        if (blockPos.IsNull()) {
            return error("%s: writing genesis block to disk failed", __func__);
        }
        CBlockIndex* pindex = m_blockman.AddToBlockIndex(block, m_chainman.m_best_header);
        ReceivedBlockTransactions(block, pindex, blockPos);
    } catch (const std::runtime_error& e) {
        return error("%s: failed to write genesis block: %s", __func__, e.what());
    }

    return true;
}

void Chainstate::LoadExternalBlockFile(
    FILE* fileIn,
    FlatFilePos* dbp,
    std::multimap<uint256, FlatFilePos>* blocks_with_unknown_parent)
{
    AssertLockNotHeld(m_chainstate_mutex);

    // Either both should be specified (-reindex), or neither (-loadblock).
    assert(!dbp == !blocks_with_unknown_parent);

    const auto start{SteadyClock::now()};
    const CChainParams& params{m_chainman.GetParams()};

    int nLoaded = 0;
    try {
        // This takes over fileIn and calls fclose() on it in the CBufferedFile destructor
        CBufferedFile blkdat(fileIn, 2*MAX_BLOCK_SERIALIZED_SIZE, MAX_BLOCK_SERIALIZED_SIZE+8, SER_DISK, CLIENT_VERSION);
        // nRewind indicates where to resume scanning in case something goes wrong,
        // such as a block fails to deserialize.
        uint64_t nRewind = blkdat.GetPos();
        while (!blkdat.eof()) {
            if (ShutdownRequested()) return;

            blkdat.SetPos(nRewind);
            nRewind++; // start one byte further next time, in case of failure
            blkdat.SetLimit(); // remove former limit
            unsigned int nSize = 0;
            try {
                // locate a header
                unsigned char buf[CMessageHeader::MESSAGE_START_SIZE];
                blkdat.FindByte(params.MessageStart()[0]);
                nRewind = blkdat.GetPos() + 1;
                blkdat >> buf;
                if (memcmp(buf, params.MessageStart(), CMessageHeader::MESSAGE_START_SIZE)) {
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
                {
                    LOCK(cs_main);
                    // detect out of order blocks, and store them for later
                    if (hash != params.GetConsensus().hashGenesisBlock && !m_blockman.LookupBlockIndex(header.hashPrevBlock)) {
                        LogPrint(BCLog::REINDEX, "%s: Out of order block %s, parent %s not known\n", __func__, hash.ToString(),
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
                        std::shared_ptr<CBlock> pblock{std::make_shared<CBlock>()};
                        blkdat >> *pblock;
                        nRewind = blkdat.GetPos();

                        BlockValidationState state;
                        if (AcceptBlock(pblock, state, nullptr, true, dbp, nullptr, true)) {
                            nLoaded++;
                        }
                        if (state.IsError()) {
                            break;
                        }
                    } else if (hash != params.GetConsensus().hashGenesisBlock && pindex->nHeight % 1000 == 0) {
                        LogPrint(BCLog::REINDEX, "Block Import: already had block %s at height %d\n", hash.ToString(), pindex->nHeight);
                    }
                }

                // Activate the genesis block so normal node progress can continue
                if (hash == params.GetConsensus().hashGenesisBlock) {
                    BlockValidationState state;
                    if (!ActivateBestChain(state, nullptr)) {
                        break;
                    }
                }

                NotifyHeaderTip(*this);

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
                        if (ReadBlockFromDisk(*pblockrecursive, it->second, params.GetConsensus())) {
                            LogPrint(BCLog::REINDEX, "%s: Processing out of order child %s of %s\n", __func__, pblockrecursive->GetHash().ToString(),
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
                        NotifyHeaderTip(*this);
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
                LogPrint(BCLog::REINDEX, "%s: unexpected data at file offset 0x%x - %s. continuing\n", __func__, (nRewind - 1), e.what());
            }
        }
    } catch (const std::runtime_error& e) {
        AbortNode(std::string("System error: ") + e.what());
    }
    LogPrintf("Loaded %i blocks from external file in %dms\n", nLoaded, Ticks<std::chrono::milliseconds>(SteadyClock::now() - start));
}

void Chainstate::CheckBlockIndex()
{
    if (!m_chainman.ShouldCheckBlockIndex()) {
        return;
    }

    LOCK(cs_main);

    // During a reindex, we read the genesis block and call CheckBlockIndex before ActivateBestChain,
    // so we have the genesis block in m_blockman.m_block_index but no active chain. (A few of the
    // tests when iterating the block tree require that m_chain has been initialized.)
    if (m_chain.Height() < 0) {
        assert(m_blockman.m_block_index.size() <= 1);
        return;
    }

    // Build forward-pointing map of the entire block tree.
    std::multimap<CBlockIndex*,CBlockIndex*> forward;
    for (auto& [_, block_index] : m_blockman.m_block_index) {
        forward.emplace(block_index.pprev, &block_index);
    }

    assert(forward.size() == m_blockman.m_block_index.size());

    std::pair<std::multimap<CBlockIndex*,CBlockIndex*>::iterator,std::multimap<CBlockIndex*,CBlockIndex*>::iterator> rangeGenesis = forward.equal_range(nullptr);
    CBlockIndex *pindex = rangeGenesis.first->second;
    rangeGenesis.first++;
    assert(rangeGenesis.first == rangeGenesis.second); // There is only one index entry with parent nullptr.

    // Iterate over the entire block tree, using depth-first search.
    // Along the way, remember whether there are blocks on the path from genesis
    // block being explored which are the first to have certain properties.
    size_t nNodes = 0;
    int nHeight = 0;
    CBlockIndex* pindexFirstInvalid = nullptr; // Oldest ancestor of pindex which is invalid.
    // SYSCOIN
    CBlockIndex* pindexFirstConflicting = nullptr; // Oldest ancestor of pindex which has BLOCK_CONFLICT_CHAINLOCK
    CBlockIndex* pindexFirstMissing = nullptr; // Oldest ancestor of pindex which does not have BLOCK_HAVE_DATA.
    CBlockIndex* pindexFirstNeverProcessed = nullptr; // Oldest ancestor of pindex for which nTx == 0.
    CBlockIndex* pindexFirstNotTreeValid = nullptr; // Oldest ancestor of pindex which does not have BLOCK_VALID_TREE (regardless of being valid or not).
    CBlockIndex* pindexFirstNotTransactionsValid = nullptr; // Oldest ancestor of pindex which does not have BLOCK_VALID_TRANSACTIONS (regardless of being valid or not).
    CBlockIndex* pindexFirstNotChainValid = nullptr; // Oldest ancestor of pindex which does not have BLOCK_VALID_CHAIN (regardless of being valid or not).
    CBlockIndex* pindexFirstNotScriptsValid = nullptr; // Oldest ancestor of pindex which does not have BLOCK_VALID_SCRIPTS (regardless of being valid or not).
    while (pindex != nullptr) {
        nNodes++;
        if (pindexFirstInvalid == nullptr && pindex->nStatus & BLOCK_FAILED_VALID) pindexFirstInvalid = pindex;
        // SYSCOIN
        if (pindexFirstConflicting == nullptr && pindex->nStatus & BLOCK_CONFLICT_CHAINLOCK) {
            pindexFirstConflicting = pindex;
        }
        // Assumed-valid index entries will not have data since we haven't downloaded the
        // full block yet.
        if (pindexFirstMissing == nullptr && !(pindex->nStatus & BLOCK_HAVE_DATA) && !pindex->IsAssumedValid()) {
            pindexFirstMissing = pindex;
        }
        if (pindexFirstNeverProcessed == nullptr && pindex->nTx == 0) pindexFirstNeverProcessed = pindex;
        if (pindex->pprev != nullptr && pindexFirstNotTreeValid == nullptr && (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_TREE) pindexFirstNotTreeValid = pindex;

        if (pindex->pprev != nullptr && !pindex->IsAssumedValid()) {
            // Skip validity flag checks for BLOCK_ASSUMED_VALID index entries, since these
            // *_VALID_MASK flags will not be present for index entries we are temporarily assuming
            // valid.
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
            assert(pindex->GetBlockHash() == m_chainman.GetConsensus().hashGenesisBlock); // Genesis block's hash must match.
            assert(pindex == m_chain.Genesis()); // The current active chain's genesis block must be this block.
        }
        if (!pindex->HaveTxsDownloaded()) assert(pindex->nSequenceId <= 0); // nSequenceId can't be set positive for blocks that aren't linked (negative is used for preciousblock)
        // VALID_TRANSACTIONS is equivalent to nTx > 0 for all nodes (whether or not pruning has occurred).
        // HAVE_DATA is only equivalent to nTx > 0 (or VALID_TRANSACTIONS) if no pruning has occurred.
        // Unless these indexes are assumed valid and pending block download on a
        // background chainstate.
        if (!m_blockman.m_have_pruned && !pindex->IsAssumedValid()) {
            // If we've never pruned, then HAVE_DATA should be equivalent to nTx > 0
            assert(!(pindex->nStatus & BLOCK_HAVE_DATA) == (pindex->nTx == 0));
            assert(pindexFirstMissing == pindexFirstNeverProcessed);
        } else {
            // If we have pruned, then we can only say that HAVE_DATA implies nTx > 0
            if (pindex->nStatus & BLOCK_HAVE_DATA) assert(pindex->nTx > 0);
        }
        if (pindex->nStatus & BLOCK_HAVE_UNDO) assert(pindex->nStatus & BLOCK_HAVE_DATA);
        if (pindex->IsAssumedValid()) {
            // Assumed-valid blocks should have some nTx value.
            assert(pindex->nTx > 0);
            // Assumed-valid blocks should connect to the main chain.
            assert((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_TREE);
        } else {
            // Otherwise there should only be an nTx value if we have
            // actually seen a block's transactions.
            assert(((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_TRANSACTIONS) == (pindex->nTx > 0)); // This is pruning-independent.
        }
        // All parents having had data (at some point) is equivalent to all parents being VALID_TRANSACTIONS, which is equivalent to HaveTxsDownloaded().
        assert((pindexFirstNeverProcessed == nullptr) == pindex->HaveTxsDownloaded());
        assert((pindexFirstNotTransactionsValid == nullptr) == pindex->HaveTxsDownloaded());
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
        // SYSCOIN
        if (pindexFirstConflicting == nullptr) {
            // Checks for not-conflicting blocks.
            assert((pindex->nStatus & BLOCK_CONFLICT_CHAINLOCK) == 0); // The conflicting mask cannot be set for blocks without conflicting parents.
        }
        if (!CBlockIndexWorkComparator()(pindex, m_chain.Tip()) && pindexFirstNeverProcessed == nullptr) {
            // SYSCOIN
            if (pindexFirstInvalid == nullptr && pindexFirstConflicting == nullptr) {
                const bool is_active = this == &m_chainman.ActiveChainstate();

                // If this block sorts at least as good as the current tip and
                // is valid and we have all data for its parents, it must be in
                // setBlockIndexCandidates.  m_chain.Tip() must also be there
                // even if some data has been pruned.
                //
                // Don't perform this check for the background chainstate since
                // its setBlockIndexCandidates shouldn't have some entries (i.e. those past the
                // snapshot block) which do exist in the block index for the active chainstate.
                if (is_active && (pindexFirstMissing == nullptr || pindex == m_chain.Tip())) {
                    assert(setBlockIndexCandidates.count(pindex));
                }
                // If some parent is missing, then it could be that this block was in
                // setBlockIndexCandidates but had to be removed because of the missing data.
                // In this case it must be in m_blocks_unlinked -- see test below.
            }
        } else { // If this block sorts worse than the current tip or some ancestor's block has never been seen, it cannot be in setBlockIndexCandidates.
            assert(setBlockIndexCandidates.count(pindex) == 0);
        }
        // Check whether this block is in m_blocks_unlinked.
        std::pair<std::multimap<CBlockIndex*,CBlockIndex*>::iterator,std::multimap<CBlockIndex*,CBlockIndex*>::iterator> rangeUnlinked = m_blockman.m_blocks_unlinked.equal_range(pindex->pprev);
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
            assert(m_blockman.m_have_pruned); // We must have pruned.
            // This block may have entered m_blocks_unlinked if:
            //  - it has a descendant that at some point had more work than the
            //    tip, and
            //  - we tried switching to that descendant but were missing
            //    data for some intermediate block between m_chain and the
            //    tip.
            // So if this block is itself better than m_chain.Tip() and it wasn't in
            // setBlockIndexCandidates, then it must be in m_blocks_unlinked.
            if (!CBlockIndexWorkComparator()(pindex, m_chain.Tip()) && setBlockIndexCandidates.count(pindex) == 0) {
                if (pindexFirstInvalid == nullptr) {
                    assert(foundInUnlinked);
                }
            }
        }
        // assert(pindex->GetBlockHash() == pindex->GetBlockHeader().GetHash()); // Perhaps too slow
        // End: actual consistency checks.

        // Try descending into the first subnode.
        std::pair<std::multimap<CBlockIndex*,CBlockIndex*>::iterator,std::multimap<CBlockIndex*,CBlockIndex*>::iterator> range = forward.equal_range(pindex);
        if (range.first != range.second) {
            // A subnode was found.
            pindex = range.first->second;
            nHeight++;
            continue;
        }
        // This is a leaf node.
        // Move upwards until we reach a node of which we have not yet visited the last child.
        while (pindex) {
            // We are going to either move to a parent or a sibling of pindex.
            // If pindex was the first with a certain property, unset the corresponding variable.
            if (pindex == pindexFirstInvalid) pindexFirstInvalid = nullptr;
            // SYSCOIN
            if (pindex == pindexFirstConflicting) pindexFirstConflicting = nullptr;
            if (pindex == pindexFirstMissing) pindexFirstMissing = nullptr;
            if (pindex == pindexFirstNeverProcessed) pindexFirstNeverProcessed = nullptr;
            if (pindex == pindexFirstNotTreeValid) pindexFirstNotTreeValid = nullptr;
            if (pindex == pindexFirstNotTransactionsValid) pindexFirstNotTransactionsValid = nullptr;
            if (pindex == pindexFirstNotChainValid) pindexFirstNotChainValid = nullptr;
            if (pindex == pindexFirstNotScriptsValid) pindexFirstNotScriptsValid = nullptr;
            // Find our parent.
            CBlockIndex* pindexPar = pindex->pprev;
            // Find which child we just visited.
            std::pair<std::multimap<CBlockIndex*,CBlockIndex*>::iterator,std::multimap<CBlockIndex*,CBlockIndex*>::iterator> rangePar = forward.equal_range(pindexPar);
            while (rangePar.first->second != pindex) {
                assert(rangePar.first != rangePar.second); // Our parent must have at least the node we're coming from as child.
                rangePar.first++;
            }
            // Proceed to the next one.
            rangePar.first++;
            if (rangePar.first != rangePar.second) {
                // Move to the sibling.
                pindex = rangePar.first->second;
                break;
            } else {
                // Move up further.
                pindex = pindexPar;
                nHeight--;
                continue;
            }
        }
    }

    // Check that we actually traversed the entire map.
    assert(nNodes == forward.size());
}

std::string Chainstate::ToString()
{
    CBlockIndex* tip = m_chain.Tip();
    return strprintf("Chainstate [%s] @ height %d (%s)",
                     m_from_snapshot_blockhash ? "snapshot" : "ibd",
                     tip ? tip->nHeight : -1, tip ? tip->GetBlockHash().ToString() : "null");
}

bool Chainstate::ResizeCoinsCaches(size_t coinstip_size, size_t coinsdb_size)
{
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
        CoinsTip().ReallocateCache();
    }
    return ret;
}

//! Guess how far we are in the verification process at the given block index
//! require cs_main if pindex has not been validated yet (because nChainTx might be unset)
double GuessVerificationProgress(const ChainTxData& data, const CBlockIndex *pindex) {
    if (pindex == nullptr)
        return 0.0;

    int64_t nNow = time(nullptr);

    double fTxTotal;

    if (pindex->nChainTx <= data.nTxCount) {
        fTxTotal = data.nTxCount + (nNow - data.nTime) * data.dTxRate;
    } else {
        fTxTotal = pindex->nChainTx + (nNow - pindex->GetBlockTime()) * data.dTxRate;
    }

    return std::min<double>(pindex->nChainTx / fTxTotal, 1.0);
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

const AssumeutxoData* ExpectedAssumeutxo(
    const int height, const CChainParams& chainparams)
{
    const MapAssumeutxo& valid_assumeutxos_map = chainparams.Assumeutxo();
    const auto assumeutxo_found = valid_assumeutxos_map.find(height);

    if (assumeutxo_found != valid_assumeutxos_map.end()) {
        return &assumeutxo_found->second;
    }
    return nullptr;
}

static bool DeleteCoinsDBFromDisk(const fs::path db_path, bool is_snapshot)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    AssertLockHeld(::cs_main);

    if (is_snapshot) {
        fs::path base_blockhash_path = db_path / node::SNAPSHOT_BLOCKHASH_FILENAME;

        if (fs::exists(base_blockhash_path)) {
            bool removed = fs::remove(base_blockhash_path);
            if (!removed) {
                LogPrintf("[snapshot] failed to remove file %s\n",
                          fs::PathToString(base_blockhash_path));
            }
        } else {
            LogPrintf("[snapshot] snapshot chainstate dir being removed lacks %s file\n",
                    fs::PathToString(node::SNAPSHOT_BLOCKHASH_FILENAME));
        }
    }

    std::string path_str = fs::PathToString(db_path);
    LogPrintf("Removing leveldb dir at %s\n", path_str);

    // We have to destruct before this call leveldb::DB in order to release the db
    // lock, otherwise `DestroyDB` will fail. See `leveldb::~DBImpl()`.
    const bool destroyed = dbwrapper::DestroyDB(path_str, {}).ok();

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

bool ChainstateManager::ActivateSnapshot(
        AutoFile& coins_file,
        const SnapshotMetadata& metadata,
        bool in_memory)
{
    uint256 base_blockhash = metadata.m_base_blockhash;

    if (this->SnapshotBlockhash()) {
        LogPrintf("[snapshot] can't activate a snapshot-based chainstate more than once\n");
        return false;
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

    bool snapshot_ok = this->PopulateAndValidateSnapshot(
        *snapshot_chainstate, coins_file, metadata);

    // If not in-memory, persist the base blockhash for use during subsequent
    // initialization.
    if (!in_memory) {
        LOCK(::cs_main);
        if (!node::WriteSnapshotBaseBlockhash(*snapshot_chainstate)) {
            snapshot_ok = false;
        }
    }
    if (!snapshot_ok) {
        LOCK(::cs_main);
        this->MaybeRebalanceCaches();

        // PopulateAndValidateSnapshot can return (in error) before the leveldb datadir
        // has been created, so only attempt removal if we got that far.
        if (auto snapshot_datadir = node::FindSnapshotChainstateDir()) {
            // We have to destruct leveldb::DB in order to release the db lock, otherwise
            // DestroyDB() (in DeleteCoinsDBFromDisk()) will fail. See `leveldb::~DBImpl()`.
            // Destructing the chainstate (and so resetting the coinsviews object) does this.
            snapshot_chainstate.reset();
            bool removed = DeleteCoinsDBFromDisk(*snapshot_datadir, /*is_snapshot=*/true);
            if (!removed) {
                AbortNode(strprintf("Failed to remove snapshot chainstate dir (%s). "
                    "Manually remove it before restarting.\n", fs::PathToString(*snapshot_datadir)));
            }
        }
        return false;
    }

    {
        LOCK(::cs_main);
        assert(!m_snapshot_chainstate);
        m_snapshot_chainstate.swap(snapshot_chainstate);
        const bool chaintip_loaded = m_snapshot_chainstate->LoadChainTip();
        assert(chaintip_loaded);

        m_active_chainstate = m_snapshot_chainstate.get();

        LogPrintf("[snapshot] successfully activated snapshot %s\n", base_blockhash.ToString());
        LogPrintf("[snapshot] (%.2f MB)\n",
            m_snapshot_chainstate->CoinsTip().DynamicMemoryUsage() / (1000 * 1000));

        this->MaybeRebalanceCaches();
    }
    return true;
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
    const char* what() const throw() override
    {
        return "ComputeUTXOStats interrupted by shutdown.";
    }
};

static void SnapshotUTXOHashBreakpoint()
{
    if (ShutdownRequested()) throw StopHashingException();
}

bool ChainstateManager::PopulateAndValidateSnapshot(
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
        // Needed for ComputeUTXOStats and ExpectedAssumeutxo to determine the
        // height and to avoid a crash when base_blockhash.IsNull()
        LogPrintf("[snapshot] Did not find snapshot start blockheader %s\n",
                  base_blockhash.ToString());
        return false;
    }

    int base_height = snapshot_start_block->nHeight;
    auto maybe_au_data = ExpectedAssumeutxo(base_height, GetParams());

    if (!maybe_au_data) {
        LogPrintf("[snapshot] assumeutxo height in snapshot metadata not recognized " /* Continued */
                  "(%d) - refusing to load snapshot\n", base_height);
        return false;
    }

    const AssumeutxoData& au_data = *maybe_au_data;

    COutPoint outpoint;
    Coin coin;
    const uint64_t coins_count = metadata.m_coins_count;
    uint64_t coins_left = metadata.m_coins_count;

    LogPrintf("[snapshot] loading coins from snapshot %s\n", base_blockhash.ToString());
    int64_t coins_processed{0};

    while (coins_left > 0) {
        try {
            coins_file >> outpoint;
            coins_file >> coin;
        } catch (const std::ios_base::failure&) {
            LogPrintf("[snapshot] bad snapshot format or truncated snapshot after deserializing %d coins\n",
                      coins_count - coins_left);
            return false;
        }
        if (coin.nHeight > base_height ||
            outpoint.n >= std::numeric_limits<decltype(outpoint.n)>::max() // Avoid integer wrap-around in coinstats.cpp:ApplyHash
        ) {
            LogPrintf("[snapshot] bad snapshot data after deserializing %d coins\n",
                      coins_count - coins_left);
            return false;
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
            if (ShutdownRequested()) {
                return false;
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

    // Important that we set this. This and the coins_cache accesses above are
    // sort of a layer violation, but either we reach into the innards of
    // CCoinsViewCache here or we have to invert some of the Chainstate to
    // embed them in a snapshot-activation-specific CCoinsViewCache bulk load
    // method.
    coins_cache.SetBestBlock(base_blockhash);

    bool out_of_coins{false};
    try {
        coins_file >> outpoint;
    } catch (const std::ios_base::failure&) {
        // We expect an exception since we should be out of coins.
        out_of_coins = true;
    }
    if (!out_of_coins) {
        LogPrintf("[snapshot] bad snapshot - coins left over after deserializing %d coins\n",
            coins_count);
        return false;
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
            CoinStatsHashType::HASH_SERIALIZED, snapshot_coinsdb, m_blockman, SnapshotUTXOHashBreakpoint);
    } catch (StopHashingException const&) {
        return false;
    }
    if (!maybe_stats.has_value()) {
        LogPrintf("[snapshot] failed to generate coins stats\n");
        return false;
    }

    // Assert that the deserialized chainstate contents match the expected assumeutxo value.
    if (AssumeutxoHash{maybe_stats->hashSerialized} != au_data.hash_serialized) {
        LogPrintf("[snapshot] bad snapshot content hash: expected %s, got %s\n",
            au_data.hash_serialized.ToString(), maybe_stats->hashSerialized.ToString());
        return false;
    }

    snapshot_chainstate.m_chain.SetTip(*snapshot_start_block);

    // The remainder of this function requires modifying data protected by cs_main.
    LOCK(::cs_main);

    // Fake various pieces of CBlockIndex state:
    CBlockIndex* index = nullptr;

    // Don't make any modifications to the genesis block.
    // This is especially important because we don't want to erroneously
    // apply BLOCK_ASSUMED_VALID to genesis, which would happen if we didn't skip
    // it here (since it apparently isn't BLOCK_VALID_SCRIPTS).
    constexpr int AFTER_GENESIS_START{1};

    for (int i = AFTER_GENESIS_START; i <= snapshot_chainstate.m_chain.Height(); ++i) {
        index = snapshot_chainstate.m_chain[i];

        // Fake nTx so that LoadBlockIndex() loads assumed-valid CBlockIndex
        // entries (among other things)
        if (!index->nTx) {
            index->nTx = 1;
        }
        // Fake nChainTx so that GuessVerificationProgress reports accurately
        index->nChainTx = index->pprev->nChainTx + index->nTx;

        // Mark unvalidated block index entries beneath the snapshot base block as assumed-valid.
        if (!index->IsValid(BLOCK_VALID_SCRIPTS)) {
            // This flag will be removed once the block is fully validated by a
            // background chainstate.
            index->nStatus |= BLOCK_ASSUMED_VALID;
        }

        // Fake BLOCK_OPT_WITNESS so that Chainstate::NeedsRedownload()
        // won't ask to rewind the entire assumed-valid chain on startup.
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
    index->nChainTx = au_data.nChainTx;
    snapshot_chainstate.setBlockIndexCandidates.insert(snapshot_start_block);

    LogPrintf("[snapshot] validated snapshot (%.2f MB)\n",
        coins_cache.DynamicMemoryUsage() / (1000 * 1000));
    return true;
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
SnapshotCompletionResult ChainstateManager::MaybeCompleteSnapshotValidation(
      std::function<void(bilingual_str)> shutdown_fnc)
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
            "The invalid snapshot chainstate has been left on disk in case it is "
            "helpful in diagnosing the issue that caused this error."),
            PACKAGE_NAME, snapshot_tip_height, snapshot_base_height, snapshot_base_height, PACKAGE_BUGREPORT
        );

        LogPrintf("[snapshot] !!! %s\n", user_error.original);
        LogPrintf("[snapshot] deleting snapshot, reverting to validated chain, and stopping node\n");

        m_active_chainstate = m_ibd_chainstate.get();
        m_snapshot_chainstate->m_disabled = true;
        assert(!this->IsUsable(m_snapshot_chainstate.get()));
        assert(this->IsUsable(m_ibd_chainstate.get()));

        m_snapshot_chainstate->InvalidateCoinsDBOnDisk();

        shutdown_fnc(user_error);
    };

    if (index_new.GetBlockHash() != snapshot_blockhash) {
        LogPrintf("[snapshot] supposed base block %s does not match the " /* Continued */
          "snapshot base block %s (height %d). Snapshot is not valid.",
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

    auto maybe_au_data = ExpectedAssumeutxo(curr_height, ::Params());
    if (!maybe_au_data) {
        LogPrintf("[snapshot] assumeutxo data not found for height " /* Continued */
            "(%d) - refusing to validate snapshot\n", curr_height);
        handle_invalid_snapshot();
        return SnapshotCompletionResult::MISSING_CHAINPARAMS;
    }

    const AssumeutxoData& au_data = *maybe_au_data;
    std::optional<CCoinsStats> maybe_ibd_stats;
    LogPrintf("[snapshot] computing UTXO stats for background chainstate to validate " /* Continued */
        "snapshot - this could take a few minutes\n");
    try {
        maybe_ibd_stats = ComputeUTXOStats(
            CoinStatsHashType::HASH_SERIALIZED,
            &ibd_coins_db,
            m_blockman,
            SnapshotUTXOHashBreakpoint);
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


// SYSCOIN
bool CBlockIndexDB::ReadBlockHeight(const uint256& txid, uint32_t& nHeight) {
    auto it = mapCache.find(txid);
    if(it != mapCache.end()){
        nHeight = it->second;
        return true;
    } else {
        return Read(txid, nHeight);
    }
    return false;
}
bool CBlockIndexDB::FlushErase(const std::vector<std::pair<uint256,uint32_t> > &vecTXIDPairs) {	
    if(vecTXIDPairs.empty())	
        return true;
    CDBBatch batch(*this);
    FlushErase(vecTXIDPairs, batch);
    return WriteBatch(batch, true);
}	
bool CBlockIndexDB::FlushErase(const std::vector<std::pair<uint256,uint32_t> > &vecTXIDPairs, CDBBatch &batch) {	
    if(vecTXIDPairs.empty())	
        return true;
    for (const auto &pair : vecTXIDPairs) {	
        batch.Erase(pair.first);
        auto it = mapCache.find(pair.first);
        if(it != mapCache.end()){
            mapCache.erase(it);
        }
    }
    LogPrint(BCLog::SYS, "Flushing %d block index removals\n", vecTXIDPairs.size());	
    return true;
}
void CBlockIndexDB::FlushDataToCache(const std::vector<std::pair<uint256,uint32_t> > &vecTXIDPairs) {
    if(vecTXIDPairs.empty()) {
        return;
    }
    for (auto const& [key, val] : vecTXIDPairs) {
        mapCache.try_emplace(key, val);
    }
    LogPrint(BCLog::SYS, "Flushing to cache, storing %d block indexes\n", vecTXIDPairs.size());
}
bool CBlockIndexDB::FlushCacheToDisk(const uint32_t &nHeight) {	
    if(mapCache.empty()) {
        return true;
    }
    CDBBatch batch(*this);  
    Prune(nHeight, batch);
    for (auto const& [key, val] : mapCache) {
        batch.Write(key, val);
    }
    batch.Write(LAST_KNOWN_HEIGHT_TAG, nHeight);
    LogPrint(BCLog::SYS, "Flush writing %d block indexes\n", mapCache.size());	
    mapCache.clear();
    return WriteBatch(batch, true);
}
bool CBlockIndexDB::Prune(const uint32_t &nHeight, CDBBatch &batch) {
    if(MAX_BLOCK_INDEX > nHeight) {
        LogPrintf("PruneIndex not enough blocks, not pruning\n");
        return true;
    }
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    uint32_t nValue = 0;
    uint32_t cutoffHeight = nHeight - MAX_BLOCK_INDEX;
    std::vector<std::pair<uint256,uint32_t> > vecTXIDPairs;
    uint256 nKey;
    int index = 0;
    while (pcursor->Valid()) {
        index++;
        try {
            if(pcursor->GetValue(nValue) && nValue < cutoffHeight && pcursor->GetKey(nKey)) {
                vecTXIDPairs.emplace_back(std::make_pair(nKey, nValue));
            }
            pcursor->Next();
        }
        catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
    return FlushErase(vecTXIDPairs, batch);
}


void recursive_copy(const fs::path &src, const fs::path &dst)
{
  if (fs::exists(dst)){
    throw std::runtime_error(dst.generic_string() + " exists");
  }

  if (fs::is_directory(src)) {
    TryCreateDirectories(dst);
    for (const auto &item : fs::directory_iterator(src)) {
      recursive_copy(item.path(), dst/fs::u8path(fs::PathToString(item.path().filename())));
    }
  } 
  else if (fs::is_regular_file(src)) {
    fs::copy(src, dst);
  } 
  else {
    throw std::runtime_error(dst.generic_string() + " not dir or file");
  }
}
#ifdef WIN32
    #include <windows.h>
    #include <winnt.h>
    #include <winternl.h>
    #include <stdio.h>
    #include <errno.h>
    #include <assert.h>
    #include <process.h>
    pid_t fork(fs::path appIn, std::string arg)
    {
        std::string app = fs::PathToString(appIn);
        std::string appQuoted = strprintf("%s", fs::quoted(app));
        PROCESS_INFORMATION pi;
        STARTUPINFOW si;
        ZeroMemory(&pi, sizeof(pi));
        ZeroMemory(&si, sizeof(si));
        GetStartupInfoW (&si);
        si.cb = sizeof(si); 
        //Prepare CreateProcess args
        std::wstring appQuoted_w(appQuoted.length(), L' '); // Make room for characters
        std::copy(appQuoted.begin(), appQuoted.end(), appQuoted_w.begin()); // Copy string to wstring.

        std::wstring app_w(app.length(), L' '); // Make room for characters
        std::copy(app.begin(), app.end(), app_w.begin()); // Copy string to wstring.

        std::wstring arg_w(arg.length(), L' '); // Make room for characters
        std::copy(arg.begin(), arg.end(), arg_w.begin()); // Copy string to wstring.

        std::wstring input = appQuoted_w + L" " + arg_w;
        wchar_t* arg_concat = const_cast<wchar_t*>( input.c_str() );
        const wchar_t* app_const = app_w.c_str();
        LogPrintf("CreateProcessW app %s %s\n",app,arg);
        int result = CreateProcessW(app_const, arg_concat, nullptr, nullptr, FALSE, 
              CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        if(!result)
        {
            LogPrintf("CreateProcess failed (%d)\n", GetLastError());
            return 0;
        }
        pid_t pid = (pid_t)pi.dwProcessId;
        hProcessGeth = pi.hProcess;
        CloseHandle(pi.hThread);
        return pid;
    }
#endif
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

std::vector<std::string> SanitizeGethCmdLine(const fs::path& binaryURL, const fs::path& dataDir) {
    const std::vector<std::string> &cmdLine = gArgs.GetArgs("-gethcommandline");
    std::vector<std::string> cmdLineRet;
    cmdLineRet.push_back(fs::PathToString(binaryURL));
    for(const auto &cmd: cmdLine){
        cmdLineRet.push_back(cmd);
    }
    cmdLineRet.push_back("--datadir");
    cmdLineRet.push_back(fs::PathToString(dataDir));
    if(fTestNet || fRegTest) {
        cmdLineRet.push_back("--tanenbaum");
    } else {
        cmdLineRet.push_back("--mainnet");
    }
    // Geth should subscribe to our publisher
    cmdLineRet.push_back("--nevmpub");
    cmdLineRet.push_back(fNEVMSub);
    return cmdLineRet;
}
bool Chainstate::RestartGethNode() {
    if(fNEVMSub.empty()) {
        LogPrintf("RestartGethNode: Could not start Geth. zmqpubnevm not defined\n");
        return false;
    }
    StopGethNode();
#if ENABLE_ZMQ
    if (g_zmq_notification_interface) {
        UnregisterValidationInterface(g_zmq_notification_interface);
        delete g_zmq_notification_interface;
        g_zmq_notification_interface = nullptr;
    }
    g_zmq_notification_interface = CZMQNotificationInterface::Create();
    if(fNEVMConnection) {
        if(!g_zmq_notification_interface) {
            LogPrintf("RestartGethNode: Could not establish ZMQ interface connections, check your ZMQ settings and try again...\n");
        }
    }
    if (g_zmq_notification_interface) {
        RegisterValidationInterface(g_zmq_notification_interface);
    }
#endif
    if(!StartGethNode()) {
        LogPrintf("RestartGethNode: Could not start Geth\n");
        return false;
    }
    {
        LOCK(cs_main);
        if(!ResetLastBlock()) {
            LogPrintf("RestartGethNode: Could not reset last invalid block\n");
            return false;
        }
    }
    bool bResponse = false;
    GetMainSignals().NotifyNEVMComms("startnetwork", bResponse);
    if(!bResponse) {
        LogPrintf("RestartGethNode: Could not start network\n");
        return false;  
    }
    return true;
}
#ifdef WIN32
// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring &wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte (CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
#endif
fs::path FindExecPath(std::string &binArchitectureTag) {
    fs::path fpathDefault;
    #ifdef WIN32
        binArchitectureTag = "windows";
        WCHAR pszExePath[MAX_PATH];
        GetModuleFileNameW(nullptr, pszExePath, ARRAYSIZE(pszExePath));
        fpathDefault = fs::u8path(utf8_encode(std::wstring(pszExePath)));
        fpathDefault = fpathDefault.parent_path();
        return fpathDefault;
    #endif    
    #ifdef MAC_OSX
        binArchitectureTag = "darwin";
        char buf [PATH_MAX];
        uint32_t bufsize = PATH_MAX;
        if(!_NSGetExecutablePath(buf, &bufsize))
            puts(buf);
        fpathDefault = fs::u8path(std::string(buf));
        fpathDefault = fpathDefault.parent_path();
        return fpathDefault;
    #endif
    #ifndef WIN32
        binArchitectureTag = "linux";
        char pszExePath[MAX_PATH+1];
        ssize_t r = readlink("/proc/self/exe", pszExePath, sizeof(pszExePath) - 1);
        if (r == -1)
            return fpathDefault;
        pszExePath[r] = '\0';
        fpathDefault = fs::u8path(std::string(pszExePath));
        fpathDefault = fpathDefault.parent_path();
        return fpathDefault;
    #endif
    return fpathDefault;
}
bool StartGethNode()
{
    LOCK(cs_geth);

    LogPrintf("%s: Starting SysGeth\n", __func__);
    fs::path gethFilename = fs::u8path(GetGethFilename());
    std::string binArchitectureTag;
    const fs::path &fpathDefault = FindExecPath(binArchitectureTag);

    
    fs::path binaryURL = fpathDefault / "../Resources" / gethFilename;
    binaryURL = binaryURL.make_preferred();
    // current executable path
    if(!fs::exists(binaryURL)) {
        fs::path binaryURLTmp = binaryURL;
        binaryURL = fpathDefault / gethFilename;
        binaryURL = binaryURL.make_preferred();
        LogPrintf("Could not find sysgeth in %s, trying %s\n", fs::PathToString(binaryURLTmp), fs::PathToString(binaryURL));
        // current executable path + bin/[os]
        if(!fs::exists(binaryURL)) {
            fs::path binaryURLTmp = binaryURL;
            binaryURL = fpathDefault / fs::u8path("bin") / fs::u8path(binArchitectureTag) / gethFilename;
            binaryURL = binaryURL.make_preferred();
            LogPrintf("Could not find sysgeth in %s, trying %s\n", fs::PathToString(binaryURLTmp), fs::PathToString(binaryURL));
            // $path
            if(!fs::exists(binaryURL)) {
                fs::path binaryURLTmp = binaryURL;
                binaryURL = gethFilename;
                binaryURL = binaryURL.make_preferred();
                LogPrintf("Could not find sysgeth in %s, trying %s\n", fs::PathToString(binaryURLTmp), fs::PathToString(binaryURL));
                // $path + bin/[os]
                if(!fs::exists(binaryURL)) {
                    fs::path binaryURLTmp = binaryURL;
                    binaryURL = fs::u8path("bin") / fs::u8path(binArchitectureTag) / gethFilename;
                    binaryURL = binaryURL.make_preferred();
                    LogPrintf("Could not find sysgeth in %s, trying %s\n", fs::PathToString(binaryURLTmp), fs::PathToString(binaryURL));
                    // usr/local/bin
                    if(!fs::exists(binaryURL)) {
                        fs::path binaryURLTmp = binaryURL;
                        binaryURL = fs::u8path("/usr/local/bin") / gethFilename;
                        binaryURL = binaryURL.make_preferred();
                        LogPrintf("Could not find sysgeth in %s, trying %s\n", fs::PathToString(binaryURLTmp), fs::PathToString(binaryURL));
                        if(!fs::exists(binaryURL)) {
                            fs::path binaryURLTmp = binaryURL;
                            binaryURL = gArgs.GetDataDirBase() / gethFilename;
                            binaryURL = binaryURL.make_preferred();
                            LogPrintf("Could not find sysgeth in %s, trying %s\n", fs::PathToString(binaryURLTmp), fs::PathToString(binaryURL));
                        }
                    }
                }
            }
        }
    }
    if(!fs::exists(binaryURL)) {
        LogPrintf("Could not find sysgeth\n");
        return false;
    }

    const fs::path &dataDir = gArgs.GetDataDirNet() / "geth";
    std::vector<std::string> vecCmdLineStr = SanitizeGethCmdLine(binaryURL, dataDir);
    const fs::path &log = gArgs.GetDataDirNet() / "sysgeth.log";

    #ifndef WIN32
    // Prevent killed child-processes remaining as "defunct"
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sa.sa_flags = SA_NOCLDWAIT;
        
    sigaction( SIGCHLD, &sa, nullptr ) ;
        
    // Duplicate ("fork") the process. Will return zero in the child
    // process, and the child's PID in the parent (or negative on error).
    gethpid = fork() ;
    if( gethpid < 0 ) {
        LogPrintf("Could not start Geth, pid < 0 %d\n", gethpid);
        return false;
    }  
    // TODO: sanitize environment variables as per
    // https://wiki.sei.cmu.edu/confluence/display/c/ENV03-C.+Sanitize+the+environment+when+invoking+external+programs
    if( gethpid == 0 ) {  
        std::vector<char*> commandVector;
        for(const std::string &cmdStr: vecCmdLineStr) {
            commandVector.push_back(const_cast<char*>(cmdStr.c_str()));
        }  
    
        // push NULL to the end of the vector (execvp expects NULL as last element)
        commandVector.push_back(nullptr);
        char **command = commandVector.data();    
        LogPrintf("%s: Starting geth with command line: %s...\n", __func__, command[0]); 
        int err = open(fs::PathToString(log).c_str(), O_RDWR|O_CREAT|O_APPEND, 0600);
        if (err == -1) {
            LogPrintf("Could not open sysgeth.log\n");
        }
        if (-1 == dup2(err, fileno(stderr))) { LogPrintf("Cannot redirect stderr for syssgeth\n"); return false; }   
        fflush(stderr); close(err);                               
        execvp(command[0], &command[0]);
        if (errno != 0) {
            LogPrintf("Geth not found at %s\n", fs::PathToString(binaryURL));
        }
    }
    #else
        std::string commandStr;
        // the first cmd is the binary file which is not needed as binaryURL is that, in windows we only need params passed as commandStr
        vecCmdLineStr.erase(vecCmdLineStr.begin());
        for(const std::string &cmdStr: vecCmdLineStr) {
            commandStr += cmdStr + " ";
        }
        gethpid = fork(binaryURL, commandStr);
        if( gethpid <= 0 ) {
            LogPrintf("Geth not found at %s\n", fs::PathToString(binaryURL));
            return false;
        }
    #endif
    if(gethpid > 0)
        LogPrintf("%s: Geth Started with pid %d\n", __func__, gethpid);
    return true;
}
bool StopGethNode(bool bOnStart)
{
    if(!fNEVMConnection) {
        return false;
    }
    if(!bOnStart) {
        bool bResponse = false;
        // returns without waiting, signals sysgeth to shutdown
        GetMainSignals().NotifyNEVMComms("disconnect", bResponse);
        if(bResponse) {
            LogPrintf("Waiting for sysgeth to shutdown...\n");
            #ifdef WIN32
            if(hProcessGeth) {
                for (int i = 0; i < 50; ++i) {
                    LogPrintf("Geth shutdown check (%d)\n", i);
                    if(WaitForSingleObject(hProcessGeth, 0) == WAIT_OBJECT_0) {
                        CloseHandle(hProcessGeth);
                        return true;
                    }
                    UninterruptibleSleep(std::chrono::milliseconds{2000});
                }
                CloseHandle(hProcessGeth);
            }
            #else
            for (int i = 0; i < 50; ++i) {
                LogPrintf("Geth shutdown check (%d)\n", i);
                int status;
                if(waitpid(gethpid, &status, WNOHANG) != 0 && !WIFEXITED(status)){
                    return true;
                }
                UninterruptibleSleep(std::chrono::milliseconds{2000});
            }
            #endif
        }
    }
    
    #ifndef USE_SYSCALL_SANDBOX
    #if HAVE_SYSTEM
    LogPrintf("Killing any sysgeth processes that may be already running...\n");
    std::string cmd = "pkill -9 -f sysgeth";
    #ifdef WIN32
        cmd = "taskkill /F /T /IM sysgeth.exe >nul 2>&1";
    #endif
    std::thread t(runCommand, cmd);
    if (t.joinable())
        t.join();
    #endif
    #endif
    
    return true;
}
bool DoGethMaintenance() {
    if(ShutdownRequested()) {
        return false;
    }
    // hasn't started yet so start
    if(!fReindexGeth ) {
        LogPrintf("%s: Stopping Geth\n", __func__); 
        StopGethNode(true);
        LogPrintf("%s: Starting Geth because PID's were uninitialized\n", __func__);
        if(!StartGethNode()) {
            LogPrintf("%s: Failed to start Geth\n", __func__); 
            return false;
        }
    } else {
        fReindexGeth = false;
        LogPrintf("%s: Stopping Geth\n", __func__); 
        StopGethNode(true);
        // copy wallet dir if exists
        const fs::path &dataDir = gArgs.GetDataDirNet();
        const fs::path &gethDir = dataDir / "geth";
        const fs::path &gethKeyStoreDir = gethDir / "keystore";
        const fs::path &gethNodeKeyPath = gethDir / "geth" / "nodekey";
        const fs::path &keyStoreTmpDir = dataDir / "keystoretmp";
        const fs::path &nodeKeyTmpDir = dataDir / "nodekeytmp";
        bool existedKeystore = fs::exists(gethKeyStoreDir);
        bool existedKeystoreTmp = fs::exists(keyStoreTmpDir);
        if(existedKeystore && !existedKeystoreTmp){
            LogPrintf("%s: Copying keystore for Geth to a temp directory\n", __func__); 
            try{
                recursive_copy(gethKeyStoreDir, keyStoreTmpDir);
            } catch(const  std::runtime_error& e) {
                LogPrintf("Failed copying keystore geth directory to keystoretmp %s\n", e.what());
                return false;
            }
        }
        bool existedNodekey = fs::exists(gethNodeKeyPath);
        bool existedNodekeyTmp = fs::exists(nodeKeyTmpDir);
        if(existedNodekey && !existedNodekeyTmp){
            LogPrintf("%s: Copying temporary nodekey\n", __func__); 
            try{
                recursive_copy(gethNodeKeyPath, nodeKeyTmpDir);
            } catch(const  std::runtime_error& e) {
                LogPrintf("Failed copying nodekey %s\n", e.what());
                return false;
            }
        }
        LogPrintf("%s: Removing Geth data directory\n", __func__);
        // clean geth data dir
        fs::remove_all(gethDir);
        UninterruptibleSleep(std::chrono::milliseconds{100});
        // replace keystore dir
        if(existedKeystore){
            LogPrintf("%s: Replacing keystore with temp keystore directory\n", __func__);
            try{
                fs::create_directory(gethDir);
                recursive_copy(keyStoreTmpDir, gethKeyStoreDir);
            } catch(const  std::runtime_error& e) {
                LogPrintf("Failed copying keystore geth keystoretmp directory to keystore %s\n", e.what());
                return false;
            }
            fs::remove_all(keyStoreTmpDir);
        }
        // preserve nodekey file
        if(existedNodekey){
            LogPrintf("%s: Replacing nodekey with temp nodekey\n", __func__);
            try{
                fs::create_directory(gethDir / "geth");
                recursive_copy(nodeKeyTmpDir, gethNodeKeyPath);
            } catch(const  std::runtime_error& e) {
                LogPrintf("Failed copying temporary nodekey %s\n", e.what());
                return false;
            }
            fs::remove_all(nodeKeyTmpDir);
        }
        LogPrintf("%s: Restarting Geth \n", __func__);
        if(!StartGethNode()) {
            LogPrintf("%s: Failed to start Geth\n", __func__); 
            return false;
        }
        // set flag that geth is resyncing
        fGethSynced = false;
        LogPrintf("%s: Done, waiting for resync...\n", __func__);
    }
    return true;
}

void ChainstateManager::MaybeRebalanceCaches()
{
    AssertLockHeld(::cs_main);
    bool ibd_usable = this->IsUsable(m_ibd_chainstate.get());
    bool snapshot_usable = this->IsUsable(m_snapshot_chainstate.get());
    assert(ibd_usable || snapshot_usable);

    if (ibd_usable && !snapshot_usable) {
        LogPrintf("[snapshot] allocating all cache to the IBD chainstate\n");
        // Allocate everything to the IBD chainstate.
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
        if (m_snapshot_chainstate->IsInitialBlockDownload()) {
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
    Assert(opts.adjusted_time_callback);
    return std::move(opts);
}

ChainstateManager::ChainstateManager(Options options, node::BlockManager::Options blockman_options)
    : m_options{Flatten(std::move(options))},
      m_blockman{std::move(blockman_options)} {}

ChainstateManager::~ChainstateManager()
{
    LOCK(::cs_main);

    m_versionbitscache.Clear();

    // TODO: The warning cache should probably become non-global
    for (auto& i : warningcache) {
        i.clear();
    }
}
// SYSCOIN
int RPCSerializationFlags()
{
    int flag = 0;
    if (gArgs.GetIntArg("-rpcserialversion", DEFAULT_RPC_SERIALIZE_VERSION) == 0)
        flag |= SERIALIZE_TRANSACTION_NO_WITNESS;
    return flag;
}
bool ChainstateManager::DetectSnapshotChainstate(CTxMemPool* mempool)
{
    assert(!m_snapshot_chainstate);
    std::optional<fs::path> path = node::FindSnapshotChainstateDir();
    if (!path) {
        return false;
    }
    std::optional<uint256> base_blockhash = node::ReadSnapshotBaseBlockhash(*path);
    if (!base_blockhash) {
        return false;
    }
    LogPrintf("[snapshot] detected active snapshot chainstate (%s) - loading\n",
        fs::PathToString(*path));

    this->ActivateExistingSnapshot(mempool, *base_blockhash);
    return true;
}

Chainstate& ChainstateManager::ActivateExistingSnapshot(CTxMemPool* mempool, uint256 base_blockhash)
{
    assert(!m_snapshot_chainstate);
    m_snapshot_chainstate =
        std::make_unique<Chainstate>(mempool, m_blockman, *this, base_blockhash);
    LogPrintf("[snapshot] switching active chainstate to %s\n", m_snapshot_chainstate->ToString());
    m_active_chainstate = m_snapshot_chainstate.get();
    return *m_snapshot_chainstate;
}

bool IsBIP30Repeat(const CBlockIndex& block_index)
{
    return (block_index.nHeight==91842 && block_index.GetBlockHash() == uint256S("0x00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec")) ||
           (block_index.nHeight==91880 && block_index.GetBlockHash() == uint256S("0x00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721"));
}

bool IsBIP30Unspendable(const CBlockIndex& block_index)
{
    return (block_index.nHeight==91722 && block_index.GetBlockHash() == uint256S("0x00000000000271a2dc26e7667f8419f2e15416dc6955e5a6c6cdf3f2574dd08e")) ||
           (block_index.nHeight==91812 && block_index.GetBlockHash() == uint256S("0x00000000000af0aed4792b1acee3d966af36cf5def14935db8de83d6f9306f2f"));
}

void Chainstate::InvalidateCoinsDBOnDisk()
{
    AssertLockHeld(::cs_main);
    // Should never be called on a non-snapshot chainstate.
    assert(m_from_snapshot_blockhash);
    auto storage_path_maybe = this->CoinsDB().StoragePath();
    // Should never be called with a non-existent storage path.
    assert(storage_path_maybe);
    fs::path snapshot_datadir = *storage_path_maybe;

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
        AbortNode(strprintf(
            "Rename of '%s' -> '%s' failed. "
            "You should resolve this by manually moving or deleting the invalid "
            "snapshot directory %s, otherwise you will encounter the same error again "
            "on the next startup.",
            src_str, dest_str, src_str));
    }
}

const CBlockIndex* ChainstateManager::GetSnapshotBaseBlock() const
{
    const auto blockhash_op = this->SnapshotBlockhash();
    if (!blockhash_op) return nullptr;
    return Assert(m_blockman.LookupBlockIndex(*blockhash_op));
}

std::optional<int> ChainstateManager::GetSnapshotBaseHeight() const
{
    const CBlockIndex* base = this->GetSnapshotBaseBlock();
    return base ? std::make_optional(base->nHeight) : std::nullopt;
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
        LogPrintf("[snapshot] snapshot chainstate cleanup cannot happen with " /* Continued */
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

    auto rename_failed_abort = [](
                                   fs::path p_old,
                                   fs::path p_new,
                                   const fs::filesystem_error& err) {
        LogPrintf("%s: error renaming file (%s): %s\n",
                __func__, fs::PathToString(p_old), err.what());
        AbortNode(strprintf(
            "Rename of '%s' -> '%s' failed. "
            "Cannot clean up the background chainstate leveldb directory.",
            fs::PathToString(p_old), fs::PathToString(p_new)));
    };

    try {
        fs::rename(ibd_chainstate_path, tmp_old);
    } catch (const fs::filesystem_error& e) {
        rename_failed_abort(ibd_chainstate_path, tmp_old, e);
        throw;
    }

    LogPrintf("[snapshot] moving snapshot chainstate (%s) to " /* Continued */
              "default chainstate directory (%s)\n",
              fs::PathToString(snapshot_chainstate_path), fs::PathToString(ibd_chainstate_path));

    try {
        fs::rename(snapshot_chainstate_path, ibd_chainstate_path);
    } catch (const fs::filesystem_error& e) {
        rename_failed_abort(snapshot_chainstate_path, ibd_chainstate_path, e);
        throw;
    }

    if (!DeleteCoinsDBFromDisk(tmp_old, /*is_snapshot=*/false)) {
        // No need to AbortNode because once the unneeded bg chainstate data is
        // moved, it will not interfere with subsequent initialization.
        LogPrintf("Deletion of %s failed. Please remove it manually, as the " /* Continued */
                  "directory is now unnecessary.\n",
                  fs::PathToString(tmp_old));
    } else {
        LogPrintf("[snapshot] deleted background chainstate directory (%s)\n",
                  fs::PathToString(ibd_chainstate_path));
    }
    return true;
}
