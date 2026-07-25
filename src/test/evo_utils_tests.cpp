// Copyright (c) 2022-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <chainparams.h>
#include <llmq/context.h>
#include <llmq/options.h>
#include <llmq/utils.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

using node::NodeContext;

/* TODO: rename this file and test to llmq_options_test */
BOOST_AUTO_TEST_SUITE(evo_utils_tests)

void Test(NodeContext& node)
{
    using namespace llmq;
    auto tip = node.chainman->ActiveTip();
    const auto& consensus_params = Params().GetConsensus();
    BOOST_CHECK_EQUAL(node.chainman->IsQuorumTypeEnabled(consensus_params.llmqTypeDIP0024InstantSend, tip,
                                                         /*optDIP0024IsActive=*/false, /*optHaveDIP0024Quorums=*/false),
                      false);
    BOOST_CHECK_EQUAL(node.chainman->IsQuorumTypeEnabled(consensus_params.llmqTypeDIP0024InstantSend, tip,
                                                         /*optDIP0024IsActive=*/true, /*optHaveDIP0024Quorums=*/false),
                      true);
    BOOST_CHECK_EQUAL(node.chainman->IsQuorumTypeEnabled(consensus_params.llmqTypeDIP0024InstantSend, tip,
                                                         /*optDIP0024IsActive=*/true, /*optHaveDIP0024Quorums=*/true),
                      true);
    BOOST_CHECK_EQUAL(node.chainman->IsQuorumTypeEnabled(consensus_params.llmqTypeChainLocks, tip,
                                                         /*optDIP0024IsActive=*/false, /*optHaveDIP0024Quorums=*/false),
                      true);
    BOOST_CHECK_EQUAL(node.chainman->IsQuorumTypeEnabled(consensus_params.llmqTypeChainLocks, tip,
                                                         /*optDIP0024IsActive=*/true, /*optHaveDIP0024Quorums=*/false),
                      true);
    BOOST_CHECK_EQUAL(node.chainman->IsQuorumTypeEnabled(consensus_params.llmqTypeChainLocks, tip,
                                                         /*optDIP0024IsActive=*/true, /*optHaveDIP0024Quorums=*/true),
                      true);
    BOOST_CHECK_EQUAL(node.chainman->IsQuorumTypeEnabled(consensus_params.llmqTypePlatform, tip,
                                                         /*optDIP0024IsActive=*/false, /*optHaveDIP0024Quorums=*/false),
                      Params().IsTestChain());
    BOOST_CHECK_EQUAL(node.chainman->IsQuorumTypeEnabled(consensus_params.llmqTypePlatform, tip,
                                                         /*optDIP0024IsActive=*/true, /*optHaveDIP0024Quorums=*/false),
                      Params().IsTestChain());
    BOOST_CHECK_EQUAL(node.chainman->IsQuorumTypeEnabled(consensus_params.llmqTypePlatform, tip,
                                                         /*optDIP0024IsActive=*/true, /*optHaveDIP0024Quorums=*/true),
                      Params().IsTestChain());
    BOOST_CHECK_EQUAL(node.chainman->IsQuorumTypeEnabled(consensus_params.llmqTypeMnhf, tip,
                                                         /*optDIP0024IsActive=*/false, /*optHaveDIP0024Quorums=*/false),
                      true);
    BOOST_CHECK_EQUAL(node.chainman->IsQuorumTypeEnabled(consensus_params.llmqTypeMnhf, tip,
                                                         /*optDIP0024IsActive=*/true, /*optHaveDIP0024Quorums=*/false),
                      true);
    BOOST_CHECK_EQUAL(node.chainman->IsQuorumTypeEnabled(consensus_params.llmqTypeMnhf, tip,
                                                         /*optDIP0024IsActive=*/true, /*optHaveDIP0024Quorums=*/true),
                      true);
}

BOOST_FIXTURE_TEST_CASE(utils_IsQuorumTypeEnabled_tests_regtest, RegTestingSetup)
{
    Test(m_node);
}

BOOST_FIXTURE_TEST_CASE(utils_IsQuorumTypeEnabled_tests_mainnet, TestingSetup)
{
    Test(m_node);
}

// Regression: a quorum commitment naming the genesis block gives GetAllQuorumMembers() a base
// index whose pprev is null. That null used to be handed to IsQuorumTypeEnabled()'s
// gsl::not_null parameter, whose Expects() calls std::terminate() -- [[noreturn]] noexcept, so
// none of the catch(const std::exception&) handlers around special-tx processing could contain
// it, and every node connecting such a block died. It must be reported as "not enabled" instead.
BOOST_FIXTURE_TEST_CASE(genesis_quorum_base_null_pprev_is_safe, RegTestingSetup)
{
    const CBlockIndex* genesis = WITH_LOCK(::cs_main, return m_node.chainman->ActiveTip());
    BOOST_REQUIRE(genesis != nullptr);
    BOOST_REQUIRE_EQUAL(genesis->nHeight, 0);
    BOOST_REQUIRE(genesis->pprev == nullptr);

    const auto llmq_type = Params().GetConsensus().llmqTypeChainLocks;

    // The sink itself, as reached via `pQuorumBaseBlockIndex->pprev`. Passed through a variable
    // rather than a literal so this still compiles against the pre-fix gsl::not_null signature,
    // where it terminated at runtime.
    const CBlockIndex* const null_index = genesis->pprev;
    BOOST_CHECK(!m_node.chainman->IsQuorumTypeEnabled(llmq_type, null_index));

    // The GetAllQuorumMembers() path that CFinalCommitment::Verify() takes.
    const llmq::UtilParameters util_params{*Assert(m_node.dmnman), *Assert(m_node.llmq_ctx)->qsnapman,
                                           *Assert(m_node.chainman), genesis};
    BOOST_CHECK(llmq::utils::GetAllQuorumMembers(llmq_type, util_params).empty());
}

BOOST_AUTO_TEST_SUITE_END()
