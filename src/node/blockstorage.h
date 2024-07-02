// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCKSTORAGE_H
#define BITCOIN_NODE_BLOCKSTORAGE_H

#include <attributes.h>
#include <chain.h>
#include <dbwrapper.h>
#include <flatfile.h>
#include <kernel/blockmanager_opts.h>
#include <kernel/chainparams.h>
#include <kernel/cs_main.h>
#include <kernel/messagestartchars.h>
#include <primitives/block.h>
#include <streams.h>
#include <sync.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/hasher.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class BlockValidationState;
class CBlockUndo;
class Chainstate;
class ChainstateManager;
namespace Consensus {
struct Params;
}
namespace node {
struct PruneLockInfo;
};
namespace util {
class SignalInterrupt;
} // namespace util

namespace kernel {
/** Access to the block database (blocks/index/) */
class BlockTreeDB : public CDBWrapper
{
public:
    using CDBWrapper::CDBWrapper;
    bool WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo*>>& fileInfo, int nLastFile, const std::vector<const CBlockIndex*>& blockinfo, const std::unordered_map<std::string, node::PruneLockInfo>& prune_locks);
    bool ReadBlockFileInfo(int nFile, CBlockFileInfo& info);
    bool ReadLastBlockFile(int& nFile);
    bool WriteReindexing(bool fReindexing);
    void ReadReindexing(bool& fReindexing);
    bool WritePruneLock(const std::string& name, const node::PruneLockInfo&);
    bool DeletePruneLock(const std::string& name);
    bool WriteFlag(const std::string& name, bool fValue);
    bool ReadFlag(const std::string& name, bool& fValue);
    bool LoadBlockIndexGuts(const Consensus::Params& consensusParams, std::function<CBlockIndex*(const uint256&)> insertBlockIndex, const util::SignalInterrupt& interrupt)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool LoadPruneLocks(std::unordered_map<std::string, node::PruneLockInfo>& prune_locks, const util::SignalInterrupt& interrupt);
};
} // namespace kernel

namespace node {
using kernel::BlockTreeDB;

/** The pre-allocation chunk size for blk?????.dat files (since 0.8) */
static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x1000000; // 16 MiB
/** The pre-allocation chunk size for rev?????.dat files (since 0.8) */
static const unsigned int UNDOFILE_CHUNK_SIZE = 0x100000; // 1 MiB
/** The maximum size of a blk?????.dat file (since 0.8) */
static const unsigned int MAX_BLOCKFILE_SIZE = 0x8000000; // 128 MiB

/** Size of header written by WriteBlockToDisk before a serialized CBlock */
static constexpr size_t BLOCK_SERIALIZATION_HEADER_SIZE = std::tuple_size_v<MessageStartChars> + sizeof(unsigned int);

// Because validation code takes pointers to the map's CBlockIndex objects, if
// we ever switch to another associative container, we need to either use a
// container that has stable addressing (true of all std associative
// containers), or make the key a `std::unique_ptr<CBlockIndex>`
using BlockMap = std::unordered_map<uint256, CBlockIndex, BlockHasher>;

struct CBlockIndexWorkComparator {
    bool operator()(const CBlockIndex* pa, const CBlockIndex* pb) const;
};

struct CBlockIndexHeightOnlyComparator {
    /* Only compares the height of two block indices, doesn't try to tie-break */
    bool operator()(const CBlockIndex* pa, const CBlockIndex* pb) const;
};

struct PruneLockInfo {
    std::string desc; //! Arbitrary human-readable description of the lock purpose
    uint64_t height_first{std::numeric_limits<uint64_t>::max()}; //! Height of earliest block that should be kept and not pruned
    uint64_t height_last{std::numeric_limits<uint64_t>::max()}; //! Height of latest block that should be kept and not pruned
    bool temporary{true};

    SERIALIZE_METHODS(PruneLockInfo, obj)
    {
        READWRITE(obj.desc);
        READWRITE(VARINT(obj.height_first));
        READWRITE(VARINT(obj.height_last));
    }
};

enum BlockfileType {
    // Values used as array indexes - do not change carelessly.
    NORMAL = 0,
    ASSUMED = 1,
    NUM_TYPES = 2,
};

std::ostream& operator<<(std::ostream& os, const BlockfileType& type);

struct BlockfileCursor {
    // The latest blockfile number.
    int file_num{0};

