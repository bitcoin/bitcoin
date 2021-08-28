// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VALIDATION_H
#define BITCOIN_VALIDATION_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <amount.h>
#include <attributes.h>
#include <coins.h>
#include <consensus/validation.h>
#include <crypto/common.h> // for ReadLE64
#include <fs.h>
#include <node/utxo_snapshot.h>
#include <policy/feerate.h>
#include <policy/packages.h>
#include <protocol.h> // For CMessageHeader::MessageStartChars
#include <script/script_error.h>
#include <sync.h>
#include <txmempool.h> // For CTxMemPool::cs
#include <txdb.h>
#include <serialize.h>
#include <util/check.h>
#include <util/hasher.h>
#include <util/translation.h>

#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdint.h>
#include <string>
#include <thread>
#include <utility>
#include <vector>

class CChainState;
class BlockValidationState;
class CBlockIndex;
class CBlockTreeDB;
class CBlockUndo;
class CChainParams;
struct CCheckpointData;
class CInv;
class CConnman;
class CScriptCheck;
class CTxMemPool;
class ChainstateManager;
struct ChainTxData;

struct DisconnectedBlockTransactions;
struct PrecomputedTransactionData;
struct LockPoints;
struct AssumeutxoData;

/** Default for -minrelaytxfee, minimum relay fee for transactions */
static const unsigned int DEFAULT_MIN_RELAY_TX_FEE = 1000;
/** Default for -limitancestorcount, max number of in-mempool ancestors */
static const unsigned int DEFAULT_ANCESTOR_LIMIT = 25;
/** Default for -limitancestorsize, maximum kilobytes of tx + all in-mempool ancestors */
static const unsigned int DEFAULT_ANCESTOR_SIZE_LIMIT = 101;
/** Default for -limitdescendantcount, max number of in-mempool descendants */
static const unsigned int DEFAULT_DESCENDANT_LIMIT = 25;
/** Default for -limitdescendantsize, maximum kilobytes of in-mempool descendants */
static const unsigned int DEFAULT_DESCENDANT_SIZE_LIMIT = 101;
/** Default for -mempoolexpiry, expiration time for mempool transactions in hours */
static const unsigned int DEFAULT_MEMPOOL_EXPIRY = 336;
/** Maximum number of dedicated script-checking threads allowed */
static const int MAX_SCRIPTCHECK_THREADS = 15;
/** -par default (number of script-checking threads, 0 = auto) */
static const int DEFAULT_SCRIPTCHECK_THREADS = 0;
static const int64_t DEFAULT_MAX_TIP_AGE = 24 * 60 * 60;
static const bool DEFAULT_CHECKPOINTS_ENABLED = true;
static const bool DEFAULT_TXINDEX = false;
static constexpr bool DEFAULT_COINSTATSINDEX{false};
static const char* const DEFAULT_BLOCKFILTERINDEX = "0";
/** Default for -persistmempool */
static const bool DEFAULT_PERSIST_MEMPOOL = true;
/** Default for -stopatheight */
static const int DEFAULT_STOPATHEIGHT = 0;
/** Block files containing a block-height within MIN_BLOCKS_TO_KEEP of ::ChainActive().Tip() will not be pruned. */
static const unsigned int MIN_BLOCKS_TO_KEEP = 288;
static const signed int DEFAULT_CHECKBLOCKS = 6;
static const unsigned int DEFAULT_CHECKLEVEL = 3;
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

extern RecursiveMutex cs_main;
typedef std::unordered_map<uint256, CBlockIndex*, BlockHasher> BlockMap;
extern Mutex g_best_block_mutex;
extern std::condition_variable g_best_block_cv;
extern uint256 g_best_block;
/** Whether there are dedicated script-checking threads running.
 * False indicates all script checking is done on the main threadMessageHandler thread.
 */
extern bool g_parallel_script_checks;
extern bool fRequireStandard;
extern bool fCheckBlockIndex;
extern bool fCheckpointsEnabled;
/** A fee rate smaller than this is considered zero fee (for relaying, mining and transaction creation) */
extern CFeeRate minRelayTxFee;
/** If the tip is older than this (in seconds), the node is considered to be in initial block download. */
extern int64_t nMaxTipAge;

