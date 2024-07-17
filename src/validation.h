// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VALIDATION_H
#define BITCOIN_VALIDATION_H

#include <arith_uint256.h>
#include <attributes.h>
#include <chain.h>
#include <checkqueue.h>
#include <consensus/amount.h>
#include <cuckoocache.h>
#include <deploymentstatus.h>
#include <kernel/chain.h>
#include <kernel/chainparams.h>
#include <kernel/chainstatemanager_opts.h>
#include <kernel/cs_main.h> // IWYU pragma: export
#include <node/blockstorage.h>
#include <policy/feerate.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <script/script_error.h>
#include <script/sigcache.h>
#include <sync.h>
#include <txdb.h>
#include <txmempool.h> // For CTxMemPool::cs
#include <uint256.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/hasher.h>
#include <util/result.h>
#include <util/translation.h>
#include <versionbits.h>

#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdint.h>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class Chainstate;
class CTxMemPool;
class ChainstateManager;
struct ChainTxData;
class DisconnectedBlockTransactions;
struct PrecomputedTransactionData;
struct LockPoints;
struct AssumeutxoData;
namespace node {
class SnapshotMetadata;
} // namespace node
namespace Consensus {
struct Params;
} // namespace Consensus
namespace util {
class SignalInterrupt;
} // namespace util

/** Block files containing a block-height within MIN_BLOCKS_TO_KEEP of ActiveChain().Tip() will not be pruned. */
static const unsigned int MIN_BLOCKS_TO_KEEP = 288;
static const signed int DEFAULT_CHECKBLOCKS = 6;
static constexpr int DEFAULT_CHECKLEVEL{3};
// Require that user allocate at least 550 MiB for block & undo files (blk???.dat and rev???.dat)
// At 1MB per block, 288 blocks = 288MB.
// Add 15% for Undo data = 331MB
// Add 20% for Orphan block rate = 397MB
// We want the low water mark after pruning to be at least 397 MB and since we prune in
// full block file chunks, we need the high water mark which triggers the prune to be
// one 128MB block file + added 15% undo data = 147MB greater for a total of 545MB
// Setting the target to >= 550 MiB will make it likely we can respect the target.
static const uint64_t MIN_DISK_SPACE_FOR_BLOCK_FILES = 550 * 1024 * 1024;

/** Current sync state passed to tip changed callbacks. */
enum class SynchronizationState {
    INIT_REINDEX,
    INIT_DOWNLOAD,
    POST_INIT
};

extern GlobalMutex g_best_block_mutex;
extern std::condition_variable g_best_block_cv;
/** Used to notify getblocktemplate RPC of new tips. */
extern uint256 g_best_block;

/** Documentation for argument 'checklevel'. */
extern const std::vector<std::string> CHECKLEVEL_DOC;

CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& consensusParams);

bool FatalError(kernel::Notifications& notifications, BlockValidationState& state, const bilingual_str& message);

/** Guess verification progress (as a fraction between 0.0=genesis and 1.0=current tip). */
double GuessVerificationProgress(const ChainTxData& data, const CBlockIndex* pindex);

/** Prune block files up to a given height */
void PruneBlockFilesManual(Chainstate& active_chainstate, int nManualPruneHeight);

/**
* Validation result for a transaction evaluated by MemPoolAccept (single or package).
* Here are the expected fields and properties of a result depending on its ResultType, applicable to
* results returned from package evaluation:
*+---------------------------+----------------+-------------------+------------------+----------------+-------------------+
*| Field or property         |    VALID       |                 INVALID              |  MEMPOOL_ENTRY | DIFFERENT_WITNESS |
*|                           |                |--------------------------------------|                |                   |
*|                           |                | TX_RECONSIDERABLE |     Other        |                |                   |
*+---------------------------+----------------+-------------------+------------------+----------------+-------------------+
*| txid in mempool?          | yes            | no                | no*              | yes            | yes               |
*| wtxid in mempool?         | yes            | no                | no*              | yes            | no                |
*| m_state                   | yes, IsValid() | yes, IsInvalid()  | yes, IsInvalid() | yes, IsValid() | yes, IsValid()    |
*| m_vsize                   | yes            | no                | no               | yes            | no                |
*| m_base_fees               | yes            | no                | no               | yes            | no                |
*| m_effective_feerate       | yes            | yes               | no               | no             | no                |
*| m_wtxids_fee_calculations | yes            | yes               | no               | no             | no                |
*| m_other_wtxid             | no             | no                | no               | no             | yes               |
*+---------------------------+----------------+-------------------+------------------+----------------+-------------------+
* (*) Individual transaction acceptance doesn't return MEMPOOL_ENTRY and DIFFERENT_WITNESS. It returns
* INVALID, with the errors txn-already-in-mempool and txn-same-nonwitness-data-in-mempool
* respectively. In those cases, the txid or wtxid may be in the mempool for a TX_CONFLICT.
*/
struct MempoolAcceptResult {
    /** Used to indicate the results of mempool validation. */
    enum class ResultType {
        VALID, //!> Fully validated, valid.
        INVALID, //!> Invalid.
        MEMPOOL_ENTRY, //!> Valid, transaction was already in the mempool.
        DIFFERENT_WITNESS, //!> Not validated. A same-txid-different-witness tx (see m_other_wtxid) already exists in the mempool and was not replaced.
    };
    /** Result type. Present in all MempoolAcceptResults. */
    const ResultType m_result_type;

    /** Contains information about why the transaction failed. */
    const TxValidationState m_state;

    /** Mempool transactions replaced by the tx. */
    const std::list<CTransactionRef> m_replaced_transactions;
    /** Virtual size as used by the mempool, calculated using serialized size and sigops. */
    const std::optional<int64_t> m_vsize;
    /** Raw base fees in satoshis. */
    const std::optional<CAmount> m_base_fees;
    /** The feerate at which this transaction was considered. This includes any fee delta added
     * using prioritisetransaction (i.e. modified fees). If this transaction was submitted as a
     * package, this is the package feerate, which may also include its descendants and/or
     * ancestors (see m_wtxids_fee_calculations below).
     */
    const std::optional<CFeeRate> m_effective_feerate;
    /** Contains the wtxids of the transactions used for fee-related checks. Includes this
     * transaction's wtxid and may include others if this transaction was validated as part of a
     * package. This is not necessarily equivalent to the list of transactions passed to
     * ProcessNewPackage().
     * Only present when m_result_type = ResultType::VALID. */
    const std::optional<std::vector<Wtxid>> m_wtxids_fee_calculations;

    /** The wtxid of the transaction in the mempool which has the same txid but different witness. */
    const std::optional<uint256> m_other_wtxid;

    static MempoolAcceptResult Failure(TxValidationState state) {
        return MempoolAcceptResult(state);
    }

    static MempoolAcceptResult FeeFailure(TxValidationState state,
                                          CFeeRate effective_feerate,
                                          const std::vector<Wtxid>& wtxids_fee_calculations) {
        return MempoolAcceptResult(state, effective_feerate, wtxids_fee_calculations);
    }

