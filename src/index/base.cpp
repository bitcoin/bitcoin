// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/base.h>

#include <chain.h>
#include <common/args.h>
#include <dbwrapper.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <interfaces/types.h>
#include <kernel/types.h>
#include <logging.h>
#include <node/abort.h>
#include <node/blockstorage.h>
#include <node/chain.h>
#include <node/context.h>
#include <node/database_args.h>
#include <node/interface_ui.h>
#include <primitives/block.h>
#include <sync.h>
#include <tinyformat.h>
#include <uint256.h>
#include <undo.h>
#include <util/fs.h>
#include <util/string.h>
#include <util/thread.h>
#include <util/threadinterrupt.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>

#include <cassert>
#include <compare>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using interfaces::BlockInfo;
using interfaces::FoundBlock;
using kernel::ChainstateRole;
using node::MakeBlockInfo;

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
    void blockConnected(const ChainstateRole& role, const interfaces::BlockInfo& block) override;
    void blockDisconnected(const interfaces::BlockInfo& block) override;
    void chainStateFlushed(const ChainstateRole& role, const CBlockLocator& locator) override;
    BaseIndex& m_index;
    interfaces::Chain::NotifyOptions m_options = m_index.CustomOptions();
    NodeClock::time_point m_last_log_time{NodeClock::now()};
    NodeClock::time_point m_last_locator_write_time{m_last_log_time};
    //! If blocks are disconnected, m_rewind_start is set to track starting
    //! point of the rewind and m_rewind_error is set to record any errors.
    const CBlockIndex* m_rewind_start = nullptr;
    bool m_rewind_error = false;
};

