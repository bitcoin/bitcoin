// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_PARAMS_H
#define BITCOIN_CONSENSUS_PARAMS_H

#include "uint256.h"

namespace Consensus {
/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    int nSubsidyHalvingInterval;
    /** Used to check majorities for block version upgrade */
    int nMajorityEnforceBlockUpgrade;
    int nMajorityRejectBlockOutdated;
    int nMajorityWindow;
    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    int64_t nPowTargetSpacing;
    int64_t nPowTargetTimespan;
    int64_t DifficultyAdjustmentInterval() const { return nPowTargetTimespan / nPowTargetSpacing; }

    /** Maximum block size parameters */
    uint32_t nMaxSizePreFork;
    uint64_t nEarliestSizeForkTime;
    uint32_t nSizeDoubleEpoch;
    uint64_t nMaxSizeBase;
    uint8_t nMaxSizeDoublings;
    int nActivateSizeForkMajority;
    uint64_t nSizeForkGracePeriod;

    /** Maximum block size of a block with timestamp nBlockTimestamp */
    uint64_t MaxBlockSize(uint64_t nBlockTimestamp, uint64_t nSizeForkActivationTime) const {
        if (nBlockTimestamp < nEarliestSizeForkTime || nBlockTimestamp < nSizeForkActivationTime)
            return nMaxSizePreFork;
        if (nBlockTimestamp >= nEarliestSizeForkTime + nSizeDoubleEpoch * nMaxSizeDoublings)
            return nMaxSizeBase << nMaxSizeDoublings;

        // Piecewise-linear-between-doublings growth. Calculated based on a fixed
        // timestamp and not the activation time so the maximum size is
        // predictable, and so the activation time can be completely removed in
        // a future version of this code after the fork is complete.
        uint64_t timeDelta = nBlockTimestamp - nEarliestSizeForkTime;
        uint64_t doublings = timeDelta / nSizeDoubleEpoch;
        uint64_t remain = timeDelta % nSizeDoubleEpoch;
        uint64_t interpolate = (nMaxSizeBase << doublings) * remain / nSizeDoubleEpoch;
        uint64_t nMaxSize = (nMaxSizeBase << doublings) + interpolate;
        return nMaxSize;
    }
    /** Maximum number of signature ops in a block with timestamp nBlockTimestamp */
    uint64_t MaxBlockSigops(uint64_t nBlockTimestamp, uint64_t nSizeForkActivationTime) const {
        return MaxBlockSize(nBlockTimestamp, nSizeForkActivationTime)/50;
    }
    /** Maximum size of a transaction in a block with timestamp nBlockTimestamp */
    uint64_t MaxTransactionSize(uint64_t nBlockTimestamp, uint64_t nSizeForkActivationTime) const {
        if (nBlockTimestamp < nEarliestSizeForkTime || nBlockTimestamp < nSizeForkActivationTime)
            return nMaxSizePreFork;
        return 100*1000;
    }
    int ActivateSizeForkMajority() const { return nActivateSizeForkMajority; }
    uint64_t SizeForkGracePeriod() const { return nSizeForkGracePeriod; }
};
} // namespace Consensus

#endif // BITCOIN_CONSENSUS_PARAMS_H
