// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/chain.h>
#include <logging.h>
#include <sync.h>
#include <wallet/scan.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <future>

using interfaces::FoundBlock;

namespace wallet {

/**
 * Scan the block chain (starting in start_block) for transactions
 * from or to us. If fUpdate is true, found transactions that already
 * exist in the wallet will be updated. If max_height is not set, the
 * mempool will be scanned as well.
 *
 * @param[in] start_block Scan starting block. If block is not on the active
 *                        chain, the scan will return SUCCESS immediately.
 * @param[in] start_height Height of start_block
 * @param[in] max_height  Optional max scanning height. If unset there is
 *                        no maximum and scanning can continue to the tip
 *
 * @return ScanResult returning scan information and indicating success or
 *         failure. Return status will be set to SUCCESS if scan was
 *         successful. FAILURE if a complete rescan was not possible (due to
 *         pruning or corruption). USER_ABORT if the rescan was aborted before
 *         it could complete.
 *
 * @pre Caller needs to make sure start_block (and the optional stop_block) are on
 * the main chain after the addition of any new keys you want to detect
 * transactions for.
 */
ScanResult CWallet::ScanForWalletTransactions(const uint256& start_block, int start_height, std::optional<int> max_height, const WalletRescanReserver& reserver, bool fUpdate, bool save_progress)
{
    ChainScanner scanner{*this, reserver, start_block, start_height, max_height, fUpdate, save_progress};
    return scanner.Scan();
}

namespace {
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
} // namespace

bool ChainScanner::ReadBlockHash(ScanResult& result) {
    bool block_still_active = false;
    bool next_block = false;
    int block_height = m_next_block_height;
    uint256 block_hash = m_next_block_hash;
    m_wallet.chain().findBlock(block_hash, FoundBlock().inActiveChain(block_still_active).nextBlock(FoundBlock().inActiveChain(next_block).hash(m_next_block_hash)));
    m_blocks.emplace_back(block_hash, block_height);
    ++m_next_block_height;
    if (!block_still_active) {
        // Abort scan if current block is no longer active, to prevent
        // marking transactions as coming from the wrong block.
        result.last_failed_block = block_hash;
        result.status = ScanResult::FAILURE;
        return false;
    }
    if (m_max_height && block_height >= *m_max_height) return false;
    // If rescanning was triggered with cs_wallet permanently locked (AttachChain), additional blocks that were connected during the rescan
    // aren't processed here but will be processed with the pending blockConnected notifications after the lock is released.
    // If rescanning without a permanent cs_wallet lock, additional blocks that were added during the rescan will be re-processed if
    // the notification was processed and the last block height was updated.
    if (block_height >= WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHeight())) {
        return false;
    }
    if (!next_block) return false;
    return true;
}

enum FilterRes {
    FILTER_NO_MATCH,
    FILTER_MATCH,
    FILTER_NO_FILTER
};