void BaseIndexNotifications::blockConnected(const ChainstateRole& role, const interfaces::BlockInfo& block)
{
    if (!block.error.empty()) {
        m_index.FatalErrorf("%s", block.error);
        return;
    }

    // blockConnected is called an extra time with no data and state=SYNCED to
    // indicate the end of syncing.
    if (block.state == BlockInfo::SYNCED) {
        assert(!block.data);
        m_index.m_state = BaseIndex::State::UPDATING;
        if (block.height >= 0) {
            LogInfo("%s is enabled at height %d", m_index.GetName(), block.height);
        } else {
            LogInfo("%s is enabled", m_index.GetName());
        }
        return;
    }

    if (m_index.IgnoreBlockConnected(role, block)) return;

    const CBlockIndex* pindex = &m_index.BlockIndex(block.hash);
    const CBlockIndex* best_block_index = m_index.m_best_block_index.load();
    // If a new block is being connected after blocks were rewound, reset the
    // rewind state and handle any errors that happened while rewinding.
    if (m_rewind_error) {
        m_index.FatalErrorf("Failed to rewind %s to a previous chain tip",
                   m_index.GetName());
        return;
    } else if (m_rewind_start) {
        assert(!best_block_index || best_block_index->GetAncestor(pindex->nHeight - 1) == pindex->pprev);
        // Don't commit here - the committed index state must never be ahead of the
        // flushed chainstate, otherwise unclean restarts would lead to index corruption.
        // Pruning has a minimum of 288 blocks-to-keep and getting the index
        // out of sync may be possible but a users fault.
        // In case we reorg beyond the pruned depth, ReadBlock would
        // throw and lead to a graceful shutdown
        m_index.SetBestBlockIndex(pindex->pprev);
        m_rewind_start = nullptr;
    }

    // Dispatch block to child class; errors are logged internally and abort the node.
    if (block.data && !m_index.Append(block)) return;

    NodeClock::time_point current_time{0s};
    if (block.state == BlockInfo::SYNCING) {
        current_time = NodeClock::now();
        if (current_time - m_last_log_time >= SYNC_LOG_INTERVAL) {
            LogInfo("Syncing %s with block chain from height %d", m_index.GetName(), pindex->nHeight);
            m_last_log_time = current_time;
        }
    }

    // If currently syncing with the chain and adding old blocks, not new ones,
    // decide whether to commit and update the best block pointer. Otherwise,
    // when adding new blocks, always update the best block pointer, and don't
    // commit until a chainstateFlush notification is received.
    if (block.state == BlockInfo::SYNCING) {
        // Commit if reached the last block flushed by the node, or if reached
        // the end of sync and no more data is being appended, or if enough time
        // has elapsed since the last commit.
        if (block.status == BlockInfo::FLUSHED_TIP || (block.status == BlockInfo::FLUSHED && current_time - m_last_locator_write_time >= SYNC_LOCATOR_WRITE_INTERVAL)) {
            auto locator = GetLocator(*m_index.m_chain, block.hash);
            m_last_locator_write_time = current_time;
            // No need to handle errors in Commit. If it fails, the error will be already be
            // logged. The best way to recover is to continue, as index cannot be corrupted by
            // a missed commit to disk for an advanced index state.
            m_index.Commit(locator);
        } else if (block.data) {
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
    m_index.SetBestBlockIndex(pindex);
}

void BaseIndexNotifications::blockDisconnected(const interfaces::BlockInfo& block_info)
{
    // Make a mutable copy of the BlockInfo argument so block data can be
    // attached below. This is temporary and removed in upcoming commits.
    interfaces::BlockInfo block{block_info};

    if (!block.error.empty()) {
        m_index.FatalErrorf("%s", block.error);
        return;
    }

    const CBlockIndex* pindex = &m_index.BlockIndex(block.hash);
    if (!m_rewind_start) m_rewind_start = pindex;
    if (m_rewind_error) return;

    assert(!m_options.disconnect_data || block.data);
    CBlockUndo block_undo;
    if (m_options.disconnect_undo_data && !block.undo_data && block.height > 0) {
        if (!m_index.m_chainstate->m_blockman.ReadBlockUndo(block_undo, *pindex)) {
            // If undo data can't be read, subsequent CustomRemove calls will be
            // skipped, and will be a fatal error if there an attempt to connect
            // a another block to the index.
            m_rewind_error = true;
            return;
        }
        block.undo_data = &block_undo;
    }

    // If one CustomRemove call fails, subsequent calls will be skipped,
    // and there will be a fatal error if there an attempt to connect
    // a another block to the index.
    if (!m_index.CustomRemove(block)) {
        m_rewind_error = true;
        return;
    }
}

void BaseIndexNotifications::chainStateFlushed(const ChainstateRole& role, const CBlockLocator& locator)
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
    m_index.Commit(GetLocator(*m_index.m_chain, m_index.m_best_block_index.load()->GetBlockHash()));
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

CBlockLocator BaseIndex::DB::ReadBestBlock() const
{
    CBlockLocator locator;

    bool success = Read(DB_BEST_BLOCK, locator);
    if (!success) {
        locator.SetNull();
    }

    return locator;
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
// If the chain tip and index best block are the same, the UPDATING state will
// be set so BlockConnected notifications will be processed after this call and
// the index will update during node startup as the node::ImportBlocks()
// function connects blocks. Otherwise the index will stay idle until
// ImportBlocks() finishes and BaseIndex::StartBackgroundSync() is called later.
//
// If the node is being started for the first time, or if -reindex or
// -reindex-chainstate are used, the chain tip will be null at this point,
// meaning that no blocks in the chain, even a genesis block. The best block
// locator will also be null if -reindex is used or if the index is new, but
// will be non-null if -reindex-chainstate is used. Therefore:
//
// * -reindex causes the index to rebuild as the chain is rebuilt
// * -reindex-chainstate causes the index to be idle until the chainstate is
//   rebuilt and BaseIndex::StartBackgroundSync is called later.
//
// This ensures indexes are synced in the most efficient way possible in each
// case.
bool BaseIndex::Init()
{
    AssertLockNotHeld(cs_main);

    // May need reset if index is being restarted (e.g. after assumeutxo background validation)
    m_state = State::SYNCING;
    m_best_block_index = nullptr;
    {
        LOCK(m_mutex);
        assert(!m_handler);
        assert(!m_notifications);
    }

    // m_chainstate member gives indexing code access to node internals. It is
    // removed in followup https://github.com/bitcoin/bitcoin/pull/24230
    m_chainstate = WITH_LOCK(::cs_main,
                             return &m_chain->context()->chainman->ValidatedChainstate());

    const auto locator{GetDB().ReadBestBlock()};

    if (locator.IsNull()) {
        SetBestBlockIndex(nullptr);
    } else {
        // Setting the best block to the locator's top block. If it is not part of the
        // best chain, we will rewind to the fork point during index sync
        const CBlockIndex* locator_index{WITH_LOCK(::cs_main, return m_chainstate->m_blockman.LookupBlockIndex(locator.vHave.at(0)))};
        if (!locator_index) {
            return InitError(Untranslated(strprintf("best block of %s not found. Please rebuild the index.", GetName())));
        }
        SetBestBlockIndex(locator_index);
    }

    // Child init
    const CBlockIndex* start_block = m_best_block_index.load();
    auto block{start_block ? std::make_optional(interfaces::BlockRef{start_block->GetBlockHash(), start_block->nHeight}) : std::nullopt};
    if (!CustomInit(block)) {
        return false;
    }

    // Register to receive validation interface notifications.
    auto options{CustomOptions()};
    options.thread_name = GetName();
    auto notifications{std::make_shared<BaseIndexNotifications>(*this)};
    auto handler = m_chain->attachChain(notifications, block ? std::make_optional(block->hash) : std::nullopt, options);
    LOCK(m_mutex);
    m_notifications = std::move(notifications);
    m_handler = std::move(handler);
    // If handler is already sending notifications, set state to UPDATING to
    // reflect this. The UPDATING state is only set here so methods like
    // BlockUntilSyncedToCurrentChain() and GetSummary() are immediately usable,
    // and this doesn't impact processing of notifications. (The state will
    // actually already be UPDATING at this point if the first blockConnected
    // notification was received.)
    if (m_handler->connected()) m_state = State::UPDATING;
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
            GetDB().WriteBatch(batch);
        }
    }
    if (!ok) {
        LogError("Failed to commit latest %s state", GetName());
        return false;
    }
    return true;
}

bool BaseIndex::IgnoreBlockConnected(const ChainstateRole& role, const interfaces::BlockInfo& block)
{
    // Ignore events from not fully validated chains to avoid out-of-order indexing.
    //
    // TODO at some point we could parameterize whether a particular index can be
    // built out of order, but for now just do the conservative simple thing.
    if (!role.validated) {
        return true;
    }

    if (block.state == BlockInfo::SYNCING) {
        return false;
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
        // Ensure block connects to an ancestor of the current best block.
        // To allow handling reorgs, this only checks that the new block
        // connects to ancestor of the current best block, instead of checking
        // that it connects to directly to the current block. If there is a
        // reorg, blockDisconnected calls will have removed existing blocks from
        // the index, but best_block_index will not have been updated yet.
        assert(best_block_index->GetAncestor(pindex->nHeight - 1) == pindex->pprev);
    }
    return false;
}

bool BaseIndex::IgnoreChainStateFlushed(const ChainstateRole& role, const CBlockLocator& locator)
{
    // Ignore events from not fully validated chains to avoid out-of-order indexing.
    if (!role.validated) {
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

    // Only commit if the locator points directly at the best block (the typical
    // case), or if it points to an ancestor of the best block (if a reorg
    // started or a block was invalidated).
    //
    // Otherwise, if the locator does not point to the best block or one of its
    // ancestors, it means this ChainStateFlushed notification was queued ahead
    // of relevant BlockConnected notifications from ConnectTrace in validation
    // code. This can happen when -reindex-chainstate is used and many blocks
    // are connected quickly. Ignore the flush event in this case since it's
    // probably better not to commit when blocks are still in flight.
    const CBlockIndex* best_block_index = m_best_block_index.load();
    if (best_block_index->GetAncestor(locator_tip_index->nHeight) != locator_tip_index) {
        LogWarning("Locator contains block (hash=%s) not on known best "
                  "chain (tip=%s); not writing index locator",
                  locator_tip_hash.ToString(),
                  best_block_index->GetBlockHash().ToString());
        return true;
    }

    return false;
}

bool BaseIndex::BlockUntilSyncedToCurrentChain() const
{
    AssertLockNotHeld(cs_main);

    if (m_state != State::UPDATING) {
        return false;
    }

    // best_block_index may be null if here this method is called immediately after Init().
    if (const CBlockIndex* index = m_best_block_index.load()) {
        interfaces::BlockRef best_block{index->GetBlockHash(), index->nHeight};
        // Skip the queue-draining stuff if we know we're caught up with
        // m_chain.Tip().
        interfaces::BlockRef tip;
        uint256 ancestor;
        if (m_chain->getTip(FoundBlock().hash(tip.hash).height(tip.height)) &&
            m_chain->findAncestorByHeight(best_block.hash, tip.height, FoundBlock().hash(ancestor)) &&
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
    m_handler->connect();
    return true;
}

void BaseIndex::WaitForBackgroundSync()
{
    auto* handler{WITH_LOCK(m_mutex, return m_handler.get())};
    if (!handler) throw std::logic_error("Error: Cannot wait for a stopped index");
    handler->sync();
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
    summary.synced = m_state == State::UPDATING;
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

std::shared_ptr<interfaces::Chain::Notifications> BaseIndex::Notifications() const
{
    LOCK(m_mutex);
    return m_notifications;
}
