// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <checkqueue.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_check.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <cuckoocache.h>
#include <flatfile.h>
#include <hash.h>
#include <index/blockfilterindex.h>
#include <index/txindex.h>
#include <logging.h>
#include <logging/timer.h>
#include <node/blockstorage.h>
#include <node/coinstats.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <reverse_iterator.h>
#include <script/script.h>
#include <script/sigcache.h>
#include <shutdown.h>
#include <spork.h>

#include <timedata.h>
#include <tinyformat.h>
#include <txdb.h>
#include <txmempool.h>
#include <ui_interface.h>
#include <uint256.h>
#include <undo.h>
#include <util/check.h> // For NDEBUG compile time check
#include <util/strencodings.h>
#include <util/translation.h>
#include <util/system.h>
#include <validationinterface.h>
#include <warnings.h>

#include <masternode/payments.h>
#include <masternode/sync.h>

#include <evo/evodb.h>
#include <evo/specialtx.h>
#include <evo/specialtxman.h>
#include <governance/governance.h>

#include <llmq/instantsend.h>
#include <llmq/chainlocks.h>

#include <statsd_client.h>

#include <deque>
#include <optional>
#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>

#define MICRO 0.000001
#define MILLI 0.001

/** Maximum kilobytes for transactions to store for processing during reorg */
static const unsigned int MAX_DISCONNECTED_TX_POOL_SIZE = 20000;
/** The pre-allocation chunk size for blk?????.dat files (since 0.8) */
static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x1000000; // 16 MiB
/** The pre-allocation chunk size for rev?????.dat files (since 0.8) */
static const unsigned int UNDOFILE_CHUNK_SIZE = 0x100000; // 1 MiB
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

bool CBlockIndexWorkComparator::operator()(const CBlockIndex* pa, const CBlockIndex* pb) const
{
    // First sort by most total work, ...
    if (pa->nChainWork > pb->nChainWork) return false;
    if (pa->nChainWork < pb->nChainWork) return true;

    // ... then by earliest time received, ...
    if (pa->nSequenceId < pb->nSequenceId) return false;
    if (pa->nSequenceId > pb->nSequenceId) return true;

    // Use pointer address as tie breaker (should only happen with blocks
    // loaded from disk, as those all have id 0).
    if (pa < pb) return false;
    if (pa > pb) return true;

    // Identical blocks.
    return false;
}

ChainstateManager g_chainman;

CChainState& ChainstateActive()
{
    LOCK(::cs_main);
    assert(g_chainman.m_active_chainstate);
    return *g_chainman.m_active_chainstate;
}

CChain& ChainActive()
{
    LOCK(::cs_main);
    return ::ChainstateActive().m_chain;
}

/**
 * Mutex to guard access to validation specific variables, such as reading
 * or changing the chainstate.
 *
 * This may also need to be locked when updating the transaction pool, e.g. on
 * AcceptToMemoryPool. See CTxMemPool::cs comment for details.
 *
 * The transaction pool has a separate lock to allow reading from it and the
 * chainstate at the same time.
 */
RecursiveMutex cs_main;

CBlockIndex* pindexBestHeader = nullptr;
Mutex g_best_block_mutex;
std::condition_variable g_best_block_cv;
uint256 g_best_block;
bool g_parallel_script_checks{false};
std::atomic_bool fImporting(false);
std::atomic_bool fReindex(false);
bool fAddressIndex = DEFAULT_ADDRESSINDEX;
bool fTimestampIndex = DEFAULT_TIMESTAMPINDEX;
bool fSpentIndex = DEFAULT_SPENTINDEX;
bool fHavePruned = false;
bool fPruneMode = false;
bool fRequireStandard = true;
bool fCheckBlockIndex = false;
bool fCheckpointsEnabled = DEFAULT_CHECKPOINTS_ENABLED;
uint64_t nPruneTarget = 0;
int64_t nMaxTipAge = DEFAULT_MAX_TIP_AGE;

std::atomic<bool> fDIP0001ActiveAtTip{false};

uint256 hashAssumeValid;
arith_uint256 nMinimumChainWork;

CFeeRate minRelayTxFee = CFeeRate(DEFAULT_MIN_RELAY_TX_FEE);

// Internal stuff
namespace {
    CBlockIndex* pindexBestInvalid = nullptr;

    RecursiveMutex cs_LastBlockFile;
    std::vector<CBlockFileInfo> vinfoBlockFile;
    int nLastBlockFile = 0;
    /** Global flag to indicate we should check to see if there are
     *  block/undo files that should be deleted.  Set on startup
     *  or if we allocate more file space when we're in prune mode
     */
    bool fCheckForPruning = false;

    /** Dirty block index entries. */
    std::set<CBlockIndex*> setDirtyBlockIndex;

    /** Dirty block file entries. */
    std::set<int> setDirtyFileInfo;
} // anon namespace

CBlockIndex* BlockManager::LookupBlockIndex(const uint256& hash) const
{
    AssertLockHeld(cs_main);
    assert(std::addressof(g_chainman.BlockIndex()) == std::addressof(m_block_index));
    BlockMap::const_iterator it = m_block_index.find(hash);
    return it == m_block_index.end() ? nullptr : it->second;
}

CBlockIndex* BlockManager::FindForkInGlobalIndex(const CChain& chain, const CBlockLocator& locator)
{
    AssertLockHeld(cs_main);

    assert(std::addressof(g_chainman.m_blockman) == std::addressof(*this));
    // Find the latest block common to locator and chain - we expect that
    // locator.vHave is sorted descending by height.
    for (const uint256& hash : locator.vHave) {
        CBlockIndex* pindex = LookupBlockIndex(hash);
        if (pindex) {
            if (chain.Contains(pindex))
                return pindex;
        }
    }
    return chain.Genesis();
}

std::unique_ptr<CBlockTreeDB> pblocktree;

bool CheckInputScripts(const CTransaction& tx, TxValidationState &state, const CCoinsViewCache &inputs, unsigned int flags, bool cacheSigStore, bool cacheFullScriptStore, PrecomputedTransactionData& txdata, std::vector<CScriptCheck> *pvChecks = nullptr);
static FILE* OpenUndoFile(const FlatFilePos &pos, bool fReadOnly = false);
static FlatFileSeq BlockFileSeq();
static FlatFileSeq UndoFileSeq();

bool CheckFinalTx(const CBlockIndex* active_chain_tip, const CTransaction &tx, int flags)
{
    AssertLockHeld(cs_main);
    assert(active_chain_tip); // TODO: Make active_chain_tip a reference
    assert(std::addressof(*::ChainActive().Tip()) == std::addressof(*active_chain_tip));

    // By convention a negative value for flags indicates that the
    // current network-enforced consensus rules should be used. In
    // a future soft-fork scenario that would mean checking which
    // rules would be enforced for the next block and setting the
    // appropriate flags. At the present time no soft-forks are
    // scheduled, so no flags are set.
    flags = std::max(flags, 0);

    // CheckFinalTx() uses active_chain_tip.Height()+1 to evaluate
    // nLockTime because when IsFinalTx() is called within
    // CBlock::AcceptBlock(), the height of the block *being*
    // evaluated is what is used. Thus if we want to know if a
    // transaction can be part of the *next* block, we need to call
    // IsFinalTx() with one more than active_chain_tip.Height().
    const int nBlockHeight = active_chain_tip->nHeight + 1;

    // BIP113 requires that time-locked transactions have nLockTime set to
    // less than the median time of the previous block they're contained in.
    // When the next block is created its previous block will be the current
    // chain tip, so we use that to calculate the median time passed to
    // IsFinalTx() if LOCKTIME_MEDIAN_TIME_PAST is set.
    const int64_t nBlockTime = (flags & LOCKTIME_MEDIAN_TIME_PAST)
                             ? active_chain_tip->GetMedianTimePast()
                             : GetAdjustedTime();

    return IsFinalTx(tx, nBlockHeight, nBlockTime);
}

bool TestLockPointValidity(CChain& active_chain, const LockPoints* lp)
{
    AssertLockHeld(cs_main);
    assert(lp);
    // If there are relative lock times then the maxInputBlock will be set
    // If there are no relative lock times, the LockPoints don't depend on the chain
    if (lp->maxInputBlock) {
        // Check whether ::ChainActive() is an extension of the block at which the LockPoints
        // calculation was valid.  If not LockPoints are no longer valid
        assert(std::addressof(::ChainActive()) == std::addressof(active_chain));
        if (!active_chain.Contains(lp->maxInputBlock)) {
            return false;
        }
    }

    // LockPoints still valid
    return true;
}

bool CheckSequenceLocks(CChainState& active_chainstate,
                        const CTxMemPool& pool,
                        const CTransaction& tx,
                        int flags,
                        LockPoints* lp,
                        bool useExistingLockPoints)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(pool.cs);
    assert(std::addressof(::ChainstateActive()) == std::addressof(active_chainstate));

    CBlockIndex* tip = active_chainstate.m_chain.Tip();
    assert(tip != nullptr);

    CBlockIndex index;
    index.pprev = tip;
    // CheckSequenceLocks() uses active_chainstate.m_chain.Height()+1 to evaluate
    // height based locks because when SequenceLocks() is called within
    // ConnectBlock(), the height of the block *being*
    // evaluated is what is used.
    // Thus if we want to know if a transaction can be part of the
    // *next* block, we need to use one more than active_chainstate.m_chain.Height()
    index.nHeight = tip->nHeight + 1;

    std::pair<int, int64_t> lockPair;
    if (useExistingLockPoints) {
        assert(lp);
        lockPair.first = lp->height;
        lockPair.second = lp->time;
    }
    else {
        // CoinsTip() contains the UTXO set for active_chainstate.m_chain.Tip()
        CCoinsViewMemPool viewMemPool(&active_chainstate.CoinsTip(), pool);
        std::vector<int> prevheights;
        prevheights.resize(tx.vin.size());
        for (size_t txinIndex = 0; txinIndex < tx.vin.size(); txinIndex++) {
            const CTxIn& txin = tx.vin[txinIndex];
            Coin coin;
            if (!viewMemPool.GetCoin(txin.prevout, coin)) {
                return error("%s: Missing input", __func__);
            }
            if (coin.nHeight == MEMPOOL_HEIGHT) {
                // Assume all mempool transaction confirm in the next block
                prevheights[txinIndex] = tip->nHeight + 1;
            } else {
                prevheights[txinIndex] = coin.nHeight;
            }
        }
        lockPair = CalculateSequenceLocks(tx, flags, prevheights, index);
        if (lp) {
            lp->height = lockPair.first;
            lp->time = lockPair.second;
            // Also store the hash of the block with the highest height of
            // all the blocks which have sequence locked prevouts.
            // This hash needs to still be on the chain
            // for these LockPoint calculations to be valid
            // Note: It is impossible to correctly calculate a maxInputBlock
            // if any of the sequence locked inputs depend on unconfirmed txs,
            // except in the special case where the relative lock time/height
            // is 0, which is equivalent to no sequence lock. Since we assume
            // input height of tip+1 for mempool txs and test the resulting
            // lockPair from CalculateSequenceLocks against tip+1.  We know
            // EvaluateSequenceLocks will fail if there was a non-zero sequence
            // lock on a mempool input, so we can use the return value of
            // CheckSequenceLocks to indicate the LockPoints validity
            int maxInputHeight = 0;
            for (const int height : prevheights) {
                // Can ignore mempool inputs since we'll fail if they had non-zero locks
                if (height != tip->nHeight+1) {
                    maxInputHeight = std::max(maxInputHeight, height);
                }
            }
            lp->maxInputBlock = tip->GetAncestor(maxInputHeight);
        }
    }
    return EvaluateSequenceLocks(index, lockPair);
}

bool GetUTXOCoin(const COutPoint& outpoint, Coin& coin)
{
    LOCK(cs_main);
    if (!::ChainstateActive().CoinsTip().GetCoin(outpoint, coin))
        return false;
    if (coin.IsSpent())
        return false;
    return true;
}

int GetUTXOHeight(const COutPoint& outpoint)
{
    // -1 means UTXO is yet unknown or already spent
    Coin coin;
    return GetUTXOCoin(outpoint, coin) ? coin.nHeight : -1;
}

int GetUTXOConfirmations(const COutPoint& outpoint)
{
    // -1 means UTXO is yet unknown or already spent
    LOCK(cs_main);
    int nPrevoutHeight = GetUTXOHeight(outpoint);
    return (nPrevoutHeight > -1 && ::ChainActive().Tip()) ? ::ChainActive().Height() - nPrevoutHeight + 1 : -1;
}

static bool ContextualCheckTransaction(const CTransaction& tx, TxValidationState& state, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int nHeight = pindexPrev == nullptr ? 0 : pindexPrev->nHeight + 1;
    bool fDIP0001Active_context = nHeight >= consensusParams.DIP0001Height;
    bool fDIP0003Active_context = nHeight >= consensusParams.DIP0003Height;

    if (fDIP0003Active_context) {
        // check version 3 transaction types
        if (tx.nVersion >= 3) {
            if (tx.nType != TRANSACTION_NORMAL &&
                tx.nType != TRANSACTION_PROVIDER_REGISTER &&
                tx.nType != TRANSACTION_PROVIDER_UPDATE_SERVICE &&
                tx.nType != TRANSACTION_PROVIDER_UPDATE_REGISTRAR &&
                tx.nType != TRANSACTION_PROVIDER_UPDATE_REVOKE &&
                tx.nType != TRANSACTION_COINBASE &&
                tx.nType != TRANSACTION_QUORUM_COMMITMENT &&
                tx.nType != TRANSACTION_MNHF_SIGNAL &&
                tx.nType != TRANSACTION_ASSET_LOCK &&
                tx.nType != TRANSACTION_ASSET_UNLOCK) {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-type");
            }
            if (tx.IsCoinBase() && tx.nType != TRANSACTION_COINBASE)
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-cb-type");
        } else if (tx.nType != TRANSACTION_NORMAL) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-type");
        }
    }

    // Size limits
    if (fDIP0001Active_context && ::GetSerializeSize(tx, PROTOCOL_VERSION) > MAX_STANDARD_TX_SIZE)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-oversize");

    return true;
}

// Returns the script flags which should be checked for a given block
static unsigned int GetBlockScriptFlags(const CBlockIndex* pindex, const Consensus::Params& chainparams);

static void LimitMempoolSize(CTxMemPool& pool, CCoinsViewCache& coins_cache, size_t limit, std::chrono::seconds age)
    EXCLUSIVE_LOCKS_REQUIRED(pool.cs, ::cs_main)
{
    int expired = pool.Expire(GetTime<std::chrono::seconds>() - age);
    if (expired != 0) {
        LogPrint(BCLog::MEMPOOL, "Expired %i transactions from the memory pool\n", expired);
    }

    std::vector<COutPoint> vNoSpendsRemaining;
    pool.TrimToSize(limit, &vNoSpendsRemaining);
    assert(std::addressof(::ChainstateActive().CoinsTip()) == std::addressof(coins_cache));
    for (const COutPoint& removed : vNoSpendsRemaining)
        coins_cache.Uncache(removed);
}

static bool IsCurrentForFeeEstimation(CChainState& active_chainstate) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    assert(std::addressof(::ChainstateActive()) == std::addressof(active_chainstate));
    if (active_chainstate.IsInitialBlockDownload())
        return false;
    if (active_chainstate.m_chain.Tip()->GetBlockTime() < count_seconds(GetTime<std::chrono::seconds>() - MAX_FEE_ESTIMATION_TIP_AGE))
        return false;
    if (active_chainstate.m_chain.Height() < pindexBestHeader->nHeight - 1)
        return false;
    return true;
}

void CChainState::MaybeUpdateMempoolForReorg(
    DisconnectedBlockTransactions& disconnectpool,
    bool fAddToMempool)
{
    if (!m_mempool) return;

    AssertLockHeld(cs_main);
    AssertLockHeld(m_mempool->cs);
    assert(std::addressof(::ChainstateActive()) == std::addressof(*this));
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
        TxValidationState stateDummy;
        if (!fAddToMempool || (*it)->IsCoinBase() ||
            !AcceptToMemoryPool(*this, *m_mempool, stateDummy, *it,
                                true /* bypass_limits */, 0 /* nAbsurdFee */)) {
            // If the transaction doesn't make it in to the mempool, remove any
            // transactions that depend on it (which would now be orphans).
            m_mempool->removeRecursive(**it, MemPoolRemovalReason::REORG);
        } else if (m_mempool->exists((*it)->GetHash())) {
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

    // We also need to remove any now-immature transactions
    m_mempool->removeForReorg(*this, STANDARD_LOCKTIME_VERIFY_FLAGS);
    // Re-limit mempool size, in case we added any transactions
    LimitMempoolSize(
        *m_mempool,
        this->CoinsTip(),
        gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000,
        std::chrono::hours{gArgs.GetArg("-mempoolexpiry", DEFAULT_MEMPOOL_EXPIRY)});
}

// Used to avoid mempool polluting consensus critical paths if CCoinsViewMempool
// were somehow broken and returning the wrong scriptPubKeys
static bool CheckInputsFromMempoolAndCache(const CTransaction& tx, TxValidationState& state,
                 const CCoinsViewCache& view, const CTxMemPool& pool,
                 unsigned int flags, PrecomputedTransactionData& txdata, CCoinsViewCache& coins_tip)
                 EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    // pool.cs should be locked already, but go ahead and re-take the lock here
    // to enforce that mempool doesn't change between when we check the view
    // and when we actually call through to CheckInputScripts
    LOCK(pool.cs);

    assert(!tx.IsCoinBase());
    for (const CTxIn& txin : tx.vin) {
        const Coin& coin = view.AccessCoin(txin.prevout);

        // AcceptToMemoryPoolWorker has already checked that the coins are
        // available, so this shouldn't fail. If the inputs are not available
        // here then return false.
        if (coin.IsSpent()) return false;

        // Check equivalence for available inputs.
        const CTransactionRef& txFrom = pool.get(txin.prevout.hash);
        if (txFrom) {
            assert(txFrom->GetHash() == txin.prevout.hash);
            assert(txFrom->vout.size() > txin.prevout.n);
            assert(txFrom->vout[txin.prevout.n] == coin.out);
        } else {
            assert(std::addressof(::ChainstateActive().CoinsTip()) == std::addressof(coins_tip));
            const Coin& coinFromDisk = coins_tip.AccessCoin(txin.prevout);
            assert(!coinFromDisk.IsSpent());
            assert(coinFromDisk.out == coin.out);
        }
    }

    // Call CheckInputScripts() to cache signature and script validity against current tip consensus rules.
    return CheckInputScripts(tx, state, view, flags, /* cacheSigStore = */ true, /* cacheFullSciptStore = */ true, txdata);
}

namespace {

class MemPoolAccept
{
public:
    explicit MemPoolAccept(CTxMemPool& mempool, CChainState& active_chainstate) : m_pool(mempool), m_view(&m_dummy), m_viewmempool(&active_chainstate.CoinsTip(), m_pool), m_active_chainstate(active_chainstate),
        m_limit_ancestors(gArgs.GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT)),
        m_limit_ancestor_size(gArgs.GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT)*1000),
        m_limit_descendants(gArgs.GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT)),
        m_limit_descendant_size(gArgs.GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT)*1000) {
        assert(std::addressof(::ChainstateActive()) == std::addressof(m_active_chainstate));
    }

    // We put the arguments we're handed into a struct, so we can pass them
    // around easier.
    struct ATMPArgs {
        const CChainParams& m_chainparams;
        TxValidationState &m_state;
        const int64_t m_accept_time;
        const bool m_bypass_limits;
        const CAmount& m_absurd_fee;
        /*
         * Return any outpoints which were not previously present in the coins
         * cache, but were added as a result of validating the tx for mempool
         * acceptance. This allows the caller to optionally remove the cache
         * additions if the associated transaction ends up being rejected by
         * the mempool.
         */
        std::vector<COutPoint>& m_coins_to_uncache;
        const bool m_test_accept;
    };

    // Single transaction acceptance
    bool AcceptSingleTransaction(const CTransactionRef& ptx, ATMPArgs& args) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

private:
    // All the intermediate state that gets passed between the various levels
    // of checking a given transaction.
    struct Workspace {
        explicit Workspace(const CTransactionRef& ptx) : m_ptx(ptx), m_hash(ptx->GetHash()) {}
        CTxMemPool::setEntries m_ancestors;
        std::unique_ptr<CTxMemPoolEntry> m_entry;

        CAmount m_modified_fees;

        const CTransactionRef& m_ptx;
        const uint256& m_hash;
    };

    // Run the policy checks on a given transaction, excluding any script checks.
    // Looks up inputs, calculates feerate, considers replacement, evaluates
    // package limits, etc. As this function can be invoked for "free" by a peer,
    // only tests that are fast should be done here (to avoid CPU DoS).
    bool PreChecks(ATMPArgs& args, Workspace& ws) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    // Run the script checks using our policy flags. As this can be slow, we should
    // only invoke this on transactions that have otherwise passed policy checks.
    bool PolicyScriptChecks(ATMPArgs& args, Workspace& ws, PrecomputedTransactionData& txdata) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    // Re-run the script checks, using consensus flags, and try to cache the
    // result in the scriptcache. This should be done after
    // PolicyScriptChecks(). This requires that all inputs either be in our
    // utxo set or in the mempool.
    bool ConsensusScriptChecks(ATMPArgs& args, Workspace& ws, PrecomputedTransactionData &txdata) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    // Try to add the transaction to the mempool, removing any conflicts first.
    // Returns true if the transaction is in the mempool after any size
    // limiting is performed, false otherwise.
    bool Finalize(ATMPArgs& args, Workspace& ws) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_pool.cs);

    // Compare a package's feerate against minimum allowed.
    bool CheckFeeRate(size_t package_size, CAmount package_fee, TxValidationState& state)
    {
        CAmount mempoolRejectFee = m_pool.GetMinFee(gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000).GetFee(package_size);
        if (mempoolRejectFee > 0 && package_fee < mempoolRejectFee) {
            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "mempool min fee not met", strprintf("%d < %d", package_fee, mempoolRejectFee));
        }

        if (package_fee < ::minRelayTxFee.GetFee(package_size)) {
            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "min relay fee not met", strprintf("%d < %d", package_fee, ::minRelayTxFee.GetFee(package_size)));
        }
        return true;
    }

private:
    CTxMemPool& m_pool;
    CCoinsViewCache m_view;
    CCoinsViewMemPool m_viewmempool;
    CCoinsView m_dummy;
    CChainState& m_active_chainstate;

    // The package limits in effect at the time of invocation.
    const size_t m_limit_ancestors;
    const size_t m_limit_ancestor_size;
    // These may be modified while evaluating a transaction (eg to account for
    // in-mempool conflicts; see below).
    size_t m_limit_descendants;
    size_t m_limit_descendant_size;
};

bool MemPoolAccept::PreChecks(ATMPArgs& args, Workspace& ws)
{
    const CTransactionRef& ptx = ws.m_ptx;
    const CTransaction& tx = *ws.m_ptx;
    const uint256& hash = ws.m_hash;

    // Copy/alias what we need out of args
    const CChainParams& chainparams = args.m_chainparams;
    TxValidationState &state = args.m_state;
    const int64_t nAcceptTime = args.m_accept_time;
    const bool bypass_limits = args.m_bypass_limits;
    const CAmount& nAbsurdFee = args.m_absurd_fee;
    std::vector<COutPoint>& coins_to_uncache = args.m_coins_to_uncache;

    // Alias what we need out of ws
    CTxMemPool::setEntries& setAncestors = ws.m_ancestors;
    std::unique_ptr<CTxMemPoolEntry>& entry = ws.m_entry;
    CAmount& nModifiedFees = ws.m_modified_fees;

    if (!CheckTransaction(tx, state))
        return false; // state filled in by CheckTransaction

    assert(std::addressof(::ChainstateActive()) == std::addressof(m_active_chainstate));
    if (!ContextualCheckTransaction(tx, state, chainparams.GetConsensus(), m_active_chainstate.m_chain.Tip()))
        return error("%s: ContextualCheckTransaction: %s, %s", __func__, hash.ToString(), state.ToString());

    if (tx.nVersion == 3 && tx.nType == TRANSACTION_QUORUM_COMMITMENT) {
        // quorum commitment is not allowed outside of blocks
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "qc-not-allowed");
    }

    // Coinbase is only valid in a block, not as a loose transaction
    if (tx.IsCoinBase())
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "coinbase");

    // Rather not work on nonstandard transactions (unless -testnet/-regtest)
    std::string reason;
    if (fRequireStandard && !IsStandardTx(tx, reason))
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, reason);

    // Do not work on transactions that are too small.
    // A transaction with 1 empty scriptSig input and 1 P2SH output has size of 83 bytes.
    // Transactions smaller than this are not relayed to mitigate CVE-2017-12842 by not relaying
    // 64-byte transactions.
    if (::GetSerializeSize(tx, PROTOCOL_VERSION) < MIN_STANDARD_TX_SIZE)
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "tx-size-small");

    // Only accept nLockTime-using transactions that can be mined in the next
    // block; we don't want our mempool filled up with transactions that can't
    // be mined yet.
    if (!CheckFinalTx(m_active_chainstate.m_chain.Tip(), tx, STANDARD_LOCKTIME_VERIFY_FLAGS))
        return state.Invalid(TxValidationResult::TX_PREMATURE_SPEND, "non-final");

    // is it already in the memory pool?
    if (m_pool.exists(hash)) {
        statsClient.inc("transactions.duplicate", 1.0f);
        return state.Invalid(TxValidationResult::TX_CONFLICT, "txn-already-in-mempool");
    }

    llmq::CInstantSendLockPtr conflictLock = llmq::quorumInstantSendManager->GetConflictingLock(tx);
    if (conflictLock) {
        uint256 hashBlock;
        CTransactionRef txConflict = GetTransaction(/* block_index */ nullptr, &m_pool, conflictLock->txid, chainparams.GetConsensus(), hashBlock);
        if (txConflict) {
            GetMainSignals().NotifyInstantSendDoubleSpendAttempt(ptx, txConflict);
        }
        LogPrintf("ERROR: AcceptToMemoryPool : Transaction %s conflicts with locked TX %s\n", hash.ToString(), conflictLock->txid.ToString());
        return state.Invalid(TxValidationResult::TX_CONFLICT_LOCK, "tx-txlock-conflict");
    }

    if (llmq::quorumInstantSendManager->IsWaitingForTx(hash)) {
        m_pool.removeConflicts(tx);
        m_pool.removeProTxConflicts(tx);
    } else {
        // Check for conflicts with in-memory transactions
        for (const CTxIn &txin : tx.vin)
        {
            const CTransaction* ptxConflicting = m_pool.GetConflictTx(txin.prevout);
            if (ptxConflicting)
            {
                // Transaction conflicts with mempool and RBF doesn't exist in Dash
                return state.Invalid(TxValidationResult::TX_CONFLICT, "txn-mempool-conflict");
            }
        }
    }
    LockPoints lp;
    m_view.SetBackend(m_viewmempool);

    assert(std::addressof(::ChainstateActive().CoinsTip()) == std::addressof(m_active_chainstate.CoinsTip()));
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

    // Only accept BIP68 sequence locked transactions that can be mined in the next
    // block; we don't want our mempool filled up with transactions that can't
    // be mined yet.
    // Must keep pool.cs for this unless we change CheckSequenceLocks to take a
    // CoinsViewCache instead of create its own
    assert(std::addressof(::ChainstateActive()) == std::addressof(m_active_chainstate));
    if (!CheckSequenceLocks(m_active_chainstate, m_pool, tx, STANDARD_LOCKTIME_VERIFY_FLAGS, &lp))
        return state.Invalid(TxValidationResult::TX_PREMATURE_SPEND, "non-BIP68-final");

    assert(std::addressof(g_chainman.m_blockman) == std::addressof(m_active_chainstate.m_blockman));
    CAmount nFees = 0;
    if (!Consensus::CheckTxInputs(tx, state, m_view, m_active_chainstate.m_blockman.GetSpendHeight(m_view), nFees)) {
        return error("%s: Consensus::CheckTxInputs: %s, %s", __func__, tx.GetHash().ToString(), state.ToString());
    }

    // Check for non-standard pay-to-script-hash in inputs
    if (fRequireStandard && !AreInputsStandard(tx, m_view))
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "bad-txns-nonstandard-inputs");

    unsigned int nSigOps = GetTransactionSigOpCount(tx, m_view, STANDARD_SCRIPT_VERIFY_FLAGS);

    // nModifiedFees includes any fee deltas from PrioritiseTransaction
    nModifiedFees = nFees;
    m_pool.ApplyDelta(hash, nModifiedFees);

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

    assert(std::addressof(::ChainActive()) == std::addressof(m_active_chainstate.m_chain));
    entry.reset(new CTxMemPoolEntry(ptx, nFees, nAcceptTime, m_active_chainstate.m_chain.Height(),
            fSpendsCoinbase, nSigOps, lp));
    unsigned int nSize = entry->GetTxSize();

    if (nSigOps > MAX_STANDARD_TX_SIGOPS)
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, "bad-txns-too-many-sigops",
                strprintf("%d", nSigOps));

    // No transactions are allowed below minRelayTxFee except from disconnected
    // blocks
    if (!bypass_limits && !CheckFeeRate(nSize, nModifiedFees, state)) return false;

    if (nAbsurdFee && nFees > nAbsurdFee)
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD,
                "absurdly-high-fee", strprintf("%d > %d", nFees, nAbsurdFee));

    // Calculate in-mempool ancestors, up to a limit.
    std::string errString;
    if (!m_pool.CalculateMemPoolAncestors(*entry, setAncestors, m_limit_ancestors, m_limit_ancestor_size, m_limit_descendants, m_limit_descendant_size, errString)) {
        setAncestors.clear();
        // If CalculateMemPoolAncestors fails second time, we want the original error string.
        std::string dummy_err_string;
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
        if (nSize >  EXTRA_DESCENDANT_TX_SIZE_LIMIT ||
                !m_pool.CalculateMemPoolAncestors(*entry, setAncestors, 2, m_limit_ancestor_size, m_limit_descendants + 1, m_limit_descendant_size + EXTRA_DESCENDANT_TX_SIZE_LIMIT, dummy_err_string)) {
            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "too-long-mempool-chain", errString);
        }
    }

    // check special TXs after all the other checks. If we'd do this before the other checks, we might end up
    // DoS scoring a node for non-critical errors, e.g. duplicate keys because a TX is received that was already
    // mined
    // NOTE: we use UTXO here and do NOT allow mempool txes as masternode collaterals
    if (!CheckSpecialTx(tx, m_active_chainstate.m_chain.Tip(), m_active_chainstate.CoinsTip(), true, state))
        return false;

    if (m_pool.existsProviderTxConflict(tx)) {
        return state.Invalid(TxValidationResult::TX_CONFLICT, "protx-dup");
    }

    return true;
}

