// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VALIDATION_H
#define BITCOIN_VALIDATION_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <amount.h>
#include <coins.h>
#include <crypto/common.h> // for ReadLE64
#include <fs.h>
#include <policy/feerate.h>
#include <protocol.h> // For CMessageHeader::MessageStartChars
#include <script/script_error.h>
#include <sync.h>
#include <txmempool.h> // For CTxMemPool::cs
#include <txdb.h>
#include <versionbits.h>

#include <atomic>
#include <exception>
#include <map>
#include <memory>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

class CChainState;
class CBlockIndex;
class CBlockTreeDB;
class CBlockUndo;
class CChainParams;
class CInv;
class CConnman;
class CScriptCheck;
class CBlockPolicyEstimator;
class CTxMemPool;
class CValidationState;
struct ChainTxData;

struct DisconnectedBlockTransactions;
struct PrecomputedTransactionData;
struct LockPoints;

/** Default for -minrelaytxfee, minimum relay fee for transactions */
static const unsigned int DEFAULT_MIN_RELAY_TX_FEE = 10000;
/** Default for -limitancestorcount, max number of in-mempool ancestors */
static const unsigned int DEFAULT_ANCESTOR_LIMIT = 25;
/** Default for -limitancestorsize, maximum kilobytes of tx + all in-mempool ancestors */
static const unsigned int DEFAULT_ANCESTOR_SIZE_LIMIT = 101;
/** Default for -limitdescendantcount, max number of in-mempool descendants */
static const unsigned int DEFAULT_DESCENDANT_LIMIT = 25;
/** Default for -limitdescendantsize, maximum kilobytes of in-mempool descendants */
static const unsigned int DEFAULT_DESCENDANT_SIZE_LIMIT = 101;
/**
 * An extra transaction can be added to a package, as long as it only has one
 * ancestor and is no larger than this. Not really any reason to make this
 * configurable as it doesn't materially change DoS parameters.
 */
static const unsigned int EXTRA_DESCENDANT_TX_SIZE_LIMIT = 10000;
/** Default for -mempoolexpiry, expiration time for mempool transactions in hours */
static const unsigned int DEFAULT_MEMPOOL_EXPIRY = 168;
/** Maximum kilobytes for transactions to store for processing during reorg */
static const unsigned int MAX_DISCONNECTED_TX_POOL_SIZE = 20000;
/** The maximum size of a blk?????.dat file (since 0.8) */
static const unsigned int MAX_BLOCKFILE_SIZE = 0x8000000; // 128 MiB
/** The pre-allocation chunk size for blk?????.dat files (since 0.8) */
static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x1000000; // 16 MiB
/** The pre-allocation chunk size for rev?????.dat files (since 0.8) */
static const unsigned int UNDOFILE_CHUNK_SIZE = 0x100000; // 1 MiB

/** Maximum number of script-checking threads allowed */
static const int MAX_SCRIPTCHECK_THREADS = 16;
/** -par default (number of script-checking threads, 0 = auto) */
static const int DEFAULT_SCRIPTCHECK_THREADS = 0;
/** Number of blocks that can be requested at any given time from a single peer. */
static const int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16;
/** Timeout in seconds during which a peer must stall block download progress before being disconnected. */
static const unsigned int BLOCK_STALLING_TIMEOUT = 2;
/** Number of headers sent in one getheaders result. We rely on the assumption that if a peer sends
 *  less than this number, we reached its tip. Changing this value is a protocol upgrade. */
static const unsigned int MAX_HEADERS_RESULTS = 10000;
/** Maximum depth of blocks we're willing to serve as compact blocks to peers
 *  when requested. For older blocks, a regular BLOCK response will be sent. */
static const int MAX_CMPCTBLOCK_DEPTH = 5;
/** Maximum depth of blocks we're willing to respond to GETBLOCKTXN requests for. */
static const int MAX_BLOCKTXN_DEPTH = 10;
/** Size of the "block download window": how far ahead of our current height do we fetch?
 *  Larger windows tolerate larger download speed differences between peer, but increase the potential
 *  degree of disordering of blocks on disk (which make reindexing and pruning harder). We'll probably
 *  want to make this a per-peer adaptive value at some point. */