/** Block hash whose ancestors we will assume to have valid scripts without checking them. */
extern uint256 hashAssumeValid;

/** Minimum work we will assume exists on some valid chain. */
extern arith_uint256 nMinimumChainWork;

/** Best header we've seen so far (used for getheaders queries' starting points). */
extern CBlockIndex *pindexBestHeader;

/** Documentation for argument 'checklevel'. */
extern const std::vector<std::string> CHECKLEVEL_DOC;

/** Unload database information */
void UnloadBlockIndex(CTxMemPool* mempool, ChainstateManager& chainman);
/** Run instances of script checking worker threads */
void StartScriptCheckWorkerThreads(int threads_num);
/** Stop all of the script checking worker threads */
void StopScriptCheckWorkerThreads();

CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& consensusParams);

bool AbortNode(BlockValidationState& state, const std::string& strMessage, const bilingual_str& userMessage = bilingual_str{});

/** Guess verification progress (as a fraction between 0.0=genesis and 1.0=current tip). */
double GuessVerificationProgress(const ChainTxData& data, const CBlockIndex* pindex);

/** Prune block files up to a given height */
void PruneBlockFilesManual(CChainState& active_chainstate, int nManualPruneHeight);

/**
* Validation result for a single transaction mempool acceptance.
*/
struct MempoolAcceptResult {
    /** Used to indicate the results of mempool validation. */
    enum class ResultType {
        VALID, //!> Fully validated, valid.
        INVALID, //!> Invalid.
    };
    const ResultType m_result_type;
    const TxValidationState m_state;

    // The following fields are only present when m_result_type = ResultType::VALID
    /** Mempool transactions replaced by the tx per BIP 125 rules. */
    const std::optional<std::list<CTransactionRef>> m_replaced_transactions;
    /** Raw base fees in satoshis. */
    const std::optional<CAmount> m_base_fees;
    static MempoolAcceptResult Failure(TxValidationState state) {
        return MempoolAcceptResult(state);
    }

    static MempoolAcceptResult Success(std::list<CTransactionRef>&& replaced_txns, CAmount fees) {
        return MempoolAcceptResult(std::move(replaced_txns), fees);
    }

// Private constructors. Use static methods MempoolAcceptResult::Success, etc. to construct.
private:
    /** Constructor for failure case */
    explicit MempoolAcceptResult(TxValidationState state)
        : m_result_type(ResultType::INVALID), m_state(state) {
            Assume(!state.IsValid()); // Can be invalid or error
        }

    /** Constructor for success case */
    explicit MempoolAcceptResult(std::list<CTransactionRef>&& replaced_txns, CAmount fees)
        : m_result_type(ResultType::VALID),
        m_replaced_transactions(std::move(replaced_txns)), m_base_fees(fees) {}
};

/**
* Validation result for package mempool acceptance.
*/
struct PackageMempoolAcceptResult
{
    const PackageValidationState m_state;
    /**
    * Map from wtxid to finished MempoolAcceptResults. The client is responsible
    * for keeping track of the transaction objects themselves. If a result is not
    * present, it means validation was unfinished for that transaction. If there
    * was a package-wide error (see result in m_state), m_tx_results will be empty.
    */
    std::map<const uint256, const MempoolAcceptResult> m_tx_results;

    explicit PackageMempoolAcceptResult(PackageValidationState state,
                                        std::map<const uint256, const MempoolAcceptResult>&& results)
        : m_state{state}, m_tx_results(std::move(results)) {}

    /** Constructor to create a PackageMempoolAcceptResult from a single MempoolAcceptResult */
    explicit PackageMempoolAcceptResult(const uint256& wtxid, const MempoolAcceptResult& result)
        : m_tx_results{ {wtxid, result} } {}
};

/**
 * (Try to) add a transaction to the memory pool.
 * @param[in]  bypass_limits   When true, don't enforce mempool fee limits.
 * @param[in]  test_accept     When true, run validation checks but don't submit to mempool.
 */
