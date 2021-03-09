// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_PARAMS_H
#define BITCOIN_CONSENSUS_PARAMS_H

#include <uint256.h>
#include <limits>
#include <map>

namespace Consensus {

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_TAPROOT, // Deployment of Schnorr/Taproot (BIPs 340-342)
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
};

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct VBitsDeployment {

    /** Construct a VBitsDeployment with all paramters */
    VBitsDeployment(int bit, int startheight, int timeoutheight, int threshold, int64_t min_activation_height)
        : bit(bit), startheight(startheight), timeoutheight(timeoutheight), threshold(threshold), m_min_activation_height(min_activation_height)
        {}

    /** Construct a standard VBitsDeployment (i.e. without a minimum activation height) */
    VBitsDeployment(int bit, int startheight, int timeoutheight, int threshold)
        : bit(bit), startheight(startheight), timeoutheight(timeoutheight), threshold(threshold), m_min_activation_height(0)
        {}

    /** Construct a VBitsDeployment that is either always active or never active. Used for tests
     * For always active, use "active=true". For never active, use "active=false"
     */
    VBitsDeployment(int bit, bool active)
        : bit(bit), startheight(active ? ALWAYS_ACTIVE : NEVER_ACTIVE), timeoutheight(active ? NO_TIMEOUT : NEVER_ACTIVE), threshold(1916), m_min_activation_height(0)
        {}

    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start block height for version bits miner confirmation. Must be a retarget block, can be in the past. */
    int startheight;
    /** Timeout/expiry block height for the deployment attempt. Must be a retarget block. */
    int timeoutheight;
    /** Threshold for activation */
    int threshold;
    /**
     * If lock in occurs, delay activation until at least this block height. Activations only occur on retargets.
     */
    int64_t m_min_activation_height{0};

    /** Constant for timeoutheight very far in the future. */
    static constexpr int NO_TIMEOUT = std::numeric_limits<int>::max();

    /** Special value for startheight indicating that the deployment is always active.
     *  This is useful for testing, as it means tests don't need to deal with the activation
     *  process (which takes at least 3 BIP9 intervals). Only tests that specifically test the
     *  behaviour during activation cannot use this. */
    static constexpr int ALWAYS_ACTIVE = -1;
    /** Special value for startheight and timeoutheight (both must be set) indicating that the
     *  deployment is entirely disabled. */
    static constexpr int NEVER_ACTIVE = -2;
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    int nSubsidyHalvingInterval;
    /* Block hash that is excepted from BIP16 enforcement */
    uint256 BIP16Exception;
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;
    uint256 BIP34Hash;
    /** Block height at which BIP65 becomes active */
    int BIP65Height;
    /** Block height at which BIP66 becomes active */
    int BIP66Height;
    /** Block height at which CSV (BIP68, BIP112 and BIP113) becomes active */
    int CSVHeight;
    /** Block height at which Segwit (BIP141, BIP143 and BIP147) becomes active.
     * Note that segwit v0 script rules are enforced on all blocks except the
     * BIP 16 exception blocks. */
    int SegwitHeight;
    /** Don't warn about unknown BIP 9 activations below this height.
     * This prevents us from warning about the CSV and segwit activations. */
    int MinBIP9WarningHeight;
    /** Minimum blocks expected for a versionbits deployment threshold.
     * Used to determine whether an unknown versionbits deployment has occurred.
     */
    uint32_t m_vbits_min_threshold;
    uint32_t nMinerConfirmationWindow;
    std::map<DeploymentPos, VBitsDeployment> m_deployments;
    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;
    int64_t nPowTargetTimespan;
    int64_t DifficultyAdjustmentInterval() const { return nPowTargetTimespan / nPowTargetSpacing; }
    /** The best chain should have at least this much work */
    uint256 nMinimumChainWork;
    /** By default assume that the signatures in ancestors of this block are valid */
    uint256 defaultAssumeValid;

    /**
     * If true, witness commitments contain a payload equal to a Bitcoin Script solution
     * to the signet challenge. See BIP325.
     */
    bool signet_blocks{false};
    std::vector<uint8_t> signet_challenge;
};
} // namespace Consensus

#endif // BITCOIN_CONSENSUS_PARAMS_H