    static MempoolAcceptResult Success(std::list<CTransactionRef>&& replaced_txns,
                                       int64_t vsize,
                                       CAmount fees,
                                       CFeeRate effective_feerate,
                                       const std::vector<Wtxid>& wtxids_fee_calculations) {
        return MempoolAcceptResult(std::move(replaced_txns), vsize, fees,
                                   effective_feerate, wtxids_fee_calculations);
    }

    static MempoolAcceptResult MempoolTx(int64_t vsize, CAmount fees) {
        return MempoolAcceptResult(vsize, fees);
    }

    static MempoolAcceptResult MempoolTxDifferentWitness(const uint256& other_wtxid) {
        return MempoolAcceptResult(other_wtxid);
    }

// Private constructors. Use static methods MempoolAcceptResult::Success, etc. to construct.
private:
    /** Constructor for failure case */
    explicit MempoolAcceptResult(TxValidationState state)
        : m_result_type(ResultType::INVALID), m_state(state) {
            Assume(!state.IsValid()); // Can be invalid or error
        }

    /** Constructor for success case */
    explicit MempoolAcceptResult(std::list<CTransactionRef>&& replaced_txns,
                                 int64_t vsize,
                                 CAmount fees,
                                 CFeeRate effective_feerate,
                                 const std::vector<Wtxid>& wtxids_fee_calculations)
        : m_result_type(ResultType::VALID),
        m_replaced_transactions(std::move(replaced_txns)),
        m_vsize{vsize},
        m_base_fees(fees),
        m_effective_feerate(effective_feerate),
        m_wtxids_fee_calculations(wtxids_fee_calculations) {}

    /** Constructor for fee-related failure case */
    explicit MempoolAcceptResult(TxValidationState state,
                                 CFeeRate effective_feerate,
                                 const std::vector<Wtxid>& wtxids_fee_calculations)
        : m_result_type(ResultType::INVALID),
        m_state(state),
        m_effective_feerate(effective_feerate),
        m_wtxids_fee_calculations(wtxids_fee_calculations) {}

    /** Constructor for already-in-mempool case. It wouldn't replace any transactions. */
    explicit MempoolAcceptResult(int64_t vsize, CAmount fees)
        : m_result_type(ResultType::MEMPOOL_ENTRY), m_vsize{vsize}, m_base_fees(fees) {}

    /** Constructor for witness-swapped case. */
    explicit MempoolAcceptResult(const uint256& other_wtxid)
        : m_result_type(ResultType::DIFFERENT_WITNESS), m_other_wtxid(other_wtxid) {}
};

/**
* Validation result for package mempool acceptance.
*/
struct PackageMempoolAcceptResult
{
    PackageValidationState m_state;
    /**
    * Map from wtxid to finished MempoolAcceptResults. The client is responsible
    * for keeping track of the transaction objects themselves. If a result is not
    * present, it means validation was unfinished for that transaction. If there
    * was a package-wide error (see result in m_state), m_tx_results will be empty.
    */
    std::map<uint256, MempoolAcceptResult> m_tx_results;

    explicit PackageMempoolAcceptResult(PackageValidationState state,
                                        std::map<uint256, MempoolAcceptResult>&& results)
        : m_state{state}, m_tx_results(std::move(results)) {}

    explicit PackageMempoolAcceptResult(PackageValidationState state, CFeeRate feerate,
                                        std::map<uint256, MempoolAcceptResult>&& results)
        : m_state{state}, m_tx_results(std::move(results)) {}

    /** Constructor to create a PackageMempoolAcceptResult from a single MempoolAcceptResult */
    explicit PackageMempoolAcceptResult(const uint256& wtxid, const MempoolAcceptResult& result)
        : m_tx_results{ {wtxid, result} } {}
};

/**
 * Try to add a transaction to the mempool. This is an internal function and is exposed only for testing.
 * Client code should use ChainstateManager::ProcessTransaction()
 *
 * @param[in]  active_chainstate  Reference to the active chainstate.
 * @param[in]  tx                 The transaction to submit for mempool acceptance.
 * @param[in]  accept_time        The timestamp for adding the transaction to the mempool.
 *                                It is also used to determine when the entry expires.
 * @param[in]  bypass_limits      When true, don't enforce mempool fee and capacity limits,
 *                                and set entry_sequence to zero.
 * @param[in]  test_accept        When true, run validation checks but don't submit to mempool.
 *
 * @returns a MempoolAcceptResult indicating whether the transaction was accepted/rejected with reason.
 */
MempoolAcceptResult AcceptToMemoryPool(Chainstate& active_chainstate, const CTransactionRef& tx,
                                       int64_t accept_time, bool bypass_limits, bool test_accept)
    EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/**
* Validate (and maybe submit) a package to the mempool. See doc/policy/packages.md for full details
* on package validation rules.
* @param[in]    test_accept         When true, run validation checks but don't submit to mempool.
* @param[in]    client_maxfeerate    If exceeded by an individual transaction, rest of (sub)package evaluation is aborted.
*                                   Only for sanity checks against local submission of transactions.
* @returns a PackageMempoolAcceptResult which includes a MempoolAcceptResult for each transaction.
* If a transaction fails, validation will exit early and some results may be missing. It is also
* possible for the package to be partially submitted.
*/
PackageMempoolAcceptResult ProcessNewPackage(Chainstate& active_chainstate, CTxMemPool& pool,
                                                   const Package& txns, bool test_accept, const std::optional<CFeeRate>& client_maxfeerate)
                                                   EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/* Mempool validation helper functions */

/**
 * Check if transaction will be final in the next block to be created.
 */
bool CheckFinalTxAtTip(const CBlockIndex& active_chain_tip, const CTransaction& tx) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

/**
 * Calculate LockPoints required to check if transaction will be BIP68 final in the next block
 * to be created on top of tip.
 *
 * @param[in]   tip             Chain tip for which tx sequence locks are calculated. For
 *                              example, the tip of the current active chain.
 * @param[in]   coins_view      Any CCoinsView that provides access to the relevant coins for
 *                              checking sequence locks. For example, it can be a CCoinsViewCache
 *                              that isn't connected to anything but contains all the relevant
 *                              coins, or a CCoinsViewMemPool that is connected to the
 *                              mempool and chainstate UTXO set. In the latter case, the caller
 *                              is responsible for holding the appropriate locks to ensure that
 *                              calls to GetCoin() return correct coins.
 * @param[in]   tx              The transaction being evaluated.
 *
 * @returns The resulting height and time calculated and the hash of the block needed for
 *          calculation, or std::nullopt if there is an error.
 */
std::optional<LockPoints> CalculateLockPointsAtTip(
    CBlockIndex* tip,
    const CCoinsView& coins_view,
    const CTransaction& tx);

/**
 * Check if transaction will be BIP68 final in the next block to be created on top of tip.
 * @param[in]   tip             Chain tip to check tx sequence locks against. For example,
 *                              the tip of the current active chain.
 * @param[in]   lock_points     LockPoints containing the height and time at which this
 *                              transaction is final.
 * Simulates calling SequenceLocks() with data from the tip passed in.
 * The LockPoints should not be considered valid if CheckSequenceLocksAtTip returns false.
 */