MempoolAcceptResult AcceptToMemoryPool(CChainState& active_chainstate, CTxMemPool& pool, const CTransactionRef& tx,
                                       bool bypass_limits, bool test_accept=false) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/**
* Atomically test acceptance of a package. If the package only contains one tx, package rules still
* apply. Package validation does not allow BIP125 replacements, so the transaction(s) cannot spend
* the same inputs as any transaction in the mempool.
* @param[in]    txns                Group of transactions which may be independent or contain
*                                   parent-child dependencies. The transactions must not conflict
*                                   with each other, i.e., must not spend the same inputs. If any
*                                   dependencies exist, parents must appear anywhere in the list
*                                   before their children.
* @returns a PackageMempoolAcceptResult which includes a MempoolAcceptResult for each transaction.
* If a transaction fails, validation will exit early and some results may be missing.
*/
PackageMempoolAcceptResult ProcessNewPackage(CChainState& active_chainstate, CTxMemPool& pool,
                                                   const Package& txns, bool test_accept)
                                                   EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** Apply the effects of this transaction on the UTXO set represented by view */
void UpdateCoins(const CTransaction& tx, CCoinsViewCache& inputs, int nHeight);

/** Transaction validation functions */

/**
 * Check if transaction will be final in the next block to be created.
 *
 * Calls IsFinalTx() with current block height and appropriate block time.
 *
 * See consensus/consensus.h for flag definitions.
 */
bool CheckFinalTx(const CBlockIndex* active_chain_tip, const CTransaction &tx, int flags = -1) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/**
 * Test whether the LockPoints height and time are still valid on the current chain
 */
bool TestLockPointValidity(CChain& active_chain, const LockPoints* lp) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/**
 * Check if transaction will be BIP68 final in the next block to be created on top of tip.
 * @param[in]   tip             Chain tip to check tx sequence locks against. For example,
 *                              the tip of the current active chain.
 * @param[in]   coins_view      Any CCoinsView that provides access to the relevant coins for
 *                              checking sequence locks. For example, it can be a CCoinsViewCache
 *                              that isn't connected to anything but contains all the relevant
 *                              coins, or a CCoinsViewMemPool that is connected to the
 *                              mempool and chainstate UTXO set. In the latter case, the caller is
 *                              responsible for holding the appropriate locks to ensure that
 *                              calls to GetCoin() return correct coins.
 * Simulates calling SequenceLocks() with data from the tip passed in.
 * Optionally stores in LockPoints the resulting height and time calculated and the hash
 * of the block needed for calculation or skips the calculation and uses the LockPoints
 * passed in for evaluation.
 * The LockPoints should not be considered valid if CheckSequenceLocks returns false.
 *
 * See consensus/consensus.h for flag definitions.
 */
bool CheckSequenceLocks(CBlockIndex* tip,
                        const CCoinsView& coins_view,
                        const CTransaction& tx,
                        int flags,
                        LockPoints* lp = nullptr,
                        bool useExistingLockPoints = false);

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
    ScriptError error;
    PrecomputedTransactionData *txdata;

public:
    CScriptCheck(): ptxTo(nullptr), nIn(0), nFlags(0), cacheStore(false), error(SCRIPT_ERR_UNKNOWN_ERROR) {}
    CScriptCheck(const CTxOut& outIn, const CTransaction& txToIn, unsigned int nInIn, unsigned int nFlagsIn, bool cacheIn, PrecomputedTransactionData* txdataIn) :
        m_tx_out(outIn), ptxTo(&txToIn), nIn(nInIn), nFlags(nFlagsIn), cacheStore(cacheIn), error(SCRIPT_ERR_UNKNOWN_ERROR), txdata(txdataIn) { }

    bool operator()();

    void swap(CScriptCheck &check) {
        std::swap(ptxTo, check.ptxTo);
        std::swap(m_tx_out, check.m_tx_out);
        std::swap(nIn, check.nIn);
        std::swap(nFlags, check.nFlags);
        std::swap(cacheStore, check.cacheStore);
        std::swap(error, check.error);
        std::swap(txdata, check.txdata);
    }

    ScriptError GetScriptError() const { return error; }
};

/** Initializes the script-execution cache */
void InitScriptExecutionCache();

/** Functions for validating blocks and updating the block tree */