bool MemPoolAccept::PolicyScriptChecks(ATMPArgs& args, Workspace& ws, PrecomputedTransactionData& txdata)
{
    const CTransaction& tx = *ws.m_ptx;

    TxValidationState &state = args.m_state;

    constexpr unsigned int scriptVerifyFlags = STANDARD_SCRIPT_VERIFY_FLAGS;

    // Check input scripts and signatures.
    // This is done last to help prevent CPU exhaustion denial-of-service attacks.
    if (!CheckInputScripts(tx, state, m_view, scriptVerifyFlags, true, false, txdata)) {
        return false; // state filled in by CheckInputScripts
    }

    return true;
}

bool MemPoolAccept::ConsensusScriptChecks(ATMPArgs& args, Workspace& ws, PrecomputedTransactionData& txdata)
{
    const CTransaction& tx = *ws.m_ptx;
    const uint256& hash = ws.m_hash;

    TxValidationState &state = args.m_state;
    const CChainParams& chainparams = args.m_chainparams;

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
    assert(std::addressof(::ChainActive()) == std::addressof(m_active_chainstate.m_chain));
    unsigned int currentBlockScriptVerifyFlags = GetBlockScriptFlags(m_active_chainstate.m_chain.Tip(), chainparams.GetConsensus());
    assert(std::addressof(::ChainstateActive().CoinsTip()) == std::addressof(m_active_chainstate.CoinsTip()));
    if (!CheckInputsFromMempoolAndCache(tx, state, m_view, m_pool, currentBlockScriptVerifyFlags, txdata, m_active_chainstate.CoinsTip())) {
        return error("%s: BUG! PLEASE REPORT THIS! CheckInputScripts failed against latest-block but not STANDARD flags %s, %s",
                __func__, hash.ToString(), state.ToString());
    }

    return true;
}
bool MemPoolAccept::Finalize(ATMPArgs& args, Workspace& ws)
{
    const CTransaction& tx = *ws.m_ptx;
    const uint256& hash = ws.m_hash;
    TxValidationState &state = args.m_state;
    const bool bypass_limits = args.m_bypass_limits;

    CTxMemPool::setEntries& setAncestors = ws.m_ancestors;
    const CAmount& nModifiedFees = ws.m_modified_fees;
    std::unique_ptr<CTxMemPoolEntry>& entry = ws.m_entry;

    // This transaction should only count for fee estimation if:
    // - it's not being re-added during a reorg which bypasses typical mempool fee limits
    // - the node is not behind
    // - the transaction is not dependent on any other transactions in the mempool
    // - the transaction is not a zero fee transaction
    assert(std::addressof(::ChainstateActive()) == std::addressof(m_active_chainstate));
    bool validForFeeEstimation = (nModifiedFees != 0) && !bypass_limits && IsCurrentForFeeEstimation(m_active_chainstate) && m_pool.HasNoInputsOf(tx);

    // Store transaction in memory
    m_pool.addUnchecked(*entry, setAncestors, validForFeeEstimation);
    CAmount nValueOut = tx.GetValueOut();
    unsigned int nSigOps = GetTransactionSigOpCount(tx, m_view, STANDARD_SCRIPT_VERIFY_FLAGS);

    statsClient.count("transactions.sizeBytes", entry->GetTxSize(), 1.0f);
    statsClient.count("transactions.fees", nModifiedFees, 1.0f);
    statsClient.count("transactions.inputValue", nValueOut - nModifiedFees, 1.0f);
    statsClient.count("transactions.outputValue", nValueOut, 1.0f);
    statsClient.count("transactions.sigOps", nSigOps, 1.0f);

    // Add memory address index
    if (fAddressIndex) {
        m_pool.addAddressIndex(*entry, m_view);
    }

    // Add memory spent index
    if (fSpentIndex) {
        m_pool.addSpentIndex(*entry, m_view);
    }

    if (!bypass_limits) {
        assert(std::addressof(::ChainstateActive().CoinsTip()) == std::addressof(m_active_chainstate.CoinsTip()));
        LimitMempoolSize(m_pool, m_active_chainstate.CoinsTip(), gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000, std::chrono::hours{gArgs.GetArg("-mempoolexpiry", DEFAULT_MEMPOOL_EXPIRY)});
        if (!m_pool.exists(hash))
            return state.Invalid(TxValidationResult::TX_MEMPOOL_POLICY, "mempool full");
    }
    return true;
}

bool MemPoolAccept::AcceptSingleTransaction(const CTransactionRef& ptx, ATMPArgs& args)
{
    boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();
    AssertLockHeld(cs_main);
    LOCK(m_pool.cs); // mempool "read lock" (held through GetMainSignals().TransactionAddedToMempool())

    Workspace workspace(ptx);

    if (!PreChecks(args, workspace)) return false;

    // Only compute the precomputed transaction data if we need to verify
    // scripts (ie, other policy checks pass). We perform the inexpensive
    // checks first and avoid hashing and signature verification unless those
    // checks pass, to mitigate CPU exhaustion denial-of-service attacks.
    PrecomputedTransactionData txdata(*ptx);

    if (!PolicyScriptChecks(args, workspace, txdata)) return false;

    if (!ConsensusScriptChecks(args, workspace, txdata)) return false;

    // Tx was accepted, but not added
    if (args.m_test_accept) return true;

    if (!Finalize(args, workspace)) return false;

    const int64_t nAcceptTime = args.m_accept_time;
    GetMainSignals().TransactionAddedToMempool(ptx, nAcceptTime);

    const CTransaction& tx = *ptx;
    boost::posix_time::ptime finish = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration diff = finish - start;
    statsClient.timing("AcceptToMemoryPool_ms", diff.total_milliseconds(), 1.0f);
    statsClient.inc("transactions.accepted", 1.0f);
    statsClient.count("transactions.inputs", tx.vin.size(), 1.0f);
    statsClient.count("transactions.outputs", tx.vout.size(), 1.0f);

    return true;
}

} // anon namespace

/** (try to) add transaction to memory pool with a specified acceptance time **/
static bool AcceptToMemoryPoolWithTime(const CChainParams& chainparams, CTxMemPool& pool, CChainState& active_chainstate, TxValidationState &state, const CTransactionRef &tx,
                        int64_t nAcceptTime, bool bypass_limits,
                        const CAmount nAbsurdFee, bool test_accept) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    std::vector<COutPoint> coins_to_uncache;
    MemPoolAccept::ATMPArgs args { chainparams, state, nAcceptTime, bypass_limits, nAbsurdFee, coins_to_uncache, test_accept };

    assert(std::addressof(::ChainstateActive()) == std::addressof(active_chainstate));
    bool res = MemPoolAccept(pool, active_chainstate).AcceptSingleTransaction(tx, args);
    if (!res || test_accept) {
        if (!res) LogPrint(BCLog::MEMPOOL, "%s: %s %s (%s)\n", __func__, tx->GetHash().ToString(), state.GetRejectReason(), state.GetDebugMessage());

        // Remove coins that were not present in the coins cache before calling;
        // AcceptSingleTransaction(); this is to prevent memory DoS in case we receive a large
        // number of invalid transactions that attempt to overrun the in-memory coins cache
        // (`CCoinsViewCache::cacheCoins`).

        for (const COutPoint& hashTx : coins_to_uncache)
            active_chainstate.CoinsTip().Uncache(hashTx);
    }
    // After we've (potentially) uncached entries, ensure our coins cache is still within its size limits
    BlockValidationState state_dummy;
    active_chainstate.FlushStateToDisk(state_dummy, FlushStateMode::PERIODIC);
    return res;
}

bool AcceptToMemoryPool(CChainState& active_chainstate, CTxMemPool& pool, TxValidationState &state, const CTransactionRef &tx,
                        bool bypass_limits, const CAmount nAbsurdFee, bool test_accept)
{
    const CChainParams& chainparams = Params();
    assert(std::addressof(::ChainstateActive()) == std::addressof(active_chainstate));
    return AcceptToMemoryPoolWithTime(chainparams, pool, active_chainstate, state, tx, GetTime(), bypass_limits, nAbsurdFee, test_accept);
}

bool GetTimestampIndex(const unsigned int &high, const unsigned int &low, std::vector<uint256> &hashes)
{
    if (!fTimestampIndex)
        return error("Timestamp index not enabled");

    if (!pblocktree->ReadTimestampIndex(high, low, hashes))
        return error("Unable to get hashes for timestamps");

    return true;
}

bool GetSpentIndex(CTxMemPool& mempool, CSpentIndexKey &key, CSpentIndexValue &value)
{
    if (!fSpentIndex)
        return false;

    if (mempool.getSpentIndex(key, value))
        return true;

    if (!pblocktree->ReadSpentIndex(key, value))
        return false;

    return true;
}

bool GetAddressIndex(uint160 addressHash, int type,
                     std::vector<std::pair<CAddressIndexKey, CAmount> > &addressIndex, int start, int end)
{
    if (!fAddressIndex)
        return error("address index not enabled");

    if (!pblocktree->ReadAddressIndex(addressHash, type, addressIndex, start, end))
        return error("unable to get txids for address");

    return true;
}

bool GetAddressUnspent(uint160 addressHash, int type,
                       std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &unspentOutputs)
{
    if (!fAddressIndex)
        return error("address index not enabled");

    if (!pblocktree->ReadAddressUnspentIndex(addressHash, type, unspentOutputs))
        return error("unable to get txids for address");

    return true;
}

CTransactionRef GetTransaction(const CBlockIndex* const block_index, const CTxMemPool* const mempool, const uint256& hash, const Consensus::Params& consensusParams, uint256& hashBlock)
{
    LOCK(cs_main);

    if (block_index) {
        CBlock block;
        if (ReadBlockFromDisk(block, block_index, consensusParams)) {
            for (const auto& tx : block.vtx) {
                if (tx->GetHash() == hash) {
                    hashBlock = block_index->GetBlockHash();
                    return tx;
                }
            }
        }
        return nullptr;
    }
    if (mempool) {
        CTransactionRef ptx = mempool->get(hash);
        if (ptx) return ptx;
    }
    if (g_txindex) {
        CTransactionRef tx;
        if (g_txindex->FindTx(hash, hashBlock, tx)) return tx;
    }
    return nullptr;
}

double ConvertBitsToDouble(unsigned int nBits)
{
    int nShift = (nBits >> 24) & 0xff;

    double dDiff = (double)0x0000ffff / (double)(nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

/*
NOTE:   unlike bitcoin we are using PREVIOUS block height here,
        might be a good idea to change this to use prev bits
        but current height to avoid confusion.
*/
CAmount GetBlockSubsidyInner(int nPrevBits, int nPrevHeight, const Consensus::Params& consensusParams, bool fSuperblockPartOnly)
{
    double dDiff;
    CAmount nSubsidyBase;

    if (nPrevHeight <= 4500 && Params().NetworkIDString() == CBaseChainParams::MAIN) {
        /* a bug which caused diff to not be correctly calculated */
        dDiff = (double)0x0000ffff / (double)(nPrevBits & 0x00ffffff);
    } else {
        dDiff = ConvertBitsToDouble(nPrevBits);
    }

    if (nPrevHeight < 5465) {
        // Early ages...
        // 1111/((x+1)^2)
        nSubsidyBase = (1111.0 / (pow((dDiff+1.0),2.0)));
        if(nSubsidyBase > 500) nSubsidyBase = 500;
        else if(nSubsidyBase < 1) nSubsidyBase = 1;
    } else if (nPrevHeight < 17000 || (dDiff <= 75 && nPrevHeight < 24000)) {
        // CPU mining era
        // 11111/(((x+51)/6)^2)
        nSubsidyBase = (11111.0 / (pow((dDiff+51.0)/6.0,2.0)));
        if(nSubsidyBase > 500) nSubsidyBase = 500;
        else if(nSubsidyBase < 25) nSubsidyBase = 25;
    } else {
        // GPU/ASIC mining era
        // 2222222/(((x+2600)/9)^2)
        nSubsidyBase = (2222222.0 / (pow((dDiff+2600.0)/9.0,2.0)));
        if(nSubsidyBase > 25) nSubsidyBase = 25;
        else if(nSubsidyBase < 5) nSubsidyBase = 5;
    }

    CAmount nSubsidy = nSubsidyBase * COIN;

    // yearly decline of production by ~7.1% per year, projected ~18M coins max by year 2050+.
    for (int i = consensusParams.nSubsidyHalvingInterval; i <= nPrevHeight; i += consensusParams.nSubsidyHalvingInterval) {
        nSubsidy -= nSubsidy/14;
    }

    // this is only active on devnets
    if (nPrevHeight < consensusParams.nHighSubsidyBlocks) {
        nSubsidy *= consensusParams.nHighSubsidyFactor;
    }

    // Hard fork to reduce the block reward by 10 extra percent (allowing budget/superblocks)
    CAmount nSuperblockPart = (nPrevHeight > consensusParams.nBudgetPaymentsStartBlock) ? nSubsidy/10 : 0;

    return fSuperblockPartOnly ? nSuperblockPart : nSubsidy - nSuperblockPart;
}

CAmount GetBlockSubsidy(const CBlockIndex* const pindex, const Consensus::Params& consensusParams)
{
    if (pindex->pprev == nullptr) return Params().GenesisBlock().vtx[0]->GetValueOut();
    return GetBlockSubsidyInner(pindex->pprev->nBits, pindex->pprev->nHeight, consensusParams);
}

CAmount GetMasternodePayment(int nHeight, CAmount blockValue, int nReallocActivationHeight)
{
    CAmount ret = blockValue/5; // start at 20%

    int nMNPIBlock = Params().GetConsensus().nMasternodePaymentsIncreaseBlock;
    int nMNPIPeriod = Params().GetConsensus().nMasternodePaymentsIncreasePeriod;

                                                                      // mainnet:
    if(nHeight > nMNPIBlock)                  ret += blockValue / 20; // 158000 - 25.0% - 2014-10-24
    if(nHeight > nMNPIBlock+(nMNPIPeriod* 1)) ret += blockValue / 20; // 175280 - 30.0% - 2014-11-25
    if(nHeight > nMNPIBlock+(nMNPIPeriod* 2)) ret += blockValue / 20; // 192560 - 35.0% - 2014-12-26
    if(nHeight > nMNPIBlock+(nMNPIPeriod* 3)) ret += blockValue / 40; // 209840 - 37.5% - 2015-01-26
    if(nHeight > nMNPIBlock+(nMNPIPeriod* 4)) ret += blockValue / 40; // 227120 - 40.0% - 2015-02-27
    if(nHeight > nMNPIBlock+(nMNPIPeriod* 5)) ret += blockValue / 40; // 244400 - 42.5% - 2015-03-30
    if(nHeight > nMNPIBlock+(nMNPIPeriod* 6)) ret += blockValue / 40; // 261680 - 45.0% - 2015-05-01
    if(nHeight > nMNPIBlock+(nMNPIPeriod* 7)) ret += blockValue / 40; // 278960 - 47.5% - 2015-06-01
    if(nHeight > nMNPIBlock+(nMNPIPeriod* 9)) ret += blockValue / 40; // 313520 - 50.0% - 2015-08-03

    if (nHeight < nReallocActivationHeight) {
        // Block Reward Realocation is not activated yet, nothing to do
        return ret;
    }

    int nSuperblockCycle = Params().GetConsensus().nSuperblockCycle;
    // Actual realocation starts in the cycle next to one activation happens in
    int nReallocStart = nReallocActivationHeight - nReallocActivationHeight % nSuperblockCycle + nSuperblockCycle;

    if (nHeight < nReallocStart) {
        // Activated but we have to wait for the next cycle to start realocation, nothing to do
        return ret;
    }

    // Periods used to reallocate the masternode reward from 50% to 60%
    static std::vector<int> vecPeriods{
        513, // Period 1:  51.3%
        526, // Period 2:  52.6%
        533, // Period 3:  53.3%
        540, // Period 4:  54%
        546, // Period 5:  54.6%
        552, // Period 6:  55.2%
        557, // Period 7:  55.7%
        562, // Period 8:  56.2%
        567, // Period 9:  56.7%
        572, // Period 10: 57.2%
        577, // Period 11: 57.7%
        582, // Period 12: 58.2%
        585, // Period 13: 58.5%
        588, // Period 14: 58.8%
        591, // Period 15: 59.1%
        594, // Period 16: 59.4%
        597, // Period 17: 59.7%
        599, // Period 18: 59.9%
        600  // Period 19: 60%
    };

    int nReallocCycle = nSuperblockCycle * 3;
    int nCurrentPeriod = std::min<int>((nHeight - nReallocStart) / nReallocCycle, vecPeriods.size() - 1);

    return static_cast<CAmount>(blockValue * vecPeriods[nCurrentPeriod] / 1000);
}

CoinsViews::CoinsViews(
    std::string ldb_name,
    size_t cache_size_bytes,
    bool in_memory,
    bool should_wipe) : m_dbview(
                            GetDataDir() / ldb_name, cache_size_bytes, in_memory, should_wipe),
                        m_catcherview(&m_dbview) {}

void CoinsViews::InitCache()
{
    m_cacheview = std::make_unique<CCoinsViewCache>(&m_catcherview);
}

CChainState::CChainState(CTxMemPool* mempool,
                         BlockManager& blockman,
                         CEvoDB& evoDb,
                         const std::unique_ptr<llmq::CChainLocksHandler>& clhandler,
                         const std::unique_ptr<llmq::CInstantSendManager>& isman,
                         const std::unique_ptr<llmq::CQuorumBlockProcessor>& quorum_block_processor,
                         std::optional<uint256> from_snapshot_blockhash)
    : m_mempool(mempool),
      m_params(::Params()),
      m_clhandler(clhandler),
      m_isman(isman),
      m_quorum_block_processor(quorum_block_processor),
      m_evoDb(evoDb),
      m_blockman(blockman),
      m_from_snapshot_blockhash(from_snapshot_blockhash) {}

void CChainState::InitCoinsDB(
    size_t cache_size_bytes,
    bool in_memory,
    bool should_wipe,
    std::string leveldb_name)
{
    if (m_from_snapshot_blockhash) {
        leveldb_name += "_" + m_from_snapshot_blockhash->ToString();
    }

    m_coins_views = std::make_unique<CoinsViews>(
        leveldb_name, cache_size_bytes, in_memory, should_wipe);
}

void CChainState::InitCoinsCache(size_t cache_size_bytes)
{
    assert(m_coins_views != nullptr);
    m_coinstip_cache_size_bytes = cache_size_bytes;
    m_coins_views->InitCache();
}

// Note that though this is marked const, we may end up modifying `m_cached_finished_ibd`, which
// is a performance-related implementation detail. This function must be marked
// `const` so that `CValidationInterface` clients (which are given a `const CChainState*`)
// can call it.
//
bool CChainState::IsInitialBlockDownload() const
{
    // Optimization: pre-test latch before taking the lock.
    if (m_cached_finished_ibd.load(std::memory_order_relaxed))
        return false;

    LOCK(cs_main);
    if (m_cached_finished_ibd.load(std::memory_order_relaxed))
        return false;
    if (fImporting || fReindex)
        return true;
    if (m_chain.Tip() == nullptr)
        return true;
    if (m_chain.Tip()->nChainWork < nMinimumChainWork)
        return true;
    if (m_chain.Tip()->GetBlockTime() < (GetTime() - nMaxTipAge))
        return true;
    LogPrintf("Leaving InitialBlockDownload (latching to false)\n");
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

void CChainState::CheckForkWarningConditions()
{
    AssertLockHeld(cs_main);
    assert(std::addressof(::ChainstateActive()) == std::addressof(*this));

    // Before we get past initial download, we cannot reliably alert about forks
    // (we assume we don't get stuck on a fork before finishing our initial sync)
    if (IsInitialBlockDownload()) {
        return;
    }

    if (pindexBestInvalid && pindexBestInvalid->nChainWork > m_chain.Tip()->nChainWork + (GetBlockProof(*m_chain.Tip()) * 6)) {
        LogPrintf("%s: Warning: Found invalid chain which has higher work (at least ~6 blocks worth of work) than our best chain.\nChain state database corruption likely.\n", __func__);
        SetfLargeWorkInvalidChainFound(true);
    } else {
        SetfLargeWorkInvalidChainFound(false);
    }
}

// Called both upon regular invalid block discovery *and* InvalidateBlock
void CChainState::InvalidChainFound(CBlockIndex* pindexNew)
{
    assert(std::addressof(::ChainstateActive()) == std::addressof(*this));

    statsClient.inc("warnings.InvalidChainFound", 1.0f);

    if (!pindexBestInvalid || pindexNew->nChainWork > pindexBestInvalid->nChainWork)
        pindexBestInvalid = pindexNew;
    if (pindexBestHeader != nullptr && pindexBestHeader->GetAncestor(pindexNew->nHeight) == pindexNew) {
        pindexBestHeader = m_chain.Tip();
    }

    LogPrintf("%s: invalid block=%s  height=%d  log2_work=%.8f  date=%s\n", __func__,
      pindexNew->GetBlockHash().ToString(), pindexNew->nHeight,
      log(pindexNew->nChainWork.getdouble())/log(2.0), FormatISO8601DateTime(pindexNew->GetBlockTime()));
    CBlockIndex *tip = m_chain.Tip();
    assert (tip);
    LogPrintf("%s:  current best=%s  height=%d  log2_work=%.8f  date=%s\n", __func__,
      tip->GetBlockHash().ToString(), m_chain.Height(), log(tip->nChainWork.getdouble())/log(2.0),
      FormatISO8601DateTime(tip->GetBlockTime()));
    CheckForkWarningConditions();
}

void CChainState::ConflictingChainFound(CBlockIndex* pindexNew)
{
    assert(std::addressof(::ChainstateActive()) == std::addressof(*this));

    statsClient.inc("warnings.ConflictingChainFound", 1.0f);

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
// which does its own setBlockIndexCandidates manageent.
void CChainState::InvalidBlockFound(CBlockIndex *pindex, const BlockValidationState &state) {
    statsClient.inc("warnings.InvalidBlockFound", 1.0f);
    if (state.GetResult() != BlockValidationResult::BLOCK_MUTATED) {
        pindex->nStatus |= BLOCK_FAILED_VALID;
        m_blockman.m_failed_blocks.insert(pindex);
        setDirtyBlockIndex.insert(pindex);
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

void UpdateCoins(const CTransaction& tx, CCoinsViewCache& inputs, int nHeight)
{
    CTxUndo txundo;
    UpdateCoins(tx, inputs, txundo, nHeight);
}

bool CScriptCheck::operator()() {
    const CScript &scriptSig = ptxTo->vin[nIn].scriptSig;
    PrecomputedTransactionData txdata(*ptxTo);
    return VerifyScript(scriptSig, m_tx_out.scriptPubKey, nFlags, CachingTransactionSignatureChecker(ptxTo, nIn, m_tx_out.nValue, txdata, cacheStore), &error);
}

int BlockManager::GetSpendHeight(const CCoinsViewCache& inputs)
{
    AssertLockHeld(cs_main);
    assert(std::addressof(g_chainman.m_blockman) == std::addressof(*this));
    CBlockIndex* pindexPrev = LookupBlockIndex(inputs.GetBestBlock());
    return pindexPrev->nHeight + 1;
}


static CuckooCache::cache<uint256, SignatureCacheHasher> g_scriptExecutionCache;
static CSHA256 g_scriptExecutionCacheHasher;

void InitScriptExecutionCache() {
    // Setup the salted hasher
    uint256 nonce = GetRandHash();
    // We want the nonce to be 64 bytes long to force the hasher to process
    // this chunk, which makes later hash computations more efficient. We
    // just write our 32-byte entropy twice to fill the 64 bytes.
    g_scriptExecutionCacheHasher.Write(nonce.begin(), 32);
    g_scriptExecutionCacheHasher.Write(nonce.begin(), 32);
    // nMaxCacheSize is unsigned. If -maxsigcachesize is set to zero,
    // setup_bytes creates the minimum possible cache (2 elements).
    size_t nMaxCacheSize = std::min(std::max((int64_t)0, gArgs.GetArg("-maxsigcachesize", DEFAULT_MAX_SIG_CACHE_SIZE) / 2), MAX_MAX_SIG_CACHE_SIZE) * ((size_t) 1 << 20);
    size_t nElems = g_scriptExecutionCache.setup_bytes(nMaxCacheSize);
    LogPrintf("Using %zu MiB out of %zu/2 requested for script execution cache, able to store %zu elements\n",
            (nElems*sizeof(uint256)) >>20, (nMaxCacheSize*2)>>20, nElems);
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
bool CheckInputScripts(const CTransaction& tx, TxValidationState &state, const CCoinsViewCache &inputs, unsigned int flags, bool cacheSigStore, bool cacheFullScriptStore, PrecomputedTransactionData& txdata, std::vector<CScriptCheck> *pvChecks) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();
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
    hasher.Write(tx.GetHash().begin(), 32).Write((unsigned char*)&flags, sizeof(flags)).Finalize(hashCacheEntry.begin());
    AssertLockHeld(cs_main); //TODO: Remove this requirement by making CuckooCache not require external locks
    if (g_scriptExecutionCache.contains(hashCacheEntry, !cacheFullScriptStore)) {
        return true;
    }

    if (!txdata.m_ready) {
        txdata.Init(tx, {});
    }

    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin& coin = inputs.AccessCoin(prevout);
        assert(!coin.IsSpent());

        // We very carefully only pass in things to CScriptCheck which
        // are clearly committed to by tx' witness hash. This provides
        // a sanity check that our caching is not introducing consensus
        // failures through additional data in, eg, the coins being
        // spent being checked as a part of CScriptCheck.

        // Verify signature
        CScriptCheck check(coin.out, tx, i, flags, cacheSigStore, &txdata);
        if (pvChecks) {
            pvChecks->push_back(CScriptCheck());
            check.swap(pvChecks->back());
        } else if (!check()) {
            const bool hasNonMandatoryFlags = (flags & STANDARD_NOT_MANDATORY_VERIFY_FLAGS) != 0;
            const bool hasDIP0020Opcodes = (flags & SCRIPT_ENABLE_DIP0020_OPCODES) != 0;

            if (hasNonMandatoryFlags || !hasDIP0020Opcodes) {
                // Check whether the failure was caused by a
                // non-mandatory script verification check, such as
                // non-standard DER encodings or non-null dummy
                // arguments; if so, ensure we return NOT_STANDARD
                // instead of CONSENSUS to avoid downstream users
                // splitting the network between upgraded and
                // non-upgraded nodes by banning CONSENSUS-failing
                // data providers.
                CScriptCheck check2(coin.out, tx, i,
                        (flags & ~STANDARD_NOT_MANDATORY_VERIFY_FLAGS) | SCRIPT_ENABLE_DIP0020_OPCODES, cacheSigStore, &txdata);
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

    boost::posix_time::ptime finish = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration diff = finish - start;
    statsClient.timing("CheckInputScripts_ms", diff.total_milliseconds(), 1.0f);
    return true;
}

static bool UndoWriteToDisk(const CBlockUndo& blockundo, FlatFilePos& pos, const uint256& hashBlock, const CMessageHeader::MessageStartChars& messageStart)
{
    // Open history file to append
    CAutoFile fileout(OpenUndoFile(pos), SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("%s: OpenUndoFile failed", __func__);

    // Write index header
    unsigned int nSize = GetSerializeSize(blockundo, fileout.GetVersion());
    fileout << messageStart << nSize;

    // Write undo data
    long fileOutPos = ftell(fileout.Get());
    if (fileOutPos < 0)
        return error("%s: ftell failed", __func__);
    pos.nPos = (unsigned int)fileOutPos;
    fileout << blockundo;

    // calculate & write checksum
    CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
    hasher << hashBlock;
    hasher << blockundo;
    fileout << hasher.GetHash();

    return true;
}

bool UndoReadFromDisk(CBlockUndo& blockundo, const CBlockIndex* pindex)
{
    FlatFilePos pos = pindex->GetUndoPos();
    if (pos.IsNull()) {
        return error("%s: no undo data available", __func__);
    }

    // Open history file to read
    CAutoFile filein(OpenUndoFile(pos, true), SER_DISK, CLIENT_VERSION);
    if (filein.IsNull())
        return error("%s: OpenUndoFile failed", __func__);

    // Read block
    uint256 hashChecksum;
    CHashVerifier<CAutoFile> verifier(&filein); // We need a CHashVerifier as reserializing may lose data
    try {
        verifier << pindex->pprev->GetBlockHash();
        verifier >> blockundo;
        filein >> hashChecksum;
    }
    catch (const std::exception& e) {
        return error("%s: Deserialize or I/O error - %s", __func__, e.what());
    }

    // Verify checksum
    if (hashChecksum != verifier.GetHash())
        return error("%s: Checksum mismatch", __func__);

    return true;
}

static bool AbortNode(BlockValidationState& state, const std::string& strMessage, const bilingual_str& userMessage = bilingual_str())
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

/** Undo the effects of this block (with given index) on the UTXO set represented by coins.
 *  When FAILED is returned, view is left in an indeterminate state. */
DisconnectResult CChainState::DisconnectBlock(const CBlock& block, const CBlockIndex* pindex, CCoinsViewCache& view)
{
    AssertLockHeld(cs_main);
    assert(m_quorum_block_processor);

    bool fDIP0003Active = pindex->nHeight >= Params().GetConsensus().DIP0003Height;
    if (fDIP0003Active && !m_evoDb.VerifyBestBlock(pindex->GetBlockHash())) {
        // Nodes that upgraded after DIP3 activation will have to reindex to ensure evodb consistency
        AbortNode("Found EvoDB inconsistency, you must reindex to continue");
        return DISCONNECT_FAILED;
    }

    boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();

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

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressUnspentIndex;
    std::vector<std::pair<CSpentIndexKey, CSpentIndexValue> > spentIndex;

    if (!UndoSpecialTxsInBlock(block, pindex, *m_quorum_block_processor)) {
        error("DisconnectBlock(): UndoSpecialTxsInBlock failed");
        return DISCONNECT_FAILED;
    }

    // undo transactions in reverse order
    for (int i = block.vtx.size() - 1; i >= 0; i--) {
        const CTransaction &tx = *(block.vtx[i]);
        uint256 hash = tx.GetHash();
        bool is_coinbase = tx.IsCoinBase();

        if (fAddressIndex) {

            for (unsigned int k = tx.vout.size(); k-- > 0;) {
                const CTxOut &out = tx.vout[k];

                if (out.scriptPubKey.IsPayToScriptHash()) {
                    std::vector<unsigned char> hashBytes(out.scriptPubKey.begin()+2, out.scriptPubKey.begin()+22);

                    // undo receiving activity
                    addressIndex.push_back(std::make_pair(CAddressIndexKey(2, uint160(hashBytes), pindex->nHeight, i, hash, k, false), out.nValue));

                    // undo unspent index
                    addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(2, uint160(hashBytes), hash, k), CAddressUnspentValue()));

                } else if (out.scriptPubKey.IsPayToPublicKeyHash()) {
                    std::vector<unsigned char> hashBytes(out.scriptPubKey.begin()+3, out.scriptPubKey.begin()+23);

                    // undo receiving activity
                    addressIndex.push_back(std::make_pair(CAddressIndexKey(1, uint160(hashBytes), pindex->nHeight, i, hash, k, false), out.nValue));

                    // undo unspent index
                    addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(1, uint160(hashBytes), hash, k), CAddressUnspentValue()));

                } else if (out.scriptPubKey.IsPayToPublicKey()) {
                    uint160 hashBytes{Hash160(Span{out.scriptPubKey.data()+1, out.scriptPubKey.size() - 2})};
                    addressIndex.push_back(std::make_pair(CAddressIndexKey(1, hashBytes, pindex->nHeight, i, hash, k, false), out.nValue));
                    addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(1, hashBytes, hash, k), CAddressUnspentValue()));
                } else {
                    continue;
                }

            }

        }

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

        // restore inputs
        if (i > 0) { // not coinbases
            CTxUndo &txundo = blockUndo.vtxundo[i-1];
            if (txundo.vprevout.size() != tx.vin.size()) {
                error("DisconnectBlock(): transaction and undo data inconsistent");
                return DISCONNECT_FAILED;
            }
            for (unsigned int j = tx.vin.size(); j-- > 0;) {
                const COutPoint &out = tx.vin[j].prevout;
                int undoHeight = txundo.vprevout[j].nHeight;
                int res = ApplyTxInUndo(std::move(txundo.vprevout[j]), view, out);
                if (res == DISCONNECT_FAILED) return DISCONNECT_FAILED;
                fClean = fClean && res != DISCONNECT_UNCLEAN;

                const CTxIn input = tx.vin[j];

                if (fSpentIndex) {
                    // undo and delete the spent index
                    spentIndex.push_back(std::make_pair(CSpentIndexKey(input.prevout.hash, input.prevout.n), CSpentIndexValue()));
                }

                if (fAddressIndex) {
                    const Coin &coin = view.AccessCoin(tx.vin[j].prevout);
                    const CTxOut &prevout = coin.out;
                    if (prevout.scriptPubKey.IsPayToScriptHash()) {
                        std::vector<unsigned char> hashBytes(prevout.scriptPubKey.begin()+2, prevout.scriptPubKey.begin()+22);

                        // undo spending activity
                        addressIndex.push_back(std::make_pair(CAddressIndexKey(2, uint160(hashBytes), pindex->nHeight, i, hash, j, true), prevout.nValue * -1));

                        // restore unspent index
                        addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(2, uint160(hashBytes), input.prevout.hash, input.prevout.n), CAddressUnspentValue(prevout.nValue, prevout.scriptPubKey, undoHeight)));


                    } else if (prevout.scriptPubKey.IsPayToPublicKeyHash()) {
                        std::vector<unsigned char> hashBytes(prevout.scriptPubKey.begin()+3, prevout.scriptPubKey.begin()+23);

                        // undo spending activity
                        addressIndex.push_back(std::make_pair(CAddressIndexKey(1, uint160(hashBytes), pindex->nHeight, i, hash, j, true), prevout.nValue * -1));

                        // restore unspent index
                        addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(1, uint160(hashBytes), input.prevout.hash, input.prevout.n), CAddressUnspentValue(prevout.nValue, prevout.scriptPubKey, undoHeight)));

                    } else if (prevout.scriptPubKey.IsPayToPublicKey()) {
                        uint160 hashBytes{Hash160(Span{prevout.scriptPubKey.data()+1, prevout.scriptPubKey.size()-2})};
                        // undo spending activity
                        addressIndex.push_back(std::make_pair(CAddressIndexKey(1, hashBytes, pindex->nHeight, i, hash, j, true), prevout.nValue * -1));
                        // restore unspent index
                        addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(1, hashBytes, input.prevout.hash, input.prevout.n), CAddressUnspentValue(prevout.nValue, prevout.scriptPubKey, undoHeight)));
                    } else {
                        continue;
                    }
                }

            }
            // At this point, all of txundo.vprevout should have been moved out.
        }
    }


    if (fSpentIndex) {
        if (!pblocktree->UpdateSpentIndex(spentIndex)) {
            AbortNode("Failed to delete spent index");
            return DISCONNECT_FAILED;
        }
    }

    if (fAddressIndex) {
        if (!pblocktree->EraseAddressIndex(addressIndex)) {
            AbortNode("Failed to delete address index");
            return DISCONNECT_FAILED;
        }
        if (!pblocktree->UpdateAddressUnspentIndex(addressUnspentIndex)) {
            AbortNode("Failed to write address unspent index");
            return DISCONNECT_FAILED;
        }
    }

    if (fTimestampIndex) {
        if (!pblocktree->EraseTimestampIndex(CTimestampIndexKey(pindex->nTime, pindex->GetBlockHash()))) {
            AbortNode("Failed to delete timestamp index");
            return DISCONNECT_FAILED;
        }
    }

    // move best block pointer to prevout block
    view.SetBestBlock(pindex->pprev->GetBlockHash());
    m_evoDb.WriteBestBlock(pindex->pprev->GetBlockHash());

    boost::posix_time::ptime finish = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration diff = finish - start;
    statsClient.timing("DisconnectBlock_ms", diff.total_milliseconds(), 1.0f);

    return fClean ? DISCONNECT_OK : DISCONNECT_UNCLEAN;
}