bool CheckSequenceLocksAtTip(CBlockIndex* tip,
                             const LockPoints& lock_points);

/**
 * Closure representing one script verification
 * Note that this stores references to the spending transaction
 */
class CScriptCheck
{
private:
    CTxOut m_tx_out;
    const CTransaction *ptxTo;
    unsigned int nIn;
    unsigned int nFlags;
    bool cacheStore;
    ScriptError error{SCRIPT_ERR_UNKNOWN_ERROR};
    PrecomputedTransactionData *txdata;
    SignatureCache* m_signature_cache;

public:
    CScriptCheck(const CTxOut& outIn, const CTransaction& txToIn, SignatureCache& signature_cache, unsigned int nInIn, unsigned int nFlagsIn, bool cacheIn, PrecomputedTransactionData* txdataIn) :
        m_tx_out(outIn), ptxTo(&txToIn), nIn(nInIn), nFlags(nFlagsIn), cacheStore(cacheIn), txdata(txdataIn), m_signature_cache(&signature_cache) { }

    CScriptCheck(const CScriptCheck&) = delete;
    CScriptCheck& operator=(const CScriptCheck&) = delete;
    CScriptCheck(CScriptCheck&&) = default;
    CScriptCheck& operator=(CScriptCheck&&) = default;

    bool operator()();

    ScriptError GetScriptError() const { return error; }
};

// CScriptCheck is used a lot in std::vector, make sure that's efficient
static_assert(std::is_nothrow_move_assignable_v<CScriptCheck>);
static_assert(std::is_nothrow_move_constructible_v<CScriptCheck>);
static_assert(std::is_nothrow_destructible_v<CScriptCheck>);

/**
 * Convenience class for initializing and passing the script execution cache
 * and signature cache.
 */
class ValidationCache
{
private:
    //! Pre-initialized hasher to avoid having to recreate it for every hash calculation.
    CSHA256 m_script_execution_cache_hasher;

public:
    CuckooCache::cache<uint256, SignatureCacheHasher> m_script_execution_cache;
    SignatureCache m_signature_cache;

    ValidationCache(size_t script_execution_cache_bytes, size_t signature_cache_bytes);

    ValidationCache(const ValidationCache&) = delete;
    ValidationCache& operator=(const ValidationCache&) = delete;

    //! Return a copy of the pre-initialized hasher.
    CSHA256 ScriptExecutionCacheHasher() const { return m_script_execution_cache_hasher; }
};

/** Functions for validating blocks and updating the block tree */

/** Context-independent validity checks */
bool CheckBlock(const CBlock& block, BlockValidationState& state, const Consensus::Params& consensusParams, bool fCheckPOW = true, bool fCheckMerkleRoot = true);

/** Check a block is completely valid from start to finish (only works on top of our current best block) */
bool TestBlockValidity(BlockValidationState& state,
                       const CChainParams& chainparams,
                       Chainstate& chainstate,
                       const CBlock& block,
                       CBlockIndex* pindexPrev,
                       bool fCheckPOW = true,
                       bool fCheckMerkleRoot = true) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** Check with the proof of work on each blockheader matches the value in nBits */
bool HasValidProofOfWork(const std::vector<CBlockHeader>& headers, const Consensus::Params& consensusParams);

/** Check if a block has been mutated (with respect to its merkle root and witness commitments). */
bool IsBlockMutated(const CBlock& block, bool check_witness_root);

/** Return the sum of the claimed work on a given set of headers. No verification of PoW is done. */
arith_uint256 CalculateClaimedHeadersWork(const std::vector<CBlockHeader>& headers);

enum class VerifyDBResult {
    SUCCESS,
    CORRUPTED_BLOCK_DB,
    INTERRUPTED,
    SKIPPED_L3_CHECKS,
    SKIPPED_MISSING_BLOCKS,
};

/** RAII wrapper for VerifyDB: Verify consistency of the block and coin databases */
class CVerifyDB
{
private:
    kernel::Notifications& m_notifications;

public:
    explicit CVerifyDB(kernel::Notifications& notifications);
    ~CVerifyDB();
    [[nodiscard]] VerifyDBResult VerifyDB(
        Chainstate& chainstate,
        const Consensus::Params& consensus_params,
        CCoinsView& coinsview,
        int nCheckLevel,
        int nCheckDepth) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
};

enum DisconnectResult
{
    DISCONNECT_OK,      // All good.
    DISCONNECT_UNCLEAN, // Rolled back, but UTXO set was inconsistent with block.
    DISCONNECT_FAILED   // Something else went wrong.
};

class ConnectTrace;

/** @see Chainstate::FlushStateToDisk */
enum class FlushStateMode {
    NONE,
    IF_NEEDED,
    PERIODIC,
    ALWAYS
};

/**
 * A convenience class for constructing the CCoinsView* hierarchy used
 * to facilitate access to the UTXO set.
 *
 * This class consists of an arrangement of layered CCoinsView objects,
 * preferring to store and retrieve coins in memory via `m_cacheview` but
 * ultimately falling back on cache misses to the canonical store of UTXOs on
 * disk, `m_dbview`.
 */
class CoinsViews {

public:
    //! The lowest level of the CoinsViews cache hierarchy sits in a leveldb database on disk.
    //! All unspent coins reside in this store.
    CCoinsViewDB m_dbview GUARDED_BY(cs_main);

    //! This view wraps access to the leveldb instance and handles read errors gracefully.
    CCoinsViewErrorCatcher m_catcherview GUARDED_BY(cs_main);

    //! This is the top layer of the cache hierarchy - it keeps as many coins in memory as
    //! can fit per the dbcache setting.
    std::unique_ptr<CCoinsViewCache> m_cacheview GUARDED_BY(cs_main);

    //! This constructor initializes CCoinsViewDB and CCoinsViewErrorCatcher instances, but it
    //! *does not* create a CCoinsViewCache instance by default. This is done separately because the
    //! presence of the cache has implications on whether or not we're allowed to flush the cache's
    //! state to disk, which should not be done until the health of the database is verified.
    //!
    //! All arguments forwarded onto CCoinsViewDB.
    CoinsViews(DBParams db_params, CoinsViewOptions options);

    //! Initialize the CCoinsViewCache member.
    void InitCache() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};

enum class CoinsCacheSizeState
{
    //! The coins cache is in immediate need of a flush.
    CRITICAL = 2,
    //! The cache is at >= 90% capacity.
    LARGE = 1,
    OK = 0
};

/**
 * Chainstate stores and provides an API to update our local knowledge of the
 * current best chain.
 *
 * Eventually, the API here is targeted at being exposed externally as a
 * consumable library, so any functions added must only call
 * other class member functions, pure functions in other parts of the consensus
 * library, callbacks via the validation interface, or read/write-to-disk
 * functions (eventually this will also be via callbacks).
 *
 * Anything that is contingent on the current tip of the chain is stored here,
 * whereas block information and metadata independent of the current tip is
 * kept in `BlockManager`.
 */
