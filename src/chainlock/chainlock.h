// Copyright (c) 2019-2026 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINLOCK_CHAINLOCK_H
#define BITCOIN_CHAINLOCK_CHAINLOCK_H

#include <chain.h>
#include <chainlock/clsig.h>
#include <gsl/pointers.h>
#include <sync.h>
#include <uint256.h>

class CSporkManager;

namespace chainlock {

//! Depth of block including transactions before it's considered safe
static constexpr int32_t TX_CONFIRM_THRESHOLD{5};

class Chainlocks
{
private:
    const CSporkManager& m_sporks;

    mutable Mutex cs;
    const CBlockIndex* bestChainLockBlockIndex GUARDED_BY(cs){nullptr};

    uint256 bestChainLockHash GUARDED_BY(cs);
    chainlock::ChainLockSig bestChainLock GUARDED_BY(cs);

    chainlock::ChainLockSig bestChainLockWithKnownBlock GUARDED_BY(cs);

public:
    Chainlocks(const CSporkManager& sporkman);

    [[nodiscard]] bool IsEnabled() const;
    [[nodiscard]] bool IsSigningEnabled() const;

    [[nodiscard]] chainlock::ChainLockSig GetBestChainLock() const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    [[nodiscard]] int32_t GetBestChainLockHeight() const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    [[nodiscard]] bool HasChainLock(int nHeight, const uint256& blockHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    [[nodiscard]] bool HasConflictingChainLock(int nHeight, const uint256& blockHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool UpdateBestChainlock(const uint256& hash, const chainlock::ChainLockSig& clsig, const CBlockIndex* pindex)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    std::pair<chainlock::ChainLockSig, const CBlockIndex*> GetBestChainlockWithPindex() const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool GetChainLockByHash(const uint256& hash, chainlock::ChainLockSig& ret) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void AcceptedBlockHeader(gsl::not_null<const CBlockIndex*> pindexNew) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void ResetChainlock() EXCLUSIVE_LOCKS_REQUIRED(!cs);
};

} // namespace chainlock

#endif // BITCOIN_CHAINLOCK_CHAINLOCK_H
