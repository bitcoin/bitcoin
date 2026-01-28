// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_SCAN_H
#define BITCOIN_WALLET_SCAN_H

#include <uint256.h>
#include <util/time.h>

#include <atomic>
#include <optional>

#include <boost/signals2/signal.hpp>

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
    int m_next_block_height;
    uint256 m_next_block_hash;

    /// Queued block hashes and heights to filter and scan
    std::vector<std::pair<uint256, int>> m_blocks;
    size_t m_max_blockqueue_size{1000};
    bool m_continue{true};

    bool ReadBlockHash(ScanResult& result);
    /**
     * @return a pair of indexes into the blocks array that specify a range of blocks
     *         to scan (end index excluded) or std::nullopt if no blocks should be scanned.
     */
    std::optional<std::pair<size_t, size_t>> FilterBlocks(const std::unique_ptr<FastWalletRescanFilter>& filter, ScanResult& result);
    bool ScanBlock(const std::pair<uint256, int>& data, bool save_progress);

public:
    ChainScanner(CWallet& wallet, const WalletRescanReserver& reserver, const uint256& start_block, int start_height,
            std::optional<int> max_height, bool fUpdate, bool save_progress) :
        m_wallet(wallet),
        m_reserver(reserver),
        m_start_block(start_block),
        m_start_height(start_height),
        m_max_height(max_height),
        m_fUpdate(fUpdate),
        m_save_progress(save_progress) {
            m_blocks.reserve(m_max_blockqueue_size);
        }

    ScanResult Scan();

};

} // namespace wallet

#endif // BITCOIN_WALLET_SCAN_H
