// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <interfaces/chain.h>
#include <logging.h>
#include <primitives/block.h>
#include <sync.h>
#include <util/check.h>
#include <wallet/scan.h>
#include <wallet/wallet.h>

#include <memory>

using interfaces::FoundBlock;

namespace wallet {

int64_t ChainScanner::ScanFromTime(int64_t startTime, const WalletRescanReserver& reserver)
{
    // Find starting block. May be null if nCreateTime is greater than the
    // highest blockchain timestamp, in which case there is nothing that needs
    // to be scanned.
    int start_height = 0;
    uint256 start_block;
    bool start = m_wallet.chain().findFirstBlockWithTimeAndHeight(startTime - TIMESTAMP_WINDOW, 0, FoundBlock().hash(start_block).height(start_height));
    m_wallet.WalletLogPrintf("%s: Rescanning last %i blocks", __func__, start ? WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHeight()) - start_height + 1 : 0);

    if (start) {
        // TODO: this should take into account failure by ScanResult::USER_ABORT
        ScanResult result = Scan(start_block, start_height, /*max_height=*/{}, reserver, /*save_progress=*/false);
        if (result.status == ScanResult::FAILURE) {
            int64_t time_max;
            CHECK_NONFATAL(m_wallet.chain().findBlock(result.last_failed_block, FoundBlock().maxTime(time_max)));
            return time_max + TIMESTAMP_WINDOW + 1;
        }
    }
    return startTime;
}

bool WalletRescanReserver::reserve(bool with_passphrase) {
    assert(!m_could_reserve);
    if (!m_wallet.Scanner().TryReserve(with_passphrase)) {
        return false;
    }
    m_could_reserve = true;
    return true;
}

bool WalletRescanReserver::isReserved() const {
    return (m_could_reserve && m_wallet.Scanner().IsScanning());
}

WalletRescanReserver::~WalletRescanReserver() {
    if (m_could_reserve) {
        m_wallet.Scanner().Release();
    }
}

bool ChainScanner::TryReserve(bool with_passphrase) {
    if (m_scanning.exchange(true)) return false;
    // Discard any abort request left over from previous reservation, so
    // that an abort requested while the reservation is held always applies
    // to abort this rescan, even if it arrives before the scan loop starts.
    m_abort = false;
    m_scanning_with_passphrase = with_passphrase;
    m_scanning_start = SteadyClock::now();
    m_scanning_progress = 0;
    return true;
}

void ChainScanner::Release() {
    m_scanning = false;
    m_scanning_with_passphrase = false;
}

class FastWalletRescanFilter
{
public:
    FastWalletRescanFilter(const CWallet& wallet) : m_wallet(wallet)
    {
        // create initial filter with scripts from all ScriptPubKeyMans
        for (auto spkm : m_wallet.GetAllScriptPubKeyMans()) {
            auto desc_spkm{dynamic_cast<DescriptorScriptPubKeyMan*>(spkm)};
            assert(desc_spkm != nullptr);
            AddScriptPubKeys(desc_spkm);
            // save each range descriptor's end for possible future filter updates
            if (desc_spkm->IsHDEnabled()) {
                m_last_range_ends.emplace(desc_spkm->GetID(), desc_spkm->GetEndRange());
            }
        }
    }

    void UpdateIfNeeded()
    {
        // repopulate filter with new scripts if top-up has happened since last iteration
        for (const auto& [desc_spkm_id, last_range_end] : m_last_range_ends) {
            auto desc_spkm{dynamic_cast<DescriptorScriptPubKeyMan*>(m_wallet.GetScriptPubKeyMan(desc_spkm_id))};
            assert(desc_spkm != nullptr);
            int32_t current_range_end{desc_spkm->GetEndRange()};
            if (current_range_end > last_range_end) {
                AddScriptPubKeys(desc_spkm, last_range_end);
                m_last_range_ends.at(desc_spkm->GetID()) = current_range_end;
            }
        }
    }