class Chainstate
{
protected:
    /**
     * The ChainState Mutex
     * A lock that must be held when modifying this ChainState - held in ActivateBestChain() and
     * InvalidateBlock()
     */
    Mutex m_chainstate_mutex;

    //! Optional mempool that is kept in sync with the chain.
    //! Only the active chainstate has a mempool.
    CTxMemPool* m_mempool;

    //! Manages the UTXO set, which is a reflection of the contents of `m_chain`.
    std::unique_ptr<CoinsViews> m_coins_views;

    //! This toggle exists for use when doing background validation for UTXO
    //! snapshots.
    //!
    //! In the expected case, it is set once the background validation chain reaches the
    //! same height as the base of the snapshot and its UTXO set is found to hash to
    //! the expected assumeutxo value. It signals that we should no longer connect
    //! blocks to the background chainstate. When set on the background validation
    //! chainstate, it signifies that we have fully validated the snapshot chainstate.
    //!
    //! In the unlikely case that the snapshot chainstate is found to be invalid, this
    //! is set to true on the snapshot chainstate.
    bool m_disabled GUARDED_BY(::cs_main) {false};

    //! Cached result of LookupBlockIndex(*m_from_snapshot_blockhash)
    const CBlockIndex* m_cached_snapshot_base GUARDED_BY(::cs_main) {nullptr};

public:
    //! Reference to a BlockManager instance which itself is shared across all
    //! Chainstate instances.
    node::BlockManager& m_blockman;

    //! The chainstate manager that owns this chainstate. The reference is
    //! necessary so that this instance can check whether it is the active
    //! chainstate within deeply nested method calls.
    ChainstateManager& m_chainman;

    explicit Chainstate(
        CTxMemPool* mempool,
        node::BlockManager& blockman,
        ChainstateManager& chainman,
        std::optional<uint256> from_snapshot_blockhash = std::nullopt);

    //! Return the current role of the chainstate. See `ChainstateManager`
    //! documentation for a description of the different types of chainstates.
    //!
    //! @sa ChainstateRole
    ChainstateRole GetRole() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /**
     * Initialize the CoinsViews UTXO set database management data structures. The in-memory
     * cache is initialized separately.
     *
     * All parameters forwarded to CoinsViews.
     */
    void InitCoinsDB(
        size_t cache_size_bytes,
        bool in_memory,
        bool should_wipe,
        fs::path leveldb_name = "chainstate");

