// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_SCAN_H
#define BITCOIN_WALLET_SCAN_H

#include <uint256.h>
#include <util/time.h>

#include <atomic>
#include <optional>

namespace wallet {
class CWallet;
class WalletRescanReserver;

/** Result of a wallet scan */
struct ScanResult {
    enum { SUCCESS, FAILURE, USER_ABORT } status = SUCCESS;

    //! Hash and height of most recent block that was successfully scanned.
    //! Unset if no blocks were scanned due to read errors or the chain
    //! being empty.
    uint256 last_scanned_block;
    std::optional<int> last_scanned_height;

    //! Height of the most recent block that could not be scanned due to
    //! read errors or pruning. Will be set if status is FAILURE, unset if
    //! status is SUCCESS, and may or may not be set if status is
    //! USER_ABORT.
    uint256 last_failed_block;
};

class ChainScanner {
private:
    CWallet& m_wallet;

    std::atomic<bool> m_abort{false};
    std::atomic<bool> m_scanning{false};
    std::atomic<bool> m_scanning_with_passphrase{false};
    std::atomic<SteadyClock::time_point> m_scanning_start{SteadyClock::time_point{}};
    std::atomic<double> m_scanning_progress{0};

    //! Only WalletRescanReserver may reserve and release scans, so that
    //! reservations are always managed RAII-style.
    friend class WalletRescanReserver;
    bool TryReserve(bool with_passphrase = false);
    void Release();

public:
    explicit ChainScanner(CWallet& wallet) : m_wallet(wallet) {}

    void Abort() { m_abort = true; }
    bool IsAborting() const { return m_abort; }
    bool IsScanning() const { return m_scanning; }
    bool IsScanningWithPassphrase() const { return m_scanning_with_passphrase; }
    SteadyClock::duration ScanningDuration() const { return m_scanning ? SteadyClock::now() - m_scanning_start.load() : SteadyClock::duration{}; }
    double ScanningProgress() const { return m_scanning ? m_scanning_progress.load() : 0; }

    ScanResult Scan(const uint256& start_block, int start_height, std::optional<int> max_height,
                    const WalletRescanReserver& reserver, bool save_progress);

};

} // namespace wallet

#endif // BITCOIN_WALLET_SCAN_H
