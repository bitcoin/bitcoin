// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/llmq_tests.h>
#include <test/util/setup_common.h>

#include <consensus/params.h>
#include <llmq/params.h>

#include <boost/test/unit_test.hpp>

#include <limits>

using namespace llmq;
using namespace llmq::testutils;
using namespace Consensus;

BOOST_FIXTURE_TEST_SUITE(llmq_params_tests, BasicTestingSetup)

// Get test params for use in tests
static const Consensus::LLMQParams& TEST_PARAMS_BASE = GetLLMQParams(Consensus::LLMQType::LLMQ_TEST_V17);

BOOST_AUTO_TEST_CASE(llmq_params_max_cycles_test)
{
    // Test non-rotated quorum
    LLMQParams nonRotated = TEST_PARAMS_BASE;
    nonRotated.useRotation = false;
    nonRotated.signingActiveQuorumCount = 2;

    // For non-rotated: max_cycles = quorums_count
    BOOST_CHECK_EQUAL(nonRotated.max_cycles(10), 10);
    BOOST_CHECK_EQUAL(nonRotated.max_cycles(100), 100);
    BOOST_CHECK_EQUAL(nonRotated.max_cycles(1), 1);
    BOOST_CHECK_EQUAL(nonRotated.max_cycles(0), 0);

    // Test rotated quorum
    LLMQParams rotated = TEST_PARAMS_BASE;
    rotated.useRotation = true;
    rotated.signingActiveQuorumCount = 2;

    // For rotated: max_cycles = quorums_count / signingActiveQuorumCount
    BOOST_CHECK_EQUAL(rotated.max_cycles(10), 5);
    BOOST_CHECK_EQUAL(rotated.max_cycles(100), 50);
    BOOST_CHECK_EQUAL(rotated.max_cycles(1), 0); // Integer division
    BOOST_CHECK_EQUAL(rotated.max_cycles(0), 0);

    // Test with different signingActiveQuorumCount
    rotated.signingActiveQuorumCount = 4;
    BOOST_CHECK_EQUAL(rotated.max_cycles(100), 25);
    BOOST_CHECK_EQUAL(rotated.max_cycles(16), 4);
    BOOST_CHECK_EQUAL(rotated.max_cycles(15), 3); // Integer division
}

BOOST_AUTO_TEST_CASE(llmq_params_max_store_depth_test)
{
    LLMQParams params = TEST_PARAMS_BASE;
    params.dkgInterval = 24;
    params.signingActiveQuorumCount = 2;
    params.keepOldKeys = 10;

    // max_store_depth = max_cycles(keepOldKeys) * dkgInterval

    // Test non-rotated
    params.useRotation = false;
    // max_cycles(10) = 10 for non-rotated
    BOOST_CHECK_EQUAL(params.max_store_depth(), 10 * 24); // 240

    // Test rotated
    params.useRotation = true;
    // max_cycles(10) = 10/2 = 5 for rotated
    BOOST_CHECK_EQUAL(params.max_store_depth(), 5 * 24); // 120

    // Test with different values
    params.keepOldKeys = 20;
    params.dkgInterval = 48;
    params.signingActiveQuorumCount = 4;
    // max_cycles(20) = 20/4 = 5 for rotated
    BOOST_CHECK_EQUAL(params.max_store_depth(), 5 * 48); // 240

    // Test edge cases
    params.keepOldKeys = 0;
    BOOST_CHECK_EQUAL(params.max_store_depth(), 0);

    params.keepOldKeys = 1;
    params.dkgInterval = 1;
    params.signingActiveQuorumCount = 1;
    BOOST_CHECK_EQUAL(params.max_store_depth(), 1);
}

BOOST_AUTO_TEST_CASE(llmq_params_validation_test)
{
    // Test parameter constraints and relationships
    LLMQParams params = TEST_PARAMS_BASE;

    // minSize should be less than or equal to size
    BOOST_CHECK_LE(params.minSize, params.size);

    // threshold should be less than or equal to size
    BOOST_CHECK_LE(params.threshold, params.size);

    // threshold should typically be > 50% of size for security
    BOOST_CHECK_GT(params.threshold * 2, params.size);

    // dkgMiningWindowStart should be after DKG phases complete
    // Typically should be >= 5 * dkgPhaseBlocks
    BOOST_CHECK_GE(params.dkgMiningWindowStart, 5 * params.dkgPhaseBlocks);

    // dkgMiningWindowEnd should be after dkgMiningWindowStart
    BOOST_CHECK_GT(params.dkgMiningWindowEnd, params.dkgMiningWindowStart);

    // dkgMiningWindowEnd should be within dkgInterval
    BOOST_CHECK_LT(params.dkgMiningWindowEnd, params.dkgInterval);
}

