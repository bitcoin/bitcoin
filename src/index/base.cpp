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

constexpr uint8_t DB_BEST_BLOCK{'B'};

constexpr auto SYNC_LOG_INTERVAL{30s};
constexpr auto SYNC_LOCATOR_WRITE_INTERVAL{30s};

template <typename... Args>
void BaseIndex::FatalErrorf(util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    auto message = tfm::format(fmt, args...);
    node::AbortNode(m_chain->context()->shutdown_request, m_chain->context()->exit_status, Untranslated(message), m_chain->context()->warnings.get());
}

const CBlockIndex& BaseIndex::BlockIndex(const uint256& hash)
{
   return WITH_LOCK(cs_main, return *Assert(m_chainstate->m_blockman.LookupBlockIndex(hash)));
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
    void chainStateFlushed(ChainstateRole role, const CBlockLocator& locator) override;
    BaseIndex& m_index;
};

void BaseIndexNotifications::blockConnected(ChainstateRole role, const interfaces::BlockInfo& block)
{
    if (m_index.IgnoreBlockConnected(role, block)) return;

    const CBlockIndex* pindex = &m_index.BlockIndex(block.hash);
    const CBlockIndex* best_block_index = m_index.m_best_block_index.load();
    if (best_block_index && best_block_index != pindex->pprev && !m_index.Rewind(best_block_index, pindex->pprev)) {
        m_index.FatalErrorf("Failed to rewind %s to a previous chain tip",
                   m_index.GetName());
        return;
    }

    // Dispatch block to child class; errors are logged internally and abort the node.
    if (!m_index.Append(block)) return;

    // Setting the best block index is intentionally the last step of this
    // function, so BlockUntilSyncedToCurrentChain callers waiting for the
    // best block index to be updated can rely on the block being fully
    // processed, and the index object being safe to delete.
    m_index.SetBestBlockIndex(pindex);
}

void BaseIndexNotifications::chainStateFlushed(ChainstateRole role, const CBlockLocator& locator)
{
    if (m_index.IgnoreChainStateFlushed(role, locator)) return;

    // No need to handle errors in Commit. If it fails, the error will be already be logged. The
    // best way to recover is to continue, as index cannot be corrupted by a missed commit to disk
    // for an advanced index state.
    m_index.Commit();
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
    m_interrupt.reset();
    m_best_block_index = nullptr;
    m_synced = false;
    m_ready = false;

    // m_chainstate member gives indexing code access to node internals. It is
    // removed in followup https://github.com/bitcoin/bitcoin/pull/24230
    m_chainstate = WITH_LOCK(::cs_main,
        return &m_chain->context()->chainman->GetChainstateForIndexing());

    CBlockLocator locator;
    if (!GetDB().ReadBestBlock(locator)) {
        locator.SetNull();
    }

    auto options = CustomOptions();
    auto notifications = std::make_shared<BaseIndexNotifications>(*this);
    auto prepare_sync = [&](const interfaces::BlockInfo& block) {
        const auto block_ref{block.height >= 0 ? std::make_optional(interfaces::BlockRef{block.hash, block.height}) : std::nullopt};
        if (!locator.IsNull() && !block_ref) {
            return InitError(Untranslated(strprintf("best block of %s not found. Please rebuild the index, or disable it until the node is synced.", GetName())));
        }

        assert(!m_best_block_index && !m_synced);
        SetBestBlockIndex(block_ref ? &BlockIndex(block_ref->hash) : nullptr);

        // Call CustomInit and set m_ready. It is important to call CustomInit
        // before setting m_ready to ensure that CustomInit is always called
        // before CustomAppend. CustomAppend calls from the notification thread
        // will start happening when m_ready is true.
        if (!CustomInit(block_ref)) {
            return false;
        }
        // To prevent race conditions, m_ready = true needs to be set from the
        // validationinterface thread and the m_ready = true callback needs to
        // be queued while cs_main is held.
        //
        // Specifically, to prevent older, stale notifications currently in the
        // validation queue from being processed by the index, it is important
        // to delay setting m_ready = true until they are removed from the
        // queue, using CallFunctionInValidationInterfaceQueue. It is also
        // important to keep cs_main locked while calling
        // CallFunctionInValidationInterfaceQueue, to ensure any new
        // notifications being sent right now will be queued after the m_ready =
        // true callback, and will not be lost.
        m_synced = block.chain_tip;
        if (m_synced) {
            m_chain->context()->validation_signals->CallFunctionInValidationInterfaceQueue([this] { m_ready = true; });
        }
        return true;
    };
    auto handler = m_chain->attachChain(notifications, locator, options, prepare_sync);

    // Handler will be null if prepare_sync lambda above returned false.
    if (!handler) return false;

    LOCK(m_mutex);
    m_handler = std::move(handler);
    return true;
}