static void FlushUndoFile(int block_file, bool finalize = false)
{
    FlatFilePos undo_pos_old(block_file, vinfoBlockFile[block_file].nUndoSize);
    if (!UndoFileSeq().Flush(undo_pos_old, finalize)) {
        AbortNode("Flushing undo file to disk failed. This is likely the result of an I/O error.");
    }
}

static void FlushBlockFile(bool fFinalize = false, bool finalize_undo = false)
{
    LOCK(cs_LastBlockFile);
    FlatFilePos block_pos_old(nLastBlockFile, vinfoBlockFile[nLastBlockFile].nSize);
    if (!BlockFileSeq().Flush(block_pos_old, fFinalize)) {
        AbortNode("Flushing block file to disk failed. This is likely the result of an I/O error.");
    }
    // we do not always flush the undo file, as the chain tip may be lagging behind the incoming blocks,
    // e.g. during IBD or a sync after a node going offline
    if (!fFinalize || finalize_undo) FlushUndoFile(nLastBlockFile, finalize_undo);
}

static bool FindUndoPos(BlockValidationState &state, int nFile, FlatFilePos &pos, unsigned int nAddSize);

static bool WriteUndoDataForBlock(const CBlockUndo& blockundo, BlockValidationState& state, CBlockIndex* pindex, const CChainParams& chainparams)
{
    // Write undo information to disk
    if (pindex->GetUndoPos().IsNull()) {
        FlatFilePos _pos;
        if (!FindUndoPos(state, pindex->nFile, _pos, ::GetSerializeSize(blockundo, CLIENT_VERSION) + 40))
            return error("ConnectBlock(): FindUndoPos failed");
        if (!UndoWriteToDisk(blockundo, _pos, pindex->pprev->GetBlockHash(), chainparams.MessageStart()))
            return AbortNode(state, "Failed to write undo data");
        // rev files are written in block height order, whereas blk files are written as blocks come in (often out of order)
        // we want to flush the rev (undo) file once we've written the last block, which is indicated by the last height
        // in the block file info as below; note that this does not catch the case where the undo writes are keeping up
        // with the block writes (usually when a synced up node is getting newly mined blocks) -- this case is caught in
        // the FindBlockPos function
        if (_pos.nFile < nLastBlockFile && static_cast<uint32_t>(pindex->nHeight) == vinfoBlockFile[_pos.nFile].nHeightLast) {
            FlushUndoFile(_pos.nFile, true);
        }

        // update nUndoPos in block index
        pindex->nUndoPos = _pos.nPos;
        pindex->nStatus |= BLOCK_HAVE_UNDO;
        setDirtyBlockIndex.insert(pindex);
    }

    return true;
}

static CCheckQueue<CScriptCheck> scriptcheckqueue(128);

void StartScriptCheckWorkerThreads(int threads_num)
{
    scriptcheckqueue.StartWorkerThreads(threads_num);
}

void StopScriptCheckWorkerThreads()
{
    scriptcheckqueue.StopWorkerThreads();
}

VersionBitsCache versionbitscache GUARDED_BY(cs_main);

int32_t ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params)
{
    LOCK(cs_main);
    int32_t nVersion = VERSIONBITS_TOP_BITS;

    for (int i = 0; i < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; i++) {
        Consensus::DeploymentPos pos = Consensus::DeploymentPos(i);
        ThresholdState state = VersionBitsState(pindexPrev, params, pos, versionbitscache);
        if (state == ThresholdState::LOCKED_IN || state == ThresholdState::STARTED) {
            nVersion |= VersionBitsMask(params, static_cast<Consensus::DeploymentPos>(i));
        }
    }

    return nVersion;
}

bool GetBlockHash(uint256& hashRet, int nBlockHeight)
{
    LOCK(cs_main);
    if(::ChainActive().Tip() == nullptr) return false;
    if(nBlockHeight < -1 || nBlockHeight > ::ChainActive().Height()) return false;
    if(nBlockHeight == -1) nBlockHeight = ::ChainActive().Height();
    hashRet = ::ChainActive()[nBlockHeight]->GetBlockHash();
    return true;
}

/**
 * Threshold condition checker that triggers when unknown versionbits are seen on the network.
 */
class WarningBitsConditionChecker : public AbstractThresholdConditionChecker
{
private:
    int bit;

public:
    explicit WarningBitsConditionChecker(int bitIn) : bit(bitIn) {}

    int64_t BeginTime(const Consensus::Params& params) const override { return 0; }
    int64_t EndTime(const Consensus::Params& params) const override { return std::numeric_limits<int64_t>::max(); }
    int Period(const Consensus::Params& params) const override { return params.nMinerConfirmationWindow; }
    int Threshold(const Consensus::Params& params, int nAttempt) const override { return params.nRuleChangeActivationThreshold; }

    bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const override
    {
        return pindex->nHeight >= params.MinBIP9WarningHeight &&
               ((pindex->nVersion & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS) &&
               ((pindex->nVersion >> bit) & 1) != 0 &&
               ((ComputeBlockVersion(pindex->pprev, params) >> bit) & 1) == 0;
    }
};

static ThresholdConditionCache warningcache[VERSIONBITS_NUM_BITS] GUARDED_BY(cs_main);

static unsigned int GetBlockScriptFlags(const CBlockIndex* pindex, const Consensus::Params& consensusparams) EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
    AssertLockHeld(cs_main);

    unsigned int flags = SCRIPT_VERIFY_NONE;

    // Start enforcing P2SH (BIP16)
    if (pindex->nHeight >= consensusparams.BIP16Height) {
        flags |= SCRIPT_VERIFY_P2SH;
    }

    // Start enforcing the DERSIG (BIP66) rule
    if (pindex->nHeight >= consensusparams.BIP66Height) {
        flags |= SCRIPT_VERIFY_DERSIG;
    }

    // Start enforcing CHECKLOCKTIMEVERIFY (BIP65) rule
    if (pindex->nHeight >= consensusparams.BIP65Height) {
        flags |= SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY;
    }

    // Start enforcing BIP112 (CHECKSEQUENCEVERIFY)
    if (pindex->nHeight >= consensusparams.CSVHeight) {
        flags |= SCRIPT_VERIFY_CHECKSEQUENCEVERIFY;
    }

    // Start enforcing BIP147 (NULLDUMMY) rule using versionbits logic.
    if (pindex->nHeight >= consensusparams.BIP147Height) {
        flags |= SCRIPT_VERIFY_NULLDUMMY;
    }

    if (pindex->nHeight >= consensusparams.DIP0020Height) {
        flags |= SCRIPT_ENABLE_DIP0020_OPCODES;
    }

    return flags;
}



static int64_t nTimeCheck = 0;
static int64_t nTimeForks = 0;
static int64_t nTimeVerify = 0;
static int64_t nTimeISFilter = 0;
static int64_t nTimeSubsidy = 0;
static int64_t nTimeValueValid = 0;
static int64_t nTimePayeeValid = 0;
static int64_t nTimeProcessSpecial = 0;
static int64_t nTimeDashSpecific = 0;
static int64_t nTimeConnect = 0;
static int64_t nTimeIndexConnect = 0;
static int64_t nTimeIndexWrite = 0;
static int64_t nTimeCallbacks = 0;
static int64_t nTimeTotal = 0;
static int64_t nBlocksTotal = 0;

/** Apply the effects of this block (with given index) on the UTXO set represented by coins.
 *  Validity checks that depend on the UTXO set are also done; ConnectBlock()
 *  can fail if those validity checks fail (among other reasons). */