    // Track the height of the highest block in file_num whose undo
    // data has been written. Block data is written to block files in download
    // order, but is written to undo files in validation order, which is
    // usually in order by height. To avoid wasting disk space, undo files will
    // be trimmed whenever the corresponding block file is finalized and
    // the height of the highest block written to the block file equals the
    // height of the highest block written to the undo file. This is a
    // heuristic and can sometimes preemptively trim undo files that will write
    // more data later, and sometimes fail to trim undo files that can't have
    // more data written later.
    int undo_height{0};
};

std::ostream& operator<<(std::ostream& os, const BlockfileCursor& cursor);


/**
 * Maintains a tree of blocks (stored in `m_block_index`) which is consulted
 * to determine where the most-work tip is.
 *
 * This data is used mostly in `Chainstate` - information about, e.g.,
 * candidate tips is not maintained here.
 */
class BlockManager
{
    friend Chainstate;
    friend ChainstateManager;

private:
    const CChainParams& GetParams() const { return m_opts.chainparams; }
    const Consensus::Params& GetConsensus() const { return m_opts.chainparams.GetConsensus(); }
    /**
     * Load the blocktree off disk and into memory. Populate certain metadata
     * per index entry (nStatus, nChainWork, nTimeMax, etc.) as well as peripheral
     * collections like m_dirty_blockindex.
     */
    bool LoadBlockIndex(const std::optional<uint256>& snapshot_blockhash)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /** Return false if block file or undo file flushing fails. */
    [[nodiscard]] bool FlushBlockFile(int blockfile_num, bool fFinalize, bool finalize_undo);

    /** Return false if undo file flushing fails. */
    [[nodiscard]] bool FlushUndoFile(int block_file, bool finalize = false);

    /**
     * Helper function performing various preparations before a block can be saved to disk:
     * Returns the correct position for the block to be saved, which may be in the current or a new
     * block file depending on nAddSize. May flush the previous blockfile to disk if full, updates
     * blockfile info, and checks if there is enough disk space to save the block.
     *
     * The nAddSize argument passed to this function should include not just the size of the serialized CBlock, but also the size of
     * separator fields which are written before it by WriteBlockToDisk (BLOCK_SERIALIZATION_HEADER_SIZE).
     */
    [[nodiscard]] FlatFilePos FindNextBlockPos(unsigned int nAddSize, unsigned int nHeight, uint64_t nTime);
    [[nodiscard]] bool FlushChainstateBlockFile(int tip_height);
    bool FindUndoPos(BlockValidationState& state, int nFile, FlatFilePos& pos, unsigned int nAddSize);

    FlatFileSeq BlockFileSeq() const;
    FlatFileSeq UndoFileSeq() const;

    AutoFile OpenUndoFile(const FlatFilePos& pos, bool fReadOnly = false) const;

    /**
     * Write a block to disk. The pos argument passed to this function is modified by this call. Before this call, it should
     * point to an unused file location where separator fields will be written, followed by the serialized CBlock data.
     * After this call, it will point to the beginning of the serialized CBlock data, after the separator fields
     * (BLOCK_SERIALIZATION_HEADER_SIZE)
     */
    bool WriteBlockToDisk(const CBlock& block, FlatFilePos& pos) const;
    bool UndoWriteToDisk(const CBlockUndo& blockundo, FlatFilePos& pos, const uint256& hashBlock) const;

    bool DoPruneLocksForbidPruning(const CBlockFileInfo& block_file_info) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /* Calculate the block/rev files to delete based on height specified by user with RPC command pruneblockchain */
    void FindFilesToPruneManual(
        std::set<int>& setFilesToPrune,
        int nManualPruneHeight,
        const Chainstate& chain,
        ChainstateManager& chainman);

    /**
     * Prune block and undo files (blk???.dat and rev???.dat) so that the disk space used is less than a user-defined target.
     * The user sets the target (in MB) on the command line or in config file.  This will be run on startup and whenever new
     * space is allocated in a block or undo file, staying below the target. Changing back to unpruned requires a reindex
     * (which in this case means the blockchain must be re-downloaded.)
     *
     * Pruning functions are called from FlushStateToDisk when the m_check_for_pruning flag has been set.
     * Block and undo files are deleted in lock-step (when blk00003.dat is deleted, so is rev00003.dat.)
     * Pruning cannot take place until the longest chain is at least a certain length (CChainParams::nPruneAfterHeight).
     * Pruning will never delete a block within a defined distance (currently 288) from the active chain's tip.
     * The block index is updated by unsetting HAVE_DATA and HAVE_UNDO for any blocks that were stored in the deleted files.
     * A db flag records the fact that at least some block files have been pruned.
     *
     * @param[out]   setFilesToPrune   The set of file indices that can be unlinked will be returned
     * @param        last_prune        The last height we're able to prune, according to the prune locks
     */
    void FindFilesToPrune(
        std::set<int>& setFilesToPrune,
        int last_prune,
        const Chainstate& chain,
        ChainstateManager& chainman);

