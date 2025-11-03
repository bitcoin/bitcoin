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
} // namespace

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
    uint256 block_hash = m_start_block;
    double progress_begin = chain.guessVerificationProgress(block_hash);
    double progress_end = chain.guessVerificationProgress(end_hash);
    double progress_current = progress_begin;
    int block_height = m_start_height;
    while (!m_wallet.fAbortRescan && !chain.shutdownRequested()) {
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

        bool fetch_block{true};
        if (fast_rescan_filter) {
            fast_rescan_filter->UpdateIfNeeded();
            auto matches_block{fast_rescan_filter->MatchesBlock(block_hash)};
            if (matches_block.has_value()) {
                if (*matches_block) {
                    LogDebug(BCLog::SCAN, "Fast rescan: inspect block %d [%s] (filter matched)\n", block_height, block_hash.ToString());
                } else {
                    result.last_scanned_block = block_hash;
                    result.last_scanned_height = block_height;
                    fetch_block = false;
                }
            } else {
                LogDebug(BCLog::SCAN, "Fast rescan: inspect block %d [%s] (WARNING: block filter not found!)\n", block_height, block_hash.ToString());
            }
        }

        // Find next block separately from reading data above, because reading
        // is slow and there might be a reorg while it is read.
        bool block_still_active = false;
        bool next_block = false;
        uint256 next_block_hash;
        chain.findBlock(block_hash, FoundBlock().inActiveChain(block_still_active).nextBlock(FoundBlock().inActiveChain(next_block).hash(next_block_hash)));

        if (fetch_block) {
            // Read block data and locator if needed (the locator is usually null unless we need to save progress)
            CBlock block;
            CBlockLocator loc;
            // Find block
            FoundBlock found_block{FoundBlock().data(block)};
            if (m_save_progress && next_interval) found_block.locator(loc);
            chain.findBlock(block_hash, found_block);

            if (!block.IsNull()) {
                LOCK(m_wallet.cs_wallet);
                if (!block_still_active) {
                    // Abort scan if current block is no longer active, to prevent
                    // marking transactions as coming from the wrong block.
                    result.last_failed_block = block_hash;
                    result.status = ScanResult::FAILURE;
                    break;
                }
                for (size_t posInBlock = 0; posInBlock < block.vtx.size(); ++posInBlock) {
                    m_wallet.SyncTransaction(block.vtx[posInBlock], TxStateConfirmed{block_hash, block_height, static_cast<int>(posInBlock)}, m_fUpdate, /*rescanning_old_block=*/true);
                }
                // scan succeeded, record block as most recent successfully scanned
                result.last_scanned_block = block_hash;
                result.last_scanned_height = block_height;

                if (!loc.IsNull()) {
                    m_wallet.WalletLogPrintf("Saving scan progress %d.\n", block_height);
                    WalletBatch batch(m_wallet.GetDatabase());
                    batch.WriteBestBlock(loc);
                }
            } else {
                // could not scan block, keep scanning but record this block as the most recent failure
                result.last_failed_block = block_hash;
                result.status = ScanResult::FAILURE;
            }
        }
        if (m_max_height && block_height >= *m_max_height) {
            break;
        }
        // If rescanning was triggered with cs_wallet permanently locked (AttachChain), additional blocks that were connected during the rescan
        // aren't processed here but will be processed with the pending blockConnected notifications after the lock is released.
        // If rescanning without a permanent cs_wallet lock, additional blocks that were added during the rescan will be re-processed if
        // the notification was processed and the last block height was updated.
        if (block_height >= WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHeight())) {
            break;
        }

        {
            if (!next_block) {
                // break successfully when rescan has reached the tip, or
                // previous block is no longer on the chain due to a reorg
                break;
            }

            // increment block and verification progress
            block_hash = next_block_hash;
            ++block_height;
            progress_current = chain.guessVerificationProgress(block_hash);

            // handle updated tip hash
            const uint256 prev_tip_hash = tip_hash;
            tip_hash = WITH_LOCK(m_wallet.cs_wallet, return m_wallet.GetLastBlockHash());
            if (!m_max_height && prev_tip_hash != tip_hash) {
                // in case the tip has changed, update progress max
                progress_end = chain.guessVerificationProgress(tip_hash);
            }
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
    } else if (block_height && m_wallet.chain().shutdownRequested()) {
        m_wallet.WalletLogPrintf("Rescan interrupted by shutdown request at block %d. Progress=%f\n", block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else {
        m_wallet.WalletLogPrintf("Rescan completed in %15dms\n", Ticks<std::chrono::milliseconds>(m_reserver.now() - start_time));
    }
    return result;
}
}