bool CChainState::ConnectBlock(const CBlock& block, BlockValidationState& state, CBlockIndex* pindex,
                               CCoinsViewCache& view, bool fJustCheck)
{
    boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();

    AssertLockHeld(cs_main);
    assert(pindex);

    uint256 block_hash{block.GetHash()};
    assert(*pindex->phashBlock == block_hash);

    assert(m_clhandler);
    assert(m_isman);
    assert(m_quorum_block_processor);
    int64_t nTimeStart = GetTimeMicros();

    // Check it again in case a previous version let a bad block in
    // NOTE: We don't currently (re-)invoke ContextualCheckBlock() or
    // ContextualCheckBlockHeader() here. This means that if we add a new
    // consensus rule that is enforced in one of those two functions, then we
    // may have let in a block that violates the rule prior to updating the
    // software, and we would NOT be enforcing the rule here. Fully solving
    // upgrade from one software version to the next after a consensus rule
    // change is potentially tricky and issue-specific (see RewindBlockIndex()
    // for one general approach that was used for BIP 141 deployment).
    // Also, currently the rule against blocks more than 2 hours in the future
    // is enforced in ContextualCheckBlockHeader(); we wouldn't want to
    // re-enforce that rule here (at least until we make it impossible for
    // GetAdjustedTime() to go backward).
    if (!CheckBlock(block, state, m_params.GetConsensus(), !fJustCheck, !fJustCheck)) {
        if (state.GetResult() == BlockValidationResult::BLOCK_MUTATED) {
            // We don't write down blocks to disk if they may have been
            // corrupted, so this should be impossible unless we're having hardware
            // problems.
            return AbortNode(state, "Corrupt block found indicating potential hardware failure; shutting down");
        }
        return error("%s: Consensus::CheckBlock: %s", __func__, state.ToString());
    }

    if (pindex->pprev && pindex->phashBlock && m_clhandler->HasConflictingChainLock(pindex->nHeight, pindex->GetBlockHash())) {
        LogPrintf("ERROR: %s: conflicting with chainlock\n", __func__);
        return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "bad-chainlock");
    }

    // verify that the view's current state corresponds to the previous block
    uint256 hashPrevBlock = pindex->pprev == nullptr ? uint256() : pindex->pprev->GetBlockHash();
    assert(hashPrevBlock == view.GetBestBlock());

    if (pindex->pprev) {
        bool fDIP0003Active = pindex->nHeight >= m_params.GetConsensus().DIP0003Height;
        if (fDIP0003Active && !m_evoDb.VerifyBestBlock(pindex->pprev->GetBlockHash())) {
            // Nodes that upgraded after DIP3 activation will have to reindex to ensure evodb consistency
            return AbortNode(state, "Found EvoDB inconsistency, you must reindex to continue");
        }
    }
    nBlocksTotal++;

    // Special case for the genesis block, skipping connection of its transactions
    // (its coinbase is unspendable)
    if (block_hash == m_params.GetConsensus().hashGenesisBlock) {
        if (!fJustCheck)
            view.SetBestBlock(pindex->GetBlockHash());
        return true;
    }

    bool fScriptChecks = true;
    if (!hashAssumeValid.IsNull()) {
        // We've been configured with the hash of a block which has been externally verified to have a valid history.
        // A suitable default value is included with the software and updated from time to time.  Because validity
        //  relative to a piece of software is an objective fact these defaults can be easily reviewed.
        // This setting doesn't force the selection of any particular chain but makes validating some faster by
        //  effectively caching the result of part of the verification.
        BlockMap::const_iterator  it = m_blockman.m_block_index.find(hashAssumeValid);
        if (it != m_blockman.m_block_index.end()) {
            if (it->second->GetAncestor(pindex->nHeight) == pindex &&
                pindexBestHeader->GetAncestor(pindex->nHeight) == pindex &&
                pindexBestHeader->nChainWork >= nMinimumChainWork) {
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
                // The test against nMinimumChainWork prevents the skipping when denied access to any chain at
                //  least as good as the expected chain.
                fScriptChecks = (GetBlockProofEquivalentTime(*pindexBestHeader, *pindex, *pindexBestHeader, m_params.GetConsensus()) <= 60 * 60 * 24 * 7 * 2);
            }
        }
    }

    int64_t nTime1 = GetTimeMicros(); nTimeCheck += nTime1 - nTimeStart;
    LogPrint(BCLog::BENCHMARK, "    - Sanity checks: %.2fms [%.2fs (%.2fms/blk)]\n", MILLI * (nTime1 - nTimeStart), nTimeCheck * MICRO, nTimeCheck * MILLI / nBlocksTotal);

    // Do not allow blocks that contain transactions which 'overwrite' older transactions,
    // unless those are already completely spent.
    // If such overwrites are allowed, coinbases and transactions depending upon those
    // can be duplicated to remove the ability to spend the first instance -- even after
    // being sent to another address.
    // See BIP30, CVE-2012-1909, and http://r6.ca/blog/20120206T005236Z.html for more information.
    // This logic is not necessary for memory pool transactions, as AcceptToMemoryPool
    // already refuses previously-known transaction ids entirely.
    // This rule was originally applied to all blocks with a timestamp after March 15, 2012, 0:00 UTC.
    // Now that the whole chain is irreversibly beyond that time it is applied to all blocks except the
    // two in the chain that violate it. This prevents exploiting the issue against nodes during their
    // initial block download.
    bool fEnforceBIP30 = !((pindex->nHeight==91842 && pindex->GetBlockHash() == uint256S("0x00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec")) ||
                           (pindex->nHeight==91880 && pindex->GetBlockHash() == uint256S("0x00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721")));

    // Once BIP34 activated it was not possible to create new duplicate coinbases and thus other than starting
    // with the 2 existing duplicate coinbase pairs, not possible to create overwriting txs.  But by the
    // time BIP34 activated, in each of the existing pairs the duplicate coinbase had overwritten the first
    // before the first had been spent.  Since those coinbases are sufficiently buried it's no longer possible to create further
    // duplicate transactions descending from the known pairs either.
    // If we're on the known chain at height greater than where BIP34 activated, we can save the db accesses needed for the BIP30 check.
    assert(pindex->pprev);
    CBlockIndex* pindexBIP34height = pindex->pprev->GetAncestor(m_params.GetConsensus().BIP34Height);
    //Only continue to enforce if we're below BIP34 activation height or the block hash at that height doesn't correspond.
    fEnforceBIP30 = fEnforceBIP30 && (!pindexBIP34height || !(pindexBIP34height->GetBlockHash() == m_params.GetConsensus().BIP34Hash));

    if (fEnforceBIP30) {
        for (const auto& tx : block.vtx) {
            for (size_t o = 0; o < tx->vout.size(); o++) {
                if (view.HaveCoin(COutPoint(tx->GetHash(), o))) {
                    LogPrintf("ERROR: ConnectBlock(): tried to overwrite transaction\n");
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-txns-BIP30");
                }
            }
        }
    }

    /// DASH: Check superblock start

    // make sure old budget is the real one
    if (pindex->nHeight == m_params.GetConsensus().nSuperblockStartBlock &&
        m_params.GetConsensus().nSuperblockStartHash != uint256() &&
        block_hash != m_params.GetConsensus().nSuperblockStartHash) {
            LogPrintf("ERROR: ConnectBlock(): invalid superblock start\n");
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-sb-start");
    }
    /// END DASH

    // Start enforcing BIP68 (sequence locks)
    int nLockTimeFlags = 0;
    if (pindex->nHeight >= m_params.GetConsensus().CSVHeight) {
        nLockTimeFlags |= LOCKTIME_VERIFY_SEQUENCE;
    }

    // Get the script flags for this block
    unsigned int flags = GetBlockScriptFlags(pindex, m_params.GetConsensus());

    int64_t nTime2 = GetTimeMicros(); nTimeForks += nTime2 - nTime1;
    LogPrint(BCLog::BENCHMARK, "    - Fork checks: %.2fms [%.2fs (%.2fms/blk)]\n", MILLI * (nTime2 - nTime1), nTimeForks * MICRO, nTimeForks * MILLI / nBlocksTotal);

    CBlockUndo blockundo;

    // Precomputed transaction data pointers must not be invalidated
    // until after `control` has run the script checks (potentially
    // in multiple threads). Preallocate the vector size so a new allocation
    // doesn't invalidate pointers into the vector, and keep txsdata in scope
    // for as long as `control`.
    CCheckQueueControl<CScriptCheck> control(fScriptChecks && g_parallel_script_checks ? &scriptcheckqueue : nullptr);
    std::vector<PrecomputedTransactionData> txsdata(block.vtx.size());

    std::vector<int> prevheights;
    CAmount nFees = 0;
    int nInputs = 0;
    unsigned int nSigOps = 0;
    blockundo.vtxundo.reserve(block.vtx.size() - 1);
    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressUnspentIndex;
    std::vector<std::pair<CSpentIndexKey, CSpentIndexValue> > spentIndex;

    bool fDIP0001Active_context = pindex->nHeight >= Params().GetConsensus().DIP0001Height;

    // MUST process special txes before updating UTXO to ensure consistency between mempool and block processing
    if (!ProcessSpecialTxsInBlock(block, pindex, *m_quorum_block_processor, *m_clhandler, m_params.GetConsensus(), view, fJustCheck, fScriptChecks, state)) {
        return error("ConnectBlock(DASH): ProcessSpecialTxsInBlock for block %s failed with %s",
                     pindex->GetBlockHash().ToString(), state.ToString());
    }

    int64_t nTime2_1 = GetTimeMicros(); nTimeProcessSpecial += nTime2_1 - nTime2;
    LogPrint(BCLog::BENCHMARK, "      - ProcessSpecialTxsInBlock: %.2fms [%.2fs (%.2fms/blk)]\n", MILLI * (nTime2_1 - nTime2), nTimeProcessSpecial * MICRO, nTimeProcessSpecial * MILLI / nBlocksTotal);

    int64_t nTime2_index = 0;

    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction &tx = *(block.vtx[i]);
        const uint256 txhash = tx.GetHash();

        nInputs += tx.vin.size();

        if (!tx.IsCoinBase())
        {
            CAmount txfee = 0;
            TxValidationState tx_state;
            if (!Consensus::CheckTxInputs(tx, tx_state, view, pindex->nHeight, txfee)) {
                // Any transaction validation failure in ConnectBlock is a block consensus failure
                LogPrintf("ERROR: %s: Consensus::CheckTxInputs: %s, %s\n", __func__, tx.GetHash().ToString(), state.ToString());
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                            tx_state.GetRejectReason(), tx_state.GetDebugMessage());
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

            if (fAddressIndex || fSpentIndex)
            {
                int64_t nTime2_index1 = GetTimeMicros();

                for (size_t j = 0; j < tx.vin.size(); j++) {
                    const CTxIn input = tx.vin[j];
                    const Coin& coin = view.AccessCoin(tx.vin[j].prevout);
                    const CTxOut &prevout = coin.out;
                    uint160 hashBytes;
                    int addressType;

                    if (prevout.scriptPubKey.IsPayToScriptHash()) {
                        hashBytes = uint160(std::vector<unsigned char>(prevout.scriptPubKey.begin()+2, prevout.scriptPubKey.begin()+22));
                        addressType = 2;
                    } else if (prevout.scriptPubKey.IsPayToPublicKeyHash()) {
                        hashBytes = uint160(std::vector<unsigned char>(prevout.scriptPubKey.begin()+3, prevout.scriptPubKey.begin()+23));
                        addressType = 1;
                    } else if (prevout.scriptPubKey.IsPayToPublicKey()) {
                        hashBytes = Hash160(Span{prevout.scriptPubKey.data()+1, prevout.scriptPubKey.size()-2});
                        addressType = 1;
                    } else {
                        hashBytes.SetNull();
                        addressType = 0;
                    }

                    if (fAddressIndex && addressType > 0) {
                        // record spending activity
                        addressIndex.push_back(std::make_pair(CAddressIndexKey(addressType, hashBytes, pindex->nHeight, i, txhash, j, true), prevout.nValue * -1));

                        // remove address from unspent index
                        addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(addressType, hashBytes, input.prevout.hash, input.prevout.n), CAddressUnspentValue()));
                    }

                    if (fSpentIndex) {
                        // add the spent index to determine the txid and input that spent an output
                        // and to find the amount and address from an input
                        spentIndex.push_back(std::make_pair(CSpentIndexKey(input.prevout.hash, input.prevout.n), CSpentIndexValue(txhash, j, pindex->nHeight, prevout.nValue, addressType, hashBytes)));
                    }
                }
                nTime2_index += GetTimeMicros() - nTime2_index1;
            }

        }

        // GetTransactionSigOpCount counts 2 types of sigops:
        // * legacy (always)
        // * p2sh (when P2SH enabled in flags and excludes coinbase)
        nSigOps += GetTransactionSigOpCount(tx, view, flags);
        if (nSigOps > MaxBlockSigOps(fDIP0001Active_context)) {
            LogPrintf("ERROR: ConnectBlock(): too many sigops\n");
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-blk-sigops");
        }

        if (!tx.IsCoinBase())
        {

            std::vector<CScriptCheck> vChecks;
            bool fCacheResults = fJustCheck; /* Don't cache results if we're actually connecting blocks (still consult the cache, though) */
            TxValidationState tx_state;
            if (fScriptChecks && !CheckInputScripts(tx, tx_state, view, flags, fCacheResults, fCacheResults, txsdata[i], g_parallel_script_checks ? &vChecks : nullptr)) {
                // Any transaction validation failure in ConnectBlock is a block consensus failure
                state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                              tx_state.GetRejectReason(), tx_state.GetDebugMessage());
                LogPrintf("ERROR: ConnectBlock(): CheckInputScripts on %s failed with %s\n",
                    tx.GetHash().ToString(), state.ToString());
                return false;
            }
            control.Add(vChecks);
        }

        if (fAddressIndex) {
            int64_t nTime2_index2 = GetTimeMicros();
            for (unsigned int k = 0; k < tx.vout.size(); k++) {
                const CTxOut &out = tx.vout[k];

                if (out.scriptPubKey.IsPayToScriptHash()) {
                    std::vector<unsigned char> hashBytes(out.scriptPubKey.begin()+2, out.scriptPubKey.begin()+22);

                    // record receiving activity
                    addressIndex.push_back(std::make_pair(CAddressIndexKey(2, uint160(hashBytes), pindex->nHeight, i, txhash, k, false), out.nValue));

                    // record unspent output
                    addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(2, uint160(hashBytes), txhash, k), CAddressUnspentValue(out.nValue, out.scriptPubKey, pindex->nHeight)));

                } else if (out.scriptPubKey.IsPayToPublicKeyHash()) {
                    std::vector<unsigned char> hashBytes(out.scriptPubKey.begin()+3, out.scriptPubKey.begin()+23);

                    // record receiving activity
                    addressIndex.push_back(std::make_pair(CAddressIndexKey(1, uint160(hashBytes), pindex->nHeight, i, txhash, k, false), out.nValue));

                    // record unspent output
                    addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(1, uint160(hashBytes), txhash, k), CAddressUnspentValue(out.nValue, out.scriptPubKey, pindex->nHeight)));

                } else if (out.scriptPubKey.IsPayToPublicKey()) {
                    uint160 hashBytes{Hash160(Span{out.scriptPubKey.data()+1, out.scriptPubKey.size()-2})};
                    addressIndex.push_back(std::make_pair(CAddressIndexKey(1, hashBytes, pindex->nHeight, i, txhash, k, false), out.nValue));
                    addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(1, hashBytes, txhash, k), CAddressUnspentValue(out.nValue, out.scriptPubKey, pindex->nHeight)));
                } else {
                    continue;
                }
            }
            nTime2_index += GetTimeMicros() - nTime2_index2;
        }

        CTxUndo undoDummy;
        if (i > 0) {
            blockundo.vtxundo.push_back(CTxUndo());
        }
        UpdateCoins(tx, view, i == 0 ? undoDummy : blockundo.vtxundo.back(), pindex->nHeight);
    }

    nTimeIndexConnect += nTime2_index;
    LogPrint(BCLog::BENCHMARK, "        - Connect index: %.2fms [%.2fs (%.2fms/blk)]\n", MILLI * nTime2_index, nTimeIndexConnect * MICRO, nTimeIndexConnect * MILLI / nBlocksTotal);
    int64_t nTime3 = GetTimeMicros(); nTimeConnect += nTime3 - nTime2;
    LogPrint(BCLog::BENCHMARK, "      - Connect %u transactions: %.2fms (%.3fms/tx, %.3fms/txin) [%.2fs (%.2fms/blk)]\n", (unsigned)block.vtx.size(), MILLI * (nTime3 - nTime2), MILLI * (nTime3 - nTime2) / block.vtx.size(), nInputs <= 1 ? 0 : MILLI * (nTime3 - nTime2) / (nInputs-1), nTimeConnect * MICRO, nTimeConnect * MILLI / nBlocksTotal);


    if (!control.Wait()) {
        LogPrintf("ERROR: %s: CheckQueue failed\n", __func__);
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "block-validation-failed");
    }
    int64_t nTime4 = GetTimeMicros(); nTimeVerify += nTime4 - nTime2;
    LogPrint(BCLog::BENCHMARK, "    - Verify %u txins: %.2fms (%.3fms/txin) [%.2fs (%.2fms/blk)]\n", nInputs - 1, MILLI * (nTime4 - nTime2), nInputs <= 1 ? 0 : MILLI * (nTime4 - nTime2) / (nInputs-1), nTimeVerify * MICRO, nTimeVerify * MILLI / nBlocksTotal);


    // DASH

    // It's possible that we simply don't have enough data and this could fail
    // (i.e. block itself could be a correct one and we need to store it),
    // that's why this is in ConnectBlock. Could be the other way around however -
    // the peer who sent us this block is missing some data and wasn't able
    // to recognize that block is actually invalid.

    // DASH : CHECK TRANSACTIONS FOR INSTANTSEND

    if (m_isman->RejectConflictingBlocks()) {
        // Require other nodes to comply, send them some data in case they are missing it.
        for (const auto& tx : block.vtx) {
            // skip txes that have no inputs
            if (tx->vin.empty()) continue;
            while (llmq::CInstantSendLockPtr conflictLock = m_isman->GetConflictingLock(*tx)) {
                if (m_clhandler->HasChainLock(pindex->nHeight, pindex->GetBlockHash())) {
                    LogPrint(BCLog::ALL, "ConnectBlock(DASH): chain-locked transaction %s overrides islock %s\n",
                            tx->GetHash().ToString(), ::SerializeHash(*conflictLock).ToString());
                    m_isman->RemoveConflictingLock(::SerializeHash(*conflictLock), *conflictLock);
                } else {
                    // The node which relayed this should switch to correct chain.
                    // TODO: relay instantsend data/proof.
                    LogPrintf("ERROR: ConnectBlock(DASH): transaction %s conflicts with transaction lock %s\n", tx->GetHash().ToString(), conflictLock->txid.ToString());
                    return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "conflict-tx-lock");
                }
            }
        }
    }

    int64_t nTime5_1 = GetTimeMicros(); nTimeISFilter += nTime5_1 - nTime4;
    LogPrint(BCLog::BENCHMARK, "      - IS filter: %.2fms [%.2fs (%.2fms/blk)]\n", MILLI * (nTime5_1 - nTime4), nTimeISFilter * MICRO, nTimeISFilter * MILLI / nBlocksTotal);

    // DASH : MODIFIED TO CHECK MASTERNODE PAYMENTS AND SUPERBLOCKS

    // TODO: resync data (both ways?) and try to reprocess this block later.
    CAmount blockReward = nFees + GetBlockSubsidy(pindex, m_params.GetConsensus());
    std::string strError = "";

    int64_t nTime5_2 = GetTimeMicros(); nTimeSubsidy += nTime5_2 - nTime5_1;
    LogPrint(BCLog::BENCHMARK, "      - GetBlockSubsidy: %.2fms [%.2fs (%.2fms/blk)]\n", MILLI * (nTime5_2 - nTime5_1), nTimeSubsidy * MICRO, nTimeSubsidy * MILLI / nBlocksTotal);

    if (!MasternodePayments::IsBlockValueValid(*sporkManager, *governance, *::masternodeSync, block, pindex->nHeight, blockReward, strError)) {
        // NOTE: Do not punish, the node might be missing governance data
        LogPrintf("ERROR: ConnectBlock(DASH): %s\n", strError);
        return state.Invalid(BlockValidationResult::BLOCK_RESULT_UNSET, "bad-cb-amount");
    }

    int64_t nTime5_3 = GetTimeMicros(); nTimeValueValid += nTime5_3 - nTime5_2;
    LogPrint(BCLog::BENCHMARK, "      - IsBlockValueValid: %.2fms [%.2fs (%.2fms/blk)]\n", MILLI * (nTime5_3 - nTime5_2), nTimeValueValid * MICRO, nTimeValueValid * MILLI / nBlocksTotal);

    if (!MasternodePayments::IsBlockPayeeValid(*sporkManager, *governance, *block.vtx[0], pindex->nHeight, blockReward)) {
        // NOTE: Do not punish, the node might be missing governance data
        LogPrintf("ERROR: ConnectBlock(DASH): couldn't find masternode or superblock payments\n");
        return state.Invalid(BlockValidationResult::BLOCK_RESULT_UNSET, "bad-cb-payee");
    }

    int64_t nTime5_4 = GetTimeMicros(); nTimePayeeValid += nTime5_4 - nTime5_3;
    LogPrint(BCLog::BENCHMARK, "      - IsBlockPayeeValid: %.2fms [%.2fs (%.2fms/blk)]\n", MILLI * (nTime5_4 - nTime5_3), nTimePayeeValid * MICRO, nTimePayeeValid * MILLI / nBlocksTotal);

    int64_t nTime5 = GetTimeMicros(); nTimeDashSpecific += nTime5 - nTime4;
    LogPrint(BCLog::BENCHMARK, "    - Dash specific: %.2fms [%.2fs (%.2fms/blk)]\n", MILLI * (nTime5 - nTime4), nTimeDashSpecific * MICRO, nTimeDashSpecific * MILLI / nBlocksTotal);

    // END DASH

    if (fJustCheck)
        return true;

    if (!WriteUndoDataForBlock(blockundo, state, pindex, m_params)) {
        return false;
    }

    if (!pindex->IsValid(BLOCK_VALID_SCRIPTS)) {
        pindex->RaiseValidity(BLOCK_VALID_SCRIPTS);
        setDirtyBlockIndex.insert(pindex);
    }

    int64_t nTime6 = GetTimeMicros();

    if (fAddressIndex) {
        if (!pblocktree->WriteAddressIndex(addressIndex)) {
            return AbortNode(state, "Failed to write address index");
        }

        if (!pblocktree->UpdateAddressUnspentIndex(addressUnspentIndex)) {
            return AbortNode(state, "Failed to write address unspent index");
        }
    }

    if (fSpentIndex)
        if (!pblocktree->UpdateSpentIndex(spentIndex))
            return AbortNode(state, "Failed to write transaction index");

    if (fTimestampIndex)
        if (!pblocktree->WriteTimestampIndex(CTimestampIndexKey(pindex->nTime, pindex->GetBlockHash())))
            return AbortNode(state, "Failed to write timestamp index");

    int64_t nTime7 = GetTimeMicros(); nTimeIndexWrite += nTime7 - nTime6;
    LogPrint(BCLog::BENCHMARK, "      - Index writing: %.2fms [%.2fs (%.2fms/blk)]\n", MILLI * (nTime7 - nTime6), nTimeIndexWrite * MICRO, nTimeIndexWrite * MILLI / nBlocksTotal);

    assert(pindex->phashBlock);
    // add this block to the view's block chain
    view.SetBestBlock(pindex->GetBlockHash());
    m_evoDb.WriteBestBlock(pindex->GetBlockHash());

    int64_t nTime8 = GetTimeMicros(); nTimeCallbacks += nTime8 - nTime5;
    LogPrint(BCLog::BENCHMARK, "    - Callbacks: %.2fms [%.2fs (%.2fms/blk)]\n", MILLI * (nTime8 - nTime5), nTimeCallbacks * MICRO, nTimeCallbacks * MILLI / nBlocksTotal);

    boost::posix_time::ptime finish = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration diff = finish - start;
    statsClient.timing("ConnectBlock_ms", diff.total_milliseconds(), 1.0f);
    statsClient.gauge("blocks.tip.SizeBytes", ::GetSerializeSize(block, PROTOCOL_VERSION), 1.0f);
    statsClient.gauge("blocks.tip.Height", m_chain.Height(), 1.0f);
    statsClient.gauge("blocks.tip.Version", block.nVersion, 1.0f);
    statsClient.gauge("blocks.tip.NumTransactions", block.vtx.size(), 1.0f);
    statsClient.gauge("blocks.tip.SigOps", nSigOps, 1.0f);

    return true;
}

CoinsCacheSizeState CChainState::GetCoinsCacheSizeState()
{
    return this->GetCoinsCacheSizeState(
        m_coinstip_cache_size_bytes,
        gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000);
}

