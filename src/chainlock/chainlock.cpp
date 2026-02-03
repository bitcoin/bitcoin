// Copyright (c) 2019-2026 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainlock/chainlock.h>

#include <chainlock/clsig.h>
#include <logging.h>
#include <spork.h>

namespace chainlock {

Chainlocks::Chainlocks(const CSporkManager& sporkman) :
    m_sporks(sporkman)
{
}

bool Chainlocks::IsEnabled() const { return m_sporks.IsSporkActive(SPORK_19_CHAINLOCKS_ENABLED); }

bool Chainlocks::IsSigningEnabled() const { return m_sporks.GetSporkValue(SPORK_19_CHAINLOCKS_ENABLED) == 0; }

chainlock::ChainLockSig Chainlocks::GetBestChainLock() const
{
    LOCK(cs);
    return bestChainLock;
}

int32_t Chainlocks::GetBestChainLockHeight() const
{
    LOCK(cs);
    return bestChainLock.getHeight();
}

bool Chainlocks::HasChainLock(int nHeight, const uint256& blockHash) const
{
    LOCK(cs);

    if (!IsEnabled()) {
        return false;
    }

    if (bestChainLockBlockIndex == nullptr) {
        return false;
    }

    if (nHeight > bestChainLockBlockIndex->nHeight) {
        return false;
    }

    if (nHeight == bestChainLockBlockIndex->nHeight) {
        return blockHash == bestChainLockBlockIndex->GetBlockHash();
    }

    const auto* pAncestor = bestChainLockBlockIndex->GetAncestor(nHeight);
    return (pAncestor != nullptr) && pAncestor->GetBlockHash() == blockHash;
}

bool Chainlocks::HasConflictingChainLock(int nHeight, const uint256& blockHash) const
{
    LOCK(cs);

    if (!IsEnabled()) {
        return false;
    }

    if (bestChainLockBlockIndex == nullptr) {
        return false;
    }

    if (nHeight > bestChainLockBlockIndex->nHeight) {
        return false;
    }

    if (nHeight == bestChainLockBlockIndex->nHeight) {
        return blockHash != bestChainLockBlockIndex->GetBlockHash();
    }

    const auto* pAncestor = bestChainLockBlockIndex->GetAncestor(nHeight);
    assert(pAncestor);
    return pAncestor->GetBlockHash() != blockHash;
}

void Chainlocks::ResetChainlock()
{
    LOCK(cs);
    bestChainLockHash = uint256{};
    bestChainLock = bestChainLockWithKnownBlock = chainlock::ChainLockSig{};
    bestChainLockBlockIndex = nullptr;
}

bool Chainlocks::UpdateBestChainlock(const uint256& hash, const chainlock::ChainLockSig& clsig, const CBlockIndex* pindex)
{
    LOCK(cs);

    if (clsig.getHeight() <= bestChainLock.getHeight()) {
        // no need to process older/same CLSIGs
        return false;
    }

    bestChainLockHash = hash;
    bestChainLock = clsig;

    if (pindex) {
        if (pindex->nHeight != clsig.getHeight()) {
            // Should not happen, same as the conflict check from above.
            LogPrintf("Chainlocks::%s -- height of CLSIG (%s) does not match the specified block's height (%d)\n",
                      __func__, clsig.ToString(), pindex->nHeight);
            // Note: not relaying clsig here
            return false;
        }
        bestChainLockWithKnownBlock = clsig;
        bestChainLockBlockIndex = pindex;
    }
    // We don't know the block/header for this CLSIG yet, so bail out for now and when the
    // block/header later comes in, we will enforce the correct chain. We still relay further.
    return true;
}

std::pair<chainlock::ChainLockSig, const CBlockIndex*> Chainlocks::GetBestChainlockWithPindex() const
{
    LOCK(cs);
    return {bestChainLockWithKnownBlock, bestChainLockBlockIndex};
}

bool Chainlocks::GetChainLockByHash(const uint256& hash, chainlock::ChainLockSig& ret) const
{
    LOCK(cs);

    if (bestChainLockHash != hash) {
        // we only propagate the best one and ditch all the old ones
        return false;
    }
    ret = bestChainLock;
    return true;
}

void Chainlocks::AcceptedBlockHeader(gsl::not_null<const CBlockIndex*> pindexNew)
{
    LOCK(cs);

    if (pindexNew->GetBlockHash() == bestChainLock.getBlockHash()) {
        LogPrint(BCLog::CHAINLOCKS, "Chainlocks::%s -- block header %s came in late, updating and enforcing\n",
                 __func__, pindexNew->GetBlockHash().ToString());

        if (bestChainLock.getHeight() != pindexNew->nHeight) {
            // Should not happen, same as the conflict check from ProcessNewChainLock.
            LogPrintf("Chainlocks::%s -- height of CLSIG (%s) does not match the specified block's height (%d)\n",
                      __func__, bestChainLock.ToString(), pindexNew->nHeight);
            return;
        }

        // when EnforceBestChainLock is called later, it might end up invalidating other chains but not activating the
        // CLSIG locked chain. This happens when only the header is known but the block is still missing yet. The usual
        // block processing logic will handle this when the block arrives
        bestChainLockWithKnownBlock = bestChainLock;
        bestChainLockBlockIndex = pindexNew;
    }
}
} // namespace chainlock