static const unsigned int BLOCK_DOWNLOAD_WINDOW = 1024;
/** Time to wait (in seconds) between writing blocks/block index to disk. */
static const unsigned int DATABASE_WRITE_INTERVAL = 60 * 60;
/** Time to wait (in seconds) between flushing chainstate to disk. */
static const unsigned int DATABASE_FLUSH_INTERVAL = 24 * 60 * 60;
/** Maximum length of reject messages. */
static const unsigned int MAX_REJECT_MESSAGE_LENGTH = 111;
/** Block download timeout base, expressed in millionths of the block interval (i.e. 10 min) */
static const int64_t BLOCK_DOWNLOAD_TIMEOUT_BASE = 1000000;
/** Additional block download timeout per parallel downloading peer (i.e. 5 min) */
static const int64_t BLOCK_DOWNLOAD_TIMEOUT_PER_PEER = 500000;

static const int64_t DEFAULT_MAX_TIP_AGE = 24 * 60 * 60;
/** Maximum age of our tip in seconds for us to be considered current for fee estimation */
static const int64_t MAX_FEE_ESTIMATION_TIP_AGE = 3 * 60 * 60;

static const bool DEFAULT_CHECKPOINTS_ENABLED = true;
static const bool DEFAULT_TXINDEX = false;
static const char* const DEFAULT_BLOCKFILTERINDEX = "0";
static const unsigned int DEFAULT_BANSCORE_THRESHOLD = 100;
/** Default for -persistmempool */
static const bool DEFAULT_PERSIST_MEMPOOL = true;
/** Default for using fee filter */
static const bool DEFAULT_FEEFILTER = true;

/** Default for check work */
static const bool DEFAULT_CHECKWORK_ENABLED = false;

/** Maximum number of headers to announce when relaying blocks with headers message.*/
static const unsigned int MAX_BLOCKS_TO_ANNOUNCE = 8;

/** Maximum number of unconnecting headers announcements before DoS score */
static const int MAX_UNCONNECTING_HEADERS = 10;

/** Default for -stopatheight */
static const int DEFAULT_STOPATHEIGHT = 0;

#ifdef ENABLE_OMNICORE
/** Default for -omni */
static const bool DEFAULT_OMNICORE = false;
#endif

struct BlockHasher
{
    // this used to call `GetCheapHash()` in uint256, which was later moved; the
    // cheap hash function simply calls ReadLE64() however, so the end result is
    // identical
    size_t operator()(const uint256& hash) const { return ReadLE64(hash.begin()); }
};

extern CScript COINBASE_FLAGS;
extern CCriticalSection cs_main;
extern CBlockPolicyEstimator feeEstimator;
extern CTxMemPool mempool;
typedef std::unordered_map<uint256, CBlockIndex*, BlockHasher> BlockMap;
extern Mutex g_best_block_mutex;
extern std::condition_variable g_best_block_cv;
extern uint256 g_best_block;
extern std::atomic_bool fImporting;
extern std::atomic_bool fReindex;
extern int nScriptCheckThreads;
extern bool fRequireStandard;
extern bool fCheckBlockIndex;
extern bool fCheckpointsEnabled;
extern bool fCheckWork;
extern size_t nCoinCacheUsage;
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

/** Pruning-related variables and constants */
/** True if any block files have ever been pruned. */
extern bool fHavePruned;
/** True if we're running in -prune mode. */
extern bool fPruneMode;
/** Number of MiB of block files that we're trying to stay below. */
extern uint64_t nPruneTarget;
/** Block files containing a block-height within MIN_BLOCKS_TO_KEEP of ::ChainActive().Tip() will not be pruned. */
static const unsigned int MIN_BLOCKS_TO_KEEP = 960;
/** Minimum blocks required to signal NODE_NETWORK_LIMITED */
static const unsigned int NODE_NETWORK_LIMITED_MIN_BLOCKS = 960;

static const signed int DEFAULT_CHECKBLOCKS = 6;
static const unsigned int DEFAULT_CHECKLEVEL = 3;

// Require that user allocate at least 2800MB for block & undo files (blk???.dat and rev???.dat)
// At 2MB per block, 960 blocks = 1920MB.
// Add 15% for Undo data = 2208MB
// Add 20% for Orphan block rate = 2649MB
// We want the low water mark after pruning to be at least 2649 MB and since we prune in
// full block file chunks, we need the high water mark which triggers the prune to be
// one 128MB block file + added 15% undo data = 147MB greater for a total of 2796MB
// Setting the target to > than 2800MB will make it likely we can respect the target.
static const uint64_t MIN_DISK_SPACE_FOR_BLOCK_FILES = 2800ULL * 1024 * 1024;

