// Copyright (c) 2017-present The Bitcoin Core developers
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
#include <util/string.h>
#include <util/thread.h>
#include <util/translation.h>
#include <validation.h>

#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

using interfaces::BlockInfo;
using interfaces::FoundBlock;

constexpr uint8_t DB_BEST_BLOCK{'B'};

constexpr auto SYNC_LOG_INTERVAL{30s};
constexpr auto SYNC_LOCATOR_WRITE_INTERVAL{30s};

template <typename... Args>
void BaseIndex::FatalErrorf(util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    Interrupt(); // Cancel the sync thread
    auto message = tfm::format(fmt, args...);
    node::AbortNode(m_chain->context()->shutdown_request, m_chain->context()->exit_status, Untranslated(message), m_chain->context()->warnings.get());
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
    void setBest(const interfaces::BlockRef& block)
    {
        assert(!block.hash.IsNull());
        assert(block.height >= 0);
        m_index.SetBestBlock(block);
    }
    BaseIndex& m_index;
    interfaces::Chain::NotifyOptions m_options = m_index.CustomOptions();
    NodeClock::time_point m_last_log_time{NodeClock::now()};
    NodeClock::time_point m_last_locator_write_time{m_last_log_time};
    //! If blocks are disconnected, m_rewind_start is set to track starting
    //! point of the rewind and m_rewind_error is set to record any errors.
    std::optional<interfaces::BlockRef> m_rewind_start;
    bool m_rewind_error = false;
};