CoinsCacheSizeState CChainState::GetCoinsCacheSizeState(
    size_t max_coins_cache_size_bytes,
    size_t max_mempool_size_bytes)
{
    const int64_t nMempoolUsage = m_mempool ? m_mempool->DynamicMemoryUsage() : 0;
    int64_t cacheSize = CoinsTip().DynamicMemoryUsage();
    int64_t nTotalSpace =
        max_coins_cache_size_bytes + std::max<int64_t>(max_mempool_size_bytes - nMempoolUsage, 0);

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

bool CChainState::FlushStateToDisk(
    BlockValidationState &state,
    FlushStateMode mode,
    int nManualPruneHeight)
{
    LOCK(cs_main);
    assert(this->CanFlushToDisk());
    static std::chrono::microseconds nLastWrite{0};
    static std::chrono::microseconds nLastFlush{0};
    std::set<int> setFilesToPrune;
    bool full_flush_completed = false;

    const size_t coins_count = ::ChainstateActive().CoinsTip().GetCacheSize();
    const size_t coins_mem_usage = ::ChainstateActive().CoinsTip().DynamicMemoryUsage();

    try {
    {
        bool fFlushForPrune = false;
        bool fDoFullFlush = false;

        CoinsCacheSizeState cache_state = GetCoinsCacheSizeState();
        LOCK(cs_LastBlockFile);
        if (fPruneMode && (fCheckForPruning || nManualPruneHeight > 0) && !fReindex) {
            // make sure we don't prune above the blockfilterindexes bestblocks
            // pruning is height-based
            int last_prune = m_chain.Height(); // last height we can prune
            ForEachBlockFilterIndex([&](BlockFilterIndex& index) {
               last_prune = std::max(1, std::min(last_prune, index.GetSummary().best_block_height));
            });

            if (nManualPruneHeight > 0) {
                LOG_TIME_MILLIS_WITH_CATEGORY("find files to prune (manual)", BCLog::BENCHMARK);

                m_blockman.FindFilesToPruneManual(setFilesToPrune, std::min(last_prune, nManualPruneHeight), m_chain.Height());
            } else {
                LOG_TIME_MILLIS_WITH_CATEGORY("find files to prune", BCLog::BENCHMARK);

                m_blockman.FindFilesToPrune(setFilesToPrune, m_params.PruneAfterHeight(), m_chain.Height(), last_prune, IsInitialBlockDownload());
                fCheckForPruning = false;
            }
            if (!setFilesToPrune.empty()) {
                fFlushForPrune = true;
                if (!fHavePruned) {
                    pblocktree->WriteFlag("prunedblockfiles", true);
                    fHavePruned = true;
                }
            }
        }
        const auto nNow = GetTime<std::chrono::microseconds>();
        // Avoid writing/flushing immediately after startup.
        if (nLastWrite.count() == 0) {
            nLastWrite = nNow;
        }
        if (nLastFlush.count() == 0) {
            nLastFlush = nNow;
        }
        // The cache is large and we're within 10% and 10 MiB of the limit, but we have time now (not in the middle of a block processing).
        bool fCacheLarge = mode == FlushStateMode::PERIODIC && cache_state >= CoinsCacheSizeState::LARGE;
        // The cache is over the limit, we have to write now.
        bool fCacheCritical = mode == FlushStateMode::IF_NEEDED && cache_state >= CoinsCacheSizeState::CRITICAL;
        // The evodb cache is too large
        bool fEvoDbCacheCritical = mode == FlushStateMode::IF_NEEDED && m_evoDb.GetMemoryUsage() >= (64 << 20);
        // It's been a while since we wrote the block index to disk. Do this frequently, so we don't need to redownload after a crash.
        bool fPeriodicWrite = mode == FlushStateMode::PERIODIC && nNow > nLastWrite + DATABASE_WRITE_INTERVAL;
        // It's been very long since we flushed the cache. Do this infrequently, to optimize cache usage.
        bool fPeriodicFlush = mode == FlushStateMode::PERIODIC && nNow > nLastFlush + DATABASE_FLUSH_INTERVAL;
        // Combine all conditions that result in a full cache flush.
        fDoFullFlush = (mode == FlushStateMode::ALWAYS) || fCacheLarge || fCacheCritical || fEvoDbCacheCritical || fPeriodicFlush || fFlushForPrune;
        // Write blocks and block index to disk.
        if (fDoFullFlush || fPeriodicWrite) {
            // Depend on nMinDiskSpace to ensure we can write block index
            if (!CheckDiskSpace(GetBlocksDir())) {
                return AbortNode(state, "Disk space is too low!", _("Disk space is too low!"));
            }
            // First make sure all block and undo data is flushed to disk.
            {
                LOG_TIME_MILLIS_WITH_CATEGORY("write block and undo data to disk", BCLog::BENCHMARK);

                // First make sure all block and undo data is flushed to disk.
                FlushBlockFile();
            }

            // Then update all block file information (which may refer to block and undo files).
            {
                LOG_TIME_MILLIS_WITH_CATEGORY("write block index to disk", BCLog::BENCHMARK);

                std::vector<std::pair<int, const CBlockFileInfo*> > vFiles;
                vFiles.reserve(setDirtyFileInfo.size());
                for (std::set<int>::iterator it = setDirtyFileInfo.begin(); it != setDirtyFileInfo.end(); ) {
                    vFiles.push_back(std::make_pair(*it, &vinfoBlockFile[*it]));
                    setDirtyFileInfo.erase(it++);
                }
                std::vector<const CBlockIndex*> vBlocks;
                vBlocks.reserve(setDirtyBlockIndex.size());
                for (std::set<CBlockIndex*>::iterator it = setDirtyBlockIndex.begin(); it != setDirtyBlockIndex.end(); ) {
                    vBlocks.push_back(*it);
                    setDirtyBlockIndex.erase(it++);
                }
                if (!pblocktree->WriteBatchSync(vFiles, nLastBlockFile, vBlocks)) {
                    return AbortNode(state, "Failed to write to block index database");
                }
            }
            // Finally remove any pruned files
            if (fFlushForPrune) {
                LOG_TIME_MILLIS_WITH_CATEGORY("unlink pruned files", BCLog::BENCHMARK);

                UnlinkPrunedFiles(setFilesToPrune);
            }
            nLastWrite = nNow;
        }
        // Flush best chain related state. This can only be done if the blocks / block index write was also done.
        if (fDoFullFlush && !CoinsTip().GetBestBlock().IsNull()) {
            LOG_TIME_SECONDS(strprintf("write coins cache to disk (%d coins, %.2fkB)",
                coins_count, coins_mem_usage / 1000));

            // Typical Coin structures on disk are around 48 bytes in size.
            // Pushing a new one to the database can cause it to be written
            // twice (once in the log, and once in the tables). This is already
            // an overestimation, as most will delete an existing entry or
            // overwrite one. Still, use a conservative safety factor of 2.
            if (!CheckDiskSpace(GetDataDir(), 48 * 2 * 2 * CoinsTip().GetCacheSize())) {
                return AbortNode(state, "Disk space is too low!", _("Disk space is too low!"));
            }
            // Flush the chainstate (which may refer to block index entries).
            if (!CoinsTip().Flush())
                return AbortNode(state, "Failed to write to coin database");
            if (!m_evoDb.CommitRootTransaction()) {
                return AbortNode(state, "Failed to commit EvoDB");
            }
            nLastFlush = nNow;
            full_flush_completed = true;
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

void CChainState::ForceFlushStateToDisk()
{
    BlockValidationState state;
    if (!this->FlushStateToDisk(state, FlushStateMode::ALWAYS)) {
        LogPrintf("%s: failed to flush state (%s)\n", __func__, state.ToString());
    }
}

void CChainState::PruneAndFlush()
{
    BlockValidationState state;
    fCheckForPruning = true;

    if (!this->FlushStateToDisk(state, FlushStateMode::NONE)) {
        LogPrintf("%s: failed to flush state (%s)\n", __func__, state.ToString());
    }
}

static void DoWarning(const std::string& strWarning)
{
    static bool fWarned = false;
    SetMiscWarning(strWarning);
    if (!fWarned) {
        AlertNotify(strWarning);
        fWarned = true;
    }
}

/** Private helper function that concatenates warning messages. */
static void AppendWarning(std::string& res, const std::string& warn)
{
    if (!res.empty()) res += ", ";
    res += warn;
}

void CChainState::UpdateTip(const CBlockIndex* pindexNew)
{
    // New best block
    if (m_mempool) {
        m_mempool->AddTransactionsUpdated(1);
    }

    {
        LOCK(g_best_block_mutex);
        g_best_block = pindexNew->GetBlockHash();
        g_best_block_cv.notify_all();
    }

    std::string warningMessages;
    assert(std::addressof(::ChainstateActive()) == std::addressof(*this));
    if (!this->IsInitialBlockDownload())
    {
        const CBlockIndex* pindex = pindexNew;
        for (int bit = 0; bit < VERSIONBITS_NUM_BITS; bit++) {
            WarningBitsConditionChecker checker(bit);
            ThresholdState state = checker.GetStateFor(pindex, m_params.GetConsensus(), warningcache[bit]);
            if (state == ThresholdState::ACTIVE || state == ThresholdState::LOCKED_IN) {
                const std::string strWarning = strprintf(_("Warning: unknown new rules activated (versionbit %i)").translated, bit);
                if (state == ThresholdState::ACTIVE) {
                    DoWarning(strWarning);
                } else {
                    AppendWarning(warningMessages, strWarning);
                }
            }
        }
    }
    assert(std::addressof(::ChainstateActive()) == std::addressof(*this));
    LogPrintf("%s: new best=%s height=%d version=0x%08x log2_work=%.8g tx=%lu date='%s' progress=%f cache=%.1fMiB(%utxo) evodb_cache=%.1fMiB%s\n", __func__,
      pindexNew->GetBlockHash().ToString(), pindexNew->nHeight, pindexNew->nVersion,
      log(pindexNew->nChainWork.getdouble())/log(2.0), (unsigned long)pindexNew->nChainTx,
      FormatISO8601DateTime(pindexNew->GetBlockTime()),
      GuessVerificationProgress(m_params.TxData(), pindexNew), this->CoinsTip().DynamicMemoryUsage() * (1.0 / (1<<20)), this->CoinsTip().GetCacheSize(),
              m_evoDb.GetMemoryUsage() * (1.0 / (1<<20)),
      !warningMessages.empty() ? strprintf(" warning='%s'", warningMessages) : "");
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
bool CChainState::DisconnectTip(BlockValidationState& state, DisconnectedBlockTransactions* disconnectpool)
{
    AssertLockHeld(cs_main);
    if (m_mempool) AssertLockHeld(m_mempool->cs);

    CBlockIndex *pindexDelete = m_chain.Tip();
    assert(pindexDelete);
    // Read block from disk.
    std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
    CBlock& block = *pblock;
    if (!ReadBlockFromDisk(block, pindexDelete, m_params.GetConsensus())) {
        return error("DisconnectTip(): Failed to read block");
    }
    // Apply the block atomically to the chain state.
    int64_t nStart = GetTimeMicros();
    {
        auto dbTx = m_evoDb.BeginTransaction();

        CCoinsViewCache view(&CoinsTip());
        assert(view.GetBestBlock() == pindexDelete->GetBlockHash());
        if (DisconnectBlock(block, pindexDelete, view) != DISCONNECT_OK)
            return error("DisconnectTip(): DisconnectBlock %s failed", pindexDelete->GetBlockHash().ToString());
        bool flushed = view.Flush();
        assert(flushed);
        dbTx->Commit();
    }
    LogPrint(BCLog::BENCHMARK, "- Disconnect block: %.2fms\n", (GetTimeMicros() - nStart) * MILLI);
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

    m_chain.SetTip(pindexDelete->pprev);

    UpdateTip(pindexDelete->pprev);
    // Let wallets know transactions went from 1-confirmed to
    // 0-confirmed or conflicted:
    GetMainSignals().BlockDisconnected(pblock, pindexDelete);
    return true;
}

static int64_t nTimeReadFromDisk = 0;
static int64_t nTimeConnectTotal = 0;
static int64_t nTimeFlush = 0;
static int64_t nTimeChainState = 0;
static int64_t nTimePostConnect = 0;

struct PerBlockConnectTrace {
    CBlockIndex* pindex = nullptr;
    std::shared_ptr<const CBlock> pblock;
    PerBlockConnectTrace() {}
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
bool CChainState::ConnectTip(BlockValidationState& state, CBlockIndex* pindexNew, const std::shared_ptr<const CBlock>& pblock, ConnectTrace& connectTrace, DisconnectedBlockTransactions& disconnectpool)
{
    boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();
    AssertLockHeld(cs_main);
    if (m_mempool) AssertLockHeld(m_mempool->cs);

    assert(pindexNew->pprev == m_chain.Tip());
    // Read block from disk.
    int64_t nTime1 = GetTimeMicros();
    std::shared_ptr<const CBlock> pthisBlock;
    if (!pblock) {
        std::shared_ptr<CBlock> pblockNew = std::make_shared<CBlock>();
        if (!ReadBlockFromDisk(*pblockNew, pindexNew, m_params.GetConsensus())) {
            return AbortNode(state, "Failed to read block");
        }
        pthisBlock = pblockNew;
    } else {
        pthisBlock = pblock;
    }
    const CBlock& blockConnecting = *pthisBlock;
    // Apply the block atomically to the chain state.
    int64_t nTime2 = GetTimeMicros(); nTimeReadFromDisk += nTime2 - nTime1;
    int64_t nTime3;
    LogPrint(BCLog::BENCHMARK, "  - Load block from disk: %.2fms [%.2fs]\n", (nTime2 - nTime1) * MILLI, nTimeReadFromDisk * MICRO);
    {
        auto dbTx = m_evoDb.BeginTransaction();

        CCoinsViewCache view(&CoinsTip());
        bool rv = ConnectBlock(blockConnecting, state, pindexNew, view);
        GetMainSignals().BlockChecked(blockConnecting, state);
        if (!rv) {
            if (state.IsInvalid())
                InvalidBlockFound(pindexNew, state);
            return error("%s: ConnectBlock %s failed, %s", __func__, pindexNew->GetBlockHash().ToString(), state.ToString());
        }
        nTime3 = GetTimeMicros(); nTimeConnectTotal += nTime3 - nTime2;
        assert(nBlocksTotal > 0);
        LogPrint(BCLog::BENCHMARK, "  - Connect total: %.2fms [%.2fs (%.2fms/blk)]\n", (nTime3 - nTime2) * MILLI, nTimeConnectTotal * MICRO, nTimeConnectTotal * MILLI / nBlocksTotal);
        bool flushed = view.Flush();
        assert(flushed);
        dbTx->Commit();
    }
    int64_t nTime4 = GetTimeMicros(); nTimeFlush += nTime4 - nTime3;
    LogPrint(BCLog::BENCHMARK, "  - Flush: %.2fms [%.2fs (%.2fms/blk)]\n", (nTime4 - nTime3) * MILLI, nTimeFlush * MICRO, nTimeFlush * MILLI / nBlocksTotal);
    // Write the chain state to disk, if necessary.
    if (!FlushStateToDisk(state, FlushStateMode::IF_NEEDED)) {
        return false;
    }
    int64_t nTime5 = GetTimeMicros(); nTimeChainState += nTime5 - nTime4;
    LogPrint(BCLog::BENCHMARK, "  - Writing chainstate: %.2fms [%.2fs (%.2fms/blk)]\n", (nTime5 - nTime4) * MILLI, nTimeChainState * MICRO, nTimeChainState * MILLI / nBlocksTotal);
    // Remove conflicting transactions from the mempool.;
    if (m_mempool) {
        m_mempool->removeForBlock(blockConnecting.vtx, pindexNew->nHeight);
        m_mempool->removeExpiredAssetUnlock(pindexNew->nHeight);
        disconnectpool.removeForBlock(blockConnecting.vtx);
    }
    // Update m_chain & related variables.
    m_chain.SetTip(pindexNew);
    UpdateTip(pindexNew);

    int64_t nTime6 = GetTimeMicros(); nTimePostConnect += nTime6 - nTime5; nTimeTotal += nTime6 - nTime1;
    LogPrint(BCLog::BENCHMARK, "  - Connect postprocess: %.2fms [%.2fs (%.2fms/blk)]\n", (nTime6 - nTime5) * MILLI, nTimePostConnect * MICRO, nTimePostConnect * MILLI / nBlocksTotal);
    LogPrint(BCLog::BENCHMARK, "- Connect block: %.2fms [%.2fs (%.2fms/blk)]\n", (nTime6 - nTime1) * MILLI, nTimeTotal * MICRO, nTimeTotal * MILLI / nBlocksTotal);

    boost::posix_time::ptime finish = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration diff = finish - start;
    statsClient.timing("ConnectTip_ms", diff.total_milliseconds(), 1.0f);

    connectTrace.BlockConnected(pindexNew, std::move(pthisBlock));
    return true;
}

/**
 * Return the tip of the chain with the most work in it, that isn't
 * known to be invalid (it's however far from certain to be valid).
 */
CBlockIndex* CChainState::FindMostWorkChain() {
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
            bool fConflictingChain = pindexTest->nStatus & BLOCK_CONFLICT_CHAINLOCK;
            bool fMissingData = !(pindexTest->nStatus & BLOCK_HAVE_DATA);
            if (fFailedChain || fMissingData || fConflictingChain) {
                // Candidate chain is not usable (either invalid or conflicting or missing data)
                if (fFailedChain && (pindexBestInvalid == nullptr || pindexNew->nChainWork > pindexBestInvalid->nChainWork))
                    pindexBestInvalid = pindexNew;
                CBlockIndex *pindexFailed = pindexNew;
                // Remove the entire chain from the set.
                while (pindexTest != pindexFailed) {
                    if (fFailedChain) {
                        pindexFailed->nStatus |= BLOCK_FAILED_CHILD;
                    } else if (fConflictingChain) {
                        // We don't need data for conflciting blocks
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
void CChainState::PruneBlockIndexCandidates() {
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
bool CChainState::ActivateBestChainStep(BlockValidationState& state, CBlockIndex* pindexMostWork, const std::shared_ptr<const CBlock>& pblock, bool& fInvalidFound, ConnectTrace& connectTrace)
{
    AssertLockHeld(cs_main);
    if (m_mempool) AssertLockHeld(m_mempool->cs);
    assert(std::addressof(::ChainstateActive()) == std::addressof(*this));

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
    if (m_mempool) m_mempool->check(*this);

    CheckForkWarningConditions();

    return true;
}

static bool NotifyHeaderTip(CChainState& chainstate) LOCKS_EXCLUDED(cs_main) {
    bool fNotify = false;
    bool fInitialBlockDownload = false;
    static CBlockIndex* pindexHeaderOld = nullptr;
    CBlockIndex* pindexHeader = nullptr;
    {
        LOCK(cs_main);
        pindexHeader = pindexBestHeader;

        if (pindexHeader != pindexHeaderOld) {
            fNotify = true;
            assert(std::addressof(::ChainstateActive()) == std::addressof(chainstate));
            fInitialBlockDownload = chainstate.IsInitialBlockDownload();
            pindexHeaderOld = pindexHeader;
        }
    }
    // Send block tip changed notifications without cs_main
    if (fNotify) {
        uiInterface.NotifyHeaderTip(fInitialBlockDownload, pindexHeader);
        GetMainSignals().NotifyHeaderTip(pindexHeader, fInitialBlockDownload);
    }
    return fNotify;
}

static void LimitValidationInterfaceQueue() LOCKS_EXCLUDED(cs_main) {
    AssertLockNotHeld(cs_main);

    if (GetMainSignals().CallbacksPending() > 10) {
        SyncWithValidationInterfaceQueue();
    }
}

bool CChainState::ActivateBestChain(BlockValidationState& state, std::shared_ptr<const CBlock> pblock)
{
    // Note that while we're often called here from ProcessNewBlock, this is
    // far from a guarantee. Things in the P2P/RPC will often end up calling
    // us in the middle of ProcessNewBlock - do not assume pblock is set
    // sanely for performance or correctness!
    AssertLockNotHeld(cs_main);

    // ABC maintains a fair degree of expensive-to-calculate internal state
    // because this function periodically releases cs_main so that it does not lock up other threads for too long
    // during large connects - and to allow for e.g. the callback queue to drain
    // we use m_cs_chainstate to enforce mutual exclusion so that only one caller may execute this function at a time
    LOCK(m_cs_chainstate);

    boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();

    CBlockIndex *pindexMostWork = nullptr;
    CBlockIndex *pindexNewTip = nullptr;
    int nStopAtHeight = gArgs.GetArg("-stopatheight", DEFAULT_STOPATHEIGHT);
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
            } while (!m_chain.Tip() || (starting_tip && CBlockIndexWorkComparator()(m_chain.Tip(), starting_tip)));
            if (!blocks_connected) return true;

            const CBlockIndex* pindexFork = m_chain.FindFork(starting_tip);
            bool fInitialDownload = IsInitialBlockDownload();

            // Notify external listeners about the new tip.
            // Enqueue while holding cs_main to ensure that UpdatedBlockTip is called in the order in which blocks are connected
            if (pindexFork != pindexNewTip) {
                // Notify ValidationInterface subscribers
                GetMainSignals().SynchronousUpdatedBlockTip(pindexNewTip, pindexFork, fInitialDownload);
                GetMainSignals().UpdatedBlockTip(pindexNewTip, pindexFork, fInitialDownload);

                // Always notify the UI if a new block tip was connected
                uiInterface.NotifyBlockTip(fInitialDownload, pindexNewTip);
            }
        }
        // When we reach this point, we switched to a new tip (stored in pindexNewTip).

        if (nStopAtHeight && pindexNewTip && pindexNewTip->nHeight >= nStopAtHeight) StartShutdown();

        // We check shutdown only after giving ActivateBestChainStep a chance to run once so that we
        // never shutdown before connecting the genesis block during LoadChainTip(). Previously this
        // caused an assert() failure during shutdown in such cases as the UTXO DB flushing checks
        // that the best block hash is non-null.
        if (ShutdownRequested()) break;
    } while (pindexNewTip != pindexMostWork);
    CheckBlockIndex();

    boost::posix_time::ptime finish = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration diff = finish - start;
    statsClient.timing("ActivateBestChain_ms", diff.total_milliseconds(), 1.0f);

    // Write changes periodically to disk, after relay.
    if (!FlushStateToDisk(state, FlushStateMode::PERIODIC)) {
        return false;
    }

    return true;
}

bool CChainState::PreciousBlock(BlockValidationState& state, CBlockIndex* pindex)
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
        if (pindex->IsValid(BLOCK_VALID_TRANSACTIONS) && !(pindex->nStatus & BLOCK_CONFLICT_CHAINLOCK) && pindex->HaveTxsDownloaded()) {
            setBlockIndexCandidates.insert(pindex);
            PruneBlockIndexCandidates();
        }
    }

    return ActivateBestChain(state, std::shared_ptr<const CBlock>());
}

bool CChainState::InvalidateBlock(BlockValidationState& state, CBlockIndex* pindex)
{
    // Genesis block can't be invalidated
    assert(pindex);
    if (pindex->nHeight == 0) return false;

    CBlockIndex* to_mark_failed = pindex;
    bool pindex_was_in_chain = false;
    int disconnected = 0;

    // We do not allow ActivateBestChain() to run while InvalidateBlock() is
    // running, as that could cause the tip to change while we disconnect
    // blocks.
    LOCK(m_cs_chainstate);

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
        for (const auto& entry : m_blockman.m_block_index) {
            CBlockIndex *candidate = entry.second;
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
        const CBlockIndex* pindexOldTip = m_chain.Tip();

        if (pindex == pindexBestHeader) {
            pindexBestInvalid = pindexBestHeader;
            pindexBestHeader = pindexBestHeader->pprev;
        }

        // ActivateBestChain considers blocks already in m_chain
        // unconditionally valid already, so force disconnect away from it.
        DisconnectedBlockTransactions disconnectpool;
        bool ret = DisconnectTip(state, &disconnectpool);
        // DisconnectTip will add transactions to disconnectpool.
        // Adjust the mempool to be consistent with the new tip, adding
        // transactions back to the mempool if disconnecting was successful,
        // and we're not doing a very deep invalidation (in which case
        // keeping the mempool up to date is probably futile anyway).
        assert(std::addressof(::ChainstateActive()) == std::addressof(*this));
        MaybeUpdateMempoolForReorg(disconnectpool, /* fAddToMempool = */ (++disconnected <= 10) && ret);
        if (!ret) return false;
        assert(invalid_walk_tip->pprev == m_chain.Tip());

        if (pindexOldTip == pindexBestHeader) {
            pindexBestInvalid = pindexBestHeader;
            pindexBestHeader = pindexBestHeader->pprev;
        }

        // We immediately mark the disconnected blocks as invalid.
        // This prevents a case where pruned nodes may fail to invalidateblock
        // and be left unable to start as they have no tip candidates (as there
        // are no blocks that meet the "have data and are not invalid per
        // nStatus" criteria for inclusion in setBlockIndexCandidates).
        invalid_walk_tip->nStatus |= BLOCK_FAILED_VALID;
        setDirtyBlockIndex.insert(invalid_walk_tip);
        setBlockIndexCandidates.erase(invalid_walk_tip);
        setBlockIndexCandidates.insert(invalid_walk_tip->pprev);
        if (invalid_walk_tip->pprev == to_mark_failed && (to_mark_failed->nStatus & BLOCK_FAILED_VALID)) {
            // We only want to mark the last disconnected block as BLOCK_FAILED_VALID; its children
            // need to be BLOCK_FAILED_CHILD instead.
            to_mark_failed->nStatus = (to_mark_failed->nStatus ^ BLOCK_FAILED_VALID) | BLOCK_FAILED_CHILD;
            setDirtyBlockIndex.insert(to_mark_failed);
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
        setDirtyBlockIndex.insert(to_mark_failed);
        setBlockIndexCandidates.erase(to_mark_failed);
        m_blockman.m_failed_blocks.insert(to_mark_failed);

        // If any new blocks somehow arrived while we were disconnecting
        // (above), then the pre-calculation of what should go into
        // setBlockIndexCandidates may have missed entries. This would
        // technically be an inconsistency in the block index, but if we clean
        // it up here, this should be an essentially unobservable error.
        // Loop back over all block index entries and add any missing entries
        // to setBlockIndexCandidates.
        BlockMap::iterator it = g_chainman.BlockIndex().begin();
        while (it != g_chainman.BlockIndex().end()) {
            if (it->second->IsValid(BLOCK_VALID_TRANSACTIONS) && !(it->second->nStatus & BLOCK_CONFLICT_CHAINLOCK) && it->second->HaveTxsDownloaded() && !setBlockIndexCandidates.value_comp()(it->second, m_chain.Tip())) {
                setBlockIndexCandidates.insert(it->second);
            }
            it++;
        }

        InvalidChainFound(to_mark_failed);
        GetMainSignals().SynchronousUpdatedBlockTip(m_chain.Tip(), nullptr, IsInitialBlockDownload());
        GetMainSignals().UpdatedBlockTip(m_chain.Tip(), nullptr, IsInitialBlockDownload());
    }

    // Only notify about a new block tip if the active chain was modified.
    if (pindex_was_in_chain) {
        uiInterface.NotifyBlockTip(IsInitialBlockDownload(), to_mark_failed->pprev);
    }
    return true;
}

void CChainState::EnforceBlock(BlockValidationState& state, const CBlockIndex *pindex)
{
    AssertLockNotHeld(::cs_main);

    LOCK2(m_cs_chainstate, ::cs_main);
    assert(std::addressof(::ChainstateActive()) == std::addressof(*this));

    const CBlockIndex* pindex_walk = pindex;

    while (pindex_walk && !m_chain.Contains(pindex_walk)) {
        // Mark all blocks that have the same prevBlockHash but are not equal to blockHash as conflicting
        auto itp = g_chainman.PrevBlockIndex().equal_range(pindex_walk->pprev->GetBlockHash());
        for (auto jt = itp.first; jt != itp.second; ++jt) {
            if (jt->second == pindex_walk) {
                continue;
            }
            if (!MarkConflictingBlock(state, jt->second)) {
                LogPrintf("CChainState::%s -- MarkConflictingBlock failed: %s\n", __func__, state.ToString());
                // This should not have happened and we are in a state were it's not safe to continue anymore
                assert(false);
            }
            LogPrintf("CChainState::%s -- marked block %s as conflicting\n",
                      __func__, jt->second->GetBlockHash().ToString());
        }
        pindex_walk = pindex_walk->pprev;
    }
    // In case blocks from the enforced chain are invalid at the moment, reconsider them.
    if (!pindex->IsValid()) {
        ResetBlockFailureFlags(m_blockman.LookupBlockIndex(pindex->GetBlockHash()));
    }
}

bool CChainState::MarkConflictingBlock(BlockValidationState& state, CBlockIndex *pindex)
{
    AssertLockHeld(cs_main);
    assert(std::addressof(::ChainstateActive()) == std::addressof(*this));

    // We first disconnect backwards and then mark the blocks as conflicting.

    bool pindex_was_in_chain = false;
    CBlockIndex *conflicting_walk_tip = m_chain.Tip();

    if (pindex == pindexBestHeader) {
        pindexBestHeader = pindexBestHeader->pprev;
    }

    {
    LOCK(MempoolMutex()); // Lock for as long as disconnectpool is in scope to make sure UpdateMempoolForReorg is called after DisconnectTip without unlocking in between
    DisconnectedBlockTransactions disconnectpool;
    while (m_chain.Contains(pindex)) {
        const CBlockIndex* pindexOldTip = m_chain.Tip();
        pindex_was_in_chain = true;
        // ActivateBestChain considers blocks already in m_chain
        // unconditionally valid already, so force disconnect away from it.
        if (!DisconnectTip(state, &disconnectpool)) {
            // It's probably hopeless to try to make the mempool consistent
            // here if DisconnectTip failed, but we can try.
            MaybeUpdateMempoolForReorg(disconnectpool, false);
            return false;
        }
        if (pindexOldTip == pindexBestHeader) {
            pindexBestHeader = pindexBestHeader->pprev;
        }
    }

    // Now mark the blocks we just disconnected as descendants conflicting
    // (note this may not be all descendants).
    while (pindex_was_in_chain && conflicting_walk_tip != pindex) {
        conflicting_walk_tip->nStatus |= BLOCK_CONFLICT_CHAINLOCK;
        setBlockIndexCandidates.erase(conflicting_walk_tip);
        conflicting_walk_tip = conflicting_walk_tip->pprev;
    }

    // Mark the block itself as conflicting.
    pindex->nStatus |= BLOCK_CONFLICT_CHAINLOCK;
    setBlockIndexCandidates.erase(pindex);

    // DisconnectTip will add transactions to disconnectpool; try to add these
    // back to the mempool.
        MaybeUpdateMempoolForReorg(disconnectpool, true);
    } // m_mempool.cs

    // The resulting new best tip may not be in setBlockIndexCandidates anymore, so
    // add it again.
    BlockMap::iterator it = g_chainman.BlockIndex().begin();
    while (it != g_chainman.BlockIndex().end()) {
        if (it->second->IsValid(BLOCK_VALID_TRANSACTIONS) && !(it->second->nStatus & BLOCK_CONFLICT_CHAINLOCK) && it->second->HaveTxsDownloaded() && !setBlockIndexCandidates.value_comp()(it->second, m_chain.Tip())) {
            setBlockIndexCandidates.insert(it->second);
        }
        it++;
    }

    ConflictingChainFound(pindex);
    GetMainSignals().SynchronousUpdatedBlockTip(m_chain.Tip(), nullptr, IsInitialBlockDownload());
    GetMainSignals().UpdatedBlockTip(m_chain.Tip(), nullptr, IsInitialBlockDownload());

    // Only notify about a new block tip if the active chain was modified.
    if (pindex_was_in_chain) {
        uiInterface.NotifyBlockTip(IsInitialBlockDownload(), pindex->pprev);
    }
    return true;
}

void CChainState::ResetBlockFailureFlags(CBlockIndex *pindex) {
    AssertLockHeld(cs_main);

    if (!pindex) {
        if (pindexBestInvalid && pindexBestInvalid->GetAncestor(m_chain.Height()) == m_chain.Tip()) {
            LogPrintf("%s: the best known invalid block (%s) is ahead of our tip, reconsidering\n",
                    __func__, pindexBestInvalid->GetBlockHash().ToString());
            pindex = pindexBestInvalid;
        } else {
            return;
        }
    }

    int nHeight = pindex->nHeight;

    // Remove the invalidity flag from this block and all its descendants.
    BlockMap::iterator it = m_blockman.m_block_index.begin();
    while (it != m_blockman.m_block_index.end()) {
        if (!it->second->IsValid() && it->second->GetAncestor(nHeight) == pindex) {
            it->second->nStatus &= ~BLOCK_FAILED_MASK;
            setDirtyBlockIndex.insert(it->second);
            if (it->second->IsValid(BLOCK_VALID_TRANSACTIONS) && !(it->second->nStatus & BLOCK_CONFLICT_CHAINLOCK) && it->second->HaveTxsDownloaded() && setBlockIndexCandidates.value_comp()(m_chain.Tip(), it->second)) {
                setBlockIndexCandidates.insert(it->second);
            }
            if (it->second == pindexBestInvalid) {
                // Reset invalid block marker if it was pointing to one of those.
                pindexBestInvalid = nullptr;
            }
            m_blockman.m_failed_blocks.erase(it->second);
        }
        it++;
    }

    // Remove the invalidity flag from all ancestors too.
    while (pindex != nullptr) {
        if (pindex->nStatus & BLOCK_FAILED_MASK) {
            pindex->nStatus &= ~BLOCK_FAILED_MASK;
            setDirtyBlockIndex.insert(pindex);
            if (pindex->IsValid(BLOCK_VALID_TRANSACTIONS) && !(pindex->nStatus & BLOCK_CONFLICT_CHAINLOCK) && pindex->HaveTxsDownloaded() && setBlockIndexCandidates.value_comp()(m_chain.Tip(), pindex)) {
                setBlockIndexCandidates.insert(pindex);
            }
            if (pindex == pindexBestInvalid) {
                // Reset invalid block marker if it was pointing to one of those.
                pindexBestInvalid = nullptr;
            }
            m_blockman.m_failed_blocks.erase(pindex);
            // Mark all nearest BLOCK_FAILED_CHILD descendants (if any) as BLOCK_FAILED_VALID
            auto itp = m_blockman.m_prev_block_index.equal_range(pindex->GetBlockHash());
            for (auto jt = itp.first; jt != itp.second; ++jt) {
                if (jt->second->nStatus & BLOCK_FAILED_CHILD) {
                    jt->second->nStatus |= BLOCK_FAILED_VALID;
                    m_blockman.m_failed_blocks.insert(jt->second);
                    setDirtyBlockIndex.insert(jt->second);
                    setBlockIndexCandidates.erase(jt->second);
                }
            }
        }
        pindex = pindex->pprev;
    }
}

CBlockIndex* BlockManager::AddToBlockIndex(const CBlockHeader& block, enum BlockStatus nStatus)
{
    assert(!(nStatus & BLOCK_FAILED_MASK)); // no failed blocks allowed
    AssertLockHeld(cs_main);

    // Check for duplicate
    uint256 hash = block.GetHash();
    BlockMap::iterator it = m_block_index.find(hash);
    if (it != m_block_index.end())
        return it->second;

    // Construct new block index object
    CBlockIndex* pindexNew = new CBlockIndex(block);
    // We assign the sequence id to blocks only when the full data is available,
    // to avoid miners withholding blocks but broadcasting headers, to get a
    // competitive advantage.
    pindexNew->nSequenceId = 0;
    BlockMap::iterator mi = m_block_index.insert(std::make_pair(hash, pindexNew)).first;
    pindexNew->phashBlock = &((*mi).first);
    BlockMap::iterator miPrev = m_block_index.find(block.hashPrevBlock);
    if (miPrev != m_block_index.end())
    {
        pindexNew->pprev = (*miPrev).second;
        pindexNew->nHeight = pindexNew->pprev->nHeight + 1;
        pindexNew->BuildSkip();
    }
    pindexNew->nTimeMax = (pindexNew->pprev ? std::max(pindexNew->pprev->nTimeMax, pindexNew->nTime) : pindexNew->nTime);
    pindexNew->nChainWork = (pindexNew->pprev ? pindexNew->pprev->nChainWork : 0) + GetBlockProof(*pindexNew);
    if (nStatus & BLOCK_VALID_MASK) {
        pindexNew->RaiseValidity(nStatus);
        if (pindexBestHeader == nullptr || pindexBestHeader->nChainWork < pindexNew->nChainWork)
            pindexBestHeader = pindexNew;
    } else {
        pindexNew->RaiseValidity(BLOCK_VALID_TREE); // required validity level
        pindexNew->nStatus |= nStatus;
    }

    setDirtyBlockIndex.insert(pindexNew);

    // track prevBlockHash -> pindex (multimap)
    if (pindexNew->pprev) {
        m_prev_block_index.emplace(pindexNew->pprev->GetBlockHash(), pindexNew);
    }

    return pindexNew;
}

/** Mark a block as having its data received and checked (up to BLOCK_VALID_TRANSACTIONS). */
void CChainState::ReceivedBlockTransactions(const CBlock& block, CBlockIndex* pindexNew, const FlatFilePos& pos)
{
    pindexNew->nTx = block.vtx.size();
    pindexNew->nChainTx = 0;
    pindexNew->nFile = pos.nFile;
    pindexNew->nDataPos = pos.nPos;
    pindexNew->nUndoPos = 0;
    pindexNew->nStatus |= BLOCK_HAVE_DATA;
    pindexNew->RaiseValidity(BLOCK_VALID_TRANSACTIONS);
    setDirtyBlockIndex.insert(pindexNew);

    if (pindexNew->pprev == nullptr || pindexNew->pprev->HaveTxsDownloaded()) {
        // If pindexNew is the genesis block or all parents are BLOCK_VALID_TRANSACTIONS.
        std::deque<CBlockIndex*> queue;
        queue.push_back(pindexNew);

        // Recursively process any descendant blocks that now may be eligible to be connected.
        while (!queue.empty()) {
            CBlockIndex *pindex = queue.front();
            queue.pop_front();
            pindex->nChainTx = (pindex->pprev ? pindex->pprev->nChainTx : 0) + pindex->nTx;
            {
                LOCK(cs_nBlockSequenceId);
                pindex->nSequenceId = nBlockSequenceId++;
            }
            if (m_chain.Tip() == nullptr || !setBlockIndexCandidates.value_comp()(pindex, m_chain.Tip())) {
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

// TODO move to blockstorage
bool FindBlockPos(FlatFilePos& pos, unsigned int nAddSize, unsigned int nHeight, CChain& active_chain, uint64_t nTime, bool fKnown = false)
{
    LOCK(cs_LastBlockFile);

    unsigned int nFile = fKnown ? pos.nFile : nLastBlockFile;
    if (vinfoBlockFile.size() <= nFile) {
        vinfoBlockFile.resize(nFile + 1);
    }

    bool finalize_undo = false;
    if (!fKnown) {
        while (vinfoBlockFile[nFile].nSize + nAddSize >= (gArgs.GetBoolArg("-fastprune", false) ? 0x10000 /* 64kb */ : MAX_BLOCKFILE_SIZE)) {
            // when the undo file is keeping up with the block file, we want to flush it explicitly
            // when it is lagging behind (more blocks arrive than are being connected), we let the
            // undo block write case handle it
            assert(std::addressof(::ChainActive()) == std::addressof(active_chain));
            finalize_undo = (vinfoBlockFile[nFile].nHeightLast == (unsigned int)active_chain.Tip()->nHeight);
            nFile++;
            if (vinfoBlockFile.size() <= nFile) {
                vinfoBlockFile.resize(nFile + 1);
            }
        }
        pos.nFile = nFile;
        pos.nPos = vinfoBlockFile[nFile].nSize;
    }

    if ((int)nFile != nLastBlockFile) {
        if (!fKnown) {
            LogPrintf("Leaving block file %i: %s\n", nLastBlockFile, vinfoBlockFile[nLastBlockFile].ToString());
        }
        FlushBlockFile(!fKnown, finalize_undo);
        nLastBlockFile = nFile;
    }

    vinfoBlockFile[nFile].AddBlock(nHeight, nTime);
    if (fKnown)
        vinfoBlockFile[nFile].nSize = std::max(pos.nPos + nAddSize, vinfoBlockFile[nFile].nSize);
    else
        vinfoBlockFile[nFile].nSize += nAddSize;

    if (!fKnown) {
        bool out_of_space;
        size_t bytes_allocated = BlockFileSeq().Allocate(pos, nAddSize, out_of_space);
        if (out_of_space) {
            return AbortNode("Disk space is too low!", _("Disk space is too low!"));
        }
        if (bytes_allocated != 0 && fPruneMode) {
            fCheckForPruning = true;
        }
    }

    setDirtyFileInfo.insert(nFile);
    return true;
}

static bool FindUndoPos(BlockValidationState &state, int nFile, FlatFilePos &pos, unsigned int nAddSize)
{
    pos.nFile = nFile;

    LOCK(cs_LastBlockFile);

    pos.nPos = vinfoBlockFile[nFile].nUndoSize;
    vinfoBlockFile[nFile].nUndoSize += nAddSize;
    setDirtyFileInfo.insert(nFile);

    bool out_of_space;
    size_t bytes_allocated = UndoFileSeq().Allocate(pos, nAddSize, out_of_space);
    if (out_of_space) {
        return AbortNode(state, "Disk space is too low!", _("Disk space is too low!"));
    }
    if (bytes_allocated != 0 && fPruneMode) {
        fCheckForPruning = true;
    }

    return true;
}

static bool CheckBlockHeader(const CBlockHeader& block, BlockValidationState& state, const Consensus::Params& consensusParams, bool fCheckPOW = true)
{
    // Check proof of work matches claimed amount
    if (fCheckPOW && !CheckProofOfWork(block.GetHash(), block.nBits, consensusParams))
        return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "high-hash", "proof of work failed");

    // Check DevNet
    if (!consensusParams.hashDevnetGenesisBlock.IsNull() &&
            block.hashPrevBlock == consensusParams.hashGenesisBlock &&
            block.GetHash() != consensusParams.hashDevnetGenesisBlock) {
        LogPrintf("ERROR: CheckBlockHeader(): wrong devnet genesis\n");
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "devnet-genesis");
    }

    return true;
}

bool CheckBlock(const CBlock& block, BlockValidationState& state, const Consensus::Params& consensusParams, bool fCheckPOW, bool fCheckMerkleRoot)
{
    // These are checks that are independent of context.

    boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();

    if (block.fChecked)
        return true;

    // Check that the header is valid (particularly PoW).  This is mostly
    // redundant with the call in AcceptBlockHeader.
    if (!CheckBlockHeader(block, state, consensusParams, fCheckPOW))
        return false;

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

    // Size limits (relaxed)
    if (block.vtx.empty() || block.vtx.size() > MaxBlockSize() || ::GetSerializeSize(block, PROTOCOL_VERSION) > MaxBlockSize())
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
    unsigned int nSigOps = 0;
    for (const auto& tx : block.vtx)
    {
        nSigOps += GetLegacySigOpCount(*tx);
    }
    // sigops limits (relaxed)
    if (nSigOps > MaxBlockSigOps())
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-blk-sigops", "out-of-bounds SigOpCount");

    if (fCheckPOW && fCheckMerkleRoot)
        block.fChecked = true;

    boost::posix_time::ptime finish = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration diff = finish - start;
    statsClient.timing("CheckBlock_us", diff.total_microseconds(), 1.0f);

    return true;
}

CBlockIndex* BlockManager::GetLastCheckpoint(const CCheckpointData& data)
{
    const MapCheckpoints& checkpoints = data.mapCheckpoints;

    for (const MapCheckpoints::value_type& i : reverse_iterate(checkpoints))
    {
        const uint256& hash = i.second;
        assert(std::addressof(g_chainman.m_blockman) == std::addressof(*this));
        CBlockIndex* pindex = LookupBlockIndex(hash);
        if (pindex) {
            return pindex;
        }
    }
    return nullptr;
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
static bool ContextualCheckBlockHeader(const CBlockHeader& block, BlockValidationState& state, BlockManager& blockman, const CChainParams& params, const CBlockIndex* pindexPrev, int64_t nAdjustedTime) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    assert(pindexPrev != nullptr);
    const int nHeight = pindexPrev->nHeight + 1;

    // Check proof of work
    const Consensus::Params& consensusParams = params.GetConsensus();
    if(Params().NetworkIDString() == CBaseChainParams::MAIN && nHeight <= 68589){
        // architecture issues with DGW v1 and v2)
        unsigned int nBitsNext = GetNextWorkRequired(pindexPrev, &block, consensusParams);
        double n1 = ConvertBitsToDouble(block.nBits);
        double n2 = ConvertBitsToDouble(nBitsNext);

        if (abs(n1-n2) > n1*0.5) {
            LogPrintf("ERROR: %s : incorrect proof of work (DGW pre-fork) - %f %f %f at %d\n", __func__, abs(n1-n2), n1, n2, nHeight);
            return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "bad-diffbits");
        }
    } else {
        if (block.nBits != GetNextWorkRequired(pindexPrev, &block, consensusParams))
            return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "bad-diffbits", strprintf("incorrect proof of work at %d", nHeight));
    }

    // Check against checkpoints
    if (fCheckpointsEnabled) {
        // Don't accept any forks from the main chain prior to last checkpoint.
        // GetLastCheckpoint finds the last checkpoint in MapCheckpoints that's in our
        // BlockIndex().
        assert(std::addressof(g_chainman.m_blockman) == std::addressof(blockman));
        CBlockIndex* pcheckpoint = blockman.GetLastCheckpoint(params.Checkpoints());
        if (pcheckpoint && nHeight < pcheckpoint->nHeight) {
            LogPrintf("ERROR: %s: forked chain older than last checkpoint (height %d)\n", __func__, nHeight);
            return state.Invalid(BlockValidationResult::BLOCK_CHECKPOINT, "bad-fork-prior-to-checkpoint");
        }
    }

    // Check timestamp against prev
    if (block.GetBlockTime() <= pindexPrev->GetMedianTimePast())
        return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "time-too-old", strprintf("block's timestamp is too early %d %d", block.GetBlockTime(), pindexPrev->GetMedianTimePast()));

    // Check timestamp
    if (block.GetBlockTime() > nAdjustedTime + MAX_FUTURE_BLOCK_TIME)
        return state.Invalid(BlockValidationResult::BLOCK_TIME_FUTURE, "time-too-new", strprintf("block timestamp too far in the future %d %d", block.GetBlockTime(), nAdjustedTime + 2 * 60 * 60));

    // check for version 2, 3 and 4 upgrades
    if((block.nVersion < 2 && nHeight >= consensusParams.BIP34Height) ||
       (block.nVersion < 3 && nHeight >= consensusParams.BIP66Height) ||
       (block.nVersion < 4 && nHeight >= consensusParams.BIP65Height))
            return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, strprintf("bad-version(0x%08x)", block.nVersion),
                                 strprintf("rejected nVersion=0x%08x block", block.nVersion));

    return true;
}

/** NOTE: This function is not currently invoked by ConnectBlock(), so we
 *  should consider upgrade issues if we change which consensus rules are
 *  enforced in this function (eg by adding a new consensus rule). See comment
 *  in ConnectBlock().
 *  Note that -reindex-chainstate skips the validation that happens here!
 */
static bool ContextualCheckBlock(const CBlock& block, BlockValidationState& state, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    AssertLockHeld(cs_main);
    const int nHeight = pindexPrev == nullptr ? 0 : pindexPrev->nHeight + 1;

    // Start enforcing BIP113 (Median Time Past).
    int nLockTimeFlags = 0;
    if (nHeight >= consensusParams.CSVHeight) {
        assert(pindexPrev != nullptr);
        nLockTimeFlags |= LOCKTIME_MEDIAN_TIME_PAST;
    }

    int64_t nLockTimeCutoff = (nLockTimeFlags & LOCKTIME_MEDIAN_TIME_PAST)
                              ? pindexPrev->GetMedianTimePast()
                              : block.GetBlockTime();

    bool fDIP0001Active_context = nHeight >= consensusParams.DIP0001Height;
    bool fDIP0003Active_context = nHeight >= consensusParams.DIP0003Height;

    // Size limits
    unsigned int nMaxBlockSize = MaxBlockSize(fDIP0001Active_context);
    if (block.vtx.empty() || block.vtx.size() > nMaxBlockSize || ::GetSerializeSize(block, PROTOCOL_VERSION) > nMaxBlockSize)
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-blk-length", "size limits failed");

    // Check that all transactions are finalized and not over-sized
    // Also count sigops
    unsigned int nSigOps = 0;
    for (const auto& tx : block.vtx) {
        if (!IsFinalTx(*tx, nHeight, nLockTimeCutoff)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-txns-nonfinal", "non-final transaction");
        }
        TxValidationState tx_state;
        if (!ContextualCheckTransaction(*tx, tx_state, consensusParams, pindexPrev)) {
            // ContextCheckTransaction() does validation checks than only should
            // fails as consensus failures.
            assert(tx_state.GetResult() == TxValidationResult::TX_CONSENSUS);
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, tx_state.GetRejectReason(),
                                 strprintf("Transaction check failed (tx hash %s) %s", tx->GetHash().ToString(), tx_state.GetDebugMessage()));
        }
        nSigOps += GetLegacySigOpCount(*tx);
    }

    // Check sigops
    if (nSigOps > MaxBlockSigOps(fDIP0001Active_context))
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-blk-sigops", "out-of-bounds SigOpCount");

    // Enforce rule that the coinbase starts with serialized block height
    // After DIP3/DIP4 activation, we don't enforce the height in the input script anymore.
    // The CbTx special transaction payload will then contain the height, which is checked in CheckCbTx
    if (nHeight >= consensusParams.BIP34Height && !fDIP0003Active_context)
    {
        CScript expect = CScript() << nHeight;
        if (block.vtx[0]->vin[0].scriptSig.size() < expect.size() ||
            !std::equal(expect.begin(), expect.end(), block.vtx[0]->vin[0].scriptSig.begin())) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-height", "block height mismatch in coinbase");
        }
    }

    if (fDIP0003Active_context) {
        if (block.vtx[0]->nType != TRANSACTION_COINBASE) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-type", "coinbase is not a CbTx");
        }
    }

    return true;
}

bool BlockManager::AcceptBlockHeader(const CBlockHeader& block, BlockValidationState& state, const CChainParams& chainparams, CBlockIndex** ppindex)
{
    AssertLockHeld(cs_main);
    // Check for duplicate
    uint256 hash = block.GetHash();
    BlockMap::iterator miSelf = m_block_index.find(hash);
    CBlockIndex *pindex = nullptr;

    // TODO : ENABLE BLOCK CACHE IN SPECIFIC CASES
    if (hash != chainparams.GetConsensus().hashGenesisBlock) {
        if (miSelf != m_block_index.end()) {
            // Block header is already known.
            pindex = miSelf->second;
            if (ppindex)
                *ppindex = pindex;
            if (pindex->nStatus & BLOCK_FAILED_MASK) {
                LogPrintf("ERROR: %s: block %s is marked invalid\n", __func__, hash.ToString());
                return state.Invalid(BlockValidationResult::BLOCK_CACHED_INVALID, "duplicate");
            }
            if (pindex->nStatus & BLOCK_CONFLICT_CHAINLOCK) {
                LogPrintf("ERROR: %s: block %s is marked conflicting\n", __func__, hash.ToString());
                return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "duplicate");
            }
            return true;
        }

        if (!CheckBlockHeader(block, state, chainparams.GetConsensus())) {
            LogPrint(BCLog::VALIDATION, "%s: Consensus::CheckBlockHeader: %s, %s\n", __func__, hash.ToString(), state.ToString());
            return false;
        }

        // Get prev block index
        CBlockIndex* pindexPrev = nullptr;
        BlockMap::iterator mi = m_block_index.find(block.hashPrevBlock);
        if (mi == m_block_index.end()) {
            LogPrintf("ERROR: %s: prev block not found\n", __func__);
            return state.Invalid(BlockValidationResult::BLOCK_MISSING_PREV, "prev-blk-not-found");
        }
        pindexPrev = (*mi).second;
        assert(pindexPrev);

        if (pindexPrev->nStatus & BLOCK_FAILED_MASK) {
            LogPrintf("ERROR: %s: prev block invalid\n", __func__);
            return state.Invalid(BlockValidationResult::BLOCK_INVALID_PREV, "bad-prevblk");
        }

        if (pindexPrev->nStatus & BLOCK_CONFLICT_CHAINLOCK) {
            // it's ok-ish, the other node is probably missing the latest chainlock
            LogPrintf("ERROR: %s: prev block %s conflicts with chainlock\n", __func__, block.hashPrevBlock.ToString());
            return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "bad-prevblk-chainlock");
        }

        if (!ContextualCheckBlockHeader(block, state, *this, chainparams, pindexPrev, GetAdjustedTime()))
            return error("%s: Consensus::ContextualCheckBlockHeader: %s, %s", __func__, hash.ToString(), state.ToString());

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
                        setDirtyBlockIndex.insert(invalid_walk);
                        invalid_walk = invalid_walk->pprev;
                    }
                    LogPrintf("ERROR: %s: prev block invalid\n", __func__);
                    return state.Invalid(BlockValidationResult::BLOCK_INVALID_PREV, "bad-prevblk");
                }
            }
        }

        if (llmq::chainLocksHandler->HasConflictingChainLock(pindexPrev->nHeight + 1, hash)) {
            if (pindex == nullptr) {
                AddToBlockIndex(block, BLOCK_CONFLICT_CHAINLOCK);
            }
            LogPrintf("ERROR: %s: header %s conflicts with chainlock\n", __func__, hash.ToString());
            return state.Invalid(BlockValidationResult::BLOCK_CHAINLOCK, "bad-chainlock");
        }
    }
    if (pindex == nullptr)
        pindex = AddToBlockIndex(block);

    if (ppindex)
        *ppindex = pindex;

    // Notify external listeners about accepted block header
    GetMainSignals().AcceptedBlockHeader(pindex);

    return true;
}

