// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/args.h>
#include <index/base.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <kernel/chain.h>
#include <logging.h>
#include <node/abort.h>
#include <node/blockstorage.h>
#include <node/context.h>
#include <node/database_args.h>
#include <node/interface_ui.h>
#include <tinyformat.h>
#include <undo.h>
#include <util/thread.h>
#include <util/translation.h>
#include <validation.h> // For g_chainman

#include <string>
#include <utility>

using interfaces::FoundBlock;

constexpr uint8_t DB_BEST_BLOCK{'B'};

constexpr auto SYNC_LOG_INTERVAL{30s};
constexpr auto SYNC_LOCATOR_WRITE_INTERVAL{30s};

template <typename... Args>
void BaseIndex::FatalErrorf(const char* fmt, const Args&... args)
{
    Interrupt(); // Cancel the sync thread
    auto message = tfm::format(fmt, args...);
    node::AbortNode(m_chain->context()->shutdown, m_chain->context()->exit_status, Untranslated(message), m_chain->context()->warnings.get());
}

CBlockLocator GetLocator(interfaces::Chain& chain, const uint256& block_hash)
{
    CBlockLocator locator;
    bool found = chain.findBlock(block_hash, interfaces::FoundBlock().locator(locator));
    assert(found);
    assert(!locator.IsNull());
    return locator;
}

class BaseIndexNotifications : public interfaces::Chain::Notifications
{
public:
    BaseIndexNotifications(BaseIndex& index) : m_index(index) {}
    void blockConnected(ChainstateRole role, const interfaces::BlockInfo& block) override;
    void blockDisconnected(const interfaces::BlockInfo& block) override;
    void chainStateFlushed(ChainstateRole role, const CBlockLocator& locator) override;
    std::optional<interfaces::BlockKey> getBest()
    {
        LOCK(m_index.m_mutex);
        return m_index.m_best_block;
    }
    void setBest(const interfaces::BlockKey& block)
    {
        assert(!block.hash.IsNull());
        assert(block.height >= 0);
        m_index.SetBestBlock(block);
    }
    BaseIndex& m_index;
    std::chrono::steady_clock::time_point m_last_log_time{0s};
    std::chrono::steady_clock::time_point m_last_locator_write_time{0s};
    //! As blocks are disconnected, index is updated but not committed to until
    //! the next flush or block connection. m_rewind_start points to the first
    //! block that has been disconnected and not flushed yet. m_rewind_error
    //! is set if a block failed to disconnect.
    std::optional<interfaces::BlockKey> m_rewind_start;
    bool m_rewind_error = false;
};

void BaseIndexNotifications::blockConnected(ChainstateRole role, const interfaces::BlockInfo& block)
{
    if (!block.error.empty()) {
        m_index.FatalErrorf("%s", block.error);
        return;
    }

    if (!block.data) {
        // Null block.data means block is the ending block at the end of a sync,
        // so just update the best block and m_synced.
        if (block.height >= 0) {
            setBest({block.hash, block.height});
        } else {
            assert(!getBest());
        }
        if (block.chain_tip) {
            m_index.m_synced = true;
            if (block.height >= 0) {
                LogPrintf("%s is enabled at height %d\n", m_index.GetName(), block.height);
            } else {
                LogPrintf("%s is enabled\n", m_index.GetName());
            }
        }
        return;
    }

    if (m_index.IgnoreBlockConnected(role, block)) return;

    // If blocks were disconnected, flush index state to disk before connecting new blocks.
    bool rewind_ok = !m_rewind_start || !m_rewind_error;
    if (m_rewind_start && rewind_ok) {
        auto best_block = getBest();
        // Assert m_best_block is null or is parent of new connected block, or is
        // descendant of parent of new connected block.
        if (best_block && best_block->hash != *block.prev_hash) {
            uint256 best_ancestor_hash;
            assert(m_index.m_chain->findAncestorByHeight(best_block->hash, block.height - 1, FoundBlock().hash(best_ancestor_hash)));
            assert(best_ancestor_hash == *block.prev_hash);
        }
        chainStateFlushed(role, GetLocator(*m_index.m_chain, *block.prev_hash));
        setBest({*block.prev_hash, block.height-1});
        rewind_ok = getBest()->hash == *block.prev_hash;
    }

    if (!rewind_ok) {
        m_index.FatalErrorf("%s: Failed to rewind index %s to a previous chain tip",
                   __func__, m_index.GetName());
        return;
    }

    std::chrono::steady_clock::time_point current_time{0s};
    if (!block.chain_tip) {
        current_time = std::chrono::steady_clock::now();
        if (m_last_log_time + SYNC_LOG_INTERVAL < current_time) {
            LogPrintf("Syncing %s with block chain from height %d\n",
                      m_index.GetName(), block.height);
            m_last_log_time = current_time;
        }
    }

    if (!m_index.CustomAppend(block)) {
        m_index.FatalErrorf("%s: Failed to write block %s to index",
                   __func__, block.hash.ToString());
        return;
    }

    if (!block.chain_tip && (m_last_locator_write_time + SYNC_LOCATOR_WRITE_INTERVAL < current_time)) {
        auto locator = GetLocator(*m_index.m_chain, block.hash);
        m_last_locator_write_time = current_time;
        // No need to handle errors in Commit. If it fails, the error will be already be
        // logged. The best way to recover is to continue, as index cannot be corrupted by
        // a missed commit to disk for an advanced index state.
        m_index.Commit(locator);
    } else if (!block.chain_tip) {
        // Only update index best block between flushes if fully synced.
        // Decision to let the best block pointer lag during sync seems a
        // little arbitrary, but has been behavior since syncing was introduced
        // in #13033, so preserving it in case anything depends on it.
        return;
    }

    // Setting the best block index is intentionally the last step of this
    // function, so BlockUntilSyncedToCurrentChain callers waiting for the
    // best block index to be updated can rely on the block being fully
    // processed, and the index object being safe to delete.
    setBest({block.hash, block.height});
}

