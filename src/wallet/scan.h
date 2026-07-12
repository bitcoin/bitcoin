// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_SCAN_H
#define BITCOIN_WALLET_SCAN_H

#include <uint256.h>
#include <util/time.h>

#include <atomic>
#include <functional>
#include <memory>
#include <vector>
#include <optional>

namespace wallet {
class CWallet;
class FastWalletRescanFilter;
class ParallelFilterChecker;

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

/** RAII object to check and reserve a wallet rescan */
class WalletRescanReserver
{
private:
    using Clock = std::chrono::steady_clock;
    using NowFn = std::function<Clock::time_point()>;
    CWallet& m_wallet;
    bool m_could_reserve{false};
    NowFn m_now;
public:
    explicit WalletRescanReserver(CWallet& w) : m_wallet(w) {}

    bool reserve(bool with_passphrase = false);
    bool isReserved() const;

    Clock::time_point now() const { return m_now ? m_now() : Clock::now(); }
    void setNow(NowFn now) { m_now = std::move(now); }

    ~WalletRescanReserver();
};

class ChainScanner {
private:
    CWallet& m_wallet;

    std::atomic<bool> m_abort{false};
    std::atomic<bool> m_scanning{false};
    std::atomic<bool> m_scanning_with_passphrase{false};
    std::atomic<SteadyClock::time_point> m_scanning_start{SteadyClock::time_point{}};
    std::atomic<double> m_scanning_progress{0};

    //! Progress window and tip tracked across Scan loop iterations. The
    //! current block's progress is a plain local in Scan; only the window
    //! bounds are shared with the helpers, and UpdateTipIfChanged is the
    //! sole mutator.
    struct LoopState {
        double progress_begin{0};
        double progress_end{0};
        uint256 tip_hash;
    };

    //! A block queued for filtering and scanning. still_active records
    //! whether the block was in the active chain when it was queued.
    struct QueuedBlock {
        uint256 hash;
        int height;
        bool still_active;
    };

    //! State of a single Scan() call, local to Scan() and passed through the
    //! scan helpers.
    struct ScanContext {
        //! Defined in scan.cpp where ParallelFilterChecker is complete, as
        //! required by the checker member's destructor.
        explicit ScanContext(std::optional<int> max_height_in);

        //! Optional height limit of the scan range.
        const std::optional<int> max_height;
        //! The next block to read, if any.
        std::optional<std::pair<uint256, int>> next_block;
        //! Checks block filters in parallel on its own thread pool; only set
        //! when the wallet is configured with more than one thread..
        std::unique_ptr<ParallelFilterChecker> checker;
    };

    friend class ParallelFilterChecker;

    //! Consume ctx.next_block: record whether it is still in the active
    //! chain, and queue its active-chain successor into ctx.next_block if it
    //! exists and is within the scan range.
    std::optional<QueuedBlock> ReadNextBlock(ScanContext& ctx);
    /**
     * Read and filter the next block, recording filter-skipped blocks as
     * scanned.
     * @return the blocks that are ready to be scanned
     */
    std::vector<QueuedBlock> ReadNextBlocks(FastWalletRescanFilter* filter, ScanContext& ctx, ScanResult& result, double& progress_current);
    bool ScanBlock(const uint256& block_hash, int block_height, bool save_progress);
    void UpdateProgress(const LoopState& state, double progress_current, int block_height);
    void UpdateTipIfChanged(LoopState& state);

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

    /** Scan active chain for relevant transactions after importing keys. Should
     * be called whenever new keys are added to the wallet, with the oldest key
     * creation time.
     * @return Earliest timestamp that could be successfully scanned from. Timestamp
     * returned will be higher than startTime if relevant blocks could not be read. */
    int64_t ScanFromTime(int64_t startTime, const WalletRescanReserver& reserver);
    ScanResult Scan(const uint256& start_block, int start_height, std::optional<int> max_height,
                    const WalletRescanReserver& reserver, bool save_progress);

};

} // namespace wallet

#endif // BITCOIN_WALLET_SCAN_H
