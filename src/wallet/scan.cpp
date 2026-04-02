// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/chain.h>
#include <logging.h>
#include <sync.h>
#include <wallet/scan.h>
#include <wallet/wallet.h>

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

enum FilterRes {
    FILTER_NO_MATCH,
    FILTER_MATCH,
    FILTER_NO_FILTER,
};
} // namespace

class FilterExecutor {
public:
    FastWalletRescanFilter& m_filter;
    ChainScanner& m_scanner;
    ScanResult& m_result;
    int m_last_submitted_height = -1;

    virtual ~FilterExecutor() = default;
    virtual void Submit() = 0;
    virtual std::optional<FilterRes> TryPop() = 0;
    virtual bool ReadNextBlock() = 0;
    virtual size_t Pending() const = 0;

protected:
    explicit FilterExecutor(ChainScanner& scanner, FastWalletRescanFilter& filter, ScanResult& result)
        : m_filter(filter), m_scanner(scanner), m_result(result) {
            if (result.last_scanned_height)
                m_last_submitted_height = *result.last_scanned_height;
        }
public:
    bool CanSubmit() {
        auto& blocks = m_scanner.m_blocks;
        return !blocks.empty() && blocks.back().second > m_last_submitted_height;
    }
};

class InlineFilterExecutor : public FilterExecutor {
    std::deque<FilterRes> m_results;

public:
    explicit InlineFilterExecutor(ChainScanner& scanner, FastWalletRescanFilter& filter, ScanResult& result)
        : FilterExecutor(scanner, filter, result) {}

    void Submit() override {
        size_t i = 0;
        while (i < m_scanner.m_blocks.size() && m_scanner.m_blocks[i].second <= m_last_submitted_height) {
            i++;
        }
        if (i >= m_scanner.m_blocks.size()) return;

        auto& [hash, height] = m_scanner.m_blocks[i];
        auto matches = m_filter.MatchesBlock(hash);
        if (!matches.has_value())  m_results.push_back(FILTER_NO_FILTER);
        else if (*matches)         m_results.push_back(FILTER_MATCH);
        else                       m_results.push_back(FILTER_NO_MATCH);

        m_last_submitted_height = height;
    }

    std::optional<FilterRes> TryPop() override {
        if (m_results.empty()) return std::nullopt;
        auto front = m_results.front();
        m_results.pop_front();
        return front;
    }

    bool ReadNextBlock() override {
        if (!m_scanner.m_next_block) return false;
        auto block = m_scanner.ReadNextBlock(m_result);
        if (!block) return false;
        m_scanner.m_blocks.emplace_back(*block);
        return true;
    }

    size_t Pending() const override { return m_results.size(); }
};

std::optional<std::pair<uint256, int>> ChainScanner::ReadNextBlock(ScanResult& result) {
    if (!m_next_block) return std::nullopt;

    auto [block_hash, block_height] = *m_next_block;
    m_next_block.reset();

    bool block_still_active = false;
    bool has_next_block = false;
    uint256 next_block_hash;
    m_wallet.chain().findBlock(block_hash, FoundBlock().inActiveChain(block_still_active).nextBlock(FoundBlock().inActiveChain(has_next_block).hash(next_block_hash)));

    if (!block_still_active) {
        // Abort scan if current block is no longer active, to prevent
        // marking transactions as coming from the wrong block.
        result.last_failed_block = block_hash;
        result.status = ScanResult::FAILURE;
        return std::nullopt;
    }

    // Continue scanning if the next block exists, is within range, and hasn't reached the current tip.
    // If scanning with cs_wallet locked (AttachChain), blocks connected during rescan are handled
    // after scanning is complete via blockConnected notifications.
    // Without the lock, newly added blocks are re-processed if the notifications were handled
    // and the last block height was updated.
    if (has_next_block &&
        (!m_max_height || block_height < *m_max_height) &&
        block_height < WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHeight())) {
        m_next_block = {{next_block_hash, block_height + 1}};
    }

    return std::make_pair(block_hash, block_height);
}

size_t ChainScanner::ReadNextBlocks(const std::unique_ptr<FastWalletRescanFilter>& filter, ScanResult& result) {
    if (!filter) {
        // Slow scan: no block filter index, inspect all blocks.
        auto next_block = ReadNextBlock(result);
        if (!next_block) return 0;
        m_blocks.emplace_back(*next_block);
        return 1;
    }

    filter->UpdateIfNeeded();
    auto executor = InlineFilterExecutor(*this, *filter, result);

    size_t matched_count = 0;
    while (executor.ReadNextBlock() || executor.CanSubmit() || executor.Pending() > 0) {
        if (auto filter_res = executor.TryPop()) {
            const auto& [hash, height] = m_blocks[matched_count];
            if (*filter_res == FILTER_NO_MATCH) {
                if (matched_count > 0) {
                    // Stop scanning just before this block.
                    break;
                }
                // No match, this block will be skipped
                result.last_scanned_block = hash;
                result.last_scanned_height = height;
                m_progress_current = m_wallet.chain().guessVerificationProgress(hash);
                m_blocks.pop_front();
            } else {
                if (*filter_res == FILTER_MATCH)
                    LogDebug(BCLog::SCAN, "Fast rescan: inspect block %d [%s] (filter matched)\n", height, hash.ToString());
                else // FILTER_NO_FILTER
                    LogDebug(BCLog::SCAN, "Fast rescan: inspect block %d [%s] (WARNING: block filter not found!)\n", height, hash.ToString());

                matched_count++;
            }
        }
        executor.Submit();
    }
    return matched_count;
}