    std::optional<bool> MatchesBlock(const uint256& block_hash) const
    {
        return m_wallet.chain().blockFilterMatchesAny(BlockFilterType::BASIC, block_hash, m_filter_set);
    }

private:
    const CWallet& m_wallet;
    /** Map for keeping track of each range descriptor's last seen end range.
      * This information is used to detect whether new addresses were derived
      * (that is, if the current end range is larger than the saved end range)
      * after processing a block and hence a filter set update is needed to
      * take possible keypool top-ups into account.
      */
    std::map<uint256, int32_t> m_last_range_ends;
    GCSFilter::ElementSet m_filter_set;

    void AddScriptPubKeys(const DescriptorScriptPubKeyMan* desc_spkm, int32_t last_range_end = 0)
    {
        for (const auto& script_pub_key : desc_spkm->GetScriptPubKeys(last_range_end)) {
            m_filter_set.emplace(script_pub_key.begin(), script_pub_key.end());
        }
    }
};

struct ChainScanner::ScanContext {
    explicit ScanContext(std::optional<int> max_height_in) : max_height{max_height_in} {}

    ScanResult result;
    //! Optional height limit of the scan range.
    const std::optional<int> max_height;
    //! The next block to read, if any.
    std::optional<std::pair<uint256, int>> next_block;
    std::unique_ptr<FastWalletRescanFilter> filter;
    //! Verification progress of the last scanned or skipped block.
    double progress_current{0};
    //! Hash and height of the last scanned, skipped or failed block.
    uint256 block_hash;
    int block_height{0};
    //! Set once the wallet's tip is reached or a block to scan left the active chain.
    bool done{false};
};

std::optional<ChainScanner::QueuedBlock> ChainScanner::ReadNextBlock(ScanContext& ctx) {
    if (!ctx.next_block) return std::nullopt;
    const auto [block_hash, block_height] = *ctx.next_block;
    // Blocks connected after this lookup are picked up by Scan() once the
    // wallet's tip moves past the last processed block.
    ctx.next_block = FindNextBlock(ctx, block_hash, block_height);
    return QueuedBlock{block_hash, block_height};
}

std::optional<std::pair<uint256, int>> ChainScanner::FindNextBlock(const ScanContext& ctx, const uint256& block_hash, int block_height) {
    bool has_next_block = false;
    uint256 next_block_hash;
    m_wallet.chain().findBlock(block_hash, FoundBlock().nextBlock(FoundBlock().inActiveChain(has_next_block).hash(next_block_hash)));
    if (!has_next_block || (ctx.max_height && block_height >= *ctx.max_height)) return std::nullopt;
    return {{next_block_hash, block_height + 1}};
}

void ChainScanner::RecordSkipped(ScanContext& ctx, const QueuedBlock& block) {
    ctx.result.last_scanned_block = block.hash;
    ctx.result.last_scanned_height = block.height;
    ctx.block_hash = block.hash;
    ctx.block_height = block.height;
    ctx.progress_current = m_wallet.chain().guessVerificationProgress(block.hash);
    StopAtWalletTip(ctx, block.height);
}

void ChainScanner::StopAtWalletTip(ScanContext& ctx, int block_height) {
    // Re-read the wallet's tip height after the block was processed so a tip
    // extension that happened meanwhile is picked up. If scanning with
    // cs_wallet locked (AttachChain), blocks connected during rescan are
    // handled after scanning is complete via blockConnected notifications.
    // Without the lock, newly added blocks are re-processed here if the
    // notifications were handled and the last block height was updated.
    if (block_height >= WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHeight())) ctx.done = true;
}

std::vector<ChainScanner::QueuedBlock> ChainScanner::NextBlocksSerial(ScanContext& ctx) {
    const auto block = ReadNextBlock(ctx);
    if (!block) return {};

    if (!ctx.filter) {
        // Slow scan: no block filter index, inspect all blocks.
        return {*block};
    }

    ctx.filter->UpdateIfNeeded();

    auto matches_block{ctx.filter->MatchesBlock(block->hash)};
    if (matches_block.has_value() && *matches_block) {
        LogDebug(BCLog::SCAN, "Fast rescan: inspect block %d [%s] (filter matched)\n", block->height, block->hash.ToString());
        return {*block};
    } else if (!matches_block.has_value()) {
        LogDebug(BCLog::SCAN, "Fast rescan: inspect block %d [%s] (WARNING: block filter not found!)\n", block->height, block->hash.ToString());
        return {*block};
    }

    // No match: this block needs no inspection and is skipped, even if it
    // left the active chain.
    RecordSkipped(ctx, *block);
    return {};
}