void BaseIndexNotifications::blockConnected(ChainstateRole role, const interfaces::BlockInfo& block)
{
    if (!block.error.empty()) {
        m_index.FatalErrorf("%s", block.error);
        return;
    }

    // blockConnected is called an extra time without block data and with
    // chain_tip=true as a signal that sync has caught up.
    if (!block.data && block.chain_tip) {
        m_index.m_synced = true;
        if (block.height >= 0) {
            LogPrintf("%s is enabled at height %d\n", m_index.GetName(), block.height);
        } else {
            LogPrintf("%s is enabled\n", m_index.GetName());
        }
        return;
    }

    if (m_index.IgnoreBlockConnected(role, block)) return;

    // If a new block is being connected after blocks were rewound, reset the
    // rewind state and handle any errors that happened while rewinding.
    if (m_rewind_error) {
        m_index.FatalErrorf("Failed to rewind %s to a previous chain tip",
                   m_index.GetName());
        return;
    } else if (m_rewind_start) {
        auto best_block = m_index.GetBestBlock();
        // Assert m_best_block is null or is parent of new connected block, or is
        // descendant of parent of new connected block.
        if (best_block && best_block->hash != *block.prev_hash) {
            uint256 best_ancestor_hash;
            assert(m_index.m_chain->findAncestorByHeight(best_block->hash, block.height - 1, FoundBlock().hash(best_ancestor_hash)));
            assert(best_ancestor_hash == *block.prev_hash);
        }

        // Don't commit here - the committed index state must never be ahead of the
        // flushed chainstate, otherwise unclean restarts would lead to index corruption.
        // Pruning has a minimum of 288 blocks-to-keep and getting the index
        // out of sync may be possible but a users fault.
        // In case we reorg beyond the pruned depth, ReadBlock would
        // throw and lead to a graceful shutdown
        setBest({*block.prev_hash, block.height-1});
        m_rewind_start = std::nullopt;
    }

    // Dispatch block to child class; errors are logged internally and abort the node.
    if (block.data && !m_index.Append(block)) return;

    NodeClock::time_point current_time{0s};
    if (!block.chain_tip) {
        current_time = NodeClock::now();
        if (current_time - m_last_log_time >= SYNC_LOG_INTERVAL) {
            LogInfo("Syncing %s with block chain from height %d", m_index.GetName(), block.height);
            m_last_log_time = current_time;
        }
    }

    // If currently syncing with the chain and adding old blocks, not new ones,
    // decide whether to commit and update the best block pointer. Otherwise,
    // when adding new blocks, always update the best block pointer, and don't
    // commit until a chainstateFlush notification is received.
    if (!block.chain_tip) {
        // Commit if reached the last block flushed by the node, or if reached
        // the end of sync and no more data is being appended, or if enough time
        // has elapsed since the last commit.
        if (block.status == BlockInfo::FLUSHED_TIP || !block.data || (block.status == BlockInfo::FLUSHED && current_time - m_last_locator_write_time >= SYNC_LOCATOR_WRITE_INTERVAL)) {
            auto locator = GetLocator(*m_index.m_chain, block.hash);
            m_last_locator_write_time = current_time;
            // No need to handle errors in Commit. If it fails, the error will be already be
            // logged. The best way to recover is to continue, as index cannot be corrupted by
            // a missed commit to disk for an advanced index state.
            m_index.Commit(locator);
        } else {
            // Do not update index best block between commits if syncing.
            // Decision to let the best block pointer lag during sync seems a
            // little arbitrary, but has been behavior since syncing was introduced
            // in #13033, so preserving it in case anything depends on it.
            return;
        }
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

    auto best_block = m_index.GetBestBlock();
    if (!m_rewind_start) m_rewind_start = best_block;
    if (m_rewind_error) return;

    assert(!m_options.disconnect_data || block.data);
    assert(!m_options.disconnect_undo_data || block.undo_data || block.height <= 0);

    // If one CustomRemove call fails, subsequent calls will be skipped,
    // and there will be a fatal error if there an attempt to connect
    // a another block to the index.
    if (!m_index.CustomRemove(block)) {
        m_rewind_error = true;
        return;
    }
}

void BaseIndexNotifications::chainStateFlushed(ChainstateRole role, const CBlockLocator& locator)
{
    if (m_index.IgnoreChainStateFlushed(role, locator)) return;

    // No need to handle errors in Commit. If it fails, the error will be already be logged. The
    // best way to recover is to continue, as index cannot be corrupted by a missed commit to disk
    // for an advanced index state.
    //
    // Intentionally call GetLocator() here and do NOT pass the
    // chainStateFlushed locator argument to Commit(). It is specifically not
    // safe to use the chainStateFlushed locator here since d96c59cc5cd2 from
    // #25740, which sends chainStateFlushed and blockConnected notifications
    // out of order, so the locator sometimes points to a block which hasn't
    // been indexed yet. In general, calling GetLocator() here is safer than
    // trusting the locator argument, and this code was changed to stop saving
    // the locator argument in 4368384f1d26 from #14121.
    auto best_block = m_index.GetBestBlock();
    if (best_block) m_index.Commit(GetLocator(*m_index.m_chain, best_block->hash));
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

    // m_chainstate member gives indexing code access to node internals. It is
    // removed in followup https://github.com/bitcoin/bitcoin/pull/24230
    m_chainstate = WITH_LOCK(::cs_main,
        return &m_chain->context()->chainman->GetChainstateForIndexing());

    CBlockLocator locator;
    if (!GetDB().ReadBestBlock(locator)) {
        locator.SetNull();
    }

    auto options = CustomOptions();
    options.thread_name = GetName();
    auto notifications = std::make_shared<BaseIndexNotifications>(*this);
    auto prepare_sync = [&](const interfaces::BlockInfo& block) {
        const auto block_ref{block.height >= 0 ? std::make_optional(interfaces::BlockRef{block.hash, block.height}) : std::nullopt};
        if (!locator.IsNull() && !block_ref) {
            return InitError(Untranslated(strprintf("best block of %s not found. Please rebuild the index, or disable it until the node is synced.", GetName())));
        }

        assert(!GetBestBlock() && !m_synced);
        SetBestBlock(block_ref);

        if (!CustomInit(block_ref)) {
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

bool BaseIndex::Append(const interfaces::BlockInfo& block_info)
{
    if (!CustomAppend(block_info)) {
        FatalErrorf("Failed to write block %s to index database",
                    block_info.hash.ToString());
        return false;
    }

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
        LogError("Failed to commit latest %s state", GetName());
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

    const uint256& locator_tip_hash = locator.vHave.front();
    int locator_tip_height{-1};
    if (!m_chain->findBlock(locator_tip_hash, FoundBlock().height(locator_tip_height))) {
        FatalErrorf("First block (hash=%s) in locator was not found",
                   locator_tip_hash.ToString());
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
    auto best_block = GetBestBlock();
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

    if (const auto best_block = GetBestBlock()) {
        // Skip the queue-draining stuff if we know we're caught up with
        // m_chain.Tip().
        interfaces::BlockRef tip;
        uint256 ancestor;
        if (m_chain->getTip(FoundBlock().hash(tip.hash).height(tip.height)) &&
            m_chain->findAncestorByHeight(best_block->hash, tip.height, FoundBlock().hash(ancestor)) &&
            ancestor == tip.hash) {
            return true;
        }
    }

    LogInfo("%s is catching up on block notifications", GetName());
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
    if (const auto best_block = GetBestBlock()) {
        summary.best_block_height = best_block->height;
        summary.best_block_hash = best_block->hash;
    } else {
        summary.best_block_height = 0;
        summary.best_block_hash = m_chain->getBlockHash(0);
    }
    return summary;
}

std::optional<interfaces::BlockRef> BaseIndex::GetBestBlock() const
{
    LOCK(m_mutex);
    return m_best_block;
}

void BaseIndex::SetBestBlock(const std::optional<interfaces::BlockRef>& block)
{
    assert(!m_chainstate->m_blockman.IsPruneMode() || AllowPrune());

    if (AllowPrune() && block) {
        node::PruneLockInfo prune_lock;
        prune_lock.height_first = block->height;
        WITH_LOCK(::cs_main, m_chainstate->m_blockman.UpdatePruneLock(GetName(), prune_lock));
    }

    // Intentionally set m_best_block as the last step in this function,
    // after updating prune locks above, and after making any other references
    // to *this, so the BlockUntilSyncedToCurrentChain function (which checks
    // m_best_block as an optimization) can be used to wait for the last
    // BlockConnected notification and safely assume that prune locks are
    // updated and that the index object is safe to delete.
    WITH_LOCK(m_mutex, m_best_block = block);
}

std::shared_ptr<interfaces::Chain::Notifications> BaseIndex::Notifications() const
{
    LOCK(m_mutex);
    return m_notifications;
}