// Exposed wrapper for AcceptBlockHeader
bool ChainstateManager::ProcessNewBlockHeaders(const std::vector<CBlockHeader>& headers, BlockValidationState& state, const CChainParams& chainparams, const CBlockIndex** ppindex)
{
    assert(std::addressof(::ChainstateActive()) == std::addressof(ActiveChainstate()));
    AssertLockNotHeld(cs_main);
    {
        LOCK(cs_main);
        for (const CBlockHeader& header : headers) {
            CBlockIndex *pindex = nullptr; // Use a temp pindex instead of ppindex to avoid a const_cast
            bool accepted = m_blockman.AcceptBlockHeader(
                header, state, chainparams, &pindex);
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
            const int64_t blocks_left{(GetTime() - last_accepted.GetBlockTime()) / chainparams.GetConsensus().nPowTargetSpacing};
            const double progress{100.0 * last_accepted.nHeight / (last_accepted.nHeight + blocks_left)};
            LogPrintf("Synchronizing blockheaders, height: %d (~%.2f%%)\n", last_accepted.nHeight, progress);
        }
    }
    return true;
}

/** Store block on disk. If dbp is non-nullptr, the file is known to already reside on disk */
bool CChainState::AcceptBlock(const std::shared_ptr<const CBlock>& pblock, BlockValidationState& state, CBlockIndex** ppindex, bool fRequested, const FlatFilePos* dbp, bool* fNewBlock)
{
    boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();

    const CBlock& block = *pblock;

    if (fNewBlock) *fNewBlock = false;
    AssertLockHeld(cs_main);

    CBlockIndex *pindexDummy = nullptr;
    CBlockIndex *&pindex = ppindex ? *ppindex : pindexDummy;

    bool accepted_header = m_blockman.AcceptBlockHeader(block, state, m_params, &pindex);
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
    bool fTooFarAhead = (pindex->nHeight > int(m_chain.Height() + MIN_BLOCKS_TO_KEEP));

    // TODO: Decouple this function from the block download logic by removing fRequested
    // This requires some new chain data structure to efficiently look up if a
    // block is in a chain leading to a candidate for best tip, despite not
    // being such a candidate itself.

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
        if (pindex->nChainWork < nMinimumChainWork) return true;
    }

    if (!CheckBlock(block, state, m_params.GetConsensus()) ||
        !ContextualCheckBlock(block, state, m_params.GetConsensus(), pindex->pprev)) {
        if (state.IsInvalid() && state.GetResult() != BlockValidationResult::BLOCK_MUTATED) {
            pindex->nStatus |= BLOCK_FAILED_VALID;
            setDirtyBlockIndex.insert(pindex);
        }
        return error("%s: %s", __func__, state.ToString());
    }

    // Header is valid/has work, merkle tree is good...RELAY NOW
    // (but if it does not build on our best tip, let the SendMessages loop relay it)
    if (!IsInitialBlockDownload() && m_chain.Tip() == pindex->pprev)
        GetMainSignals().NewPoWValidBlock(pindex, pblock);

    // Write block to history file
    if (fNewBlock) *fNewBlock = true;
    assert(std::addressof(::ChainActive()) == std::addressof(m_chain));
    try {
        FlatFilePos blockPos = SaveBlockToDisk(block, pindex->nHeight, m_chain, m_params, dbp);
        if (blockPos.IsNull()) {
            state.Error(strprintf("%s: Failed to find position to write new block to disk", __func__));
            return false;
        }
        ReceivedBlockTransactions(block, pindex, blockPos);
    } catch (const std::runtime_error& e) {
        return AbortNode(state, std::string("System error: ") + e.what());
    }

    if (::ChainstateActive().CanFlushToDisk()) {
        ::ChainstateActive().FlushStateToDisk(state, FlushStateMode::NONE);
    }

    CheckBlockIndex();

    boost::posix_time::ptime finish = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration diff = finish - start;
    statsClient.timing("AcceptBlock_us", diff.total_microseconds(), 1.0f);

    return true;
}

bool ChainstateManager::ProcessNewBlock(const CChainParams& chainparams, const std::shared_ptr<const CBlock> pblock, bool fForceProcessing, bool* fNewBlock)
{
    AssertLockNotHeld(cs_main);
    assert(std::addressof(::ChainstateActive()) == std::addressof(ActiveChainstate()));

    {
        CBlockIndex *pindex = nullptr;
        if (fNewBlock) *fNewBlock = false;
        BlockValidationState state;

        // CheckBlock() does not support multi-threaded block validation because CBlock::fChecked can cause data race.
        // Therefore, the following critical section must include the CheckBlock() call as well.
        LOCK(cs_main);

        // Skipping AcceptBlock() for CheckBlock() failures means that we will never mark a block as invalid if
        // CheckBlock() fails.  This is protective against consensus failure if there are any unknown forms of block
        // malleability that cause CheckBlock() to fail; see e.g. CVE-2012-2459 and
        // https://lists.linuxfoundation.org/pipermail/bitcoin-dev/2019-February/016697.html.  Because CheckBlock() is
        // not very expensive, the anti-DoS benefits of caching failure (of a definitely-invalid block) are not substantial.
        bool ret = CheckBlock(*pblock, state, chainparams.GetConsensus());
        if (ret) {
            // Store to disk
            ret = ActiveChainstate().AcceptBlock(pblock, state, &pindex, fForceProcessing, nullptr, fNewBlock);
        }
        if (!ret) {
            GetMainSignals().BlockChecked(*pblock, state);
            return error("%s: AcceptBlock FAILED: %s", __func__, state.ToString());
        }
    }

    NotifyHeaderTip(ActiveChainstate());

    BlockValidationState state; // Only used to report errors, not invalidity - ignore it
    if (!ActiveChainstate().ActivateBestChain(state, pblock))
        return error("%s: ActivateBestChain failed: %s", __func__, state.ToString());

    LogPrintf("%s : ACCEPTED\n", __func__);
    return true;
}

bool TestBlockValidity(BlockValidationState& state,
                       llmq::CChainLocksHandler& clhandler,
                       CEvoDB& evoDb,
                       const CChainParams& chainparams,
                       CChainState& chainstate,
                       const CBlock& block,
                       CBlockIndex* pindexPrev,
                       bool fCheckPOW,
                       bool fCheckMerkleRoot)
{
    AssertLockHeld(cs_main);
    assert(std::addressof(::ChainstateActive()) == std::addressof(chainstate));
    assert(pindexPrev && pindexPrev == chainstate.m_chain.Tip());

    auto bls_legacy_scheme = bls::bls_legacy_scheme.load();

    uint256 hash = block.GetHash();
    if (clhandler.HasConflictingChainLock(pindexPrev->nHeight + 1, hash)) {
        LogPrintf("ERROR: %s: conflicting with chainlock\n", __func__);
        return state.Invalid(BlockValidationResult::BLOCK_INVALID_PREV, "bad-chainlock");
    }

    CCoinsViewCache viewNew(&chainstate.CoinsTip());
    uint256 block_hash(block.GetHash());
    CBlockIndex indexDummy(block);
    indexDummy.pprev = pindexPrev;
    indexDummy.nHeight = pindexPrev->nHeight + 1;
    indexDummy.phashBlock = &block_hash;

    // begin tx and let it rollback
    auto dbTx = evoDb.BeginTransaction();

    // NOTE: CheckBlockHeader is called by CheckBlock
    assert(std::addressof(g_chainman.m_blockman) == std::addressof(chainstate.m_blockman));
    if (!ContextualCheckBlockHeader(block, state, chainstate.m_blockman, chainparams, pindexPrev, GetAdjustedTime()))
        return error("%s: Consensus::ContextualCheckBlockHeader: %s", __func__, state.ToString());
    if (!CheckBlock(block, state, chainparams.GetConsensus(), fCheckPOW, fCheckMerkleRoot))
        return error("%s: Consensus::CheckBlock: %s", __func__, state.ToString());
    if (!ContextualCheckBlock(block, state, chainparams.GetConsensus(), pindexPrev))
        return error("%s: Consensus::ContextualCheckBlock: %s", __func__, state.ToString());
    if (!chainstate.ConnectBlock(block, state, &indexDummy, viewNew, true))
        return false;

    assert(state.IsValid());

    // we could switch to another scheme while testing, switch back to the original one
    if (bls_legacy_scheme != bls::bls_legacy_scheme.load()) {
        bls::bls_legacy_scheme.store(bls_legacy_scheme);
        LogPrintf("%s: bls_legacy_scheme=%d\n", __func__, bls::bls_legacy_scheme.load());
    }

    return true;
}

/**
 * BLOCK PRUNING CODE
 */

/* Calculate the amount of disk space the block & undo files currently use */
uint64_t CalculateCurrentUsage()
{
    LOCK(cs_LastBlockFile);

    uint64_t retval = 0;
    for (const CBlockFileInfo &file : vinfoBlockFile) {
        retval += file.nSize + file.nUndoSize;
    }
    return retval;
}

void BlockManager::PruneOneBlockFile(const int fileNumber)
{
    AssertLockHeld(cs_main);
    LOCK(cs_LastBlockFile);

    for (const auto& entry : m_block_index) {
        CBlockIndex* pindex = entry.second;
        if (pindex->nFile == fileNumber) {
            pindex->nStatus &= ~BLOCK_HAVE_DATA;
            pindex->nStatus &= ~BLOCK_HAVE_UNDO;
            pindex->nFile = 0;
            pindex->nDataPos = 0;
            pindex->nUndoPos = 0;
            setDirtyBlockIndex.insert(pindex);

            // Prune from m_blocks_unlinked -- any block we prune would have
            // to be downloaded again in order to consider its chain, at which
            // point it would be considered as a candidate for
            // m_blocks_unlinked or setBlockIndexCandidates.
            auto range = m_blocks_unlinked.equal_range(pindex->pprev);
            while (range.first != range.second) {
                std::multimap<CBlockIndex *, CBlockIndex *>::iterator _it = range.first;
                range.first++;
                if (_it->second == pindex) {
                    m_blocks_unlinked.erase(_it);
                }
            }
        }
    }

    vinfoBlockFile[fileNumber].SetNull();
    setDirtyFileInfo.insert(fileNumber);
}


void UnlinkPrunedFiles(const std::set<int>& setFilesToPrune)
{
    for (std::set<int>::iterator it = setFilesToPrune.begin(); it != setFilesToPrune.end(); ++it) {
        FlatFilePos pos(*it, 0);
        fs::remove(BlockFileSeq().FileName(pos));
        fs::remove(UndoFileSeq().FileName(pos));
        LogPrintf("Prune: %s deleted blk/rev (%05u)\n", __func__, *it);
    }
}

void BlockManager::FindFilesToPruneManual(std::set<int>& setFilesToPrune, int nManualPruneHeight, int chain_tip_height)
{
    assert(fPruneMode && nManualPruneHeight > 0);

    LOCK2(cs_main, cs_LastBlockFile);
    if (chain_tip_height < 0) {
        return;
    }

    // last block to prune is the lesser of (user-specified height, MIN_BLOCKS_TO_KEEP from the tip)
    unsigned int nLastBlockWeCanPrune = std::min((unsigned)nManualPruneHeight, chain_tip_height - MIN_BLOCKS_TO_KEEP);
    int count = 0;
    for (int fileNumber = 0; fileNumber < nLastBlockFile; fileNumber++) {
        if (vinfoBlockFile[fileNumber].nSize == 0 || vinfoBlockFile[fileNumber].nHeightLast > nLastBlockWeCanPrune) {
            continue;
        }
        PruneOneBlockFile(fileNumber);
        setFilesToPrune.insert(fileNumber);
        count++;
    }
    LogPrintf("Prune (Manual): prune_height=%d removed %d blk/rev pairs\n", nLastBlockWeCanPrune, count);
}

/* This function is called from the RPC code for pruneblockchain */
void PruneBlockFilesManual(CChainState& active_chainstate, int nManualPruneHeight)
{
    BlockValidationState state;
    assert(std::addressof(::ChainstateActive()) == std::addressof(active_chainstate));
    if (!active_chainstate.FlushStateToDisk(state, FlushStateMode::NONE, nManualPruneHeight)) {
        LogPrintf("%s: failed to flush state (%s)\n", __func__, state.ToString());
    }
}