bool ChainScanner::HasPendingBlocks(const ScanContext& ctx) {
    return ctx.next_block.has_value();
}

void ChainScanner::UpdateProgress(const LoopState& state, double progress_current, int block_height) {
    m_scanning_progress = 0;
    double progress_diff = state.progress_end - state.progress_begin;

    // avoid divide-by-zero for single block scan range (i.e. start and stop hashes are equal)
    if (progress_diff <= 0.0) return;
    m_scanning_progress = (progress_current - state.progress_begin) / progress_diff;

    if (block_height % 100 == 0) {
        m_wallet.ShowProgress(strprintf("[%s] %s", m_wallet.DisplayName(), _("Rescanning…")),
                              std::max(1, std::min(99, (int)(m_scanning_progress.load() * 100))));
    }
}

void ChainScanner::UpdateTipIfChanged(LoopState& state) {
    const uint256 new_tip = WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHash());
    if (new_tip != state.tip_hash) {
        state.tip_hash = new_tip;
        state.progress_end = m_wallet.chain().guessVerificationProgress(state.tip_hash);
    }
}

ChainScanner::ScanBlockResult ChainScanner::ScanBlock(const uint256& block_hash, int block_height, bool save_progress) {
    // Read block data and locator if needed (the locator is usually null unless we need to save progress)
    CBlock block;
    CBlockLocator loc;
    // Find block
    FoundBlock found_block{FoundBlock().data(block)};
    if (save_progress) found_block.locator(loc);
    m_wallet.chain().findBlock(block_hash, found_block);

    if (block.IsNull()) return ScanBlockResult::READ_FAILED;

    {
        // cs_wallet is a RecursiveMutex; ScanBlock may be called
        // with cs_wallet already held as in AttachChain or without it.
        LOCK(m_wallet.cs_wallet);
        // Check the block is still active with cs_wallet held, to prevent
        // marking transactions as coming from the wrong block. If it is
        // disconnected after this check, the blockDisconnected notification,
        // which needs cs_wallet, runs after the transactions are synced and
        // undoes their confirmation.
        bool active{false};
        m_wallet.chain().findBlock(block_hash, FoundBlock().inActiveChain(active));
        if (!active) return ScanBlockResult::NOT_ACTIVE;
        for (size_t posInBlock = 0; posInBlock < block.vtx.size(); ++posInBlock) {
            m_wallet.SyncTransaction(
                block.vtx[posInBlock], TxStateConfirmed{block_hash, block_height,
                static_cast<int>(posInBlock)},
                /*rescanning_old_block=*/true);
        }

        if (!loc.IsNull()) {
            m_wallet.WalletLogPrintf("Saving scan progress %d.", block_height);
            WalletBatch batch(m_wallet.GetDatabase());
            batch.WriteBestBlock(loc);
        }
    }
    return ScanBlockResult::SCANNED;
}