    //! Initialize the in-memory coins cache (to be done after the health of the on-disk database
    //! is verified).
    void InitCoinsCache(size_t cache_size_bytes) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! @returns whether or not the CoinsViews object has been fully initialized and we can
    //!          safely flush this object to disk.
    bool CanFlushToDisk() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        return m_coins_views && m_coins_views->m_cacheview;
    }

    //! The current chain of blockheaders we consult and build on.
    //! @see CChain, CBlockIndex.
    CChain m_chain;

    /**
     * The blockhash which is the base of the snapshot this chainstate was created from.
     *
     * std::nullopt if this chainstate was not created from a snapshot.
     */
    const std::optional<uint256> m_from_snapshot_blockhash;

    /**
     * The base of the snapshot this chainstate was created from.
     *
     * nullptr if this chainstate was not created from a snapshot.
     */
    const CBlockIndex* SnapshotBase() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /**
     * The set of all CBlockIndex entries that have as much work as our current
     * tip or more, and transaction data needed to be validated (with
     * BLOCK_VALID_TRANSACTIONS for each block and its parents back to the
     * genesis block or an assumeutxo snapshot block). Entries may be failed,
     * though, and pruning nodes may be missing the data for the block.
     */
    std::set<CBlockIndex*, node::CBlockIndexWorkComparator> setBlockIndexCandidates;

    //! @returns A reference to the in-memory cache of the UTXO set.
    CCoinsViewCache& CoinsTip() EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        Assert(m_coins_views);
        return *Assert(m_coins_views->m_cacheview);
    }

    //! @returns A reference to the on-disk UTXO set database.
    CCoinsViewDB& CoinsDB() EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        return Assert(m_coins_views)->m_dbview;
    }

    //! @returns A pointer to the mempool.
    CTxMemPool* GetMempool()
    {
        return m_mempool;
    }

    //! @returns A reference to a wrapped view of the in-memory UTXO set that
    //!     handles disk read errors gracefully.
    CCoinsViewErrorCatcher& CoinsErrorCatcher() EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        return Assert(m_coins_views)->m_catcherview;
    }

    //! Destructs all objects related to accessing the UTXO set.
    void ResetCoinsViews() { m_coins_views.reset(); }

    //! Does this chainstate have a UTXO set attached?
    bool HasCoinsViews() const { return (bool)m_coins_views; }

    //! The cache size of the on-disk coins view.
    size_t m_coinsdb_cache_size_bytes{0};

    //! The cache size of the in-memory coins view.
    size_t m_coinstip_cache_size_bytes{0};

    //! Resize the CoinsViews caches dynamically and flush state to disk.
    //! @returns true unless an error occurred during the flush.
    bool ResizeCoinsCaches(size_t coinstip_size, size_t coinsdb_size)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /**
     * Update the on-disk chain state.
     * The caches and indexes are flushed depending on the mode we're called with
     * if they're too large, if it's been a while since the last write,
     * or always and in all cases if we're in prune mode and are deleting files.
     *
     * If FlushStateMode::NONE is used, then FlushStateToDisk(...) won't do anything
     * besides checking if we need to prune.
     *
     * @returns true unless a system error occurred
     */
    bool FlushStateToDisk(
        BlockValidationState& state,
        FlushStateMode mode,
        int nManualPruneHeight = 0);

    //! Unconditionally flush all changes to disk.
    void ForceFlushStateToDisk();

    //! Prune blockfiles from the disk if necessary and then flush chainstate changes
    //! if we pruned.
    void PruneAndFlush();

    /**
     * Find the best known block, and make it the tip of the block chain. The
     * result is either failure or an activated best chain. pblock is either
     * nullptr or a pointer to a block that is already loaded (to avoid loading
     * it again from disk).
     *
     * ActivateBestChain is split into steps (see ActivateBestChainStep) so that
     * we avoid holding cs_main for an extended period of time; the length of this
     * call may be quite long during reindexing or a substantial reorg.
     *
     * May not be called with cs_main held. May not be called in a
     * validationinterface callback.
     *
     * Note that if this is called while a snapshot chainstate is active, and if
     * it is called on a background chainstate whose tip has reached the base block
     * of the snapshot, its execution will take *MINUTES* while it hashes the
     * background UTXO set to verify the assumeutxo value the snapshot was activated
     * with. `cs_main` will be held during this time.
     *
     * @returns true unless a system error occurred
     */
    bool ActivateBestChain(
        BlockValidationState& state,
        std::shared_ptr<const CBlock> pblock = nullptr)
        EXCLUSIVE_LOCKS_REQUIRED(!m_chainstate_mutex)
        LOCKS_EXCLUDED(::cs_main);

    // Block (dis)connection on a given view:
    DisconnectResult DisconnectBlock(const CBlock& block, const CBlockIndex* pindex, CCoinsViewCache& view)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool ConnectBlock(const CBlock& block, BlockValidationState& state, CBlockIndex* pindex,
                      CCoinsViewCache& view, bool fJustCheck = false) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    // Apply the effects of a block disconnection on the UTXO set.
    bool DisconnectTip(BlockValidationState& state, DisconnectedBlockTransactions* disconnectpool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_mempool->cs);

    // Manual block validity manipulation:
    /** Mark a block as precious and reorganize.
     *
     * May not be called in a validationinterface callback.
     */
    bool PreciousBlock(BlockValidationState& state, CBlockIndex* pindex)
        EXCLUSIVE_LOCKS_REQUIRED(!m_chainstate_mutex)
        LOCKS_EXCLUDED(::cs_main);

    /** Mark a block as invalid. */
    bool InvalidateBlock(BlockValidationState& state, CBlockIndex* pindex)
        EXCLUSIVE_LOCKS_REQUIRED(!m_chainstate_mutex)
        LOCKS_EXCLUDED(::cs_main);

    /** Remove invalidity status from a block and its descendants. */
    void ResetBlockFailureFlags(CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /** Replay blocks that aren't fully applied to the database. */
    bool ReplayBlocks();

    /** Whether the chain state needs to be redownloaded due to lack of witness data */
    [[nodiscard]] bool NeedsRedownload() const EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    /** Ensures we have a genesis block in the block tree, possibly writing one to disk. */
    bool LoadGenesisBlock();

    void TryAddBlockIndexCandidate(CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    void PruneBlockIndexCandidates();

    void ClearBlockIndexCandidates() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Find the last common block of this chain and a locator. */
    const CBlockIndex* FindForkInGlobalIndex(const CBlockLocator& locator) const EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /** Update the chain tip based on database information, i.e. CoinsTip()'s best block. */
    bool LoadChainTip() EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    //! Dictates whether we need to flush the cache to disk or not.
    //!
    //! @return the state of the size of the coins cache.
    CoinsCacheSizeState GetCoinsCacheSizeState() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    CoinsCacheSizeState GetCoinsCacheSizeState(
        size_t max_coins_cache_size_bytes,
        size_t max_mempool_size_bytes) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    std::string ToString() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! Indirection necessary to make lock annotations work with an optional mempool.
    RecursiveMutex* MempoolMutex() const LOCK_RETURNED(m_mempool->cs)
    {
        return m_mempool ? &m_mempool->cs : nullptr;
    }

private:
    bool ActivateBestChainStep(BlockValidationState& state, CBlockIndex* pindexMostWork, const std::shared_ptr<const CBlock>& pblock, bool& fInvalidFound, ConnectTrace& connectTrace) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_mempool->cs);
    bool ConnectTip(BlockValidationState& state, CBlockIndex* pindexNew, const std::shared_ptr<const CBlock>& pblock, ConnectTrace& connectTrace, DisconnectedBlockTransactions& disconnectpool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_mempool->cs);

    void InvalidBlockFound(CBlockIndex* pindex, const BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    CBlockIndex* FindMostWorkChain() EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    bool RollforwardBlock(const CBlockIndex* pindex, CCoinsViewCache& inputs) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    void CheckForkWarningConditions() EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void InvalidChainFound(CBlockIndex* pindexNew) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /**
     * Make mempool consistent after a reorg, by re-adding or recursively erasing
     * disconnected block transactions from the mempool, and also removing any
     * other transactions from the mempool that are no longer valid given the new
     * tip/height.
     *
     * Note: we assume that disconnectpool only contains transactions that are NOT
     * confirmed in the current chain nor already in the mempool (otherwise,
     * in-mempool descendants of such transactions would be removed).
     *
     * Passing fAddToMempool=false will skip trying to add the transactions back,
     * and instead just erase from the mempool as needed.
     */
    void MaybeUpdateMempoolForReorg(
        DisconnectedBlockTransactions& disconnectpool,
        bool fAddToMempool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_mempool->cs);

    /** Check warning conditions and do some notifications on new chain tip set. */
    void UpdateTip(const CBlockIndex* pindexNew)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    SteadyClock::time_point m_last_write{};
    SteadyClock::time_point m_last_flush{};

    /**
     * In case of an invalid snapshot, rename the coins leveldb directory so
     * that it can be examined for issue diagnosis.
     */
    [[nodiscard]] util::Result<void> InvalidateCoinsDBOnDisk() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    friend ChainstateManager;
};

enum class SnapshotCompletionResult {
    SUCCESS,
    SKIPPED,

    // Expected assumeutxo configuration data is not found for the height of the
    // base block.
    MISSING_CHAINPARAMS,

    // Failed to generate UTXO statistics (to check UTXO set hash) for the background
    // chainstate.
    STATS_FAILED,

    // The UTXO set hash of the background validation chainstate does not match
    // the one expected by assumeutxo chainparams.
    HASH_MISMATCH,

    // The blockhash of the current tip of the background validation chainstate does
    // not match the one expected by the snapshot chainstate.
    BASE_BLOCKHASH_MISMATCH,
};

/**
 * Provides an interface for creating and interacting with one or two
 * chainstates: an IBD chainstate generated by downloading blocks, and
 * an optional snapshot chainstate loaded from a UTXO snapshot. Managed
 * chainstates can be maintained at different heights simultaneously.
 *
 * This class provides abstractions that allow the retrieval of the current
 * most-work chainstate ("Active") as well as chainstates which may be in
 * background use to validate UTXO snapshots.
 *
 * Definitions:
 *
 * *IBD chainstate*: a chainstate whose current state has been "fully"
 *   validated by the initial block download process.
 *
 * *Snapshot chainstate*: a chainstate populated by loading in an
 *    assumeutxo UTXO snapshot.
 *
 * *Active chainstate*: the chainstate containing the current most-work
 *    chain. Consulted by most parts of the system (net_processing,
 *    wallet) as a reflection of the current chain and UTXO set.
 *    This may either be an IBD chainstate or a snapshot chainstate.
 *
 * *Background IBD chainstate*: an IBD chainstate for which the
 *    IBD process is happening in the background while use of the
 *    active (snapshot) chainstate allows the rest of the system to function.
 */
class ChainstateManager
{
private:
    //! The chainstate used under normal operation (i.e. "regular" IBD) or, if
    //! a snapshot is in use, for background validation.
    //!
    //! Its contents (including on-disk data) will be deleted *upon shutdown*
    //! after background validation of the snapshot has completed. We do not
    //! free the chainstate contents immediately after it finishes validation
    //! to cautiously avoid a case where some other part of the system is still
    //! using this pointer (e.g. net_processing).
    //!
    //! Once this pointer is set to a corresponding chainstate, it will not
    //! be reset until init.cpp:Shutdown().
    //!
    //! It is important for the pointer to not be deleted until shutdown,
    //! because cs_main is not always held when the pointer is accessed, for
    //! example when calling ActivateBestChain, so there's no way you could
    //! prevent code from using the pointer while deleting it.
    std::unique_ptr<Chainstate> m_ibd_chainstate GUARDED_BY(::cs_main);

    //! A chainstate initialized on the basis of a UTXO snapshot. If this is
    //! non-null, it is always our active chainstate.
    //!
    //! Once this pointer is set to a corresponding chainstate, it will not
    //! be reset until init.cpp:Shutdown().
    //!
    //! It is important for the pointer to not be deleted until shutdown,
    //! because cs_main is not always held when the pointer is accessed, for
    //! example when calling ActivateBestChain, so there's no way you could
    //! prevent code from using the pointer while deleting it.
    std::unique_ptr<Chainstate> m_snapshot_chainstate GUARDED_BY(::cs_main);

    //! Points to either the ibd or snapshot chainstate; indicates our
    //! most-work chain.
    Chainstate* m_active_chainstate GUARDED_BY(::cs_main) {nullptr};

    CBlockIndex* m_best_invalid GUARDED_BY(::cs_main){nullptr};

    /** The last header for which a headerTip notification was issued. */
    CBlockIndex* m_last_notified_header GUARDED_BY(GetMutex()){nullptr};

    bool NotifyHeaderTip() LOCKS_EXCLUDED(GetMutex());

    //! Internal helper for ActivateSnapshot().
    //!
    //! De-serialization of a snapshot that is created with
    //! CreateUTXOSnapshot() in rpc/blockchain.cpp.
    //! To reduce space the serialization format of the snapshot avoids
    //! duplication of tx hashes. The code takes advantage of the guarantee by
    //! leveldb that keys are lexicographically sorted.
    [[nodiscard]] bool PopulateAndValidateSnapshot(
        Chainstate& snapshot_chainstate,
        AutoFile& coins_file,
        const node::SnapshotMetadata& metadata);

    /**
     * If a block header hasn't already been seen, call CheckBlockHeader on it, ensure
     * that it doesn't descend from an invalid block, and then add it to m_block_index.
     * Caller must set min_pow_checked=true in order to add a new header to the
     * block index (permanent memory storage), indicating that the header is
     * known to be part of a sufficiently high-work chain (anti-dos check).
     */
    bool AcceptBlockHeader(
        const CBlockHeader& block,
        BlockValidationState& state,
        CBlockIndex** ppindex,
        bool min_pow_checked) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    friend Chainstate;

    /** Most recent headers presync progress update, for rate-limiting. */
    std::chrono::time_point<std::chrono::steady_clock> m_last_presync_update GUARDED_BY(::cs_main) {};

    std::array<ThresholdConditionCache, VERSIONBITS_NUM_BITS> m_warningcache GUARDED_BY(::cs_main);

    //! Return true if a chainstate is considered usable.
    //!
    //! This is false when a background validation chainstate has completed its
    //! validation of an assumed-valid chainstate, or when a snapshot
    //! chainstate has been found to be invalid.
    bool IsUsable(const Chainstate* const cs) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        return cs && !cs->m_disabled;
    }

    //! A queue for script verifications that have to be performed by worker threads.
    CCheckQueue<CScriptCheck> m_script_check_queue;

    //! Timers and counters used for benchmarking validation in both background
    //! and active chainstates.
    SteadyClock::duration GUARDED_BY(::cs_main) time_check{};
    SteadyClock::duration GUARDED_BY(::cs_main) time_forks{};
    SteadyClock::duration GUARDED_BY(::cs_main) time_connect{};
    SteadyClock::duration GUARDED_BY(::cs_main) time_verify{};
    SteadyClock::duration GUARDED_BY(::cs_main) time_undo{};
    SteadyClock::duration GUARDED_BY(::cs_main) time_index{};
    SteadyClock::duration GUARDED_BY(::cs_main) time_total{};
    int64_t GUARDED_BY(::cs_main) num_blocks_total{0};
    SteadyClock::duration GUARDED_BY(::cs_main) time_connect_total{};
    SteadyClock::duration GUARDED_BY(::cs_main) time_flush{};
    SteadyClock::duration GUARDED_BY(::cs_main) time_chainstate{};
    SteadyClock::duration GUARDED_BY(::cs_main) time_post_connect{};

public:
    using Options = kernel::ChainstateManagerOpts;

    explicit ChainstateManager(const util::SignalInterrupt& interrupt, Options options, node::BlockManager::Options blockman_options);

    //! Function to restart active indexes; set dynamically to avoid a circular
    //! dependency on `base/index.cpp`.
    std::function<void()> restart_indexes = std::function<void()>();

    const CChainParams& GetParams() const { return m_options.chainparams; }
    const Consensus::Params& GetConsensus() const { return m_options.chainparams.GetConsensus(); }
    bool ShouldCheckBlockIndex() const;
    const arith_uint256& MinimumChainWork() const { return *Assert(m_options.minimum_chain_work); }
    const uint256& AssumedValidBlock() const { return *Assert(m_options.assumed_valid_block); }
    kernel::Notifications& GetNotifications() const { return m_options.notifications; };

    /**
     * Make various assertions about the state of the block index.
     *
     * By default this only executes fully when using the Regtest chain; see: m_options.check_block_index.
     */
    void CheckBlockIndex();

    /**
     * Alias for ::cs_main.
     * Should be used in new code to make it easier to make ::cs_main a member
     * of this class.
     * Generally, methods of this class should be annotated to require this
     * mutex. This will make calling code more verbose, but also help to:
     * - Clarify that the method will acquire a mutex that heavily affects
     *   overall performance.
     * - Force call sites to think how long they need to acquire the mutex to
     *   get consistent results.
     */
    RecursiveMutex& GetMutex() const LOCK_RETURNED(::cs_main) { return ::cs_main; }

    const util::SignalInterrupt& m_interrupt;
    const Options m_options;
    std::thread m_thread_load;
    //! A single BlockManager instance is shared across each constructed
    //! chainstate to avoid duplicating block metadata.
    node::BlockManager m_blockman;

    ValidationCache m_validation_cache;

    /**
     * Whether initial block download has ended and IsInitialBlockDownload
     * should return false from now on.
     *
     * Mutable because we need to be able to mark IsInitialBlockDownload()
     * const, which latches this for caching purposes.
     */
    mutable std::atomic<bool> m_cached_finished_ibd{false};

    /**
     * Every received block is assigned a unique and increasing identifier, so we
     * know which one to give priority in case of a fork.
     */
    /** Blocks loaded from disk are assigned id 0, so start the counter at 1. */
    int32_t nBlockSequenceId GUARDED_BY(::cs_main) = 1;
    /** Decreasing counter (used by subsequent preciousblock calls). */
    int32_t nBlockReverseSequenceId = -1;
    /** chainwork for the last block that preciousblock has been applied to. */
    arith_uint256 nLastPreciousChainwork = 0;

    // Reset the memory-only sequence counters we use to track block arrival
    // (used by tests to reset state)
    void ResetBlockSequenceCounters() EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        nBlockSequenceId = 1;
        nBlockReverseSequenceId = -1;
    }


    /**
     * In order to efficiently track invalidity of headers, we keep the set of
     * blocks which we tried to connect and found to be invalid here (ie which
     * were set to BLOCK_FAILED_VALID since the last restart). We can then
     * walk this set and check if a new header is a descendant of something in
     * this set, preventing us from having to walk m_block_index when we try
     * to connect a bad block and fail.
     *
     * While this is more complicated than marking everything which descends
     * from an invalid block as invalid at the time we discover it to be
     * invalid, doing so would require walking all of m_block_index to find all
     * descendants. Since this case should be very rare, keeping track of all
     * BLOCK_FAILED_VALID blocks in a set should be just fine and work just as
     * well.
     *
     * Because we already walk m_block_index in height-order at startup, we go
     * ahead and mark descendants of invalid blocks as FAILED_CHILD at that time,
     * instead of putting things in this set.
     */
    std::set<CBlockIndex*> m_failed_blocks;

    /** Best header we've seen so far (used for getheaders queries' starting points). */
    CBlockIndex* m_best_header GUARDED_BY(::cs_main){nullptr};

    //! The total number of bytes available for us to use across all in-memory
    //! coins caches. This will be split somehow across chainstates.
    int64_t m_total_coinstip_cache{0};
    //
    //! The total number of bytes available for us to use across all leveldb
    //! coins databases. This will be split somehow across chainstates.
    int64_t m_total_coinsdb_cache{0};

    //! Instantiate a new chainstate.
    //!
    //! @param[in] mempool              The mempool to pass to the chainstate
    //                                  constructor
    Chainstate& InitializeChainstate(CTxMemPool* mempool) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! Get all chainstates currently being used.
    std::vector<Chainstate*> GetAll();

    //! Construct and activate a Chainstate on the basis of UTXO snapshot data.
    //!
    //! Steps:
    //!
    //! - Initialize an unused Chainstate.
    //! - Load its `CoinsViews` contents from `coins_file`.
    //! - Verify that the hash of the resulting coinsdb matches the expected hash
    //!   per assumeutxo chain parameters.
    //! - Wait for our headers chain to include the base block of the snapshot.
    //! - "Fast forward" the tip of the new chainstate to the base of the snapshot,
    //!   faking nTx* block index data along the way.
    //! - Move the new chainstate to `m_snapshot_chainstate` and make it our
    //!   ChainstateActive().
    [[nodiscard]] util::Result<void> ActivateSnapshot(
        AutoFile& coins_file, const node::SnapshotMetadata& metadata, bool in_memory);

    //! Once the background validation chainstate has reached the height which
    //! is the base of the UTXO snapshot in use, compare its coins to ensure
    //! they match those expected by the snapshot.
    //!
    //! If the coins match (expected), then mark the validation chainstate for
    //! deletion and continue using the snapshot chainstate as active.
    //! Otherwise, revert to using the ibd chainstate and shutdown.
    SnapshotCompletionResult MaybeCompleteSnapshotValidation() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! Returns nullptr if no snapshot has been loaded.
    const CBlockIndex* GetSnapshotBaseBlock() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! The most-work chain.
    Chainstate& ActiveChainstate() const;
    CChain& ActiveChain() const EXCLUSIVE_LOCKS_REQUIRED(GetMutex()) { return ActiveChainstate().m_chain; }
    int ActiveHeight() const EXCLUSIVE_LOCKS_REQUIRED(GetMutex()) { return ActiveChain().Height(); }
    CBlockIndex* ActiveTip() const EXCLUSIVE_LOCKS_REQUIRED(GetMutex()) { return ActiveChain().Tip(); }

    //! The state of a background sync (for net processing)
    bool BackgroundSyncInProgress() const EXCLUSIVE_LOCKS_REQUIRED(GetMutex()) {
        return IsUsable(m_snapshot_chainstate.get()) && IsUsable(m_ibd_chainstate.get());
    }

    //! The tip of the background sync chain
    const CBlockIndex* GetBackgroundSyncTip() const EXCLUSIVE_LOCKS_REQUIRED(GetMutex()) {
        return BackgroundSyncInProgress() ? m_ibd_chainstate->m_chain.Tip() : nullptr;
    }

    node::BlockMap& BlockIndex() EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        return m_blockman.m_block_index;
    }

    /**
     * Track versionbit status
     */
    mutable VersionBitsCache m_versionbitscache;

    //! @returns true if a snapshot-based chainstate is in use. Also implies
    //!          that a background validation chainstate is also in use.
    bool IsSnapshotActive() const;

    std::optional<uint256> SnapshotBlockhash() const;

    //! Is there a snapshot in use and has it been fully validated?
    bool IsSnapshotValidated() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        return m_snapshot_chainstate && m_ibd_chainstate && m_ibd_chainstate->m_disabled;
    }

    /** Check whether we are doing an initial block download (synchronizing from disk or network) */
    bool IsInitialBlockDownload() const;

    /**
     * Import blocks from an external file
     *
     * During reindexing, this function is called for each block file (datadir/blocks/blk?????.dat).
     * It reads all blocks contained in the given file and attempts to process them (add them to the
     * block index). The blocks may be out of order within each file and across files. Often this
     * function reads a block but finds that its parent hasn't been read yet, so the block can't be
     * processed yet. The function will add an entry to the blocks_with_unknown_parent map (which is
     * passed as an argument), so that when the block's parent is later read and processed, this
     * function can re-read the child block from disk and process it.
     *
     * Because a block's parent may be in a later file, not just later in the same file, the
     * blocks_with_unknown_parent map must be passed in and out with each call. It's a multimap,
     * rather than just a map, because multiple blocks may have the same parent (when chain splits
     * or stale blocks exist). It maps from parent-hash to child-disk-position.
     *
     * This function can also be used to read blocks from user-specified block files using the
     * -loadblock= option. There's no unknown-parent tracking, so the last two arguments are omitted.
     *
     *
     * @param[in]     file_in                       File containing blocks to read
     * @param[in]     dbp                           (optional) Disk block position (only for reindex)
     * @param[in,out] blocks_with_unknown_parent    (optional) Map of disk positions for blocks with
     *                                              unknown parent, key is parent block hash
     *                                              (only used for reindex)
     * */
    void LoadExternalBlockFile(
        AutoFile& file_in,
        FlatFilePos* dbp = nullptr,
        std::multimap<uint256, FlatFilePos>* blocks_with_unknown_parent = nullptr);

    /**
     * Process an incoming block. This only returns after the best known valid
     * block is made active. Note that it does not, however, guarantee that the
     * specific block passed to it has been checked for validity!
     *
     * If you want to *possibly* get feedback on whether block is valid, you must
     * install a CValidationInterface (see validationinterface.h) - this will have
     * its BlockChecked method called whenever *any* block completes validation.
     *
     * Note that we guarantee that either the proof-of-work is valid on block, or
     * (and possibly also) BlockChecked will have been called.
     *
     * May not be called in a validationinterface callback.
     *
     * @param[in]   block The block we want to process.
     * @param[in]   force_processing Process this block even if unrequested; used for non-network block sources.
     * @param[in]   min_pow_checked  True if proof-of-work anti-DoS checks have
     *                               been done by caller for headers chain
     *                               (note: only affects headers acceptance; if
     *                               block header is already present in block
     *                               index then this parameter has no effect)
     * @param[out]  new_block A boolean which is set to indicate if the block was first received via this call
     * @returns     If the block was processed, independently of block validity
     */
    bool ProcessNewBlock(const std::shared_ptr<const CBlock>& block, bool force_processing, bool min_pow_checked, bool* new_block) LOCKS_EXCLUDED(cs_main);

    /**
     * Process incoming block headers.
     *
     * May not be called in a
     * validationinterface callback.
     *
     * @param[in]  block The block headers themselves
     * @param[in]  min_pow_checked  True if proof-of-work anti-DoS checks have been done by caller for headers chain
     * @param[out] state This may be set to an Error state if any error occurred processing them
     * @param[out] ppindex If set, the pointer will be set to point to the last new block index object for the given headers
     */
    bool ProcessNewBlockHeaders(const std::vector<CBlockHeader>& block, bool min_pow_checked, BlockValidationState& state, const CBlockIndex** ppindex = nullptr) LOCKS_EXCLUDED(cs_main);

    /**
     * Sufficiently validate a block for disk storage (and store on disk).
     *
     * @param[in]   pblock          The block we want to process.
     * @param[in]   fRequested      Whether we requested this block from a
     *                              peer.
     * @param[in]   dbp             The location on disk, if we are importing
     *                              this block from prior storage.
     * @param[in]   min_pow_checked True if proof-of-work anti-DoS checks have
     *                              been done by caller for headers chain
     *
     * @param[out]  state       The state of the block validation.
     * @param[out]  ppindex     Optional return parameter to get the
     *                          CBlockIndex pointer for this block.
     * @param[out]  fNewBlock   Optional return parameter to indicate if the
     *                          block is new to our storage.
     *
     * @returns   False if the block or header is invalid, or if saving to disk fails (likely a fatal error); true otherwise.
     */
    bool AcceptBlock(const std::shared_ptr<const CBlock>& pblock, BlockValidationState& state, CBlockIndex** ppindex, bool fRequested, const FlatFilePos* dbp, bool* fNewBlock, bool min_pow_checked) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    void ReceivedBlockTransactions(const CBlock& block, CBlockIndex* pindexNew, const FlatFilePos& pos) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /**
     * Try to add a transaction to the memory pool.
     *
     * @param[in]  tx              The transaction to submit for mempool acceptance.
     * @param[in]  test_accept     When true, run validation checks but don't submit to mempool.
     */
    [[nodiscard]] MempoolAcceptResult ProcessTransaction(const CTransactionRef& tx, bool test_accept=false)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    //! Load the block tree and coins database from disk, initializing state if we're running with -reindex
    bool LoadBlockIndex() EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    //! Check to see if caches are out of balance and if so, call
    //! ResizeCoinsCaches() as needed.
    void MaybeRebalanceCaches() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Update uncommitted block structures (currently: only the witness reserved value). This is safe for submitted blocks. */
    void UpdateUncommittedBlockStructures(CBlock& block, const CBlockIndex* pindexPrev) const;

    /** Produce the necessary coinbase commitment for a block (modifies the hash, don't call for mined blocks). */
    std::vector<unsigned char> GenerateCoinbaseCommitment(CBlock& block, const CBlockIndex* pindexPrev) const;

    /** This is used by net_processing to report pre-synchronization progress of headers, as
     *  headers are not yet fed to validation during that time, but validation is (for now)
     *  responsible for logging and signalling through NotifyHeaderTip, so it needs this
     *  information. */
    void ReportHeadersPresync(const arith_uint256& work, int64_t height, int64_t timestamp);

    //! When starting up, search the datadir for a chainstate based on a UTXO
    //! snapshot that is in the process of being validated.
    bool DetectSnapshotChainstate() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    void ResetChainstates() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! Remove the snapshot-based chainstate and all on-disk artifacts.
    //! Used when reindex{-chainstate} is called during snapshot use.
    [[nodiscard]] bool DeleteSnapshotChainstate() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! Switch the active chainstate to one based on a UTXO snapshot that was loaded
    //! previously.
    Chainstate& ActivateExistingSnapshot(uint256 base_blockhash) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! If we have validated a snapshot chain during this runtime, copy its
    //! chainstate directory over to the main `chainstate` location, completing
    //! validation of the snapshot.
    //!
    //! If the cleanup succeeds, the caller will need to ensure chainstates are
    //! reinitialized, since ResetChainstates() will be called before leveldb
    //! directories are moved or deleted.
    //!
    //! @sa node/chainstate:LoadChainstate()
    bool ValidatedSnapshotCleanup() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! @returns the chainstate that indexes should consult when ensuring that an
    //!   index is synced with a chain where we can expect block index entries to have
    //!   BLOCK_HAVE_DATA beneath the tip.
    //!
    //!   In other words, give us the chainstate for which we can reasonably expect
    //!   that all blocks beneath the tip have been indexed. In practice this means
    //!   when using an assumed-valid chainstate based upon a snapshot, return only the
    //!   fully validated chain.
    Chainstate& GetChainstateForIndexing() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! Return the [start, end] (inclusive) of block heights we can prune.
    //!
    //! start > end is possible, meaning no blocks can be pruned.
    std::pair<int, int> GetPruneRange(
        const Chainstate& chainstate, int last_height_can_prune) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! Return the height of the base block of the snapshot in use, if one exists, else
    //! nullopt.
    std::optional<int> GetSnapshotBaseHeight() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    CCheckQueue<CScriptCheck>& GetCheckQueue() { return m_script_check_queue; }

    ~ChainstateManager();
};

/** Deployment* info via ChainstateManager */
template<typename DEP>
bool DeploymentActiveAfter(const CBlockIndex* pindexPrev, const ChainstateManager& chainman, DEP dep)
{
    return DeploymentActiveAfter(pindexPrev, chainman.GetConsensus(), dep, chainman.m_versionbitscache);
}

template<typename DEP>
bool DeploymentActiveAt(const CBlockIndex& index, const ChainstateManager& chainman, DEP dep)
{
    return DeploymentActiveAt(index, chainman.GetConsensus(), dep, chainman.m_versionbitscache);
}

template<typename DEP>
bool DeploymentEnabled(const ChainstateManager& chainman, DEP dep)
{
    return DeploymentEnabled(chainman.GetConsensus(), dep);
}

/** Identifies blocks that overwrote an existing coinbase output in the UTXO set (see BIP30) */
bool IsBIP30Repeat(const CBlockIndex& block_index);

/** Identifies blocks which coinbase output was subsequently overwritten in the UTXO set (see BIP30) */
bool IsBIP30Unspendable(const CBlockIndex& block_index);

#endif // BITCOIN_VALIDATION_H