    RecursiveMutex cs_LastBlockFile;
    std::vector<CBlockFileInfo> m_blockfile_info;

    //! Since assumedvalid chainstates may be syncing a range of the chain that is very
    //! far away from the normal/background validation process, we should segment blockfiles
    //! for assumed chainstates. Otherwise, we might have wildly different height ranges
    //! mixed into the same block files, which would impair our ability to prune
    //! effectively.
    //!
    //! This data structure maintains separate blockfile number cursors for each
    //! BlockfileType. The ASSUMED state is initialized, when necessary, in FindNextBlockPos().
    //!
    //! The first element is the NORMAL cursor, second is ASSUMED.
    std::array<std::optional<BlockfileCursor>, BlockfileType::NUM_TYPES>
        m_blockfile_cursors GUARDED_BY(cs_LastBlockFile) = {
            BlockfileCursor{},
            std::nullopt,
    };
    int MaxBlockfileNum() const EXCLUSIVE_LOCKS_REQUIRED(cs_LastBlockFile)
    {
        static const BlockfileCursor empty_cursor;
        const auto& normal = m_blockfile_cursors[BlockfileType::NORMAL].value_or(empty_cursor);
        const auto& assumed = m_blockfile_cursors[BlockfileType::ASSUMED].value_or(empty_cursor);
        return std::max(normal.file_num, assumed.file_num);
    }

    /** Global flag to indicate we should check to see if there are
     *  block/undo files that should be deleted.  Set on startup
     *  or if we allocate more file space when we're in prune mode
     */
    bool m_check_for_pruning = false;

    const bool m_prune_mode;

    /** Dirty block index entries. */
    std::set<CBlockIndex*> m_dirty_blockindex;

    /** Dirty block file entries. */
    std::set<int> m_dirty_fileinfo;

public:
    /**
     * Map from external index name to oldest block that must not be pruned.
     *
     * @note Internally, only blocks before height (height_first - PRUNE_LOCK_BUFFER - 1) and
     * after height (height_last + PRUNE_LOCK_BUFFER) will be pruned, but callers should
     * avoid assuming any particular buffer size.
     */
    std::unordered_map<std::string, PruneLockInfo> m_prune_locks GUARDED_BY(::cs_main);

private:
    BlockfileType BlockfileTypeForHeight(int height);

    const kernel::BlockManagerOpts m_opts;

public:
    using Options = kernel::BlockManagerOpts;

    explicit BlockManager(const util::SignalInterrupt& interrupt, Options opts)
        : m_prune_mode{opts.prune_target > 0},
          m_opts{std::move(opts)},
          m_interrupt{interrupt} {}

    const util::SignalInterrupt& m_interrupt;
    std::atomic<bool> m_importing{false};

    /**
     * Whether all blockfiles have been added to the block tree database.
     * Normally true, but set to false when a reindex is requested and the
     * database is wiped. The value is persisted in the database across restarts
     * and will be false until reindexing completes.
     */
    std::atomic_bool m_blockfiles_indexed{true};

    BlockMap m_block_index GUARDED_BY(cs_main);

    /**
     * The height of the base block of an assumeutxo snapshot, if one is in use.
     *
     * This controls how blockfiles are segmented by chainstate type to avoid
     * comingling different height regions of the chain when an assumedvalid chainstate
     * is in use. If heights are drastically different in the same blockfile, pruning
     * suffers.
     *
     * This is set during ActivateSnapshot() or upon LoadBlockIndex() if a snapshot
     * had been previously loaded. After the snapshot is validated, this is unset to
     * restore normal LoadBlockIndex behavior.
     */
    std::optional<int> m_snapshot_height;

    std::vector<CBlockIndex*> GetAllBlockIndices() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /**
     * All pairs A->B, where A (or one of its ancestors) misses transactions, but B has transactions.
     * Pruned nodes may have entries where B is missing data.
     */
    std::multimap<CBlockIndex*, CBlockIndex*> m_blocks_unlinked;

    std::unique_ptr<BlockTreeDB> m_block_tree_db GUARDED_BY(::cs_main);