ScanResult ChainScanner::Scan(const uint256& start_block, int start_height, std::optional<int> max_height,
                              const WalletRescanReserver& reserver, bool save_progress) {
    constexpr auto INTERVAL_TIME{60s};
    auto current_time{reserver.now()};
    auto start_time{reserver.now()};

    assert(reserver.isReserved());
    auto& chain = m_wallet.chain();

    ScanContext ctx{max_height};
    if (chain.hasBlockFilterIndex(BlockFilterType::BASIC)) ctx.filter = std::make_unique<FastWalletRescanFilter>(m_wallet);

    m_wallet.WalletLogPrintf("Rescan started from block %s... (%s)", start_block.ToString(),
                ctx.filter ? "fast variant using block filters" : "slow variant inspecting all blocks");

    // show rescan progress in GUI as dialog or on splashscreen, if rescan required on startup (e.g. due to corruption)
    m_wallet.ShowProgress(strprintf("[%s] %s", m_wallet.DisplayName(), _("Rescanning…")), 0);

    auto& result = ctx.result;
    LoopState state;
    state.tip_hash = WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHash());
    uint256 end_hash = state.tip_hash;
    if (max_height) chain.findAncestorByHeight(state.tip_hash, *max_height, FoundBlock().hash(end_hash));
    state.progress_begin = chain.guessVerificationProgress(start_block);
    state.progress_end = chain.guessVerificationProgress(end_hash);
    ctx.progress_current = state.progress_begin;
    ctx.next_block = {{start_block, start_height}};
    ctx.block_hash = start_block;
    ctx.block_height = start_height;
    // Report progress for the given height, logging every INTERVAL_TIME.
    // Returns whether the interval elapsed.
    const auto update_progress = [&](int height) {
        UpdateProgress(state, ctx.progress_current, height);
        const bool next_interval = reserver.now() >= current_time + INTERVAL_TIME;
        if (next_interval) {
            current_time = reserver.now();
            m_wallet.WalletLogPrintf("Still rescanning. At block %d. Progress=%f", height, ctx.progress_current);
        }
        return next_interval;
    };
    while (!ctx.done && HasPendingBlocks(ctx) && !m_abort && !chain.shutdownRequested()) {
        const std::vector<QueuedBlock> blocks_to_scan = NextBlocksSerial(ctx);
        // Only skipped blocks were processed; report progress for the last one.
        if (blocks_to_scan.empty()) update_progress(ctx.block_height);
        for (const QueuedBlock& block : blocks_to_scan) {
            ctx.block_hash = block.hash;
            ctx.block_height = block.height;

            ctx.progress_current = chain.guessVerificationProgress(block.hash);
            const bool next_interval = update_progress(block.height);

            const ScanBlockResult scan_result{ScanBlock(block.hash, block.height, save_progress && next_interval)};
            if (scan_result == ScanBlockResult::NOT_ACTIVE) {
                // Abort scan if a block that needs to be inspected is no longer
                // active. A block skipped by the filter stays skipped: it has
                // no successor in the active chain, so the scan ends
                // successfully at the reorg point and the replacement blocks
                // are handled by blockConnected notifications.
                result.last_failed_block = block.hash;
                result.status = ScanResult::FAILURE;
                ctx.done = true;
                break;
            }
            if (scan_result == ScanBlockResult::SCANNED) {
                result.last_scanned_block = block.hash;
                result.last_scanned_height = block.height;
            } else {
                // could not scan block, keep scanning but record this block as the most recent failure
                result.last_failed_block = block.hash;
                result.status = ScanResult::FAILURE;
            }

            StopAtWalletTip(ctx, block.height);
            if (ctx.done) break;
        }
        if (!ctx.done && !ctx.max_height) {
            UpdateTipIfChanged(state);
            // The wallet's tip moved past the last processed block after its
            // successor was looked up: continue from its successor, if any.
            if (!HasPendingBlocks(ctx)) ctx.next_block = FindNextBlock(ctx, ctx.block_hash, ctx.block_height);
        }
    }
    if (!max_height) {
        m_wallet.WalletLogPrintf("Scanning current mempool transactions.");
        WITH_LOCK(m_wallet.cs_wallet, chain.requestMempoolTransactions(m_wallet));
    }
    m_wallet.ShowProgress(strprintf("[%s] %s", m_wallet.DisplayName(), _("Rescanning…")), 100); // hide progress dialog in GUI
    if (m_abort) {
        m_wallet.WalletLogPrintf("Rescan aborted at block %d. Progress=%f", ctx.block_height, ctx.progress_current);
        result.status = ScanResult::USER_ABORT;
    } else if (chain.shutdownRequested()) {
        m_wallet.WalletLogPrintf("Rescan interrupted by shutdown request at block %d. Progress=%f", ctx.block_height, ctx.progress_current);
        result.status = ScanResult::USER_ABORT;
    } else {
        m_wallet.WalletLogPrintf("Rescan completed in %15dms", Ticks<std::chrono::milliseconds>(reserver.now() - start_time));
    }
    return result;
}
}