static const CBlockIndex* NextSyncBlock(const CBlockIndex* pindex_prev, CChain& chain) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    if (!pindex_prev) {
        return chain.Genesis();
    }

    const CBlockIndex* pindex = chain.Next(pindex_prev);
    if (pindex) {
        return pindex;
    }

    // Since block is not in the chain, return the next block in the chain AFTER the last common ancestor.
    // Caller will be responsible for rewinding back to the common ancestor.
    return chain.Next(chain.FindFork(pindex_prev));
}

bool BaseIndex::Append(const interfaces::BlockInfo& new_block)
{
    // Make a mutable copy of the BlockInfo argument so data and undo_data can
    // be attached below. This is temporary and removed in upcoming commits.
    interfaces::BlockInfo block_info{new_block};
    const CBlockIndex* pindex = &BlockIndex(block_info.hash);

    CBlock block;
    if (!block_info.data) { // disk lookup if block data wasn't provided
        if (!m_chainstate->m_blockman.ReadBlock(block, *pindex)) {
            FatalErrorf("Failed to read block %s from disk",
                        pindex->GetBlockHash().ToString());
            return false;
        }
        block_info.data = &block;
    }

    CBlockUndo block_undo;
    if (CustomOptions().connect_undo_data) {
        if (pindex->nHeight > 0 && !m_chainstate->m_blockman.ReadBlockUndo(block_undo, *pindex)) {
            FatalErrorf("Failed to read undo block data %s from disk",
                        pindex->GetBlockHash().ToString());
            return false;
        }
        block_info.undo_data = &block_undo;
    }

    if (!CustomAppend(block_info)) {
        FatalErrorf("Failed to write block %s to index database",
                    pindex->GetBlockHash().ToString());
        return false;
    }

    return true;
}