void BlockManager::FindFilesToPrune(std::set<int>& setFilesToPrune, uint64_t nPruneAfterHeight, int chain_tip_height, int prune_height, bool is_ibd)
{
    LOCK2(cs_main, cs_LastBlockFile);
    if (chain_tip_height < 0 || nPruneTarget == 0) {
        return;
    }
    if ((uint64_t)chain_tip_height <= nPruneAfterHeight) {
        return;
    }

    unsigned int nLastBlockWeCanPrune = std::min(prune_height, chain_tip_height - static_cast<int>(MIN_BLOCKS_TO_KEEP));
    uint64_t nCurrentUsage = CalculateCurrentUsage();
    // We don't check to prune until after we've allocated new space for files
    // So we should leave a buffer under our target to account for another allocation
    // before the next pruning.
    uint64_t nBuffer = BLOCKFILE_CHUNK_SIZE + UNDOFILE_CHUNK_SIZE;
    uint64_t nBytesToPrune;
    int count = 0;

    if (nCurrentUsage + nBuffer >= nPruneTarget) {
        // On a prune event, the chainstate DB is flushed.
        // To avoid excessive prune events negating the benefit of high dbcache
        // values, we should not prune too rapidly.
        // So when pruning in IBD, increase the buffer a bit to avoid a re-prune too soon.
        if (is_ibd) {
            // Since this is only relevant during IBD, we use a fixed 10%
            nBuffer += nPruneTarget / 10;
        }

        for (int fileNumber = 0; fileNumber < nLastBlockFile; fileNumber++) {
            nBytesToPrune = vinfoBlockFile[fileNumber].nSize + vinfoBlockFile[fileNumber].nUndoSize;

            if (vinfoBlockFile[fileNumber].nSize == 0) {
                continue;
            }

            if (nCurrentUsage + nBuffer < nPruneTarget) { // are we below our target?
                break;
            }

            // don't prune files that could have a block within MIN_BLOCKS_TO_KEEP of the main chain's tip but keep scanning
            if (vinfoBlockFile[fileNumber].nHeightLast > nLastBlockWeCanPrune) {
                continue;
            }

            PruneOneBlockFile(fileNumber);
            // Queue up the files for removal
            setFilesToPrune.insert(fileNumber);
            nCurrentUsage -= nBytesToPrune;
            count++;
        }
    }

    LogPrint(BCLog::PRUNE, "Prune: target=%dMiB actual=%dMiB diff=%dMiB max_prune_height=%d removed %d blk/rev pairs\n",
           nPruneTarget/1024/1024, nCurrentUsage/1024/1024,
           ((int64_t)nPruneTarget - (int64_t)nCurrentUsage)/1024/1024,
           nLastBlockWeCanPrune, count);
}

static FlatFileSeq BlockFileSeq()
{
    return FlatFileSeq(GetBlocksDir(), "blk", gArgs.GetBoolArg("-fastprune", false) ? 0x4000 /* 16kb */ : BLOCKFILE_CHUNK_SIZE);
}

static FlatFileSeq UndoFileSeq()
{
    return FlatFileSeq(GetBlocksDir(), "rev", UNDOFILE_CHUNK_SIZE);
}

FILE* OpenBlockFile(const FlatFilePos &pos, bool fReadOnly) {
    return BlockFileSeq().Open(pos, fReadOnly);
}

/** Open an undo file (rev?????.dat) */
static FILE* OpenUndoFile(const FlatFilePos &pos, bool fReadOnly) {
    return UndoFileSeq().Open(pos, fReadOnly);
}

fs::path GetBlockPosFilename(const FlatFilePos &pos)
{
    return BlockFileSeq().FileName(pos);
}

CBlockIndex * BlockManager::InsertBlockIndex(const uint256& hash)
{
    AssertLockHeld(cs_main);

    if (hash.IsNull())
        return nullptr;

    // Return existing
    BlockMap::iterator mi = m_block_index.find(hash);
    if (mi != m_block_index.end())
        return (*mi).second;

    // Create new
    CBlockIndex* pindexNew = new CBlockIndex();
    mi = m_block_index.insert(std::make_pair(hash, pindexNew)).first;
    pindexNew->phashBlock = &((*mi).first);

    return pindexNew;
}

bool BlockManager::LoadBlockIndex(
    const Consensus::Params& consensus_params,
    CBlockTreeDB& blocktree,
    std::set<CBlockIndex*, CBlockIndexWorkComparator>& block_index_candidates)
{
    if (!blocktree.LoadBlockIndexGuts(consensus_params, [this](const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main) { return this->InsertBlockIndex(hash); }))
        return false;

    // Calculate nChainWork
    std::vector<std::pair<int, CBlockIndex*> > vSortedByHeight;
    vSortedByHeight.reserve(m_block_index.size());
    for (const std::pair<const uint256, CBlockIndex*>& item : m_block_index)
    {
        CBlockIndex* pindex = item.second;
        vSortedByHeight.push_back(std::make_pair(pindex->nHeight, pindex));

        // build m_blockman.m_prev_block_index
        if (pindex->pprev) {
            m_prev_block_index.emplace(pindex->pprev->GetBlockHash(), pindex);
        }
    }
    sort(vSortedByHeight.begin(), vSortedByHeight.end());
    for (const std::pair<int, CBlockIndex*>& item : vSortedByHeight)
    {
        if (ShutdownRequested()) return false;
        CBlockIndex* pindex = item.second;
        pindex->nChainWork = (pindex->pprev ? pindex->pprev->nChainWork : 0) + GetBlockProof(*pindex);
        pindex->nTimeMax = (pindex->pprev ? std::max(pindex->pprev->nTimeMax, pindex->nTime) : pindex->nTime);
        // We can link the chain of blocks for which we've received transactions at some point.
        // Pruned nodes may have deleted the block.
        if (pindex->nTx > 0) {
            if (pindex->pprev) {
                if (pindex->pprev->HaveTxsDownloaded()) {
                    pindex->nChainTx = pindex->pprev->nChainTx + pindex->nTx;
                } else {
                    pindex->nChainTx = 0;
                    m_blocks_unlinked.insert(std::make_pair(pindex->pprev, pindex));
                }
            } else {
                pindex->nChainTx = pindex->nTx;
            }
        }
        if (!(pindex->nStatus & BLOCK_FAILED_MASK) && pindex->pprev && (pindex->pprev->nStatus & BLOCK_FAILED_MASK)) {
            pindex->nStatus |= BLOCK_FAILED_CHILD;
            setDirtyBlockIndex.insert(pindex);
        }
        if (pindex->IsValid(BLOCK_VALID_TRANSACTIONS) && (pindex->HaveTxsDownloaded() || pindex->pprev == nullptr)) {
            block_index_candidates.insert(pindex);
        }
        if (pindex->nStatus & BLOCK_FAILED_MASK && (!pindexBestInvalid || pindex->nChainWork > pindexBestInvalid->nChainWork))
            pindexBestInvalid = pindex;
        if (pindex->pprev)
            pindex->BuildSkip();
        if (pindex->IsValid(BLOCK_VALID_TREE) && (pindexBestHeader == nullptr || CBlockIndexWorkComparator()(pindexBestHeader, pindex)))
            pindexBestHeader = pindex;
    }

    return true;
}

void BlockManager::Unload() {
    m_failed_blocks.clear();
    m_blocks_unlinked.clear();

    for (const BlockMap::value_type& entry : m_block_index) {
        delete entry.second;
    }

    m_block_index.clear();
    m_prev_block_index.clear();
}

bool CChainState::LoadBlockIndexDB()
{
    assert(std::addressof(::ChainstateActive()) == std::addressof(*this));
    if (!m_blockman.LoadBlockIndex(
            m_params.GetConsensus(), *pblocktree,
            setBlockIndexCandidates)) {
        return false;
    }

    // Load block file info
    pblocktree->ReadLastBlockFile(nLastBlockFile);
    vinfoBlockFile.resize(nLastBlockFile + 1);
    LogPrintf("%s: last block file = %i\n", __func__, nLastBlockFile);
    for (int nFile = 0; nFile <= nLastBlockFile; nFile++) {
        pblocktree->ReadBlockFileInfo(nFile, vinfoBlockFile[nFile]);
    }
    LogPrintf("%s: last block file info: %s\n", __func__, vinfoBlockFile[nLastBlockFile].ToString());
    for (int nFile = nLastBlockFile + 1; true; nFile++) {
        CBlockFileInfo info;
        if (pblocktree->ReadBlockFileInfo(nFile, info)) {
            vinfoBlockFile.push_back(info);
        } else {
            break;
        }
    }

    // Check presence of blk files
    LogPrintf("Checking all blk files are present...\n");
    std::set<int> setBlkDataFiles;
    for (const std::pair<const uint256, CBlockIndex*>& item : m_blockman.m_block_index) {
        CBlockIndex* pindex = item.second;
        if (pindex->nStatus & BLOCK_HAVE_DATA) {
            setBlkDataFiles.insert(pindex->nFile);
        }
    }
    for (std::set<int>::iterator it = setBlkDataFiles.begin(); it != setBlkDataFiles.end(); it++)
    {
        FlatFilePos pos(*it, 0);
        if (CAutoFile(OpenBlockFile(pos, true), SER_DISK, CLIENT_VERSION).IsNull()) {
            return false;
        }
    }

    // Check whether we have ever pruned block & undo files
    pblocktree->ReadFlag("prunedblockfiles", fHavePruned);
    if (fHavePruned)
        LogPrintf("LoadBlockIndexDB(): Block files have previously been pruned\n");

    // Check whether we need to continue reindexing
    bool fReindexing = false;
    pblocktree->ReadReindexing(fReindexing);
    if(fReindexing) fReindex = true;

    // Check whether we have an address index
    pblocktree->ReadFlag("addressindex", fAddressIndex);
    LogPrintf("%s: address index %s\n", __func__, fAddressIndex ? "enabled" : "disabled");

    // Check whether we have a timestamp index
    pblocktree->ReadFlag("timestampindex", fTimestampIndex);
    LogPrintf("%s: timestamp index %s\n", __func__, fTimestampIndex ? "enabled" : "disabled");

    // Check whether we have a spent index
    pblocktree->ReadFlag("spentindex", fSpentIndex);
    LogPrintf("%s: spent index %s\n", __func__, fSpentIndex ? "enabled" : "disabled");

    return true;
}

void CChainState::LoadMempool(const ArgsManager& args)
{
    if (!m_mempool) return;
    if (args.GetBoolArg("-persistmempool", DEFAULT_PERSIST_MEMPOOL)) {
        assert(std::addressof(::ChainstateActive()) == std::addressof(*this));
        ::LoadMempool(*m_mempool, *this);
    }
    m_mempool->SetIsLoaded(!ShutdownRequested());
}

bool CChainState::LoadChainTip()
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
    m_chain.SetTip(pindex);
    PruneBlockIndexCandidates();

    tip = m_chain.Tip();
    LogPrintf("Loaded best chain: hashBestChain=%s height=%d date=%s progress=%f\n",
              tip->GetBlockHash().ToString(),
              m_chain.Height(),
              FormatISO8601DateTime(tip->GetBlockTime()),
              GuessVerificationProgress(m_params.TxData(), tip));
    return true;
}

CVerifyDB::CVerifyDB()
{
    uiInterface.ShowProgress(_("Verifying blocks...").translated, 0, false);
}

CVerifyDB::~CVerifyDB()
{
    uiInterface.ShowProgress("", 100, false);
}

bool CVerifyDB::VerifyDB(
    CChainState& chainstate,
    const CChainParams& chainparams,
    CCoinsView& coinsview,
    CEvoDB& evoDb,
    int nCheckLevel, int nCheckDepth)
{
    AssertLockHeld(cs_main);

    assert(std::addressof(::ChainstateActive()) == std::addressof(chainstate));
    if (chainstate.m_chain.Tip() == nullptr || chainstate.m_chain.Tip()->pprev == nullptr)
        return true;

    // begin tx and let it rollback
    auto dbTx = evoDb.BeginTransaction();

    // Verify blocks in the best chain
    if (nCheckDepth <= 0 || nCheckDepth > chainstate.m_chain.Height())
        nCheckDepth = chainstate.m_chain.Height();
    nCheckLevel = std::max(0, std::min(4, nCheckLevel));
    LogPrintf("Verifying last %i blocks at level %i\n", nCheckDepth, nCheckLevel);
    CCoinsViewCache coins(&coinsview);
    CBlockIndex* pindex;
    CBlockIndex* pindexFailure = nullptr;
    int nGoodTransactions = 0;
    BlockValidationState state;
    int reportDone = 0;
    LogPrintf("[0%%]..."); /* Continued */

    const bool is_snapshot_cs{!chainstate.m_from_snapshot_blockhash};

    for (pindex = chainstate.m_chain.Tip(); pindex && pindex->pprev; pindex = pindex->pprev) {
        const int percentageDone = std::max(1, std::min(99, (int)(((double)(chainstate.m_chain.Height() - pindex->nHeight)) / (double)nCheckDepth * (nCheckLevel >= 4 ? 50 : 100))));
        if (reportDone < percentageDone/10) {
            // report every 10% step
            LogPrintf("[%d%%]...", percentageDone); /* Continued */
            reportDone = percentageDone/10;
        }
        uiInterface.ShowProgress(_("Verifying blocks...").translated, percentageDone, false);
        if (pindex->nHeight <= chainstate.m_chain.Height()-nCheckDepth)
            break;
        if ((fPruneMode || is_snapshot_cs) && !(pindex->nStatus & BLOCK_HAVE_DATA)) {
            // If pruning or running under an assumeutxo snapshot, only go
            // back as far as we have data.
            LogPrintf("VerifyDB(): block verification stopping at height %d (pruning, no data)\n", pindex->nHeight);
            break;
        }
        CBlock block;
        // check level 0: read from disk
        if (!ReadBlockFromDisk(block, pindex, chainparams.GetConsensus()))
            return error("VerifyDB(): *** ReadBlockFromDisk failed at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
        // check level 1: verify block validity
        if (nCheckLevel >= 1 && !CheckBlock(block, state, chainparams.GetConsensus()))
            return error("%s: *** found bad block at %d, hash=%s (%s)\n", __func__,
                         pindex->nHeight, pindex->GetBlockHash().ToString(), state.ToString());
        // check level 2: verify undo validity
        if (nCheckLevel >= 2 && pindex) {
            CBlockUndo undo;
            if (!pindex->GetUndoPos().IsNull()) {
                if (!UndoReadFromDisk(undo, pindex)) {
                    return error("VerifyDB(): *** found bad undo data at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                }
            }
        }
        // check level 3: check for inconsistencies during memory-only disconnect of tip blocks
        size_t curr_coins_usage = coins.DynamicMemoryUsage() + chainstate.CoinsTip().DynamicMemoryUsage();

        if (nCheckLevel >= 3 && curr_coins_usage <= chainstate.m_coinstip_cache_size_bytes) {
            assert(coins.GetBestBlock() == pindex->GetBlockHash());
            DisconnectResult res = chainstate.DisconnectBlock(block, pindex, coins);
            if (res == DISCONNECT_FAILED) {
                return error("VerifyDB(): *** irrecoverable inconsistency in block data at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
            }
            if (res == DISCONNECT_UNCLEAN) {
                nGoodTransactions = 0;
                pindexFailure = pindex;
            } else {
                nGoodTransactions += block.vtx.size();
            }
        }
        if (ShutdownRequested()) return true;
    }
    if (pindexFailure)
        return error("VerifyDB(): *** coin database inconsistencies found (last %i blocks, %i good transactions before that)\n", chainstate.m_chain.Height() - pindexFailure->nHeight + 1, nGoodTransactions);

    // store block count as we move pindex at check level >= 4
    int block_count = chainstate.m_chain.Height() - pindex->nHeight;

    // check level 4: try reconnecting blocks
    if (nCheckLevel >= 4) {
        while (pindex != chainstate.m_chain.Tip()) {
            const int percentageDone = std::max(1, std::min(99, 100 - (int)(((double)(chainstate.m_chain.Height() - pindex->nHeight)) / (double)nCheckDepth * 50)));
            if (reportDone < percentageDone/10) {
                // report every 10% step
                LogPrintf("[%d%%]...", percentageDone); /* Continued */
                reportDone = percentageDone/10;
            }
            uiInterface.ShowProgress(_("Verifying blocks...").translated, percentageDone, false);
            pindex = chainstate.m_chain.Next(pindex);
            CBlock block;
            if (!ReadBlockFromDisk(block, pindex, chainparams.GetConsensus()))
                return error("VerifyDB(): *** ReadBlockFromDisk failed at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
            if (!chainstate.ConnectBlock(block, state, pindex, coins))
                return error("VerifyDB(): *** found unconnectable block at %d, hash=%s (%s)", pindex->nHeight, pindex->GetBlockHash().ToString(), state.ToString());
            if (ShutdownRequested()) return true;
        }
    }

    LogPrintf("[DONE].\n");
    LogPrintf("No coin database inconsistencies in last %i blocks (%i transactions)\n", block_count, nGoodTransactions);

    return true;
}

/** Apply the effects of a block on the utxo cache, ignoring that it may already have been applied. */
bool CChainState::RollforwardBlock(const CBlockIndex* pindex, CCoinsViewCache& inputs)
{
    assert(m_quorum_block_processor);

    // TODO: merge with ConnectBlock
    CBlock block;
    if (!ReadBlockFromDisk(block, pindex, m_params.GetConsensus())) {
        return error("RollforwardBlock(): ReadBlockFromDisk failed at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
    }

    // MUST process special txes before updating UTXO to ensure consistency between mempool and block processing
    BlockValidationState state;
    if (!ProcessSpecialTxsInBlock(block, pindex, *m_quorum_block_processor, *m_clhandler, m_params.GetConsensus(), inputs, false /*fJustCheck*/, false /*fScriptChecks*/, state)) {
        return error("RollforwardBlock(DASH): ProcessSpecialTxsInBlock for block %s failed with %s",
            pindex->GetBlockHash().ToString(), state.ToString());
    }

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressUnspentIndex;
    std::vector<std::pair<CSpentIndexKey, CSpentIndexValue> > spentIndex;

    for (size_t i = 0; i < block.vtx.size(); i++) {
        const CTransactionRef& tx = block.vtx[i];
        const uint256 txhash = tx->GetHash();

        if (!tx->IsCoinBase()) {
            // Update indexes
            if (fAddressIndex || fSpentIndex) {
                for (size_t j = 0; j < tx->vin.size(); j++) {
                    const CTxIn input = tx->vin[j];
                    const Coin& coin = inputs.AccessCoin(tx->vin[j].prevout);
                    const CTxOut& prevout = coin.out;
                    uint160 hashBytes;
                    int addressType;

                    if (prevout.scriptPubKey.IsPayToScriptHash()) {
                        hashBytes = uint160(std::vector<unsigned char>(prevout.scriptPubKey.begin()+2, prevout.scriptPubKey.begin()+22));
                        addressType = 2;
                    } else if (prevout.scriptPubKey.IsPayToPublicKeyHash()) {
                        hashBytes = uint160(std::vector<unsigned char>(prevout.scriptPubKey.begin()+3, prevout.scriptPubKey.begin()+23));
                        addressType = 1;
                    } else if (prevout.scriptPubKey.IsPayToPublicKey()) {
                        hashBytes = Hash160(Span{prevout.scriptPubKey.data()+1, prevout.scriptPubKey.size()-2});
                        addressType = 1;
                    } else {
                        hashBytes.SetNull();
                        addressType = 0;
                    }

                    if (fAddressIndex && addressType > 0) {
                        // record spending activity
                        addressIndex.push_back(std::make_pair(CAddressIndexKey(addressType, hashBytes, pindex->nHeight, i, txhash, j, true), prevout.nValue * -1));

                        // remove address from unspent index
                        addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(addressType, hashBytes, input.prevout.hash, input.prevout.n), CAddressUnspentValue()));
                    }

                    if (fSpentIndex) {
                        // add the spent index to determine the txid and input that spent an output
                        // and to find the amount and address from an input
                        spentIndex.push_back(std::make_pair(CSpentIndexKey(input.prevout.hash, input.prevout.n), CSpentIndexValue(txhash, j, pindex->nHeight, prevout.nValue, addressType, hashBytes)));
                    }
                }
            }

            if (fAddressIndex) {
                for (size_t k = 0; k < tx->vout.size(); k++) {
                    const CTxOut& out = tx->vout[k];

                    if (out.scriptPubKey.IsPayToScriptHash()) {
                        std::vector<unsigned char> hashBytes(out.scriptPubKey.begin()+2, out.scriptPubKey.begin()+22);

                        // record receiving activity
                        addressIndex.push_back(std::make_pair(CAddressIndexKey(2, uint160(hashBytes), pindex->nHeight, i, txhash, k, false), out.nValue));

                        // record unspent output
                        addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(2, uint160(hashBytes), txhash, k), CAddressUnspentValue(out.nValue, out.scriptPubKey, pindex->nHeight)));

                    } else if (out.scriptPubKey.IsPayToPublicKeyHash()) {
                        std::vector<unsigned char> hashBytes(out.scriptPubKey.begin()+3, out.scriptPubKey.begin()+23);

                        // record receiving activity
                        addressIndex.push_back(std::make_pair(CAddressIndexKey(1, uint160(hashBytes), pindex->nHeight, i, txhash, k, false), out.nValue));

                        // record unspent output
                        addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(1, uint160(hashBytes), txhash, k), CAddressUnspentValue(out.nValue, out.scriptPubKey, pindex->nHeight)));

                    } else if (out.scriptPubKey.IsPayToPublicKey()) {
                        uint160 hashBytes{Hash160(Span{out.scriptPubKey.data()+1, out.scriptPubKey.size()-2})};
                        addressIndex.push_back(std::make_pair(CAddressIndexKey(1, hashBytes, pindex->nHeight, i, txhash, k, false), out.nValue));
                        addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(1, hashBytes, txhash, k), CAddressUnspentValue(out.nValue, out.scriptPubKey, pindex->nHeight)));
                    } else {
                        continue;
                    }
                }
            }

            for (const CTxIn &txin : tx->vin) {
                inputs.SpendCoin(txin.prevout);
            }
        }
        // Pass check = true as every addition may be an overwrite.
        AddCoins(inputs, *tx, pindex->nHeight, true);
    }

    if (fAddressIndex) {
        if (!pblocktree->WriteAddressIndex(addressIndex)) {
            return error("RollforwardBlock(DASH): Failed to write address index");
        }

        if (!pblocktree->UpdateAddressUnspentIndex(addressUnspentIndex)) {
            return error("RollforwardBlock(DASH): Failed to write address unspent index");
        }
    }

    if (fSpentIndex) {
        if (!pblocktree->UpdateSpentIndex(spentIndex))
            return error("RollforwardBlock(DASH): Failed to write transaction index");
    }

    if (fTimestampIndex) {
        if (!pblocktree->WriteTimestampIndex(CTimestampIndexKey(pindex->nTime, pindex->GetBlockHash())))
            return error("RollforwardBlock(DASH): Failed to write timestamp index");
    }

    return true;
}