std::optional<std::pair<size_t, size_t>> ChainScanner::FilterBlocks(const std::unique_ptr<FastWalletRescanFilter>& filter, ScanResult& result) {
    if (!filter) {
        // Slow scan: no filter, scan all blocks
        m_continue = ReadBlockHash(result);
        return std::make_pair<size_t, size_t>(0, m_blocks.size());
    }
    filter->UpdateIfNeeded();
    auto* thread_pool = m_wallet.m_thread_pool;
    // ThreadPool pointer should never be null here
    // during normal operation because it should
    // have been passed from the WalletContext to
    // the CWallet constructor
    assert(thread_pool != nullptr);
    std::optional<std::pair<size_t, size_t>> range;
    size_t workers_count = thread_pool->WorkersCount();
    size_t i = 0;
    size_t completed = 0;
    std::vector<std::future<FilterRes>> futures;
    futures.reserve(workers_count);

    while ((m_continue && completed < m_max_blockqueue_size) || i < m_blocks.size() || !futures.empty()) {
        const bool result_ready = futures.size() > 0 && futures[0].wait_for(std::chrono::seconds::zero()) == std::future_status::ready;
        if (result_ready) {
            const auto& scan_res{futures[0].get()};
            completed++;
            futures.erase(futures.begin());
            if (scan_res == FILTER_NO_MATCH) {
                if (range.has_value()) {
                    break;
                }
                continue;
            }
            int current_block_index = completed - 1;
            if (scan_res == FilterRes::FILTER_MATCH) {
                LogDebug(BCLog::SCAN, "Fast rescan: inspect block %d [%s] (filter matched)\n", m_blocks[current_block_index].second, m_blocks[current_block_index].first.ToString());
            } else { // FILTER_NO_FILTER
                LogDebug(BCLog::SCAN, "Fast rescan: inspect block %d [%s] (WARNING: block filter not found!)\n", m_blocks[current_block_index].second, m_blocks[current_block_index].first.ToString());
            }

            if (!range.has_value()) range = std::make_pair(current_block_index, current_block_index + 1);
            else range->second = current_block_index + 1;
        }

        const size_t job_gap = workers_count - futures.size();
        if (job_gap > 0 && i < m_blocks.size()) {
            for (size_t j = 0; j < job_gap && i < m_blocks.size(); ++j, ++i) {
                auto block = m_blocks[i];
                futures.emplace_back(thread_pool->Submit([&filter, block = std::move(block)]() {
                    const auto matches_block{filter->MatchesBlock(block.first)};
                    if (matches_block.has_value()) {
                        if (*matches_block) {
                            return FilterRes::FILTER_MATCH;
                        } else {
                            return FilterRes::FILTER_NO_MATCH;
                        }
                    } else {
                        return FilterRes::FILTER_NO_FILTER;
                    }
                }));
            }
        }

        // If m_max_blockqueue_size blocks have been filtered,
        // stop reading more blocks for now, to give the
        // main scanning loop a chance to update progress
        // and erase some blocks from the queue.
        if (m_continue && completed < m_max_blockqueue_size) m_continue = ReadBlockHash(result);
        else if (!futures.empty()) thread_pool->ProcessTask();
    }

    for (auto& fut : futures) {
        // Wait for all remaining futures to complete
        // to avoid data race on FastWalletRescanFilter::m_filter_set
        fut.wait();
    }

    return range;
}

bool ChainScanner::ScanBlock(const std::pair<uint256, int>& data, bool save_progress) {
    // Read block data and locator if needed (the locator is usually null unless we need to save progress)
    CBlock block;
    CBlockLocator loc;
    // Find block
    FoundBlock found_block{FoundBlock().data(block)};
    if (save_progress) found_block.locator(loc);
    m_wallet.chain().findBlock(data.first, found_block);

    if (block.IsNull()) return false;

    {
        LOCK(m_wallet.cs_wallet);
        for (size_t posInBlock = 0; posInBlock < block.vtx.size(); ++posInBlock) {
            m_wallet.SyncTransaction(
                block.vtx[posInBlock], TxStateConfirmed{data.first, data.second,
                static_cast<int>(posInBlock)}, m_fUpdate,
                /*rescanning_old_block=*/true);
        }

        if (!loc.IsNull()) {
            m_wallet.WalletLogPrintf("Saving scan progress %d.\n", data.second);
            WalletBatch batch(m_wallet.GetDatabase());
            batch.WriteBestBlock(loc);
        }
    }
    return true;
}

