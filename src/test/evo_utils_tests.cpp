// Copyright (c) 2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <llmq/utils.h>
#include <llmq/params.h>

#include <chainparams.h>

#include <validation.h>

#include <boost/test/unit_test.hpp>

/* TODO: rename this file and test to llmq_utils_test */
BOOST_AUTO_TEST_SUITE(evo_utils_tests)

void Test()
{
    using namespace llmq;
    const auto& consensus_params = Params().GetConsensus();
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypeInstantSend, nullptr, false, false), true);
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypeInstantSend, nullptr, true, false), true);
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypeInstantSend, nullptr, true, true), false);
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypeDIP0024InstantSend, nullptr, false, false), false);
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypeDIP0024InstantSend, nullptr, true, false), true);
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypeDIP0024InstantSend, nullptr, true, true), true);
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypeChainLocks, nullptr, false, false), true);
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypeChainLocks, nullptr, false, false), true);
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypeChainLocks, nullptr, true, false), true);
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypePlatform, nullptr, true, false), Params().IsTestChain());
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypePlatform, nullptr, true, true), Params().IsTestChain());
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypePlatform, nullptr, true, true), Params().IsTestChain());
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypeMnhf, nullptr, true, false), true);
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypeMnhf, nullptr, true, true), true);
    BOOST_CHECK_EQUAL(CLLMQUtils::IsQuorumTypeEnabledInternal(consensus_params.llmqTypeMnhf, nullptr, true, true), true);
}

BOOST_FIXTURE_TEST_CASE(utils_IsQuorumTypeEnabled_tests_regtest, RegTestingSetup)
{
    Test();
}

BOOST_FIXTURE_TEST_CASE(utils_IsQuorumTypeEnabled_tests_mainnet, BasicTestingSetup)
{
    Test();
}

BOOST_AUTO_TEST_SUITE_END()
