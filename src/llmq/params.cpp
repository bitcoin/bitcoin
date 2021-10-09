// Copyright (c) 2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/params.h>

/**
 * llmq_test
 * This quorum is only used for testing
 *
 */
Consensus::LLMQParams llmq_test = {
    .type = Consensus::LLMQ_TEST,
    .name = "llmq_test",
    .size = 3,
    .minSize = 2,
    .threshold = 2,

    .dkgInterval = 24, // one DKG per hour
    .dkgPhaseBlocks = 2,
    .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
    .dkgMiningWindowEnd = 18,
    .dkgBadVotesThreshold = 2,

    .signingActiveQuorumCount = 2, // just a few ones to allow easier testing

    .keepOldConnections = 3,
    .recoveryMembers = 3,
};

/**
 * llmq_test (Dash Core 0.17) aka llmq_test_v17
 * This quorum is only used for testing
 *
 */
Consensus::LLMQParams llmq_test_v17 = {
    .type = Consensus::LLMQ_TEST_V17,
    .name = "llmq_test_v17",
    .size = 3,
    .minSize = 2,
    .threshold = 2,

    .dkgInterval = 24, // one DKG per hour
    .dkgPhaseBlocks = 2,
    .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
    .dkgMiningWindowEnd = 18,
    .dkgBadVotesThreshold = 2,

    .signingActiveQuorumCount = 2, // just a few ones to allow easier testing

    .keepOldConnections = 3,
    .recoveryMembers = 3,
};

/**
 * llmq_devnet
 * This quorum is only used for testing on devnets
 *
 */
Consensus::LLMQParams llmq_devnet = {
    .type = Consensus::LLMQ_DEVNET,
    .name = "llmq_devnet",
    .size = 10,
    .minSize = 7,
    .threshold = 6,

    .dkgInterval = 24, // one DKG per hour
    .dkgPhaseBlocks = 2,
    .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
    .dkgMiningWindowEnd = 18,
    .dkgBadVotesThreshold = 7,

    .signingActiveQuorumCount = 3, // just a few ones to allow easier testing

    .keepOldConnections = 4,
    .recoveryMembers = 6,
};

/**
 * llmq50_60
 * This quorum is deployed on mainnet and requires
 * 40 - 50 participants
 *
 */
Consensus::LLMQParams llmq50_60 = {
    .type = Consensus::LLMQ_50_60,
    .name = "llmq_50_60",
    .size = 50,
    .minSize = 40,
    .threshold = 30,

    .dkgInterval = 24, // one DKG per hour
    .dkgPhaseBlocks = 2,
    .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
    .dkgMiningWindowEnd = 18,
    .dkgBadVotesThreshold = 40,

    .signingActiveQuorumCount = 24, // a full day worth of LLMQs

    .keepOldConnections = 25,
    .recoveryMembers = 25,
};

/**
 * llmq400_60
 * This quorum is deployed on mainnet and requires
 * 300 - 400 participants
 *
 */
Consensus::LLMQParams llmq400_60 = {
    .type = Consensus::LLMQ_400_60,
    .name = "llmq_400_60",
    .size = 400,
    .minSize = 300,
    .threshold = 240,

    .dkgInterval = 24 * 12, // one DKG every 12 hours
    .dkgPhaseBlocks = 4,
    .dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
    .dkgMiningWindowEnd = 28,
    .dkgBadVotesThreshold = 300,

    .signingActiveQuorumCount = 4, // two days worth of LLMQs

    .keepOldConnections = 5,
    .recoveryMembers = 100,
};

/**
 * llmq400_85
 * This quorum is deployed on mainnet and requires
 * 300 - 400 participants _with_ a supermajority
 *
 * Used for deployment and min-proto-version signalling
 */
Consensus::LLMQParams llmq400_85 = {
    .type = Consensus::LLMQ_400_85,
    .name = "llmq_400_85",
    .size = 400,
    .minSize = 350,
    .threshold = 340,

    .dkgInterval = 24 * 24, // one DKG every 24 hours
    .dkgPhaseBlocks = 4,
    .dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
    .dkgMiningWindowEnd = 48,   // give it a larger mining window to make sure it is mined
    .dkgBadVotesThreshold = 300,

    .signingActiveQuorumCount = 4, // four days worth of LLMQs

    .keepOldConnections = 5,
    .recoveryMembers = 100,
};

/**
 * llmq100_67
 * This quorum is deployed on mainnet and requires
 * 80 - 100 participants
 *
 * Used by Dash Platform
 */
Consensus::LLMQParams llmq100_67 = {
    .type = Consensus::LLMQ_100_67,
    .name = "llmq_100_67",
    .size = 100,
    .minSize = 80,
    .threshold = 67,

    .dkgInterval = 24, // one DKG per hour
    .dkgPhaseBlocks = 2,
    .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
    .dkgMiningWindowEnd = 18,
    .dkgBadVotesThreshold = 80,

    .signingActiveQuorumCount = 24, // a full day worth of LLMQs

    .keepOldConnections = 25,
    .recoveryMembers = 50,
};