    bool WriteBlockIndexDB() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool LoadBlockIndexDB(const std::optional<uint256>& snapshot_blockhash)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /**
     * Remove any pruned block & undo files that are still on disk.
     * This could happen on some systems if the file was still being read while unlinked,
     * or if we crash before unlinking.
     */
    void ScanAndUnlinkAlreadyPrunedFiles() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    CBlockIndex* AddToBlockIndex(const CBlockHeader& block, CBlockIndex*& best_header) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    /** Create a new block index entry for a given block hash */
    CBlockIndex* InsertBlockIndex(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    //! Mark one block file as pruned (modify associated database entries)
    void PruneOneBlockFile(const int fileNumber) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    CBlockIndex* LookupBlockIndex(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    const CBlockIndex* LookupBlockIndex(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /** Get block file info entry for one block file */
    CBlockFileInfo* GetBlockFileInfo(size_t n);

    bool WriteUndoDataForBlock(const CBlockUndo& blockundo, BlockValidationState& state, CBlockIndex& block)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Store block on disk and update block file statistics.
     *
     * @param[in]  block        the block to be stored
     * @param[in]  nHeight      the height of the block
     *
     * @returns in case of success, the position to which the block was written to
     *          in case of an error, an empty FlatFilePos
     */
    FlatFilePos SaveBlockToDisk(const CBlock& block, int nHeight);

    /** Update blockfile info while processing a block during reindex. The block must be available on disk.
     *
     * @param[in]  block        the block being processed
     * @param[in]  nHeight      the height of the block
     * @param[in]  pos          the position of the serialized CBlock on disk. This is the position returned
     *                          by WriteBlockToDisk pointing at the CBlock, not the separator fields before it
     */
    void UpdateBlockInfo(const CBlock& block, unsigned int nHeight, const FlatFilePos& pos);

    /** Whether running in -prune mode. */
    [[nodiscard]] bool IsPruneMode() const { return m_prune_mode; }

    /** Attempt to stay below this number of bytes of block files. */
    [[nodiscard]] uint64_t GetPruneTarget() const { return m_opts.prune_target; }
    static constexpr auto PRUNE_TARGET_MANUAL{std::numeric_limits<uint64_t>::max()};

    [[nodiscard]] bool LoadingBlocks() const { return m_importing || !m_blockfiles_indexed; }

    /** Calculate the amount of disk space the block & undo files currently use */
    uint64_t CalculateCurrentUsage();

    //! Returns last CBlockIndex* that is a checkpoint
    const CBlockIndex* GetLastCheckpoint(const CCheckpointData& data) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    //! Check if all blocks in the [upper_block, lower_block] range have data available.
    //! The caller is responsible for ensuring that lower_block is an ancestor of upper_block
    //! (part of the same chain).
    bool CheckBlockDataAvailability(const CBlockIndex& upper_block LIFETIMEBOUND, const CBlockIndex& lower_block LIFETIMEBOUND) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! Find the first stored ancestor of start_block immediately after the last
    //! pruned ancestor. Return value will never be null. Caller is responsible
    //! for ensuring that start_block has data is not pruned.
    const CBlockIndex* GetFirstStoredBlock(const CBlockIndex& start_block LIFETIMEBOUND, const CBlockIndex* lower_block=nullptr) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** True if any block files have ever been pruned. */
    bool m_have_pruned = false;

    //! Check whether the block associated with this index entry is pruned or not.
    bool IsBlockPruned(const CBlockIndex& block) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    bool PruneLockExists(const std::string& name) const SHARED_LOCKS_REQUIRED(::cs_main);
    //! Create or update a prune lock identified by its name
    bool UpdatePruneLock(const std::string& name, const PruneLockInfo& lock_info, bool sync=false) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool DeletePruneLock(const std::string& name) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    /** Open a block file (blk?????.dat) */
    AutoFile OpenBlockFile(const FlatFilePos& pos, bool fReadOnly = false) const;

    /** Translation to a filesystem path */
    fs::path GetBlockPosFilename(const FlatFilePos& pos) const;

    /**
     *  Actually unlink the specified files
     */
    void UnlinkPrunedFiles(const std::set<int>& setFilesToPrune) const;

    /** Functions for disk access for blocks */
    bool ReadBlockFromDisk(CBlock& block, const FlatFilePos& pos) const;
    bool ReadBlockFromDisk(CBlock& block, const CBlockIndex& index) const;
    bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const FlatFilePos& pos) const;

    bool UndoReadFromDisk(CBlockUndo& blockundo, const CBlockIndex& index) const;

    void CleanupBlockRevFiles() const;
};

void ImportBlocks(ChainstateManager& chainman, std::vector<fs::path> vImportFiles);
} // namespace node

#endif // BITCOIN_NODE_BLOCKSTORAGE_H
