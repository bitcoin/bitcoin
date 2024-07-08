// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_CONSENSUS_PARAMS_H
#define SYSCOIN_CONSENSUS_PARAMS_H

#include <uint256.h>

#include <limits>
// SYSCOIN
#include <cmath>
#include <map>
#include <vector>

namespace Consensus {

/**
 * A buried deployment is one where the height of the activation has been hardcoded into
 * the client implementation long after the consensus change has activated. See BIP 90.
 */
enum BuriedDeployment : int16_t {
    // buried deployments get negative values to avoid overlap with DeploymentPos
    DEPLOYMENT_HEIGHTINCB = std::numeric_limits<int16_t>::min(),
    DEPLOYMENT_CLTV,
    DEPLOYMENT_DERSIG,
    DEPLOYMENT_CSV,
    DEPLOYMENT_SEGWIT,
};
constexpr bool ValidDeployment(BuriedDeployment dep) { return dep <= DEPLOYMENT_SEGWIT; }

enum DeploymentPos : uint16_t {
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_TAPROOT, // Deployment of Schnorr/Taproot (BIPs 340-342)
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in deploymentinfo.cpp
    MAX_VERSION_BITS_DEPLOYMENTS
};
constexpr bool ValidDeployment(DeploymentPos dep) { return dep < MAX_VERSION_BITS_DEPLOYMENTS; }

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit{28};
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime{NEVER_ACTIVE};
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout{NEVER_ACTIVE};
    /** If lock in occurs, delay activation until at least this block
     *  height.  Note that activation will only occur on a retarget
     *  boundary.
     */
    int min_activation_height{0};

    /** Constant for nTimeout very far in the future. */
    static constexpr int64_t NO_TIMEOUT = std::numeric_limits<int64_t>::max();

    /** Special value for nStartTime indicating that the deployment is always active.
     *  This is useful for testing, as it means tests don't need to deal with the activation
     *  process (which takes at least 3 BIP9 intervals). Only tests that specifically test the
     *  behaviour during activation cannot use this. */
    static constexpr int64_t ALWAYS_ACTIVE = -1;

    /** Special value for nStartTime indicating that the deployment is never active.
     *  This is useful for integrating the code changes for a new feature
     *  prior to deploying it on some or all networks. */
    static constexpr int64_t NEVER_ACTIVE = -2;
};

enum
{
    LLMQ_NONE = 0xff,

    LLMQ_400_60 = 2, // 400 members, 240 (60%) threshold, one every 12 hours

    // for testing only
    LLMQ_TEST = 100, // 3 members, 2 (66%) threshold, one per hour. Params might differ when -llmqtestparams is used

};

// Configures a LLMQ and its DKG
// See https://github.com/dashpay/dips/blob/master/dip-0006.md for more details
struct LLMQParams {
    uint8_t type;

    // not consensus critical, only used in logging, RPC and UI
    std::string name;

    // the size of the quorum, e.g. 50 or 400
    int size;

    // The minimum number of valid members after the DKK. If less members are determined valid, no commitment can be
    // created. Should be higher then the threshold to allow some room for failing nodes, otherwise quorum might end up
    // not being able to ever created a recovered signature if more nodes fail after the DKG
    int minSize;

    // The threshold required to recover a final signature. Should be at least 50%+1 of the quorum size. This value
    // also controls the size of the public key verification vector and has a large influence on the performance of
    // recovery. It also influences the amount of minimum messages that need to be exchanged for a single signing session.
    // This value has the most influence on the security of the quorum. The number of total malicious masternodes
    // required to negatively influence signing sessions highly correlates to the threshold percentage.
    int threshold;

    // The interval in number blocks for DKGs and the creation of LLMQs. If set to 24 for example, a DKG will start
    // every 24 blocks, which is approximately once every hour.
    int dkgInterval;

    // The number of blocks per phase in a DKG session. There are 6 phases plus the mining phase that need to be processed
    // per DKG. Set this value to a number of blocks so that each phase has enough time to propagate all required
    // messages to all members before the next phase starts. If blocks are produced too fast, whole DKG sessions will
    // fail.
    int dkgPhaseBlocks;

    // The starting block inside the DKG interval for when mining of commitments starts. The value is inclusive.
    // Starting from this block, the inclusion of (possibly null) commitments is enforced until the first non-null
    // commitment is mined. The chosen value should be at least 5 * dkgPhaseBlocks so that it starts right after the
    // finalization phase.
    int dkgMiningWindowStart;

    // The ending block inside the DKG interval for when mining of commitments ends. The value is inclusive.
    // Choose a value so that miners have enough time to receive the commitment and mine it. Also take into consideration
    // that miners might omit real commitments and revert to always including null commitments. The mining window should
    // be large enough so that other miners have a chance to produce a block containing a non-null commitment. The window
    // should at the same time not be too large so that not too much space is wasted with null commitments in case a DKG
    // session failed.
    int dkgMiningWindowEnd;

    // In the complaint phase, members will vote on other members being bad (missing valid contribution). If at least
    // dkgBadVotesThreshold have voted for another member to be bad, it will considered to be bad by all other members
    // as well. This serves as a protection against late-comers who send their contribution on the bring of
    // phase-transition, which would otherwise result in inconsistent views of the valid members set
    int dkgBadVotesThreshold;

    // Number of quorums to consider "active" for signing sessions
    int signingActiveQuorumCount;

    // Used for intra-quorum communication. This is the number of quorums for which we should keep old connections. This
    // should be at least one more then the active quorums set.
    int keepOldConnections;

    // How many members should we try to send all sigShares to before we give up.
    int recoveryMembers;
    public:

