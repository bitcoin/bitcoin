// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_SCAN_H
#define BITCOIN_WALLET_SCAN_H

#include <uint256.h>

#include <atomic>
#include <deque>
#include <optional>

namespace wallet {
class CWallet;
class WalletRescanReserver;
struct ScanResult;

namespace {
class FastWalletRescanFilter;
} // namespace

class ChainScanner {
private:
    CWallet& m_wallet;
    const WalletRescanReserver& m_reserver;
    const uint256& m_start_block;
    int m_start_height;
    std::optional<int> m_max_height;
    bool m_fUpdate;
    bool m_save_progress;
    std::optional<std::pair<uint256, int>> m_next_block;

    // Progress tracking
    double m_progress_begin{0};
    double m_progress_end{0};
    double m_progress_current{0};
    uint256 m_tip_hash;

    /// Queued block hashes and heights to filter and scan
    std::deque<std::pair<uint256, int>> m_blocks;

    std::optional<std::pair<uint256, int>> ReadNextBlock(ScanResult& result);
    /**
     * @return the number of blocks to be scanned in m_blocks
     */
    size_t ReadNextBlocks(const std::unique_ptr<FastWalletRescanFilter>& filter, ScanResult& result);
    bool ScanBlock(const uint256& block_hash, int block_height, bool save_progress);
    void UpdateProgress(int block_height);
    void UpdateTipIfChanged();
    void ProcessBlock(const uint256& block_hash, int block_height, bool next_interval, ScanResult& result);

public:
    ChainScanner(CWallet& wallet, const WalletRescanReserver& reserver, const uint256& start_block, int start_height,
            std::optional<int> max_height, bool fUpdate, bool save_progress) :
        m_wallet(wallet),
        m_reserver(reserver),
        m_start_block(start_block),
        m_start_height(start_height),
        m_max_height(max_height),
        m_fUpdate(fUpdate),
        m_save_progress(save_progress) {}

    ScanResult Scan();

};

} // namespace wallet

#endif // BITCOIN_WALLET_SCAN_H