void BaseIndexNotifications::blockDisconnected(const interfaces::BlockInfo& block)
{
    if (!block.error.empty()) {
        m_index.FatalErrorf("%s", block.error);
        return;
    }

    assert(block.data);
    auto best_block = getBest();
    if (!m_rewind_start) m_rewind_start = best_block;
    // If one CustomRemove call fails, subsequent calls will be skipped,
    // and there will be a fatal error if there an attempt to connect
    // a another block to the index.
    if (!m_rewind_error) m_rewind_error = !m_index.CustomRemove(block);
}

void BaseIndexNotifications::chainStateFlushed(ChainstateRole role, const CBlockLocator& locator)
{
    if (m_index.IgnoreChainStateFlushed(role, locator)) return;

    // No need to handle errors in Commit. If it fails, the error will be already be logged. The
    // best way to recover is to continue, as index cannot be corrupted by a missed commit to disk
    // for an advanced index state.
    // In the case of a reorg, ensure persisted block locator is not stale.
    // Pruning has a minimum of 288 blocks-to-keep and getting the index
    // out of sync may be possible but a users fault.
    // In case we reorg beyond the pruned depth, ReadBlockFromDisk would
    // throw and lead to a graceful shutdown
    if (!m_index.Commit(locator) && m_rewind_start) {
        // If commit fails, revert the best block index to avoid corruption.
        setBest(*m_rewind_start);
    }
    m_rewind_start = std::nullopt;
    m_rewind_error = false;
}

BaseIndex::DB::DB(const fs::path& path, size_t n_cache_size, bool f_memory, bool f_wipe, bool f_obfuscate) :
    CDBWrapper{DBParams{
        .path = path,
        .cache_bytes = n_cache_size,
        .memory_only = f_memory,
        .wipe_data = f_wipe,
        .obfuscate = f_obfuscate,
        .options = [] { DBOptions options; node::ReadDatabaseArgs(gArgs, options); return options; }()}}
{}

bool BaseIndex::DB::ReadBestBlock(CBlockLocator& locator) const
{
    bool success = Read(DB_BEST_BLOCK, locator);
    if (!success) {
        locator.SetNull();
    }
    return success;
}

void BaseIndex::DB::WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator)
{
    batch.Write(DB_BEST_BLOCK, locator);
}

BaseIndex::BaseIndex(std::unique_ptr<interfaces::Chain> chain, std::string name)
    : m_chain{std::move(chain)}, m_name{std::move(name)} {}

BaseIndex::~BaseIndex()
{
    //! Assert Stop() was called before this base destructor. Notification
    //! handlers call pure virtual methods like GetName(), so if they are still
    //! being called at this point, they would segfault.
    LOCK(m_mutex);
    assert(!m_notifications);
    assert(!m_handler);
}

