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

class ChainScanner {
private:
    CWallet& m_wallet;
    const WalletRescanReserver& m_reserver;
    const uint256& m_start_block;
    int m_start_height;
    std::optional<int> m_max_height;
    bool m_fUpdate;
    bool m_save_progress;

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