/**
 * Process an incoming block. This only returns after the best known valid
 * block is made active. Note that it does not, however, guarantee that the
 * specific block passed to it has been checked for validity!
 *
 * If you want to *possibly* get feedback on whether pblock is valid, you must
 * install a CValidationInterface (see validationinterface.h) - this will have
 * its BlockChecked method called whenever *any* block completes validation.
 *
 * Note that we guarantee that either the proof-of-work is valid on pblock, or
 * (and possibly also) BlockChecked will have been called.
 *
 * May not be called in a
 * validationinterface callback.
 *
 * @param[in]   pblock  The block we want to process.
 * @param[in]   fForceProcessing Process this block even if unrequested; used for non-network block sources and whitelisted peers.
 * @param[out]  fNewBlock A boolean which is set to indicate if the block was first received via this call
 * @returns     If the block was processed, independently of block validity
 */
bool ProcessNewBlock(const CChainParams& chainparams, const std::shared_ptr<const CBlock> pblock, bool fForceProcessing, bool* fNewBlock) LOCKS_EXCLUDED(cs_main);

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
 * @param[out] first_invalid First header that fails validation, if one exists
 */
bool ProcessNewBlockHeaders(const std::vector<CBlockHeader>& block, CValidationState& state, const CChainParams& chainparams, const CBlockIndex** ppindex = nullptr, CBlockHeader* first_invalid = nullptr) LOCKS_EXCLUDED(cs_main);

/** Open a block file (blk?????.dat) */
FILE* OpenBlockFile(const FlatFilePos &pos, bool fReadOnly = false);
/** Translation to a filesystem path */
fs::path GetBlockPosFilename(const FlatFilePos &pos);
/** Import blocks from an external file */
bool LoadExternalBlockFile(const CChainParams& chainparams, FILE* fileIn, FlatFilePos *dbp = nullptr);
/** Ensures we have a genesis block in the block tree, possibly writing one to disk. */
bool LoadGenesisBlock(const CChainParams& chainparams);
/** Load the block tree and coins database from disk,
 * initializing state if we're running with -reindex. */
bool LoadBlockIndex(const CChainParams& chainparams) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
/** Unload database information */
void UnloadBlockIndex();
/** Run an instance of the script checking thread */
void ThreadScriptCheck(int worker_num);
/** Retrieve a transaction (from memory pool, or from disk, if possible) */
bool GetTransaction(const uint256& hash, CTransactionRef& tx, const Consensus::Params& params, uint256& hashBlock, const CBlockIndex* const blockIndex = nullptr);
/**
 * Find the best known block, and make it the tip of the block chain
 *
 * May not be called with cs_main held. May not be called in a
 * validationinterface callback.
 */
bool ActivateBestChain(CValidationState& state, const CChainParams& chainparams, std::shared_ptr<const CBlock> pblock = std::shared_ptr<const CBlock>()) LOCKS_EXCLUDED(cs_main);
/** Get block subsidy */
CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& consensusParams) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** Get block reward */
std::vector<CTxOut> GetBlockReward(const CBlockIndex* pindexPrev, const CAmount& nFees, const CAccountID& generatorID, const uint64_t& nPlotterId, const CCoinsViewCache& view, const Consensus::Params& consensusParams) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** Guess verification progress (as a fraction between 0.0=genesis and 1.0=current tip). */
double GuessVerificationProgress(const ChainTxData& data, const CBlockIndex* pindex);

/** Calculate the amount of disk space the block & undo files currently use */
uint64_t CalculateCurrentUsage();

/**
 *  Mark one block file as pruned.
 */
void PruneOneBlockFile(const int fileNumber) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/**
 *  Actually unlink the specified files
 */
void UnlinkPrunedFiles(const std::set<int>& setFilesToPrune);

/** Prune block files up to a given height */
void PruneBlockFilesManual(int nManualPruneHeight);

/** Abort with a message */
void DoAbortNode(const std::string& strMessage, const std::string& userMessage);

/** Sends out an alert */
void DoWarning(const std::string& strWarning);

/** (try to) add transaction to memory pool
 * plTxnReplaced will be appended to with all transactions replaced from mempool **/
bool AcceptToMemoryPool(CTxMemPool& pool, CValidationState &state, const CTransactionRef &tx,
                        bool* pfMissingInputs, std::list<CTransactionRef>* plTxnReplaced,
                        bool bypass_limits, const CAmount nAbsurdFee, bool test_accept=false) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** Get the BIP9 state for a given deployment at the current tip. */