/** Context-independent validity checks */
bool CheckBlock(const CBlock& block, BlockValidationState& state, const Consensus::Params& consensusParams, bool fCheckPOW = true, bool fCheckMerkleRoot = true);

/** Check a block is completely valid from start to finish (only works on top of our current best block) */
bool TestBlockValidity(BlockValidationState& state,
                       const CChainParams& chainparams,
                       CChainState& chainstate,
                       const CBlock& block,
                       CBlockIndex* pindexPrev,
                       bool fCheckPOW = true,
                       bool fCheckMerkleRoot = true) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** Update uncommitted block structures (currently: only the witness reserved value). This is safe for submitted blocks. */
void UpdateUncommittedBlockStructures(CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams);

/** Produce the necessary coinbase commitment for a block (modifies the hash, don't call for mined blocks). */
std::vector<unsigned char> GenerateCoinbaseCommitment(CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams);

/** RAII wrapper for VerifyDB: Verify consistency of the block and coin databases */
class CVerifyDB {
public:
    CVerifyDB();
    ~CVerifyDB();
    bool VerifyDB(
        CChainState& chainstate,
        const CChainParams& chainparams,
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

/** @see CChainState::FlushStateToDisk */
enum class FlushStateMode {
    NONE,
    IF_NEEDED,
    PERIODIC,
    ALWAYS
};

struct CBlockIndexWorkComparator
{
    bool operator()(const CBlockIndex *pa, const CBlockIndex *pb) const;
};

/**
 * Maintains a tree of blocks (stored in `m_block_index`) which is consulted
 * to determine where the most-work tip is.
 *
 * This data is used mostly in `CChainState` - information about, e.g.,
 * candidate tips is not maintained here.
 */
class BlockManager
{
    friend CChainState;

private:
    /* Calculate the block/rev files to delete based on height specified by user with RPC command pruneblockchain */
    void FindFilesToPruneManual(std::set<int>& setFilesToPrune, int nManualPruneHeight, int chain_tip_height);

    /**
     * Prune block and undo files (blk???.dat and undo???.dat) so that the disk space used is less than a user-defined target.
     * The user sets the target (in MB) on the command line or in config file.  This will be run on startup and whenever new
     * space is allocated in a block or undo file, staying below the target. Changing back to unpruned requires a reindex
     * (which in this case means the blockchain must be re-downloaded.)
     *
     * Pruning functions are called from FlushStateToDisk when the global fCheckForPruning flag has been set.
     * Block and undo files are deleted in lock-step (when blk00003.dat is deleted, so is rev00003.dat.)
     * Pruning cannot take place until the longest chain is at least a certain length (100000 on mainnet, 1000 on testnet, 1000 on regtest).
     * Pruning will never delete a block within a defined distance (currently 288) from the active chain's tip.
     * The block index is updated by unsetting HAVE_DATA and HAVE_UNDO for any blocks that were stored in the deleted files.
     * A db flag records the fact that at least some block files have been pruned.
     *
     * @param[out]   setFilesToPrune   The set of file indices that can be unlinked will be returned
     */
    void FindFilesToPrune(std::set<int>& setFilesToPrune, uint64_t nPruneAfterHeight, int chain_tip_height, int prune_height, bool is_ibd);

public:
    BlockMap m_block_index GUARDED_BY(cs_main);

    /** In order to efficiently track invalidity of headers, we keep the set of
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

    /**
     * All pairs A->B, where A (or one of its ancestors) misses transactions, but B has transactions.
     * Pruned nodes may have entries where B is missing data.
     */
    std::multimap<CBlockIndex*, CBlockIndex*> m_blocks_unlinked;

    std::unique_ptr<CBlockTreeDB> m_block_tree_db GUARDED_BY(::cs_main);

    bool LoadBlockIndexDB(std::set<CBlockIndex*, CBlockIndexWorkComparator>& setBlockIndexCandidates) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /**
     * Load the blocktree off disk and into memory. Populate certain metadata
     * per index entry (nStatus, nChainWork, nTimeMax, etc.) as well as peripheral
     * collections like setDirtyBlockIndex.
     *
     * @param[out] block_index_candidates  Fill this set with any valid blocks for
     *                                     which we've downloaded all transactions.
     */
    bool LoadBlockIndex(
        const Consensus::Params& consensus_params,
        std::set<CBlockIndex*, CBlockIndexWorkComparator>& block_index_candidates)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /** Clear all data members. */
    void Unload() EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    CBlockIndex* AddToBlockIndex(const CBlockHeader& block) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    /** Create a new block index entry for a given block hash */
    CBlockIndex* InsertBlockIndex(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    //! Mark one block file as pruned (modify associated database entries)
    void PruneOneBlockFile(const int fileNumber) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /**
     * If a block header hasn't already been seen, call CheckBlockHeader on it, ensure
     * that it doesn't descend from an invalid block, and then add it to m_block_index.
     */
    bool AcceptBlockHeader(
        const CBlockHeader& block,
        BlockValidationState& state,
        const CChainParams& chainparams,
        CBlockIndex** ppindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    CBlockIndex* LookupBlockIndex(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /** Find the last common block between the parameter chain and a locator. */
    CBlockIndex* FindForkInGlobalIndex(const CChain& chain, const CBlockLocator& locator) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    //! Returns last CBlockIndex* that is a checkpoint
    CBlockIndex* GetLastCheckpoint(const CCheckpointData& data) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /**
     * Return the spend height, which is one more than the inputs.GetBestBlock().
     * While checking, GetBestBlock() refers to the parent block. (protected by cs_main)
     * This is also true for mempool checks.
     */
    int GetSpendHeight(const CCoinsViewCache& inputs) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    ~BlockManager() {
        Unload();
    }
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
    CoinsViews(std::string ldb_name, size_t cache_size_bytes, bool in_memory, bool should_wipe);

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
 * CChainState stores and provides an API to update our local knowledge of the
 * current best chain.
 *
 * Eventually, the API here is targeted at being exposed externally as a
 * consumable libconsensus library, so any functions added must only call
 * other class member functions, pure functions in other parts of the consensus
 * library, callbacks via the validation interface, or read/write-to-disk
 * functions (eventually this will also be via callbacks).
 *
 * Anything that is contingent on the current tip of the chain is stored here,
 * whereas block information and metadata independent of the current tip is
 * kept in `BlockManager`.
 */
class CChainState
{
protected:
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

    /**
     * the ChainState CriticalSection
     * A lock that must be held when modifying this ChainState - held in ActivateBestChain()
     */
    RecursiveMutex m_cs_chainstate;

    /**
     * Whether this chainstate is undergoing initial block download.
     *
     * Mutable because we need to be able to mark IsInitialBlockDownload()
     * const, which latches this for caching purposes.
     */
    mutable std::atomic<bool> m_cached_finished_ibd{false};

    //! Optional mempool that is kept in sync with the chain.
    //! Only the active chainstate has a mempool.
    CTxMemPool* m_mempool;

    const CChainParams& m_params;

    //! Manages the UTXO set, which is a reflection of the contents of `m_chain`.
    std::unique_ptr<CoinsViews> m_coins_views;

public:
    //! Reference to a BlockManager instance which itself is shared across all
    //! CChainState instances.
    BlockManager& m_blockman;

    explicit CChainState(
        CTxMemPool* mempool,
        BlockManager& blockman,
        std::optional<uint256> from_snapshot_blockhash = std::nullopt);

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
        std::string leveldb_name = "chainstate");

    //! Initialize the in-memory coins cache (to be done after the health of the on-disk database
    //! is verified).
    void InitCoinsCache(size_t cache_size_bytes) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! @returns whether or not the CoinsViews object has been fully initialized and we can
    //!          safely flush this object to disk.
    bool CanFlushToDisk() const EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
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
     * The set of all CBlockIndex entries with BLOCK_VALID_TRANSACTIONS (for itself and all ancestors) and
     * as good as our current tip or better. Entries may be failed, though, and pruning nodes may be
     * missing the data for the block.
     */
    std::set<CBlockIndex*, CBlockIndexWorkComparator> setBlockIndexCandidates;

    //! @returns A reference to the in-memory cache of the UTXO set.
    CCoinsViewCache& CoinsTip() EXCLUSIVE_LOCKS_REQUIRED(cs_main)
    {
        assert(m_coins_views->m_cacheview);
        return *m_coins_views->m_cacheview.get();
    }

    //! @returns A reference to the on-disk UTXO set database.
    CCoinsViewDB& CoinsDB() EXCLUSIVE_LOCKS_REQUIRED(cs_main)
    {
        return m_coins_views->m_dbview;
    }

    //! @returns A reference to a wrapped view of the in-memory UTXO set that
    //!     handles disk read errors gracefully.
    CCoinsViewErrorCatcher& CoinsErrorCatcher() EXCLUSIVE_LOCKS_REQUIRED(cs_main)
    {
        return m_coins_views->m_catcherview;
    }

    //! Destructs all objects related to accessing the UTXO set.
    void ResetCoinsViews() { m_coins_views.reset(); }

    //! The cache size of the on-disk coins view.
    size_t m_coinsdb_cache_size_bytes{0};

    //! The cache size of the in-memory coins view.
    size_t m_coinstip_cache_size_bytes{0};

    //! Resize the CoinsViews caches dynamically and flush state to disk.
    //! @returns true unless an error occurred during the flush.
    bool ResizeCoinsCaches(size_t coinstip_size, size_t coinsdb_size)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Import blocks from an external file */
    void LoadExternalBlockFile(FILE* fileIn, FlatFilePos* dbp = nullptr);

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
     * @returns true unless a system error occurred
     */
    bool ActivateBestChain(
        BlockValidationState& state,
        std::shared_ptr<const CBlock> pblock = nullptr) LOCKS_EXCLUDED(cs_main);

    bool AcceptBlock(const std::shared_ptr<const CBlock>& pblock, BlockValidationState& state, CBlockIndex** ppindex, bool fRequested, const FlatFilePos* dbp, bool* fNewBlock) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    // Block (dis)connection on a given view:
    DisconnectResult DisconnectBlock(const CBlock& block, const CBlockIndex* pindex, CCoinsViewCache& view);
    bool ConnectBlock(const CBlock& block, BlockValidationState& state, CBlockIndex* pindex,
                      CCoinsViewCache& view, bool fJustCheck = false) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    // Apply the effects of a block disconnection on the UTXO set.
    bool DisconnectTip(BlockValidationState& state, DisconnectedBlockTransactions* disconnectpool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_mempool->cs);

    // Manual block validity manipulation:
    /** Mark a block as precious and reorganize.
     *
     * May not be called in a validationinterface callback.
     */
    bool PreciousBlock(BlockValidationState& state, CBlockIndex* pindex) LOCKS_EXCLUDED(cs_main);
    /** Mark a block as invalid. */
    bool InvalidateBlock(BlockValidationState& state, CBlockIndex* pindex) LOCKS_EXCLUDED(cs_main);
    /** Remove invalidity status from a block and its descendants. */
    void ResetBlockFailureFlags(CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /** Replay blocks that aren't fully applied to the database. */
    bool ReplayBlocks();

    /** Whether the chain state needs to be redownloaded due to lack of witness data */
    [[nodiscard]] bool NeedsRedownload() const EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    /** Ensures we have a genesis block in the block tree, possibly writing one to disk. */
    bool LoadGenesisBlock();

    void PruneBlockIndexCandidates();

    void UnloadBlockIndex() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Check whether we are doing an initial block download (synchronizing from disk or network) */
    bool IsInitialBlockDownload() const;

    /**
     * Make various assertions about the state of the block index.
     *
     * By default this only executes fully when using the Regtest chain; see: fCheckBlockIndex.
     */
    void CheckBlockIndex();

    /** Load the persisted mempool from disk */
    void LoadMempool(const ArgsManager& args);

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

private:
    bool ActivateBestChainStep(BlockValidationState& state, CBlockIndex* pindexMostWork, const std::shared_ptr<const CBlock>& pblock, bool& fInvalidFound, ConnectTrace& connectTrace) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_mempool->cs);
    bool ConnectTip(BlockValidationState& state, CBlockIndex* pindexNew, const std::shared_ptr<const CBlock>& pblock, ConnectTrace& connectTrace, DisconnectedBlockTransactions& disconnectpool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_mempool->cs);

    void InvalidBlockFound(CBlockIndex* pindex, const BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    CBlockIndex* FindMostWorkChain() EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void ReceivedBlockTransactions(const CBlock& block, CBlockIndex* pindexNew, const FlatFilePos& pos) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    bool RollforwardBlock(const CBlockIndex* pindex, CCoinsViewCache& inputs) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    void CheckForkWarningConditions() EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void InvalidChainFound(CBlockIndex* pindexNew) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    //! Indirection necessary to make lock annotations work with an optional mempool.
    RecursiveMutex* MempoolMutex() const LOCK_RETURNED(m_mempool->cs)
    {
        return m_mempool ? &m_mempool->cs : nullptr;
    }

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

    friend ChainstateManager;
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
 *
 * *Validated chainstate*: the most-work chainstate which has been validated
 *   locally via initial block download. This will be the snapshot chainstate
 *   if a snapshot was loaded and all blocks up to the snapshot starting point
 *   have been downloaded and validated (via background validation), otherwise
 *   it will be the IBD chainstate.
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
    //! This is especially important when, e.g., calling ActivateBestChain()
    //! on all chainstates because we are not able to hold ::cs_main going into
    //! that call.
    std::unique_ptr<CChainState> m_ibd_chainstate GUARDED_BY(::cs_main);

    //! A chainstate initialized on the basis of a UTXO snapshot. If this is
    //! non-null, it is always our active chainstate.
    //!
    //! Once this pointer is set to a corresponding chainstate, it will not
    //! be reset until init.cpp:Shutdown().
    //!
    //! This is especially important when, e.g., calling ActivateBestChain()
    //! on all chainstates because we are not able to hold ::cs_main going into
    //! that call.
    std::unique_ptr<CChainState> m_snapshot_chainstate GUARDED_BY(::cs_main);

    //! Points to either the ibd or snapshot chainstate; indicates our
    //! most-work chain.
    //!
    //! Once this pointer is set to a corresponding chainstate, it will not
    //! be reset until init.cpp:Shutdown().
    //!
    //! This is especially important when, e.g., calling ActivateBestChain()
    //! on all chainstates because we are not able to hold ::cs_main going into
    //! that call.
    CChainState* m_active_chainstate GUARDED_BY(::cs_main) {nullptr};

    //! If true, the assumed-valid chainstate has been fully validated
    //! by the background validation chainstate.
    bool m_snapshot_validated{false};

    //! Internal helper for ActivateSnapshot().
    [[nodiscard]] bool PopulateAndValidateSnapshot(
        CChainState& snapshot_chainstate,
        CAutoFile& coins_file,
        const SnapshotMetadata& metadata);

public:
    std::thread m_load_block;
    //! A single BlockManager instance is shared across each constructed
    //! chainstate to avoid duplicating block metadata.
    BlockManager m_blockman GUARDED_BY(::cs_main);

    //! The total number of bytes available for us to use across all in-memory
    //! coins caches. This will be split somehow across chainstates.
    int64_t m_total_coinstip_cache{0};
    //
    //! The total number of bytes available for us to use across all leveldb
    //! coins databases. This will be split somehow across chainstates.
    int64_t m_total_coinsdb_cache{0};

    //! Instantiate a new chainstate and assign it based upon whether it is
    //! from a snapshot.
    //!
    //! @param[in] mempool              The mempool to pass to the chainstate
    //                                  constructor
    //! @param[in] snapshot_blockhash   If given, signify that this chainstate
    //!                                 is based on a snapshot.
    CChainState& InitializeChainstate(
        CTxMemPool* mempool,
        const std::optional<uint256>& snapshot_blockhash = std::nullopt)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! Get all chainstates currently being used.
    std::vector<CChainState*> GetAll();

    //! Construct and activate a Chainstate on the basis of UTXO snapshot data.
    //!
    //! Steps:
    //!
    //! - Initialize an unused CChainState.
    //! - Load its `CoinsViews` contents from `coins_file`.
    //! - Verify that the hash of the resulting coinsdb matches the expected hash
    //!   per assumeutxo chain parameters.
    //! - Wait for our headers chain to include the base block of the snapshot.
    //! - "Fast forward" the tip of the new chainstate to the base of the snapshot,
    //!   faking nTx* block index data along the way.
    //! - Move the new chainstate to `m_snapshot_chainstate` and make it our
    //!   ChainstateActive().
    [[nodiscard]] bool ActivateSnapshot(
        CAutoFile& coins_file, const SnapshotMetadata& metadata, bool in_memory);

    //! The most-work chain.
    CChainState& ActiveChainstate() const;
    CChain& ActiveChain() const { return ActiveChainstate().m_chain; }
    int ActiveHeight() const { return ActiveChain().Height(); }
    CBlockIndex* ActiveTip() const { return ActiveChain().Tip(); }

    BlockMap& BlockIndex() EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        return m_blockman.m_block_index;
    }

    //! @returns true if a snapshot-based chainstate is in use. Also implies
    //!          that a background validation chainstate is also in use.
    bool IsSnapshotActive() const;

    std::optional<uint256> SnapshotBlockhash() const;

    //! Is there a snapshot in use and has it been fully validated?
    bool IsSnapshotValidated() const { return m_snapshot_validated; }

    //! @returns true if this chainstate is being used to validate an active
    //!          snapshot in the background.
    bool IsBackgroundIBD(CChainState* chainstate) const;

    //! Return the most-work chainstate that has been fully validated.
    //!
    //! During background validation of a snapshot, this is the IBD chain. After
    //! background validation has completed, this is the snapshot chain.
    CChainState& ValidatedChainstate() const;

    CChain& ValidatedChain() const { return ValidatedChainstate().m_chain; }
    CBlockIndex* ValidatedTip() const { return ValidatedChain().Tip(); }

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
     * @param[out]  new_block A boolean which is set to indicate if the block was first received via this call
     * @returns     If the block was processed, independently of block validity
     */
    bool ProcessNewBlock(const CChainParams& chainparams, const std::shared_ptr<const CBlock>& block, bool force_processing, bool* new_block) LOCKS_EXCLUDED(cs_main);

    /**
     * Process incoming block headers.
     *
     * May not be called in a
     * validationinterface callback.
     *
     * @param[in]  block The block headers themselves
     * @param[out] state This may be set to an Error state if any error occurred processing them
     * @param[in]  chainparams The params for the chain we want to connect to
     * @param[out] ppindex If set, the pointer will be set to point to the last new block index object for the given headers
     */
    bool ProcessNewBlockHeaders(const std::vector<CBlockHeader>& block, BlockValidationState& state, const CChainParams& chainparams, const CBlockIndex** ppindex = nullptr) LOCKS_EXCLUDED(cs_main);

    //! Load the block tree and coins database from disk, initializing state if we're running with -reindex
    bool LoadBlockIndex() EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    //! Unload block index and chain data before shutdown.
    void Unload() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! Clear (deconstruct) chainstate data.
    void Reset();

    //! Check to see if caches are out of balance and if so, call
    //! ResizeCoinsCaches() as needed.
    void MaybeRebalanceCaches() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    ~ChainstateManager() {
        LOCK(::cs_main);
        UnloadBlockIndex(/* mempool */ nullptr, *this);
        Reset();
    }
};

using FopenFn = std::function<FILE*(const fs::path&, const char*)>;

/** Dump the mempool to disk. */
bool DumpMempool(const CTxMemPool& pool, FopenFn mockable_fopen_function = fsbridge::fopen, bool skip_file_commit = false);

/** Load the mempool from disk. */
bool LoadMempool(CTxMemPool& pool, CChainState& active_chainstate, FopenFn mockable_fopen_function = fsbridge::fopen);

/**
 * Return the expected assumeutxo value for a given height, if one exists.
 *
 * @param[in] height Get the assumeutxo value for this height.
 *
 * @returns empty if no assumeutxo configuration exists for the given height.
 */
const AssumeutxoData* ExpectedAssumeutxo(const int height, const CChainParams& params);

#endif // BITCOIN_VALIDATION_H