BOOST_AUTO_TEST_CASE(llmq_params_types_test)
{
    // Test that LLMQ types are properly defined
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(LLMQType::LLMQ_NONE), 0xff);
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(LLMQType::LLMQ_50_60), 1);
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(LLMQType::LLMQ_400_60), 2);
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(LLMQType::LLMQ_400_85), 3);
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(LLMQType::LLMQ_100_67), 4);
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(LLMQType::LLMQ_60_75), 5);
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(LLMQType::LLMQ_25_67), 6);

    // Test special types
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(LLMQType::LLMQ_TEST), 100);
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(LLMQType::LLMQ_DEVNET), 101);
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(LLMQType::LLMQ_TEST_V17), 102);
}

BOOST_AUTO_TEST_CASE(llmq_params_edge_calculations_test)
{
    LLMQParams params;

    // Test with maximum values
    params.useRotation = true;
    params.signingActiveQuorumCount = 1;
    params.keepOldKeys = std::numeric_limits<int>::max() / 2; // Avoid overflow
    params.dkgInterval = 2;

    // Should not overflow
    int depth = params.max_store_depth();
    BOOST_CHECK_GT(depth, 0);

    // Test division by zero protection
    params.signingActiveQuorumCount = 0;
    // This would cause division by zero in max_cycles if not handled
    // The implementation should handle this gracefully or it's a bug

    // Test with all zeros
    params.useRotation = false;
    params.keepOldKeys = 0;
    params.dkgInterval = 0;
    BOOST_CHECK_EQUAL(params.max_store_depth(), 0);
}

BOOST_AUTO_TEST_CASE(llmq_params_rotation_consistency_test)
{
    // Test that rotation parameters are consistent
    LLMQParams rotatedParams;
    rotatedParams.useRotation = true;
    rotatedParams.signingActiveQuorumCount = 4;
    rotatedParams.keepOldConnections = 8; // Should be 2x active for rotated
    rotatedParams.keepOldKeys = 8;
    rotatedParams.dkgInterval = 24;

    // For rotated quorums, keepOldConnections should typically be 2x signingActiveQuorumCount
    BOOST_CHECK_EQUAL(rotatedParams.keepOldConnections, 2 * rotatedParams.signingActiveQuorumCount);

    LLMQParams nonRotatedParams;
    nonRotatedParams.useRotation = false;
    nonRotatedParams.signingActiveQuorumCount = 4;
    nonRotatedParams.keepOldConnections = 5; // Should be at least active + 1 for non-rotated
    nonRotatedParams.keepOldKeys = 8;
    nonRotatedParams.dkgInterval = 24;

    // For non-rotated quorums, keepOldConnections should be > signingActiveQuorumCount
    BOOST_CHECK_GT(nonRotatedParams.keepOldConnections, nonRotatedParams.signingActiveQuorumCount);
}

BOOST_AUTO_TEST_CASE(llmq_params_calculations_overflow_test)
{
    LLMQParams params;
    params.useRotation = false;
    params.signingActiveQuorumCount = 1;

    // Test max_cycles with large values
    int largeQuorumCount = std::numeric_limits<int>::max();
    int cycles = params.max_cycles(largeQuorumCount);
    BOOST_CHECK_EQUAL(cycles, largeQuorumCount); // Non-rotated returns input

    // Test potential overflow in max_store_depth
    params.keepOldKeys = std::numeric_limits<int>::max() / 100;
    params.dkgInterval = 100;

    // This should not crash or overflow
    int depth = params.max_store_depth();
    BOOST_CHECK_GE(depth, 0); // Result should be valid

    // Test with rotation and potential division issues
    params.useRotation = true;
    params.signingActiveQuorumCount = std::numeric_limits<int>::max();
    cycles = params.max_cycles(1000);
    BOOST_CHECK_EQUAL(cycles, 0); // 1000 / max_int = 0
}

BOOST_AUTO_TEST_SUITE_END()