ThresholdState VersionBitsTipState(const Consensus::Params& params, Consensus::DeploymentPos pos);

/** Get the numerical statistics for the BIP9 state for a given deployment at the current tip. */
BIP9Stats VersionBitsTipStatistics(const Consensus::Params& params, Consensus::DeploymentPos pos);

/** Get the block height at which the BIP9 deployment switched into the state for the block building on the current tip. */
int VersionBitsTipStateSinceHeight(const Consensus::Params& params, Consensus::DeploymentPos pos);


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
bool CheckFinalTx(const CTransaction &tx, int flags = -1) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/**
 * Test whether the LockPoints height and time are still valid on the current chain
 */
bool TestLockPointValidity(const LockPoints* lp) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/**
 * Check if transaction will be BIP 68 final in the next block to be created.
 *
 * Simulates calling SequenceLocks() with data from the tip of the current active chain.
 * Optionally stores in LockPoints the resulting height and time calculated and the hash
 * of the block needed for calculation or skips the calculation and uses the LockPoints
 * passed in for evaluation.
 * The LockPoints should not be considered valid if CheckSequenceLocks returns false.
 *
 * See consensus/consensus.h for flag definitions.
 */
bool CheckSequenceLocks(const CTxMemPool& pool, const CTransaction& tx, int flags, LockPoints* lp = nullptr, bool useExistingLockPoints = false) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

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


/** Functions for disk access for blocks */
bool ReadBlockFromDisk(CBlock& block, const FlatFilePos& pos, const Consensus::Params& consensusParams);
bool ReadBlockFromDisk(CBlock& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams);
bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const FlatFilePos& pos, const CMessageHeader::MessageStartChars& message_start);
bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const CBlockIndex* pindex, const CMessageHeader::MessageStartChars& message_start);

bool UndoReadFromDisk(CBlockUndo& blockundo, const CBlockIndex* pindex);

/** Functions for validating blocks and updating the block tree */

/** Context-independent validity checks */
bool CheckBlock(const CBlock& block, CValidationState& state, const CChainParams& chainparams, bool fCheckWork = true, bool fCheckMerkleRoot = true);

/** Check a block is completely valid from start to finish (only works on top of our current best block) */
bool TestBlockValidity(CValidationState& state, const CChainParams& chainparams, const CBlock& block, CBlockIndex* pindexPrev, bool fCheckWork = true, bool fCheckMerkleRoot = true) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** Check whether witness commitments are required for a block, and whether to enforce NULLDUMMY (BIP 147) rules.
 *  Note that transaction witness validation rules are always enforced when P2SH is enforced. */
bool IsWitnessEnabled(const CBlockIndex* pindexPrev, const Consensus::Params& params);

/** When there are blocks in the active chain with missing data, rewind the chainstate and remove them from the block index */
bool RewindBlockIndex(const CChainParams& params) LOCKS_EXCLUDED(cs_main);

/** Compute at which vout of the block's coinbase transaction the witness commitment occurs, or -1 if not found */
int GetWitnessCommitmentIndex(const CBlock& block);

/** Update uncommitted block structures (currently: only the witness reserved value). This is safe for submitted blocks. */
void UpdateUncommittedBlockStructures(CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams);

/** Produce the necessary coinbase commitment for a block (modifies the hash, don't call for mined blocks). */
std::vector<unsigned char> GenerateCoinbaseCommitment(CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams);

/** RAII wrapper for VerifyDB: Verify consistency of the block and coin databases */
class CVerifyDB {
public:
    CVerifyDB();
    ~CVerifyDB();
    bool VerifyDB(const CChainParams& chainparams, CCoinsView *coinsview, int nCheckLevel, int nCheckDepth);
};