    // For how many blocks recent DKG info should be kept
    [[ nodiscard ]] constexpr int max_store_depth() const
    {
        return (signingActiveQuorumCount * 2) * dkgInterval;
    }
};
/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    // SYSCOIN
    uint32_t nNEVMChainID;
    std::vector<unsigned char> vchSYSXBurnMethodSignature;
    std::vector<unsigned char> vchSYSXERC20Manager;
    std::vector<unsigned char> vchTokenFreezeMethod;
    unsigned int nSeniorityHeight1;
    double nSeniorityLevel1;
    unsigned int nSeniorityHeight2;
    double nSeniorityLevel2;
    bool bTestnet{false};
    int nBridgeStartBlock;
    int nNEVMStartBlock;
    int nPODAStartBlock;
    int nRolluxStartBlock;
    int nV19StartBlock;
    int nUTXOAssetsBlock;
    uint64_t nMinMNSubsidySats;
        
    int nSuperblockStartBlock;
    int nSuperblockCycle; // in blocks
    int nSuperblockMaturityWindow; // in blocks
    int nGovernanceMinQuorum; // Min absolute vote count to trigger an action
    int nGovernanceFilterElements;
    int nMasternodeMinimumConfirmations;
    int nSubsidyHalvingInterval;
    /**
     * Hashes of blocks that
     * - are known to be consensus valid, and
     * - buried in the chain, and
     * - fail if the default script verify flags are applied.
     */
    std::map<uint256, uint32_t> script_flag_exceptions;
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
    /**
     * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargeting period,
     * (nPowTargetTimespan / nPowTargetSpacing) which is also used for BIP9 deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];
    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;
    int64_t nPowTargetTimespan;
    int SuperBlockCycle(int nHeight) const { 
        if (nHeight >= nNEVMStartBlock) {
            return nSuperblockCycle;
        } else {
            return nSuperblockCycle*2.5;
        }
    }
    double Seniority(int nHeight, int nStartHeight) const {
        unsigned int nSeniorityAge = 0;
        if (nHeight > nNEVMStartBlock) {
            const unsigned int nDifferenceInBlocksPreNEVM = std::max(nNEVMStartBlock - nStartHeight, 0);
            const unsigned int nDifferenceInBlocksPostNEVM = nHeight - std::max(nStartHeight, nNEVMStartBlock);
            nSeniorityAge = nDifferenceInBlocksPreNEVM + nDifferenceInBlocksPostNEVM*2.5;
        } else {
            nSeniorityAge = nHeight - nStartHeight;
        } 
        if(nSeniorityAge >= nSeniorityHeight2)
            return nSeniorityLevel2;
        else if(nSeniorityAge >= nSeniorityHeight1)
            return nSeniorityLevel1;
        return 0;
    }
    int SubsidyHalvingIntervals(int nHeight) const { 
        if(bTestnet) {
            if (nHeight >= nNEVMStartBlock) {
                return nHeight/nSubsidyHalvingInterval;
            } else {
                return nHeight/(nSubsidyHalvingInterval*2.5);
            }
        }
        if (nHeight >= nNEVMStartBlock) {
            static double forkIntervals = nNEVMStartBlock/(nSubsidyHalvingInterval*2.5);
            return floor(forkIntervals + (((double)(nHeight-nNEVMStartBlock))/((double)nSubsidyHalvingInterval)));
        } else {
            return nHeight/(nSubsidyHalvingInterval*2.5);
        }
    }
    int64_t PowTargetSpacing(int nHeight) const {
        if(nHeight >= nNEVMStartBlock) {
            return nPowTargetSpacing; 
        } else {
            return (nPowTargetSpacing/2.5); 
        }
    }
    int64_t DifficultyAdjustmentInterval(int nHeight) const {
        return nPowTargetTimespan / PowTargetSpacing(nHeight);
    }
    /** The best chain should have at least this much work */
    uint256 nMinimumChainWork;
    /** By default assume that the signatures in ancestors of this block are valid */
    uint256 defaultAssumeValid;
    /** Auxpow parameters */
    int32_t nAuxpowChainId;
    int32_t nAuxpowOldChainId;
    int nAuxpowStartHeight;
    bool fStrictChainId;
    int nLegacyBlocksBefore; // -1 for "always allow"

     /**
     * Check whether or not to allow legacy blocks at the given height.
     * @param nHeight Height of the block to check.
     * @return True if it is allowed to have a legacy version.
     */
    bool AllowLegacyBlocks(unsigned nHeight) const
    {
        if (nLegacyBlocksBefore < 0)
            return true;
        return static_cast<int> (nHeight) < nLegacyBlocksBefore;
    }
    // SYSCOIN
    /** Block height at which DIP0003 becomes active */
    int DIP0003Height;
    /** Block height at which DIP0003 becomes enforced */
    int DIP0003EnforcementHeight;
    LLMQParams llmqTypeChainLocks;
        /**
     * If true, witness commitments contain a payload equal to a Bitcoin Script solution
     * to the signet challenge. See BIP325.
     */
    bool signet_blocks{false};
    std::vector<uint8_t> signet_challenge;

    int DeploymentHeight(BuriedDeployment dep) const
    {
        switch (dep) {
        case DEPLOYMENT_HEIGHTINCB:
            return BIP34Height;
        case DEPLOYMENT_CLTV:
            return BIP65Height;
        case DEPLOYMENT_DERSIG:
            return BIP66Height;
        case DEPLOYMENT_CSV:
            return CSVHeight;
        case DEPLOYMENT_SEGWIT:
            return SegwitHeight;
        } // no default case, so the compiler can warn about missing cases
        return std::numeric_limits<int>::max();
    }
};

} // namespace Consensus
#endif // SYSCOIN_CONSENSUS_PARAMS_H