ScanResult ChainScanner::Scan() {
    constexpr auto INTERVAL_TIME{60s};
    auto current_time{m_reserver.now()};
    auto start_time{m_reserver.now()};

    assert(m_reserver.isReserved());
    auto& chain = m_wallet.chain();

    std::unique_ptr<FastWalletRescanFilter> fast_rescan_filter;
    if (chain.hasBlockFilterIndex(BlockFilterType::BASIC)) fast_rescan_filter = std::make_unique<FastWalletRescanFilter>(m_wallet);

    m_wallet.WalletLogPrintf("Rescan started from block %s... (%s)\n", m_start_block.ToString(),
                fast_rescan_filter ? "fast variant using block filters" : "slow variant inspecting all blocks");

    m_wallet.fAbortRescan = false;
    // show rescan progress in GUI as dialog or on splashscreen, if rescan required on startup (e.g. due to corruption)
    m_wallet.ShowProgress(strprintf("[%s] %s", m_wallet.DisplayName(), _("Rescanning…")), 0);
    uint256 tip_hash = WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHash());
    uint256 end_hash = tip_hash;
    if (m_max_height) chain.findAncestorByHeight(tip_hash, *m_max_height, FoundBlock().hash(end_hash));

    ScanResult result;

    double progress_begin = chain.guessVerificationProgress(m_start_block);
    double progress_end = chain.guessVerificationProgress(end_hash);
    double progress_current = progress_begin;

    auto start_block = m_start_block;
    auto& block_hash = start_block;
    int block_height = m_start_height;
    m_next_block_height = m_start_height;
    m_next_block_hash = m_start_block;

    size_t start_index = 0;
    size_t end_index = 0;
    while((m_continue || !m_blocks.empty()) && !m_wallet.fAbortRescan && !chain.shutdownRequested()) {
        auto range = FilterBlocks(fast_rescan_filter, result);
        // If no blocks to scan, mark current batch as scanned
        start_index = range.has_value() ? range->first : m_blocks.size();
        end_index = range.has_value() ? range->second : m_blocks.size();
        if (start_index > 0) {
            // Some blocks at the start of the batch were skipped.
            // Update last scanned block to indicate that these
            // blocks have been scanned.
            block_hash = m_blocks[start_index - 1].first;
            block_height = m_blocks[start_index - 1].second;
            result.last_scanned_block = block_hash;
            result.last_scanned_height = block_height;
        }

        for (size_t i = start_index; i < end_index; ++i) {
            block_hash = m_blocks[i].first;
            block_height = m_blocks[i].second;
            if (progress_end - progress_begin > 0.0) {
                m_wallet.m_scanning_progress = (progress_current - progress_begin) / (progress_end - progress_begin);
            } else { // avoid divide-by-zero for single block scan range (i.e. start and stop hashes are equal)
                m_wallet.m_scanning_progress = 0;
            }
            if (block_height % 100 == 0 && progress_end - progress_begin > 0.0) {
                m_wallet.ShowProgress(strprintf("[%s] %s", m_wallet.DisplayName(), _("Rescanning…")), std::max(1, std::min(99, (int)(m_wallet.m_scanning_progress * 100))));
            }

            bool next_interval = m_reserver.now() >= current_time + INTERVAL_TIME;
            if (next_interval) {
                current_time = m_reserver.now();
                m_wallet.WalletLogPrintf("Still rescanning. At block %d. Progress=%f\n", block_height, progress_current);
            }

            if (ScanBlock(m_blocks[i], m_save_progress && next_interval)) {
                result.last_scanned_block = block_hash;
                result.last_scanned_height = block_height;
            } else {
                // could not scan block, keep scanning but record this block as the most recent failure
                result.last_failed_block = block_hash;
                result.status = ScanResult::FAILURE;
            }
        }

        m_blocks.erase(m_blocks.begin(), m_blocks.begin() + end_index);

        progress_current = chain.guessVerificationProgress(block_hash);

        // handle updated tip hash
        const uint256 prev_tip_hash = tip_hash;
        tip_hash = WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHash());
        if (!m_max_height && prev_tip_hash != tip_hash) {
            // in case the tip has changed, update progress max
            progress_end = chain.guessVerificationProgress(tip_hash);
        }
    }

    if (!m_max_height) {
        m_wallet.WalletLogPrintf("Scanning current mempool transactions.\n");
        WITH_LOCK(m_wallet.cs_wallet, chain.requestMempoolTransactions(m_wallet));
    }

    m_wallet.ShowProgress(strprintf("[%s] %s", m_wallet.DisplayName(), _("Rescanning…")), 100); // hide progress dialog in GUI
    if (block_height && m_wallet.fAbortRescan) {
        m_wallet.WalletLogPrintf("Rescan aborted at block %d. Progress=%f\n", block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else if (block_height && chain.shutdownRequested()) {
        m_wallet.WalletLogPrintf("Rescan interrupted by shutdown request at block %d. Progress=%f\n", block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else {
        m_wallet.WalletLogPrintf("Rescan completed in %15dms\n", Ticks<std::chrono::milliseconds>(m_reserver.now() - start_time));
    }
    return result;
}
}