CBlockIndex* LookupBlockIndex(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** Find the last common block between the parameter chain and a locator. */
CBlockIndex* FindForkInGlobalIndex(const CChain& chain, const CBlockLocator& locator) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

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
class BlockManager {
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
        CBlockTreeDB& blocktree,
        std::set<CBlockIndex*, CBlockIndexWorkComparator>& block_index_candidates)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /** Clear all data members. */
    void Unload() EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    CBlockIndex* AddToBlockIndex(const CBlockHeader& block) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    /** Create a new block index entry for a given block hash */
    CBlockIndex* InsertBlockIndex(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /**
     * If a block header hasn't already been seen, call CheckBlockHeader on it, ensure
     * that it doesn't descend from an invalid block, and then add it to m_block_index.
     */
    bool AcceptBlockHeader(
        const CBlockHeader& block,
        CValidationState& state,
        const CChainParams& chainparams,
        CBlockIndex** ppindex,
        bool fCheckWork = true) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
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
 * kept in `BlockMetadataManager`.
 */
class CChainState {
private:

    /**
     * Every received block is assigned a unique and increasing identifier, so we
     * know which one to give priority in case of a fork.
     */
    CCriticalSection cs_nBlockSequenceId;
    /** Blocks loaded from disk are assigned id 0, so start the counter at 1. */
    int32_t nBlockSequenceId = 1;
    /** Decreasing counter (used by subsequent preciousblock calls). */
    int32_t nBlockReverseSequenceId = -1;
    /** chainwork for the last block that preciousblock has been applied to. */
    arith_uint256 nLastPreciousChainwork = 0;

    /**
     * the ChainState CriticalSection
     * A lock that must be held when modifying this ChainState - held in ActivateBestChain()
     */
    CCriticalSection m_cs_chainstate;

    /**
     * Whether this chainstate is undergoing initial block download.
     *
     * Mutable because we need to be able to mark IsInitialBlockDownload()
     * const, which latches this for caching purposes.
     */
    mutable std::atomic<bool> m_cached_finished_ibd{false};

    //! Reference to a BlockManager instance which itself is shared across all
    //! CChainState instances. Keeping a local reference allows us to test more
    //! easily as opposed to referencing a global.
    BlockManager& m_blockman;

    //! Manages the UTXO set, which is a reflection of the contents of `m_chain`.
    std::unique_ptr<CoinsViews> m_coins_views;

public:
    CChainState(BlockManager& blockman) : m_blockman(blockman) {}
    CChainState();

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
    void InitCoinsCache() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! @returns whether or not the CoinsViews object has been fully initialized and we can
    //!          safely flush this object to disk.
    bool CanFlushToDisk() EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
        return m_coins_views && m_coins_views->m_cacheview;
    }

    //! The current chain of blockheaders we consult and build on.
    //! @see CChain, CBlockIndex.
    CChain m_chain;

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
        const CChainParams& chainparams,
        CValidationState &state,
        FlushStateMode mode,
        int nManualPruneHeight = 0);

    //! Unconditionally flush all changes to disk.
    void ForceFlushStateToDisk();

    //! Prune blockfiles from the disk if necessary and then flush chainstate changes
    //! if we pruned.
    void PruneAndFlush();

    /**
     * Make the best chain active, in multiple steps. The result is either failure
     * or an activated best chain. pblock is either nullptr or a pointer to a block
     * that is already loaded (to avoid loading it again from disk).
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
        CValidationState& state,
        const CChainParams& chainparams,
        std::shared_ptr<const CBlock> pblock) LOCKS_EXCLUDED(cs_main);

    bool AcceptBlock(const std::shared_ptr<const CBlock>& pblock, CValidationState& state, const CChainParams& chainparams, CBlockIndex** ppindex, bool fRequested, const FlatFilePos* dbp, bool* fNewBlock) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    // Block (dis)connection on a given view:
    DisconnectResult DisconnectBlock(const CBlock& block, const CBlockIndex* pindex, CCoinsViewCache& view);
    bool ConnectBlock(const CBlock& block, CValidationState& state, CBlockIndex* pindex,
                      CCoinsViewCache& view, const CChainParams& chainparams, bool fJustCheck = false,
                      /* Track removals from the UTXO DB for this block */ std::shared_ptr<std::map<COutPoint, Coin>> removedCoins = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    // Apply the effects of a block disconnection on the UTXO set.
    bool DisconnectTip(CValidationState& state, const CChainParams& chainparams, DisconnectedBlockTransactions* disconnectpool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, ::mempool.cs);

    // Manual block validity manipulation:
    bool PreciousBlock(CValidationState& state, const CChainParams& params, CBlockIndex* pindex) LOCKS_EXCLUDED(cs_main);
    bool InvalidateBlock(CValidationState& state, const CChainParams& chainparams, CBlockIndex* pindex) LOCKS_EXCLUDED(cs_main);
    void ResetBlockFailureFlags(CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /** Replay blocks that aren't fully applied to the database. */
    bool ReplayBlocks(const CChainParams& params);
    bool RewindBlockIndex(const CChainParams& params) LOCKS_EXCLUDED(cs_main);
    bool LoadGenesisBlock(const CChainParams& chainparams);

    void PruneBlockIndexCandidates();

    void UnloadBlockIndex();

    /** Check whether we are doing an initial block download (synchronizing from disk or network) */
    bool IsInitialBlockDownload() const;

    /**
     * Make various assertions about the state of the block index.
     *
     * By default this only executes fully when using the Regtest chain; see: fCheckBlockIndex.
     */
    void CheckBlockIndex(const Consensus::Params& consensusParams);

    /** Update the chain tip based on database information, i.e. CoinsTip()'s best block. */
    bool LoadChainTip(const CChainParams& chainparams) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

private:
    bool ActivateBestChainStep(CValidationState& state, const CChainParams& chainparams, CBlockIndex* pindexMostWork, const std::shared_ptr<const CBlock>& pblock, bool& fInvalidFound, ConnectTrace& connectTrace) EXCLUSIVE_LOCKS_REQUIRED(cs_main, ::mempool.cs);
    bool ConnectTip(CValidationState& state, const CChainParams& chainparams, CBlockIndex* pindexNew, const std::shared_ptr<const CBlock>& pblock, ConnectTrace& connectTrace, DisconnectedBlockTransactions& disconnectpool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, ::mempool.cs);

    void InvalidBlockFound(CBlockIndex *pindex, const CValidationState &state) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    CBlockIndex* FindMostWorkChain() EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void ReceivedBlockTransactions(const CBlock& block, CBlockIndex* pindexNew, const FlatFilePos& pos, const Consensus::Params& consensusParams) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    bool RollforwardBlock(const CBlockIndex* pindex, CCoinsViewCache& inputs, const CChainParams& params) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    //! Mark a block as not having block data
    void EraseBlockData(CBlockIndex* index) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
};

/** Mark a block as precious and reorganize.
 *
 * May not be called in a
 * validationinterface callback.
 */
bool PreciousBlock(CValidationState& state, const CChainParams& params, CBlockIndex *pindex) LOCKS_EXCLUDED(cs_main);

/** Mark a block as invalid. */
bool InvalidateBlock(CValidationState& state, const CChainParams& chainparams, CBlockIndex* pindex) LOCKS_EXCLUDED(cs_main);

/** Remove invalidity status from a block and its descendants. */
void ResetBlockFailureFlags(CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

/** @returns the most-work valid chainstate. */
CChainState& ChainstateActive();

/** @returns the most-work chain. */
CChain& ChainActive();

/** @returns the global block index map. */
BlockMap& BlockIndex();

// Most often ::ChainstateActive() should be used instead of this, but some code
// may not be able to assume that this has been initialized yet and so must use it
// directly, e.g. init.cpp.
extern std::unique_ptr<CChainState> g_chainstate;

/** Global variable that points to the active block tree (protected by cs_main) */
extern std::unique_ptr<CBlockTreeDB> pblocktree;

/**
 * Return the spend height, which is one more than the inputs.GetBestBlock().
 * While checking, GetBestBlock() refers to the parent block. (protected by cs_main)
 * This is also true for mempool checks.
 */
int GetSpendHeight(const CCoinsViewCache& inputs);

extern VersionBitsCache versionbitscache;

/**
 * Determine what nVersion a new block should use.
 */
int32_t ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params);

/** Reject codes greater or equal to this can be returned by AcceptToMemPool
 * for transactions, to signal internal conditions. They cannot and should not
 * be sent over the P2P network.
 */
static const unsigned int REJECT_INTERNAL = 0x100;
/** Too high fee. Can not be triggered by P2P transactions */
static const unsigned int REJECT_HIGHFEE = 0x100;

/** Get block file info entry for one block file */
CBlockFileInfo* GetBlockFileInfo(size_t n);

/** Dump the mempool to disk. */
bool DumpMempool(const CTxMemPool& pool);

/** Load the mempool from disk. */
bool LoadMempool(CTxMemPool& pool);

//! Check whether the block associated with this index entry is pruned or not.
inline bool IsBlockPruned(const CBlockIndex* pblockindex)
{
    return (fHavePruned && !(pblockindex->nStatus & BLOCK_HAVE_DATA) && pblockindex->nTx > 0);
}

#endif // BITCOIN_VALIDATION_H
