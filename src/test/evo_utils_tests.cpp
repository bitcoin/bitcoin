// Copyright (c) 2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <llmq/utils.h>
#include <llmq/params.h>
#include <llmq/quorums.h>

#include <chainparams.h>

#include <validation.h>

#include <boost/test/unit_test.hpp>

/* TODO: rename this file and test to llmq_utils_test */
BOOST_AUTO_TEST_SUITE(evo_utils_tests)

void Test()
{
    using namespace llmq::utils;
    const auto& consensus_params = Params().GetConsensus();
    assert(llmq::quorumManager);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeInstantSend, *llmq::quorumManager, nullptr, false, false), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeInstantSend, *llmq::quorumManager, nullptr, true, false), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeInstantSend, *llmq::quorumManager, nullptr, true, true), false);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeDIP0024InstantSend, *llmq::quorumManager, nullptr, false, false), false);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeDIP0024InstantSend, *llmq::quorumManager, nullptr, true, false), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeDIP0024InstantSend, *llmq::quorumManager, nullptr, true, true), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeChainLocks, *llmq::quorumManager, nullptr, false, false), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeChainLocks, *llmq::quorumManager, nullptr, false, false), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeChainLocks, *llmq::quorumManager, nullptr, true, false), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypePlatform, *llmq::quorumManager, nullptr, true, false), Params().IsTestChain());
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypePlatform, *llmq::quorumManager, nullptr, true, true), Params().IsTestChain());
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypePlatform, *llmq::quorumManager, nullptr, true, true), Params().IsTestChain());
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeMnhf, *llmq::quorumManager, nullptr, true, false), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeMnhf, *llmq::quorumManager, nullptr, true, true), true);
    BOOST_CHECK_EQUAL(IsQuorumTypeEnabledInternal(consensus_params.llmqTypeMnhf, *llmq::quorumManager, nullptr, true, true), true);
}

BOOST_FIXTURE_TEST_CASE(utils_IsQuorumTypeEnabled_tests_regtest, RegTestingSetup)
{
    Test();
}

BOOST_FIXTURE_TEST_CASE(utils_IsQuorumTypeEnabled_tests_mainnet, TestingSetup)
{
    Test();
}

BOOST_AUTO_TEST_SUITE_END()