void ChainScanner::UpdateProgress(int block_height) {
    if (m_progress_end - m_progress_begin > 0.0) {
        m_wallet.m_scanning_progress = (m_progress_current - m_progress_begin) / (m_progress_end - m_progress_begin);
    } else {
        // avoid divide-by-zero for single block scan range (i.e. start and stop hashes are equal)
        m_wallet.m_scanning_progress = 0;
    }
    if (block_height % 100 == 0 && m_progress_end - m_progress_begin > 0.0) {
        m_wallet.ShowProgress(strprintf("[%s] %s", m_wallet.DisplayName(), _("Rescanning…")),
                              std::max(1, std::min(99, (int)(m_wallet.m_scanning_progress * 100))));
    }
}

void ChainScanner::UpdateTipIfChanged() {
    const uint256 new_tip = WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHash());
    if (!m_max_height && new_tip != m_tip_hash) {
        m_tip_hash = new_tip;
        m_progress_end = m_wallet.chain().guessVerificationProgress(m_tip_hash);
    }
}

void ChainScanner::ProcessBlock(const uint256& block_hash, int block_height, bool next_interval, ScanResult& result) {
    if (ScanBlock(block_hash, block_height, m_save_progress && next_interval)) {
        result.last_scanned_block = block_hash;
        result.last_scanned_height = block_height;
    } else {
        result.last_failed_block = block_hash;
        result.status = ScanResult::FAILURE;
    }
}

bool ChainScanner::ScanBlock(const uint256& block_hash, int block_height, bool save_progress) {
    // Read block data and locator if needed (the locator is usually null unless we need to save progress)
    CBlock block;
    CBlockLocator loc;
    // Find block
    FoundBlock found_block{FoundBlock().data(block)};
    if (save_progress) found_block.locator(loc);
    m_wallet.chain().findBlock(block_hash, found_block);

    if (block.IsNull()) return false;

    {
        LOCK(m_wallet.cs_wallet);
        for (size_t posInBlock = 0; posInBlock < block.vtx.size(); ++posInBlock) {
            m_wallet.SyncTransaction(
                block.vtx[posInBlock], TxStateConfirmed{block_hash, block_height,
                static_cast<int>(posInBlock)}, m_fUpdate,
                /*rescanning_old_block=*/true);
        }

        if (!loc.IsNull()) {
            m_wallet.WalletLogPrintf("Saving scan progress %d.\n", block_height);
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

    m_tip_hash = WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHash());
    uint256 end_hash = m_tip_hash;
    if (m_max_height) chain.findAncestorByHeight(m_tip_hash, *m_max_height, FoundBlock().hash(end_hash));

    ScanResult result;
    m_progress_begin = chain.guessVerificationProgress(m_start_block);
    m_progress_end = chain.guessVerificationProgress(end_hash);
    m_progress_current = m_progress_begin;
    int block_height = m_start_height;
    m_next_block = {{m_start_block, m_start_height}};

    while ((m_next_block || !m_blocks.empty()) && !m_wallet.fAbortRescan && !chain.shutdownRequested()) {
        auto matched_blocks_count = ReadNextBlocks(fast_rescan_filter, result);
        for (size_t i = 0; i < matched_blocks_count; ++i) {
            auto block_hash = m_blocks.front().first;
            block_height = m_blocks.front().second;
            m_blocks.pop_front();

            UpdateProgress(block_height);

            bool next_interval = m_reserver.now() >= current_time + INTERVAL_TIME;
            if (next_interval) {
                current_time = m_reserver.now();
                m_wallet.WalletLogPrintf("Still rescanning. At block %d. Progress=%f\n", block_height, m_progress_current);
            }

            ProcessBlock(block_hash, block_height, next_interval, result);
            m_progress_current = chain.guessVerificationProgress(block_hash);
            if (!m_max_height) UpdateTipIfChanged();
        }
    }
    if (!m_max_height) {
        m_wallet.WalletLogPrintf("Scanning current mempool transactions.\n");
        WITH_LOCK(m_wallet.cs_wallet, chain.requestMempoolTransactions(m_wallet));
    }
    m_wallet.ShowProgress(strprintf("[%s] %s", m_wallet.DisplayName(), _("Rescanning...")), 100);
    if (block_height && m_wallet.fAbortRescan) {
        m_wallet.WalletLogPrintf("Rescan aborted at block %d. Progress=%f\n", block_height, m_progress_current);
        result.status = ScanResult::USER_ABORT;
    } else if (block_height && chain.shutdownRequested()) {
        m_wallet.WalletLogPrintf("Rescan interrupted by shutdown request at block %d. Progress=%f\n", block_height, m_progress_current);
        result.status = ScanResult::USER_ABORT;
    } else {
        m_wallet.WalletLogPrintf("Rescan completed in %15dms\n", Ticks<std::chrono::milliseconds>(m_reserver.now() - start_time));
    }
    return result;
}
}