bool CChainState::ReplayBlocks()
{
    LOCK(cs_main);

    CCoinsView& db = this->CoinsDB();
    CCoinsViewCache cache(&db);

    std::vector<uint256> hashHeads = db.GetHeadBlocks();
    if (hashHeads.empty()) return true; // We're already in a consistent state.
    if (hashHeads.size() != 2) return error("ReplayBlocks(): unknown inconsistent state");

    uiInterface.ShowProgress(_("Replaying blocks...").translated, 0, false);
    LogPrintf("Replaying blocks\n");

    const CBlockIndex* pindexOld = nullptr;  // Old tip during the interrupted flush.
    const CBlockIndex* pindexNew;            // New tip during the interrupted flush.
    const CBlockIndex* pindexFork = nullptr; // Latest block common to both the old and the new tip.

    if (m_blockman.m_block_index.count(hashHeads[0]) == 0) {
        return error("ReplayBlocks(): reorganization to unknown block requested");
    }
    pindexNew = m_blockman.m_block_index[hashHeads[0]];

    if (!hashHeads[1].IsNull()) { // The old tip is allowed to be 0, indicating it's the first flush.
        if (m_blockman.m_block_index.count(hashHeads[1]) == 0) {
            return error("ReplayBlocks(): reorganization from unknown block requested");
        }
        pindexOld = m_blockman.m_block_index[hashHeads[1]];
        pindexFork = LastCommonAncestor(pindexOld, pindexNew);
        assert(pindexFork != nullptr);
        const bool fDIP0003Active = pindexOld->nHeight >= m_params.GetConsensus().DIP0003Height;
        if (fDIP0003Active && !m_evoDb.VerifyBestBlock(pindexOld->GetBlockHash())) {
            return error("ReplayBlocks(DASH): Found EvoDB inconsistency");
        }
    }

    auto dbTx = m_evoDb.BeginTransaction();

    // Rollback along the old branch.
    while (pindexOld != pindexFork) {
        if (pindexOld->nHeight > 0) { // Never disconnect the genesis block.
            CBlock block;
            if (!ReadBlockFromDisk(block, pindexOld, m_params.GetConsensus())) {
                return error("ReplayBlocks(): ReadBlockFromDisk() failed at %d, hash=%s", pindexOld->nHeight, pindexOld->GetBlockHash().ToString());
            }
            LogPrintf("Rolling back %s (%i)\n", pindexOld->GetBlockHash().ToString(), pindexOld->nHeight);
            DisconnectResult res = DisconnectBlock(block, pindexOld, cache);
            if (res == DISCONNECT_FAILED) {
                return error("ReplayBlocks(): DisconnectBlock failed at %d, hash=%s", pindexOld->nHeight, pindexOld->GetBlockHash().ToString());
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
        const CBlockIndex* pindex = pindexNew->GetAncestor(nHeight);
        LogPrintf("Rolling forward %s (%i)\n", pindex->GetBlockHash().ToString(), nHeight);
        uiInterface.ShowProgress(_("Replaying blocks...").translated, (int) ((nHeight - nForkHeight) * 100.0 / (pindexNew->nHeight - nForkHeight)) , false);
        if (!RollforwardBlock(pindex, cache)) return false;
    }

    cache.SetBestBlock(pindexNew->GetBlockHash());
    m_evoDb.WriteBestBlock(pindexNew->GetBlockHash());
    bool flushed = cache.Flush();
    assert(flushed);
    dbTx->Commit();
    uiInterface.ShowProgress("", 100, false);
    return true;
}

void CChainState::UnloadBlockIndex() {
    nBlockSequenceId = 1;
    setBlockIndexCandidates.clear();
}

// May NOT be used after any connections are up as much
// of the peer-processing logic assumes a consistent
// block index state
void UnloadBlockIndex(CTxMemPool* mempool, ChainstateManager& chainman)
{
    LOCK(cs_main);
    chainman.Unload();
    pindexBestInvalid = nullptr;
    pindexBestHeader = nullptr;
    if (mempool) mempool->clear();
    vinfoBlockFile.clear();
    nLastBlockFile = 0;
    setDirtyBlockIndex.clear();
    setDirtyFileInfo.clear();
    versionbitscache.Clear();
    for (int b = 0; b < VERSIONBITS_NUM_BITS; b++) {
        warningcache[b].clear();
    }
    fHavePruned = false;
}

bool ChainstateManager::LoadBlockIndex()
{
    AssertLockHeld(cs_main);
    // Load block index from databases
    bool needs_init = fReindex;
    if (!fReindex) {
        bool ret = ActiveChainstate().LoadBlockIndexDB();
        if (!ret) return false;
        needs_init = m_blockman.m_block_index.empty();
    }

    if (needs_init) {
        // Everything here is for *new* reindex/DBs. Thus, though
        // LoadBlockIndexDB may have set fReindex if we shut down
        // mid-reindex previously, we don't check fReindex and
        // instead only check it prior to LoadBlockIndexDB to set
        // needs_init.

        LogPrintf("Initializing databases...\n");

        // Use the provided setting for -addressindex in the new database
        fAddressIndex = gArgs.GetBoolArg("-addressindex", DEFAULT_ADDRESSINDEX);
        pblocktree->WriteFlag("addressindex", fAddressIndex);

        // Use the provided setting for -timestampindex in the new database
        fTimestampIndex = gArgs.GetBoolArg("-timestampindex", DEFAULT_TIMESTAMPINDEX);
        pblocktree->WriteFlag("timestampindex", fTimestampIndex);

        // Use the provided setting for -spentindex in the new database
        fSpentIndex = gArgs.GetBoolArg("-spentindex", DEFAULT_SPENTINDEX);
        pblocktree->WriteFlag("spentindex", fSpentIndex);
    }
    return true;
}

bool CChainState::AddGenesisBlock(const CBlock& block, BlockValidationState& state)
{
    assert(std::addressof(::ChainActive()) == std::addressof(m_chain));
    FlatFilePos blockPos = SaveBlockToDisk(block, 0, m_chain, m_params, nullptr);
    if (blockPos.IsNull())
        return error("%s: writing genesis block to disk failed (%s)", __func__, state.ToString());
    CBlockIndex* pindex = m_blockman.AddToBlockIndex(block);
    ReceivedBlockTransactions(block, pindex, blockPos);
    return true;
}

bool CChainState::LoadGenesisBlock()
{
    LOCK(cs_main);

    // Check whether we're already initialized by checking for genesis in
    // m_blockman.m_block_index. Note that we can't use m_chain here, since it is
    // set based on the coins db, not the block index db, which is the only
    // thing loaded at this point.
    if (m_blockman.m_block_index.count(m_params.GenesisBlock().GetHash()))
        return true;

    try {
        BlockValidationState state;

        if (!AddGenesisBlock(m_params.GenesisBlock(), state))
            return false;

        if (m_params.NetworkIDString() == CBaseChainParams::DEVNET) {
            // We can't continue if devnet genesis block is invalid
            std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(
                    m_params.DevNetGenesisBlock());
            bool fCheckBlock = CheckBlock(*shared_pblock, state, m_params.GetConsensus());
            assert(fCheckBlock);
            if (!AcceptBlock(shared_pblock, state, nullptr, true, nullptr, nullptr))
                return false;
        }
    } catch (const std::runtime_error &e) {
        return error("%s: failed to initialize block database: %s", __func__, e.what());
    }

    return true;
}

void CChainState::LoadExternalBlockFile(FILE* fileIn, FlatFilePos* dbp)
{
    // Map of disk positions for blocks with unknown parent (only used for reindex)
    static std::multimap<uint256, FlatFilePos> mapBlocksUnknownParent;
    int64_t nStart = GetTimeMillis();

    int nLoaded = 0;
    try {
        unsigned int nMaxBlockSize = MaxBlockSize();
        // This takes over fileIn and calls fclose() on it in the CBufferedFile destructor
        CBufferedFile blkdat(fileIn, 2*nMaxBlockSize, nMaxBlockSize+8, SER_DISK, CLIENT_VERSION);
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
                blkdat.FindByte(m_params.MessageStart()[0]);
                nRewind = blkdat.GetPos()+1;
                blkdat >> buf;
                if (memcmp(buf, m_params.MessageStart(), CMessageHeader::MESSAGE_START_SIZE)) {
                    continue;
                }
                // read size
                blkdat >> nSize;
                if (nSize < 80 || nSize > nMaxBlockSize)
                    continue;
            } catch (const std::exception&) {
                // no valid block header found; don't complain
                break;
            }
            try {
                // read block
                uint64_t nBlockPos = blkdat.GetPos();
                if (dbp)
                    dbp->nPos = nBlockPos;
                blkdat.SetLimit(nBlockPos + nSize);
                std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
                CBlock& block = *pblock;
                blkdat >> block;
                nRewind = blkdat.GetPos();

                uint256 hash = block.GetHash();
                {
                    LOCK(cs_main);
                    // detect out of order blocks, and store them for later
                    assert(std::addressof(g_chainman.m_blockman) == std::addressof(m_blockman));
                    if (hash != m_params.GetConsensus().hashGenesisBlock && !m_blockman.LookupBlockIndex(block.hashPrevBlock)) {
                        LogPrint(BCLog::REINDEX, "%s: Out of order block %s, parent %s not known\n", __func__, hash.ToString(),
                                block.hashPrevBlock.ToString());
                        if (dbp)
                            mapBlocksUnknownParent.insert(std::make_pair(block.hashPrevBlock, *dbp));
                        continue;
                    }

                    // process in case the block isn't known yet
                    assert(std::addressof(g_chainman.m_blockman) == std::addressof(m_blockman));
                    CBlockIndex* pindex = m_blockman.LookupBlockIndex(hash);
                    if (!pindex || (pindex->nStatus & BLOCK_HAVE_DATA) == 0) {
                      BlockValidationState state;
                      assert(std::addressof(::ChainstateActive()) == std::addressof(*this));
                      if (AcceptBlock(pblock, state, nullptr, true, dbp, nullptr)) {
                          nLoaded++;
                      }
                      if (state.IsError()) {
                          break;
                      }
                    } else if (hash != m_params.GetConsensus().hashGenesisBlock && pindex->nHeight % 1000 == 0) {
                        LogPrint(BCLog::REINDEX, "Block Import: already had block %s at height %d\n", hash.ToString(), pindex->nHeight);
                    }
                }

                // Activate the genesis block so normal node progress can continue
                if (hash == m_params.GetConsensus().hashGenesisBlock) {
                    assert(std::addressof(::ChainstateActive()) == std::addressof(*this));
                    BlockValidationState state;
                    if (!ActivateBestChain(state, nullptr)) {
                        break;
                    }
                }

                assert(std::addressof(::ChainstateActive()) == std::addressof(*this));
                NotifyHeaderTip(*this);

                // Recursively process earlier encountered successors of this block
                std::deque<uint256> queue;
                queue.push_back(hash);
                while (!queue.empty()) {
                    uint256 head = queue.front();
                    queue.pop_front();
                    std::pair<std::multimap<uint256, FlatFilePos>::iterator, std::multimap<uint256, FlatFilePos>::iterator> range = mapBlocksUnknownParent.equal_range(head);
                    while (range.first != range.second) {
                        std::multimap<uint256, FlatFilePos>::iterator it = range.first;
                        std::shared_ptr<CBlock> pblockrecursive = std::make_shared<CBlock>();
                        if (ReadBlockFromDisk(*pblockrecursive, it->second, m_params.GetConsensus())) {
                            LogPrint(BCLog::REINDEX, "%s: Processing out of order child %s of %s\n", __func__, pblockrecursive->GetHash().ToString(),
                                    head.ToString());
                            LOCK(cs_main);
                            assert(std::addressof(::ChainstateActive()) == std::addressof(*this));
                            BlockValidationState dummy;
                            if (AcceptBlock(pblockrecursive, dummy, nullptr, true, &it->second, nullptr)) {
                                nLoaded++;
                                queue.push_back(pblockrecursive->GetHash());
                            }
                        }
                        range.first++;
                        mapBlocksUnknownParent.erase(it);
                        assert(std::addressof(::ChainstateActive()) == std::addressof(*this));
                        NotifyHeaderTip(*this);
                    }
                }
            } catch (const std::exception& e) {
                LogPrintf("%s: Deserialize or I/O error - %s\n", __func__, e.what());
            }
        }
    } catch (const std::runtime_error& e) {
        AbortNode(std::string("System error: ") + e.what());
    }
    LogPrintf("Loaded %i blocks from external file in %dms\n", nLoaded, GetTimeMillis() - nStart);
}

void CChainState::CheckBlockIndex()
{
    if (!fCheckBlockIndex) {
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
    for (const std::pair<const uint256, CBlockIndex*>& entry : m_blockman.m_block_index) {
        forward.insert(std::make_pair(entry.second->pprev, entry.second));
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
    CBlockIndex* pindexFirstConflicing = nullptr; // Oldest ancestor of pindex which has BLOCK_CONFLICT_CHAINLOCK.
    CBlockIndex* pindexFirstMissing = nullptr; // Oldest ancestor of pindex which does not have BLOCK_HAVE_DATA.
    CBlockIndex* pindexFirstNeverProcessed = nullptr; // Oldest ancestor of pindex for which nTx == 0.
    CBlockIndex* pindexFirstNotTreeValid = nullptr; // Oldest ancestor of pindex which does not have BLOCK_VALID_TREE (regardless of being valid or not).
    CBlockIndex* pindexFirstNotTransactionsValid = nullptr; // Oldest ancestor of pindex which does not have BLOCK_VALID_TRANSACTIONS (regardless of being valid or not).
    CBlockIndex* pindexFirstNotChainValid = nullptr; // Oldest ancestor of pindex which does not have BLOCK_VALID_CHAIN (regardless of being valid or not).
    CBlockIndex* pindexFirstNotScriptsValid = nullptr; // Oldest ancestor of pindex which does not have BLOCK_VALID_SCRIPTS (regardless of being valid or not).
    while (pindex != nullptr) {
        nNodes++;
        if (pindexFirstInvalid == nullptr && pindex->nStatus & BLOCK_FAILED_VALID) pindexFirstInvalid = pindex;
        if (pindexFirstConflicing == nullptr && pindex->nStatus & BLOCK_CONFLICT_CHAINLOCK) pindexFirstConflicing = pindex;
        if (pindexFirstMissing == nullptr && !(pindex->nStatus & BLOCK_HAVE_DATA)) pindexFirstMissing = pindex;
        if (pindexFirstNeverProcessed == nullptr && pindex->nTx == 0) pindexFirstNeverProcessed = pindex;
        if (pindex->pprev != nullptr && pindexFirstNotTreeValid == nullptr && (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_TREE) pindexFirstNotTreeValid = pindex;
        if (pindex->pprev != nullptr && pindexFirstNotTransactionsValid == nullptr && (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_TRANSACTIONS) pindexFirstNotTransactionsValid = pindex;
        if (pindex->pprev != nullptr && pindexFirstNotChainValid == nullptr && (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_CHAIN) pindexFirstNotChainValid = pindex;
        if (pindex->pprev != nullptr && pindexFirstNotScriptsValid == nullptr && (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_SCRIPTS) pindexFirstNotScriptsValid = pindex;

        // Begin: actual consistency checks.
        if (pindex->pprev == nullptr) {
            // Genesis block checks.
            assert(pindex->GetBlockHash() == m_params.GetConsensus().hashGenesisBlock); // Genesis block's hash must match.
            assert(pindex == m_chain.Genesis()); // The current active chain's genesis block must be this block.
        }
        if (!pindex->HaveTxsDownloaded()) assert(pindex->nSequenceId <= 0); // nSequenceId can't be set positive for blocks that aren't linked (negative is used for preciousblock)
        // VALID_TRANSACTIONS is equivalent to nTx > 0 for all nodes (whether or not pruning has occurred).
        // HAVE_DATA is only equivalent to nTx > 0 (or VALID_TRANSACTIONS) if no pruning has occurred.
        if (!fHavePruned) {
            // If we've never pruned, then HAVE_DATA should be equivalent to nTx > 0
            assert(!(pindex->nStatus & BLOCK_HAVE_DATA) == (pindex->nTx == 0));
            assert(pindexFirstMissing == pindexFirstNeverProcessed);
        } else {
            // If we have pruned, then we can only say that HAVE_DATA implies nTx > 0
            if (pindex->nStatus & BLOCK_HAVE_DATA) assert(pindex->nTx > 0);
        }
        if (pindex->nStatus & BLOCK_HAVE_UNDO) assert(pindex->nStatus & BLOCK_HAVE_DATA);
        assert(((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_TRANSACTIONS) == (pindex->nTx > 0)); // This is pruning-independent.
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
        if (pindexFirstConflicing == nullptr) {
            // Checks for not-conflciting blocks.
            assert((pindex->nStatus & BLOCK_CONFLICT_CHAINLOCK) == 0); // The conflicting mask cannot be set for blocks without conflicting parents.
        }
        if (!CBlockIndexWorkComparator()(pindex, m_chain.Tip()) && pindexFirstNeverProcessed == nullptr) {
            if (pindexFirstInvalid == nullptr && pindexFirstConflicing == nullptr) {
                // If this block sorts at least as good as the current tip and
                // is valid and we have all data for its parents, it must be in
                // setBlockIndexCandidates.  m_chain.Tip() must also be there
                // even if some data has been pruned.
                if (pindexFirstMissing == nullptr || pindex == m_chain.Tip()) {
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
            assert(fHavePruned); // We must have pruned.
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
            if (pindex == pindexFirstConflicing) pindexFirstConflicing = nullptr;
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

std::string CChainState::ToString()
{
    CBlockIndex* tip = m_chain.Tip();
    return strprintf("Chainstate [%s] @ height %d (%s)",
                     m_from_snapshot_blockhash ? "snapshot" : "ibd",
                     tip ? tip->nHeight : -1, tip ? tip->GetBlockHash().ToString() : "null");
}

bool CChainState::ResizeCoinsCaches(size_t coinstip_size, size_t coinsdb_size)
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

std::string CBlockFileInfo::ToString() const {
    return strprintf("CBlockFileInfo(blocks=%u, size=%u, heights=%u...%u, time=%s...%s)", nBlocks, nSize, nHeightFirst, nHeightLast, FormatISO8601Date(nTimeFirst), FormatISO8601Date(nTimeLast));
}

CBlockFileInfo* GetBlockFileInfo(size_t n)
{
    LOCK(cs_LastBlockFile);

    return &vinfoBlockFile.at(n);
}

static const uint64_t MEMPOOL_DUMP_VERSION = 1;

bool LoadMempool(CTxMemPool& pool, CChainState& active_chainstate, FopenFn mockable_fopen_function)
{
    const CChainParams& chainparams = Params();
    int64_t nExpiryTimeout = gArgs.GetArg("-mempoolexpiry", DEFAULT_MEMPOOL_EXPIRY) * 60 * 60;
    FILE* filestr{mockable_fopen_function(GetDataDir() / "mempool.dat", "rb")};
    CAutoFile file(filestr, SER_DISK, CLIENT_VERSION);
    if (file.IsNull()) {
        LogPrintf("Failed to open mempool file from disk. Continuing anyway.\n");
        return false;
    }

    int64_t count = 0;
    int64_t expired = 0;
    int64_t failed = 0;
    int64_t already_there = 0;
    int64_t unbroadcast = 0;
    int64_t nNow = GetTime();

    try {
        uint64_t version;
        file >> version;
        if (version != MEMPOOL_DUMP_VERSION) {
            return false;
        }
        uint64_t num;
        file >> num;
        while (num) {
            --num;
            CTransactionRef tx;
            int64_t nTime;
            int64_t nFeeDelta;
            file >> tx;
            file >> nTime;
            file >> nFeeDelta;

            CAmount amountdelta = nFeeDelta;
            if (amountdelta) {
                pool.PrioritiseTransaction(tx->GetHash(), amountdelta);
            }
            TxValidationState state;
            if (nTime > nNow - nExpiryTimeout) {
                LOCK(cs_main);
                assert(std::addressof(::ChainstateActive()) == std::addressof(active_chainstate));
                AcceptToMemoryPoolWithTime(chainparams, pool, active_chainstate, state, tx, nTime,
                                           false /* bypass_limits */, 0 /* nAbsurdFee */, false /* test_accept */);
                if (state.IsValid()) {
                    ++count;
                } else {
                    // mempool may contain the transaction already, e.g. from
                    // wallet(s) having loaded it while we were processing
                    // mempool transactions; consider these as valid, instead of
                    // failed, but mark them as 'already there'
                    if (pool.exists(tx->GetHash())) {
                        ++already_there;
                    } else {
                        ++failed;
                    }
                }
            } else {
                ++expired;
            }
            if (ShutdownRequested())
                return false;
        }
        std::map<uint256, CAmount> mapDeltas;
        file >> mapDeltas;

        for (const auto& i : mapDeltas) {
            pool.PrioritiseTransaction(i.first, i.second);
        }

        // TODO: remove this try except in v0.22
        try {
          std::set<uint256> unbroadcast_txids;
          file >> unbroadcast_txids;
          unbroadcast = unbroadcast_txids.size();

          for (const auto& txid : unbroadcast_txids) {
            pool.AddUnbroadcastTx(txid);
          }
        } catch (const std::exception&) {
          // mempool.dat files created prior to v0.21 will not have an
          // unbroadcast set. No need to log a failure if parsing fails here.
        }

    } catch (const std::exception& e) {
        LogPrintf("Failed to deserialize mempool data on disk: %s. Continuing anyway.\n", e.what());
        return false;
    }

    LogPrintf("Imported mempool transactions from disk: %i succeeded, %i failed, %i expired, %i already there, %i waiting for initial broadcast\n", count, failed, expired, already_there, unbroadcast);
    return true;
}

bool DumpMempool(const CTxMemPool& pool, FopenFn mockable_fopen_function, bool skip_file_commit)
{
    int64_t start = GetTimeMicros();

    std::map<uint256, CAmount> mapDeltas;
    std::vector<TxMempoolInfo> vinfo;
    std::set<uint256> unbroadcast_txids;

    static Mutex dump_mutex;
    LOCK(dump_mutex);

    {
        LOCK(pool.cs);
        for (const auto &i : pool.mapDeltas) {
            mapDeltas[i.first] = i.second;
        }
        vinfo = pool.infoAll();
        unbroadcast_txids = pool.GetUnbroadcastTxs();
    }

    int64_t mid = GetTimeMicros();

    try {
        FILE* filestr{mockable_fopen_function(GetDataDir() / "mempool.dat.new", "wb")};
        if (!filestr) {
            return false;
        }

        CAutoFile file(filestr, SER_DISK, CLIENT_VERSION);

        uint64_t version = MEMPOOL_DUMP_VERSION;
        file << version;

        file << (uint64_t)vinfo.size();
        for (const auto& i : vinfo) {
            file << *(i.tx);
            file << int64_t{count_seconds(i.m_time)};
            file << int64_t{i.nFeeDelta};
            mapDeltas.erase(i.tx->GetHash());
        }

        file << mapDeltas;

        LogPrintf("Writing %d unbroadcast transactions to disk.\n", unbroadcast_txids.size());
        file << unbroadcast_txids;

        if (!skip_file_commit && !FileCommit(file.Get()))
            throw std::runtime_error("FileCommit failed");
        file.fclose();
        if (!RenameOver(GetDataDir() / "mempool.dat.new", GetDataDir() / "mempool.dat")) {
            throw std::runtime_error("Rename failed");
        }
        int64_t last = GetTimeMicros();
        LogPrintf("Dumped mempool: %gs to copy, %gs to dump\n", (mid-start)*MICRO, (last-mid)*MICRO);
    } catch (const std::exception& e) {
        LogPrintf("Failed to dump mempool: %s. Continuing anyway.\n", e.what());
        return false;
    }
    return true;
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

std::optional<uint256> ChainstateManager::SnapshotBlockhash() const {
    LOCK(::cs_main);  // for m_active_chainstate access
    if (m_active_chainstate && m_active_chainstate->m_from_snapshot_blockhash) {
        // If a snapshot chainstate exists, it will always be our active.
        return m_active_chainstate->m_from_snapshot_blockhash;
    }
    return std::nullopt;
}

std::vector<CChainState*> ChainstateManager::GetAll()
{
    LOCK(::cs_main);
    std::vector<CChainState*> out;

    if (!IsSnapshotValidated() && m_ibd_chainstate) {
        out.push_back(m_ibd_chainstate.get());
    }

    if (m_snapshot_chainstate) {
        out.push_back(m_snapshot_chainstate.get());
    }

    return out;
}

CChainState& ChainstateManager::InitializeChainstate(CTxMemPool* mempool,
                                                     CEvoDB& evoDb,
                                                     const std::unique_ptr<llmq::CChainLocksHandler>& clhandler,
                                                     const std::unique_ptr<llmq::CInstantSendManager>& isman,
                                                     const std::unique_ptr<llmq::CQuorumBlockProcessor>& quorum_block_processor,
                                                     const std::optional<uint256>& snapshot_blockhash)
{
    bool is_snapshot = snapshot_blockhash.has_value();
    std::unique_ptr<CChainState>& to_modify =
        is_snapshot ? m_snapshot_chainstate : m_ibd_chainstate;

    if (to_modify) {
        throw std::logic_error("should not be overwriting a chainstate");
    }

    to_modify.reset(new CChainState(mempool, m_blockman, evoDb, clhandler, isman, quorum_block_processor, snapshot_blockhash));

    // Snapshot chainstates and initial IBD chaintates always become active.
    if (is_snapshot || (!is_snapshot && !m_active_chainstate)) {
        LogPrintf("Switching active chainstate to %s\n", to_modify->ToString());
        m_active_chainstate = to_modify.get();
    } else {
        throw std::logic_error("unexpected chainstate activation");
    }

    return *to_modify;
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

bool ChainstateManager::ActivateSnapshot(
        CAutoFile& coins_file,
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

    auto snapshot_chainstate = WITH_LOCK(::cs_main, return std::make_unique<CChainState>(
            /* mempool */ nullptr, m_blockman, this->ActiveChainstate().m_evoDb,
            this->ActiveChainstate().m_clhandler, this->ActiveChainstate().m_isman,
            this->ActiveChainstate().m_quorum_block_processor, base_blockhash
        )
    );

    {
        LOCK(::cs_main);
        snapshot_chainstate->InitCoinsDB(
            static_cast<size_t>(current_coinsdb_cache_size * SNAPSHOT_CACHE_PERC),
            in_memory, false, "chainstate");
        snapshot_chainstate->InitCoinsCache(
            static_cast<size_t>(current_coinstip_cache_size * SNAPSHOT_CACHE_PERC));
    }

    const bool snapshot_ok = this->PopulateAndValidateSnapshot(
        *snapshot_chainstate, coins_file, metadata);

    if (!snapshot_ok) {
        WITH_LOCK(::cs_main, this->MaybeRebalanceCaches());
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

bool ChainstateManager::PopulateAndValidateSnapshot(
    CChainState& snapshot_chainstate,
    CAutoFile& coins_file,
    const SnapshotMetadata& metadata)
{
    // It's okay to release cs_main before we're done using `coins_cache` because we know
    // that nothing else will be referencing the newly created snapshot_chainstate yet.
    CCoinsViewCache& coins_cache = *WITH_LOCK(::cs_main, return &snapshot_chainstate.CoinsTip());

    uint256 base_blockhash = metadata.m_base_blockhash;

    CBlockIndex* snapshot_start_block = WITH_LOCK(::cs_main, return m_blockman.LookupBlockIndex(base_blockhash));

    if (!snapshot_start_block) {
        // Needed for GetUTXOStats and ExpectedAssumeutxo to determine the height and to avoid a crash when base_blockhash.IsNull()
        LogPrintf("[snapshot] Did not find snapshot start blockheader %s\n",
                  base_blockhash.ToString());
        return false;
    }

    int base_height = snapshot_start_block->nHeight;
    auto maybe_au_data = ExpectedAssumeutxo(base_height, ::Params());

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
    int64_t flush_now{0};
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

            if (snapshot_cache_state >=
                    CoinsCacheSizeState::CRITICAL) {
                LogPrintf("[snapshot] flushing coins cache (%.2f MB)... ", /* Continued */
                    coins_cache.DynamicMemoryUsage() / (1000 * 1000));
                flush_now = GetTimeMillis();

                // This is a hack - we don't know what the actual best block is, but that
                // doesn't matter for the purposes of flushing the cache here. We'll set this
                // to its correct value (`base_blockhash`) below after the coins are loaded.
                coins_cache.SetBestBlock(GetRandHash());

                coins_cache.Flush();
                LogPrintf("done (%.2fms)\n", GetTimeMillis() - flush_now);
            }
        }
    }

    // Important that we set this. This and the coins_cache accesses above are
    // sort of a layer violation, but either we reach into the innards of
    // CCoinsViewCache here or we have to invert some of the CChainState to
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

    LogPrintf("[snapshot] flushing snapshot chainstate to disk\n");
    // No need to acquire cs_main since this chainstate isn't being used yet.
    coins_cache.Flush(); // TODO: if #17487 is merged, add erase=false here for better performance.

    assert(coins_cache.GetBestBlock() == base_blockhash);

    CCoinsStats stats{CoinStatsHashType::HASH_SERIALIZED};
    auto breakpoint_fnc = [] { /* TODO insert breakpoint here? */ };

    // As above, okay to immediately release cs_main here since no other context knows
    // about the snapshot_chainstate.
    CCoinsViewDB* snapshot_coinsdb = WITH_LOCK(::cs_main, return &snapshot_chainstate.CoinsDB());

    if (!GetUTXOStats(snapshot_coinsdb, WITH_LOCK(::cs_main, return std::ref(m_blockman)), stats, breakpoint_fnc)) {
        LogPrintf("[snapshot] failed to generate coins stats\n");
        return false;
    }

    // Assert that the deserialized chainstate contents match the expected assumeutxo value.
    if (AssumeutxoHash{stats.hashSerialized} != au_data.hash_serialized) {
        LogPrintf("[snapshot] bad snapshot content hash: expected %s, got %s\n",
            au_data.hash_serialized.ToString(), stats.hashSerialized.ToString());
        return false;
    }

    snapshot_chainstate.m_chain.SetTip(snapshot_start_block);

    // The remainder of this function requires modifying data protected by cs_main.
    LOCK(::cs_main);

    // Fake various pieces of CBlockIndex state:
    //
    //   - nChainTx: so that we accurately report IBD-to-tip progress
    //   - nTx: so that LoadBlockIndex() loads assumed-valid CBlockIndex entries
    //       (among other things)
    //   - nStatus & BLOCK_OPT_WITNESS: so that RewindBlockIndex() doesn't zealously
    //       unwind the assumed-valid chain.
    //
    CBlockIndex* index = nullptr;
    for (int i = 0; i <= snapshot_chainstate.m_chain.Height(); ++i) {
        index = snapshot_chainstate.m_chain[i];

        if (!index->nTx) {
            index->nTx = 1;
        }
        index->nChainTx = index->pprev ? index->pprev->nChainTx + index->nTx : 1;
    }

    assert(index);
    index->nChainTx = metadata.m_nchaintx;
    snapshot_chainstate.setBlockIndexCandidates.insert(snapshot_start_block);

    LogPrintf("[snapshot] validated snapshot (%.2f MB)\n",
        coins_cache.DynamicMemoryUsage() / (1000 * 1000));
    return true;
}

CChainState& ChainstateManager::ActiveChainstate() const
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

CChainState& ChainstateManager::ValidatedChainstate() const
{
    LOCK(::cs_main);
    if (m_snapshot_chainstate && IsSnapshotValidated()) {
        return *m_snapshot_chainstate.get();
    }
    assert(m_ibd_chainstate);
    return *m_ibd_chainstate.get();
}

bool ChainstateManager::IsBackgroundIBD(CChainState* chainstate) const
{
    LOCK(::cs_main);
    return (m_snapshot_chainstate && chainstate == m_ibd_chainstate.get());
}

void ChainstateManager::Unload()
{
    for (CChainState* chainstate : this->GetAll()) {
        chainstate->m_chain.SetTip(nullptr);
        chainstate->UnloadBlockIndex();
    }

    m_blockman.Unload();
}

void ChainstateManager::Reset()
{
    LOCK(::cs_main);
    m_ibd_chainstate.reset();
    m_snapshot_chainstate.reset();
    m_active_chainstate = nullptr;
    m_snapshot_validated = false;
}

void ChainstateManager::MaybeRebalanceCaches()
{
    if (m_ibd_chainstate && !m_snapshot_chainstate) {
        LogPrintf("[snapshot] allocating all cache to the IBD chainstate\n");
        // Allocate everything to the IBD chainstate.
        m_ibd_chainstate->ResizeCoinsCaches(m_total_coinstip_cache, m_total_coinsdb_cache);
    }
    else if (m_snapshot_chainstate && !m_ibd_chainstate) {
        LogPrintf("[snapshot] allocating all cache to the snapshot chainstate\n");
        // Allocate everything to the snapshot chainstate.
        m_snapshot_chainstate->ResizeCoinsCaches(m_total_coinstip_cache, m_total_coinsdb_cache);
    }
    else if (m_ibd_chainstate && m_snapshot_chainstate) {
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