void BaseIndex::Sync()
{
    const CBlockIndex* pindex = m_best_block_index.load();
    if (!m_synced) {
        auto last_log_time{NodeClock::now()};
        auto last_locator_write_time{last_log_time};
        // Last block in the chain syncing to which is known to be flushed.
        const CBlockIndex *last_flushed{nullptr};
        // Return whether or not it is safe to call Commit(). It may not safe to
        // commit the index if the index best block is ahead of the last block
        // flushed to disk, because if the node process doesn't shut down
        // cleanly, and then is restarted, the node may not recognize the index
        // best block, and it may be impossible to rewind the index to a block
        // that is recognized because no undo data is saved either.
        auto should_commit = [&]{ return pindex && last_flushed && last_flushed->GetAncestor(pindex->nHeight) == pindex; };
        while (true) {
            if (m_interrupt) {
                LogInfo("%s: m_interrupt set; exiting ThreadSync", GetName());

                SetBestBlockIndex(pindex);
                // No need to handle errors in Commit. If it fails, the error will be already be
                // logged. The best way to recover is to continue, as index cannot be corrupted by
                // a missed commit to disk for an advanced index state.
                if (should_commit()) Commit();
                return;
            }

            const CBlockIndex* pindex_next = WITH_LOCK(cs_main,
                last_flushed = m_chainstate->LastFlushedBlock();
                return NextSyncBlock(pindex, m_chainstate->m_chain));
            // If pindex_next is null, it means pindex is the chain tip, so
            // commit data indexed so far.
            if (!pindex_next) {
                SetBestBlockIndex(pindex);
                // No need to handle errors in Commit. See rationale above.
                if (should_commit()) Commit();

                // If pindex is still the chain tip after committing, exit the
                // sync loop. It is important for cs_main to be locked while
                // enqueuing m_ready = true, otherwise a new block could be
                // attached while m_synced is still false, and it would not be
                // indexed.
                LOCK(::cs_main);
                pindex_next = NextSyncBlock(pindex, m_chainstate->m_chain);
                if (!pindex_next) {
                    m_synced = true;
                    m_chain->context()->validation_signals->CallFunctionInValidationInterfaceQueue([this] { m_ready = true; });
                    break;
                }
            }
            if (pindex_next->pprev != pindex && !Rewind(pindex, pindex_next->pprev)) {
                FatalErrorf("Failed to rewind %s to a previous chain tip", GetName());
                return;
            }
            pindex = pindex_next;

            if (!Append(kernel::MakeBlockInfo(pindex))) return; // error logged internally

            auto current_time{NodeClock::now()};
            if (current_time - last_log_time >= SYNC_LOG_INTERVAL) {
                LogInfo("Syncing %s with block chain from height %d", GetName(), pindex->nHeight);
                last_log_time = current_time;
            }

            if (current_time - last_locator_write_time >= SYNC_LOCATOR_WRITE_INTERVAL || pindex == last_flushed) {
                SetBestBlockIndex(pindex);
                last_locator_write_time = current_time;
                // No need to handle errors in Commit. See rationale above.
                if (should_commit()) Commit();
            }
        }
    }

    if (pindex) {
        LogInfo("%s is enabled at height %d", GetName(), pindex->nHeight);
    } else {
        LogInfo("%s is enabled", GetName());
    }
}

bool BaseIndex::Commit()
{
    // Don't commit anything if we haven't indexed any block yet
    // (this could happen if init is interrupted).
    bool ok = m_best_block_index != nullptr;
    if (ok) {
        CDBBatch batch(GetDB());
        ok = CustomCommit(batch);
        if (ok) {
            GetDB().WriteBestBlock(batch, GetLocator(*m_chain, m_best_block_index.load()->GetBlockHash()));
            ok = GetDB().WriteBatch(batch);
        }
    }
    if (!ok) {
        LogError("Failed to commit latest %s state", GetName());
        return false;
    }
    return true;
}

