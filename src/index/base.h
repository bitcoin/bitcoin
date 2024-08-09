// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_BASE_H
#define BITCOIN_INDEX_BASE_H

#include <dbwrapper.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <util/threadinterrupt.h>
#include <validationinterface.h>

#include <string>

class BaseIndexNotifications;
class CBlock;
class Chainstate;
class ChainstateManager;
namespace interfaces {
class Chain;
} // namespace interfaces

struct IndexSummary {
    std::string name;
    bool synced{false};
    int best_block_height{0};
    uint256 best_block_hash;
};

/**
 * Base class for indices of blockchain data. This handles block connected and
 * disconnected notifications and ensures blocks are indexed sequentially
 * according to their position in the active chain.
 *
 * In the presence of multiple chainstates (i.e. if a UTXO snapshot is loaded),
 * only the background "IBD" chainstate will be indexed to avoid building the
 * index out of order. When the background chainstate completes validation, the
 * index will be reinitialized and indexing will continue.
 */
class BaseIndex
{
protected:
    /**
     * The database stores a block locator of the chain the database is synced to
     * so that the index can efficiently determine the point it last stopped at.
     * A locator is used instead of a simple hash of the chain tip because blocks
     * and block index entries may not be flushed to disk until after this database
     * is updated.
    */
    class DB : public CDBWrapper
    {
    public:
        DB(const fs::path& path, size_t n_cache_size,
           bool f_memory = false, bool f_wipe = false, bool f_obfuscate = false);

        /// Read block locator of the chain that the index is in sync with.
        bool ReadBestBlock(CBlockLocator& locator) const;

        /// Write block locator of the chain that the index is in sync with.
        void WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator);
    };

private:
    /// Whether the index is in sync with the main chain. The flag is flipped
    /// from false to true once.
    ///
    /// Note that this will latch to true *immediately* upon startup if
    /// `m_chainstate->m_chain` is empty, which will be the case upon startup
    /// with an empty datadir if, e.g., `-txindex=1` is specified.
    std::atomic<bool> m_synced{false};

    /// The best block in the chain that the index is considered synced to, as
    /// reported by GetSummary. This field is not set in a consistent way and is
    /// hard to rely on. In some cases, this field refers to the last block that
    /// has been added to the index, regardless of whether the data was
    /// committed. In other cases, this field lags behind as blocks are added to
    /// the index, and is only updated when the data is committed and the
    /// DB_BEST_BLOCK locator is updated.
    ///
    /// More specifically:
    ///
    /// - During initial sync when m_synced is false, m_best_block_index is not
    ///   updated when blocks are added to the index. It is only updated when
    ///   the data is committed (which is every 30 seconds, and at the end of
    ///   the sync, and any time after blocks are rewound).
    ///
    /// - After initial sync when m_synced is true, m_best_block_index is set
    ///   after each block is added to the index, whether or not the data is
    ///   committed.
    ///
    /// - When there is a reorg, m_best_block is not set as individual blocks
    ///   are removed from the index, only after all blocks are removed and the
    ///   data is committed
    std::optional<interfaces::BlockKey> m_best_block GUARDED_BY(m_mutex);

    /// Mutex to let m_notifications and m_handler be accessed from multiple
    /// threads (the sync thread and the init thread).
    mutable Mutex m_mutex;
    friend class BaseIndexNotifications;
    std::shared_ptr<BaseIndexNotifications> m_notifications GUARDED_BY(m_mutex);
    std::unique_ptr<interfaces::Handler> m_handler GUARDED_BY(m_mutex);

    /// Write the current index state (eg. chain block locator and subclass-specific items) to disk.
    ///
    /// Recommendations for error handling:
    /// If called on a successor of the previous committed best block in the index, the index can
    /// continue processing without risk of corruption, though the index state will need to catch up
    /// from further behind on reboot. If the new state is not a successor of the previous state (due
    /// to a chain reorganization), the index must halt until Commit succeeds or else it could end up
    /// getting corrupted.
    bool Commit(const CBlockLocator& locator);

    virtual bool AllowPrune() const = 0;

    template <typename... Args>
    void FatalErrorf(const char* fmt, const Args&... args) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

protected:
    std::unique_ptr<interfaces::Chain> m_chain;
    const std::string m_name;

    /// Return whether to ignore stale, out-of-sync block connected event
    bool IgnoreBlockConnected(ChainstateRole role, const interfaces::BlockInfo& block) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /// Return whether to ignore stale, out-of-sync chain flushed event
    bool IgnoreChainStateFlushed(ChainstateRole role, const CBlockLocator& locator) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /// Return custom notification options for index.
    [[nodiscard]] virtual interfaces::Chain::NotifyOptions CustomOptions() { return {}; }

    /// Initialize internal state from the database and block index.
    [[nodiscard]] virtual bool CustomInit(const std::optional<interfaces::BlockKey>& block) { return true; }

    /// Write update index entries for a newly connected block.
    [[nodiscard]] virtual bool CustomAppend(const interfaces::BlockInfo& block) { return true; }

    /// Virtual method called internally by Commit that can be overridden to atomically
    /// commit more index state.
    virtual bool CustomCommit(CDBBatch& batch) { return true; }

    /// Rewind index by one block during a chain reorg.
    [[nodiscard]] virtual bool CustomRemove(const interfaces::BlockInfo& block) { return true; }

    virtual DB& GetDB() const = 0;

    /// Update the internal best block index as well as the prune lock.
    void SetBestBlock(const std::optional<interfaces::BlockKey>& block) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

public:
    BaseIndex(std::unique_ptr<interfaces::Chain> chain, std::string name);
    virtual ~BaseIndex();

    /// Get the name of the index for display in logs.
    const std::string& GetName() const LIFETIMEBOUND { return m_name; }

    /// Blocks the current thread until the index is caught up to the current
    /// state of the block chain. This only blocks if the index has gotten in
    /// sync once and only needs to process blocks in the ValidationInterface
    /// queue. If the index is catching up from far behind, this method does
    /// not block and immediately returns false.
    bool BlockUntilSyncedToCurrentChain() const LOCKS_EXCLUDED(::cs_main) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    void Interrupt() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /// Initializes the sync state and registers the instance to the
    /// validation interface so that it stays in sync with blockchain updates.
    [[nodiscard]] bool Init() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /// Starts the initial sync process on a background thread.
    [[nodiscard]] bool StartBackgroundSync() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /// Sync the index with the block index starting from the current best block.
    /// Intended to be run in its own thread, m_thread_sync, and can be
    /// interrupted with m_interrupt. Once the index gets in sync, the m_synced
    /// flag is set and the BlockConnected ValidationInterface callback takes
    /// over and the sync thread exits.
    void Sync();

    /// Stops the instance from staying in sync with blockchain updates.
    void Stop() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /// Get a summary of the index and its state.
    IndexSummary GetSummary() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};

#endif // BITCOIN_INDEX_BASE_H
