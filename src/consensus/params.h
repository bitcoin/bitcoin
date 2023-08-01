// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_PARAMS_H
#define BITCOIN_CONSENSUS_PARAMS_H

#include <uint256.h>
#include <llmq/params.h>

#include <map>

namespace Consensus {

enum DeploymentPos {
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_V20,     // Deployment of EHF, LLMQ Randomness Beacon
    DEPLOYMENT_MN_RR,   // Deployment of Masternode Reward Location Reallocation
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
    MAX_VERSION_BITS_DEPLOYMENTS
};

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout;
    /** The number of past blocks (including the block under consideration) to be taken into account for locking in a fork. */
    int64_t nWindowSize{0};
    /** A starting number of blocks, in the range of 1..nWindowSize, which must signal for a fork in order to lock it in. */
    int64_t nThresholdStart{0};
    /** A minimum number of blocks, in the range of 1..nWindowSize, which must signal for a fork in order to lock it in. */
    int64_t nThresholdMin{0};
    /** A coefficient which adjusts the speed a required number of signaling blocks is decreasing from nThresholdStart to nThresholdMin at with each period. */
    int64_t nFalloffCoeff{0};
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    uint256 hashDevnetGenesisBlock;
    int nSubsidyHalvingInterval;
    int nMasternodePaymentsStartBlock;
    int nMasternodePaymentsIncreaseBlock;
    int nMasternodePaymentsIncreasePeriod; // in blocks
    int nInstantSendConfirmationsRequired; // in blocks
    int nInstantSendKeepLock; // in blocks
    int nBudgetPaymentsStartBlock;
    int nBudgetPaymentsCycleBlocks;
    int nBudgetPaymentsWindowBlocks;
    int nSuperblockStartBlock;
    uint256 nSuperblockStartHash;
    int nSuperblockCycle; // in blocks
    int nSuperblockMaturityWindow; // in blocks
    int nGovernanceMinQuorum; // Min absolute vote count to trigger an action
    int nGovernanceFilterElements;
    int nMasternodeMinimumConfirmations;
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;
    uint256 BIP34Hash;
    /** Block height at which BIP65 becomes active */
    int BIP65Height;
    /** Block height at which BIP66 becomes active */
    int BIP66Height;
    // Deployment of BIP147 (NULLDUMMY)
    int BIP147Height;
    /** Block height at which CSV (BIP68, BIP112 and BIP113) becomes active */
    int CSVHeight;
    /** Block height at which DIP0001 becomes active */
    int DIP0001Height;
    /** Block height at which DIP0002 and DIP0003 (txv3 and deterministic MN lists) becomes active */
    int DIP0003Height;
    /** Block height at which DIP0003 becomes enforced */
    int DIP0003EnforcementHeight;
    uint256 DIP0003EnforcementHash;
    /** Block height at which DIP0008 becomes active */
    int DIP0008Height;
    /** Block height at which BRR (Block Reward Reallocation) becomes active */
    int BRRHeight;
    /** Block height at which DIP0020, DIP0021 and LLMQ_100_67 quorums become active */
    int DIP0020Height;
    /** Block height at which DIP0024 (Quorum Rotation) and decreased governance proposal fee becomes active */
    int DIP0024Height;
    /** Block height at which V19 (Basic BLS and HPMNs/EvoNodes) becomes active */
    int V19Height;
    /** Don't warn about unknown BIP 9 activations below this height.
     * This prevents us from warning about the CSV and DIP activations. */
    int MinBIP9WarningHeight;
    /**
     * Minimum blocks including miner confirmation of the total of nMinerConfirmationWindow blocks in a retargeting period,
     * (nPowTargetTimespan / nPowTargetSpacing) which is also used for BIP9 deployments.
     * Default BIP9Deployment::nThresholdStart value for deployments where it's not specified and for unknown deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t nRuleChangeActivationThreshold;
    // Default BIP9Deployment::nWindowSize value for deployments where it's not specified and for unknown deployments.
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];
    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;
    int64_t nPowTargetTimespan;
    int nPowKGWHeight;
    int nPowDGWHeight;
    int64_t DifficultyAdjustmentInterval() const { return nPowTargetTimespan / nPowTargetSpacing; }
    uint256 nMinimumChainWork;
    uint256 defaultAssumeValid;

    /** these parameters are only used on devnet and can be configured from the outside */
    int nMinimumDifficultyBlocks{0};
    int nHighSubsidyBlocks{0};
    int nHighSubsidyFactor{1};

    std::vector<LLMQParams> llmqs;
    LLMQType llmqTypeChainLocks;
    LLMQType llmqTypeInstantSend{LLMQType::LLMQ_NONE};
    LLMQType llmqTypeDIP0024InstantSend{LLMQType::LLMQ_NONE};
    LLMQType llmqTypePlatform{LLMQType::LLMQ_NONE};
    LLMQType llmqTypeMnhf{LLMQType::LLMQ_NONE};
    LLMQType llmqTypeAssetLocks{LLMQType::LLMQ_NONE};
};
} // namespace Consensus

#endif // BITCOIN_CONSENSUS_PARAMS_H