// Read index best block, register for block connected and disconnected
// notifications, and determine where best block is relative to chain tip.
//
// If the chain tip and index best block are the same, block connected and
// disconnected notifications will be enabled after this call and the index
// will update as the ImportBlocks() function connects blocks and sends
// notifications. Otherwise, when the chain tip and index best block are not
// the same, the index will stay idle until ImportBlocks() finishes and
// BaseIndex::StartBackgroundSync() is called after.
//
// If the node is being started for the first time, or if -reindex or
// -reindex-chainstate are used, the chain tip will be null at this point,
// meaning that no blocks are attached, even a genesis block. The best block
// locator will also be null if -reindex is used or if the index is new, but
// will be non-null if -reindex-chainstate is used. So -reindex will cause the
// index to be considered synced and rebuild right away as the chain is
// rebuilt, while -reindex-chainstate will cause the index to be idle until the
// chain is rebuilt and BaseIndex::StartBackgroundSync is called after.
//
// All of this just ensures that -reindex and -reindex-chainstate options both
// function efficiently. If -reindex is used, both the chainstate and index are
// wiped, and the index is considered synced right away and gets rebuilt at the
// same time as the chainstate. If -reindex-chainstate is used, only the
// chainstate is wiped, not the index, so the index will be considered not
// synced, and the chainstate will update first, and the index will start
// syncing after. So the most efficient thing should happen in both cases.
bool BaseIndex::Init()
{
    AssertLockNotHeld(cs_main);

    // May need reset if index is being restarted.
    m_synced = false;
    {
        LOCK(m_mutex);
        m_best_block.reset();
        assert(!m_handler);
        assert(!m_notifications);
    }

    CBlockLocator locator;
    if (!GetDB().ReadBestBlock(locator)) {
        locator.SetNull();
    }

    auto options = CustomOptions();
    options.thread_name = GetName();
    auto notifications = std::make_shared<BaseIndexNotifications>(*this);
    auto prepare_sync = [&](const interfaces::BlockInfo& block) {
        const auto block_key{block.height >= 0 ? std::make_optional(interfaces::BlockKey{block.hash, block.height}) : std::nullopt};
        if (!locator.IsNull() && !block_key) {
            return InitError(strprintf(Untranslated("%s: best block of the index not found. Please rebuild the index, or disable it until the node is synced."), GetName()));
        }

        assert(!WITH_LOCK(m_mutex, return m_best_block) && !m_synced);
        SetBestBlock(block_key);

        if (!CustomInit(block_key)) {
            return false;
        }
        m_synced = block.chain_tip;
        return true;
    };
    auto handler = m_chain->attachChain(notifications, locator, options, prepare_sync);

    // Handler will be null if prepare_sync lambda above returned false.
    if (!handler) return false;

    LOCK(m_mutex);
    m_notifications = std::move(notifications);
    m_handler = std::move(handler);
    return true;
}

bool BaseIndex::Commit(const CBlockLocator& locator)
{
    // Don't commit anything if we haven't indexed any block yet
    // (this could happen if init is interrupted).
    bool ok = !locator.IsNull();
    if (ok) {
        CDBBatch batch(GetDB());
        ok = CustomCommit(batch);
        if (ok) {
            GetDB().WriteBestBlock(batch, locator);
            ok = GetDB().WriteBatch(batch);
        }
    }
    if (!ok) {
        LogError("%s: Failed to commit latest %s state\n", __func__, GetName());
        return false;
    }
    return true;
}

bool BaseIndex::IgnoreBlockConnected(ChainstateRole role, const interfaces::BlockInfo& block)
{
    // Ignore events from the assumed-valid chain; we will process its blocks
    // (sequentially) after it is fully verified by the background chainstate. This
    // is to avoid any out-of-order indexing.
    //
    // TODO at some point we could parameterize whether a particular index can be
    // built out of order, but for now just do the conservative simple thing.
    if (role == ChainstateRole::ASSUMEDVALID) {
        return true;
    }
    return false;
}