bool BaseIndex::Rewind(const CBlockIndex* current_tip, const CBlockIndex* new_tip)
{
    assert(current_tip->GetAncestor(new_tip->nHeight) == new_tip);

    CBlock block;
    CBlockUndo block_undo;

    for (const CBlockIndex* iter_tip = current_tip; iter_tip != new_tip; iter_tip = iter_tip->pprev) {
        interfaces::BlockInfo block_info = kernel::MakeBlockInfo(iter_tip);
        if (CustomOptions().disconnect_data) {
            if (!m_chainstate->m_blockman.ReadBlock(block, *iter_tip)) {
                LogError("Failed to read block %s from disk",
                         iter_tip->GetBlockHash().ToString());
                return false;
            }
            block_info.data = &block;
        }
        if (CustomOptions().disconnect_undo_data && iter_tip->nHeight > 0) {
            if (!m_chainstate->m_blockman.ReadBlockUndo(block_undo, *iter_tip)) {
                return false;
            }
            block_info.undo_data = &block_undo;
        }
        if (!CustomRemove(block_info)) {
            return false;
        }
    }

    // Don't commit here - the committed index state must never be ahead of the
    // flushed chainstate, otherwise unclean restarts would lead to index corruption.
    // Pruning has a minimum of 288 blocks-to-keep and getting the index
    // out of sync may be possible but a users fault.
    // In case we reorg beyond the pruned depth, ReadBlock would
    // throw and lead to a graceful shutdown
    SetBestBlockIndex(new_tip);
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

    // Ignore BlockConnected signals until we have fully indexed the chain.
    if (!m_ready) {
        return true;
    }

    const CBlockIndex* pindex = &BlockIndex(block.hash);
    const CBlockIndex* best_block_index = m_best_block_index.load();
    if (!best_block_index) {
        if (pindex->nHeight != 0) {
            FatalErrorf("First block connected is not the genesis block (height=%d)",
                       pindex->nHeight);
            return true;
        }
    } else {
        // To allow handling reorgs, this only checks that the new block
        // connects to ancestor of the current best block, instead of checking
        // that it connects to directly to the current block. If there is a
        // reorg, Rewind call below will remove existing blocks from the index
        // before adding the new one.
        assert(best_block_index->GetAncestor(pindex->nHeight - 1) == pindex->pprev);
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

    if (!m_ready) {
        return true;
    }

    const uint256& locator_tip_hash = locator.vHave.front();
    const CBlockIndex* locator_tip_index;
    {
        LOCK(cs_main);
        locator_tip_index = m_chainstate->m_blockman.LookupBlockIndex(locator_tip_hash);
    }

    if (!locator_tip_index) {
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
    const CBlockIndex* best_block_index = m_best_block_index.load();
    if (best_block_index->GetAncestor(locator_tip_index->nHeight) != locator_tip_index) {
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

    {
        // Skip the queue-draining stuff if we know we're caught up with
        // m_chain.Tip().
        LOCK(cs_main);
        const CBlockIndex* chain_tip = m_chainstate->m_chain.Tip();
        const CBlockIndex* best_block_index = m_best_block_index.load();
        if (best_block_index->GetAncestor(chain_tip->nHeight) == chain_tip) {
            return true;
        }
    }

    LogInfo("%s is catching up on block notifications", GetName());
    m_chain->context()->validation_signals->SyncWithValidationInterfaceQueue();
    return true;
}

void BaseIndex::Interrupt()
{
    m_interrupt();
}

bool BaseIndex::StartBackgroundSync()
{
    if (WITH_LOCK(m_mutex, return !m_handler)) throw std::logic_error("Error: Cannot start a non-initialized index");

    m_thread_sync = std::thread(&util::TraceThread, GetName(), [this] { Sync(); });
    return true;
}

void BaseIndex::Stop()
{
    WITH_LOCK(m_mutex, m_handler.reset());

    if (m_thread_sync.joinable()) {
        m_thread_sync.join();
    }
}

IndexSummary BaseIndex::GetSummary() const
{
    IndexSummary summary{};
    summary.name = GetName();
    summary.synced = m_synced;
    if (const auto& pindex = m_best_block_index.load()) {
        summary.best_block_height = pindex->nHeight;
        summary.best_block_hash = pindex->GetBlockHash();
    } else {
        summary.best_block_height = 0;
        summary.best_block_hash = m_chain->getBlockHash(0);
    }
    return summary;
}

void BaseIndex::SetBestBlockIndex(const CBlockIndex* block)
{
    assert(!m_chainstate->m_blockman.IsPruneMode() || AllowPrune());

    if (AllowPrune() && block) {
        node::PruneLockInfo prune_lock;
        prune_lock.height_first = block->nHeight;
        WITH_LOCK(::cs_main, m_chainstate->m_blockman.UpdatePruneLock(GetName(), prune_lock));
    }

    // Intentionally set m_best_block_index as the last step in this function,
    // after updating prune locks above, and after making any other references
    // to *this, so the BlockUntilSyncedToCurrentChain function (which checks
    // m_best_block_index as an optimization) can be used to wait for the last
    // BlockConnected notification and safely assume that prune locks are
    // updated and that the index object is safe to delete.
    m_best_block_index = block;
}