bool BaseIndex::IgnoreChainStateFlushed(ChainstateRole role, const CBlockLocator& locator)
{
    // Ignore events from the assumed-valid chain; we will process its blocks
    // (sequentially) after it is fully verified by the background chainstate.
    if (role == ChainstateRole::ASSUMEDVALID) {
        return true;
    }

    assert(!locator.IsNull());
    const uint256& locator_tip_hash = locator.vHave.front();
    int locator_tip_height{-1};
    if (!m_chain->findBlock(locator_tip_hash, FoundBlock().height(locator_tip_height))) {
        FatalErrorf("%s: First block (hash=%s) in locator was not found",
                   __func__, locator_tip_hash.ToString());
        return true;
    }

    // Check if locator points to the last block that was connected, or ancestor
    // of it. If it does, commit the index data, and if not, ignore the flushed
    // event.
    //
    // The locator may point to an ancestor of the best block if a block was
    // invalidated, or if a reorg started and there was a BlockDisconnected
    // notification between the last BlockConnected notification and
    // ChainstateFlushed. In either case the data in the index should be up to
    // date and should be committed.
    //
    // If the locator does not point to the best blocks or one of its ancestors,
    // it means this ChainStateFlushed notification was sent early and there are
    // PerConnectBlockTrace notifications queued to be sent after it. Avoid
    // committing index data in this case until after they are sent.
    auto best_block{WITH_LOCK(m_mutex, return m_best_block)};
    uint256 ancestor;
    if (!best_block ||
        !m_chain->findAncestorByHeight(best_block->hash, locator_tip_height, FoundBlock().hash(ancestor)) ||
        ancestor != locator_tip_hash) {
        return true;
    }
    return false;
}

bool BaseIndex::BlockUntilSyncedToCurrentChain() const
{
    AssertLockNotHeld(cs_main);

    if (!m_synced) {
        return false;
    }

    if (const auto best_block{WITH_LOCK(m_mutex, return m_best_block)}) {
        // Skip the queue-draining stuff if we know we're caught up with
        // m_chain.Tip().
        interfaces::BlockKey tip;
        uint256 ancestor;
        if (m_chain->getTip(FoundBlock().hash(tip.hash).height(tip.height)) &&
            m_chain->findAncestorByHeight(best_block->hash, tip.height, FoundBlock().hash(ancestor)) &&
            ancestor == tip.hash) {
            return true;
        }
    }

    LogPrintf("%s: %s is catching up on block notifications\n", __func__, GetName());
    m_chain->waitForPendingNotifications();
    return true;
}

void BaseIndex::Interrupt()
{
    LOCK(m_mutex);
    if (m_handler) m_handler->interrupt();
    m_notifications.reset();
}

bool BaseIndex::StartBackgroundSync()
{
    LOCK(m_mutex);
    if (!m_handler) throw std::logic_error("Error: Cannot start a non-initialized index");
    m_handler->start();
    return true;
}

void BaseIndex::Stop()
{
    Interrupt();
    // Call handler destructor after releasing m_mutex. Locking the mutex is
    // required to access m_handler, but the lock should not be held while
    // destroying the handler, because the handler destructor waits for the last
    // notification to be processed, so holding the lock would deadlock if that
    // last notification also needs the lock.
    auto handler = WITH_LOCK(m_mutex, return std::move(m_handler));
}

IndexSummary BaseIndex::GetSummary() const
{
    IndexSummary summary{};
    summary.name = GetName();
    summary.synced = m_synced;
    if (const auto best_block = WITH_LOCK(m_mutex, return m_best_block)) {
        summary.best_block_height = best_block->height;
        summary.best_block_hash = best_block->hash;
    } else {
        summary.best_block_height = 0;
        summary.best_block_hash = m_chain->getBlockHash(0);
    }
    return summary;
}

void BaseIndex::SetBestBlock(const std::optional<interfaces::BlockKey>& block)
{
    assert(!m_chain->pruningEnabled() || AllowPrune());

    if (AllowPrune() && block) {
        node::PruneLockInfo prune_lock;
        prune_lock.height_first = block->height;
        m_chain->updatePruneLock(GetName(), prune_lock);
    }

    // Intentionally set m_best_block as the last step in this function,
    // after updating prune locks above, and after making any other references
    // to *this, so the BlockUntilSyncedToCurrentChain function (which checks
    // m_best_block as an optimization) can be used to wait for the last
    // BlockConnected notification and safely assume that prune locks are
    // updated and that the index object is safe to delete.
    WITH_LOCK(m_mutex, m_best_block = block);
}
