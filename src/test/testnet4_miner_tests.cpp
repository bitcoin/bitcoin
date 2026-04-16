// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <common/system.h>
#include <util/chaintype.h>
#include <interfaces/mining.h>
#include <node/miner.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/time.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <limits>

using interfaces::BlockTemplate;
using interfaces::Mining;
using node::BlockAssembler;
using node::BlockWaitOptions;
using node::GetMaximumTime;
using node::UpdateTime;

namespace testnet4_miner_tests {

struct Testnet4MinerTestingSetup : public Testnet4Setup {
    std::unique_ptr<Mining> MakeMining()
    {
        return interfaces::MakeMining(m_node, /*wait_loaded=*/false);
    }
};
} // namespace testnet4_miner_tests

BOOST_FIXTURE_TEST_SUITE(testnet4_miner_tests, Testnet4MinerTestingSetup)

BOOST_AUTO_TEST_CASE(MiningInterface)
{
    auto mining{MakeMining()};
    BOOST_REQUIRE(mining);

    BlockAssembler::Options options;
    options.include_dummy_extranonce = true;
    std::unique_ptr<BlockTemplate> block_template;

    // Set node time a few minutes past the testnet4 genesis block
    const auto template_time{3min + WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Tip()->Time())};
    NodeClockContext clock_ctx{template_time};

    block_template = mining->createNewBlock(options, /*cooldown=*/false);
    BOOST_REQUIRE(block_template);

    // The template should use the mocked system time
    BOOST_REQUIRE_EQUAL(block_template->getBlockHeader().Time(), template_time);

    const BlockWaitOptions wait_options{.timeout = MillisecondsDouble{0}, .fee_threshold = 1};

    // waitNext() should return nullptr because there is no better template
    auto should_be_nullptr = block_template->waitNext(wait_options);
    BOOST_REQUIRE(should_be_nullptr == nullptr);

    // This remains the case when exactly 20 minutes have gone by
    clock_ctx += 17min;
    should_be_nullptr = block_template->waitNext(wait_options);
    BOOST_REQUIRE(should_be_nullptr == nullptr);

    // One second later the difficulty drops and it returns a new template
    // Note that we can't test the actual difficulty change, because the
    // difficulty is already at 1.
    clock_ctx += 1s;
    block_template = block_template->waitNext(wait_options);
    BOOST_REQUIRE(block_template);
}

BOOST_AUTO_TEST_CASE(MinDifficultyBlocksFixMaxTimeBeforeFork)
{
    const auto& consensus = Params().GetConsensus();
    BOOST_REQUIRE_EQUAL(consensus.min_difficulty_blocks_fix_height, 151200);

    // pindexPrev at height 151198 -> next block height 151199, still pre-fork.
    CBlockIndex pindexPrev;
    pindexPrev.nHeight = 151198;
    pindexPrev.nTime = 1700000000;

    BOOST_CHECK_EQUAL(GetMaximumTime(&pindexPrev, consensus),
                      std::numeric_limits<int64_t>::max());
}

BOOST_AUTO_TEST_CASE(MinDifficultyBlocksFixMaxTimeAtFork)
{
    const auto& consensus = Params().GetConsensus();

    // pindexPrev at height 151200 -> next block height 151201 is the first
    // non-adjustment block post-fork subject to the cap.
    CBlockIndex pindexPrev;
    pindexPrev.nHeight = 151200;
    pindexPrev.nTime = 1700000000;

    BOOST_CHECK_EQUAL(GetMaximumTime(&pindexPrev, consensus),
                      1700000000 + consensus.nPowTargetSpacing * 2);
}

BOOST_AUTO_TEST_CASE(MinDifficultyBlocksFixMaxTimeAfterFork)
{
    const auto& consensus = Params().GetConsensus();

    // 200001 % 2016 = 417, so next block is not an adjustment block.
    CBlockIndex pindexPrev;
    pindexPrev.nHeight = 200000;
    pindexPrev.nTime = 1800000000;

    BOOST_CHECK_EQUAL(GetMaximumTime(&pindexPrev, consensus),
                      1800000000 + consensus.nPowTargetSpacing * 2);
}

BOOST_AUTO_TEST_CASE(MinDifficultyBlocksFixMaxTimeOnActivationAdjustmentBlock)
{
    const auto& consensus = Params().GetConsensus();

    // The activation height 151200 = 2016 * 75 is itself a difficulty-
    // adjustment block. The cap must not apply to it.
    CBlockIndex pindexPrev;
    pindexPrev.nHeight = 151199;
    pindexPrev.nTime = 1700000000;

    BOOST_CHECK_EQUAL(GetMaximumTime(&pindexPrev, consensus),
                      std::numeric_limits<int64_t>::max());
}

BOOST_AUTO_TEST_CASE(MinDifficultyBlocksFixMaxTimeOnLaterAdjustmentBlock)
{
    const auto& consensus = Params().GetConsensus();

    // Next block height 153216 = 2016 * 76 is the first post-activation
    // adjustment block after the fork epoch. Cap must not apply.
    CBlockIndex pindexPrev;
    pindexPrev.nHeight = 153215;
    pindexPrev.nTime = 1700000000;

    BOOST_CHECK_EQUAL(GetMaximumTime(&pindexPrev, consensus),
                      std::numeric_limits<int64_t>::max());
}

BOOST_AUTO_TEST_CASE(MinDifficultyBlocksFixMaxTimeDisabledOnOtherChains)
{
    // Chains that do not set min_difficulty_blocks_fix_height (default 0)
    // must not have any upper bound on the block timestamp.
    const auto mainnet_params = CreateChainParams(*m_node.args, ChainType::MAIN);
    const auto& consensus = mainnet_params->GetConsensus();
    BOOST_REQUIRE_EQUAL(consensus.min_difficulty_blocks_fix_height, 0);

    CBlockIndex pindexPrev;
    pindexPrev.nHeight = 500000;
    pindexPrev.nTime = 1700000000;

    BOOST_CHECK_EQUAL(GetMaximumTime(&pindexPrev, consensus),
                      std::numeric_limits<int64_t>::max());
}

BOOST_AUTO_TEST_CASE(MinDifficultyBlocksFixUpdateTimeClampsWhenNowPastCap)
{
    const auto& consensus = Params().GetConsensus();
    // Height 151201 is post-fork and not a retarget boundary, so
    // GetNextWorkRequired does not need to walk the pprev chain.
    CBlockIndex pindexPrev;
    pindexPrev.nHeight = 151201;
    pindexPrev.nTime = 1700000000;
    pindexPrev.nBits = UintToArith256(consensus.powLimit).GetCompact();

    // Wall-clock 3000s past pindexPrev is well above the 1200s cap.
    NodeClockContext clock_ctx{std::chrono::seconds{1700000000 + 3000}};

    CBlockHeader header;
    header.nTime = 0;
    UpdateTime(&header, consensus, &pindexPrev);

    BOOST_CHECK_EQUAL(header.nTime, 1700000000 + consensus.nPowTargetSpacing * 2);
}

BOOST_AUTO_TEST_CASE(MinDifficultyBlocksFixUpdateTimeUsesNowWhenBelowCap)
{
    const auto& consensus = Params().GetConsensus();
    CBlockIndex pindexPrev;
    pindexPrev.nHeight = 151201;
    pindexPrev.nTime = 1700000000;
    pindexPrev.nBits = UintToArith256(consensus.powLimit).GetCompact();

    // Wall-clock 500s past pindexPrev is below the 1200s cap.
    NodeClockContext clock_ctx{std::chrono::seconds{1700000000 + 500}};

    CBlockHeader header;
    header.nTime = 0;
    UpdateTime(&header, consensus, &pindexPrev);

    BOOST_CHECK_EQUAL(header.nTime, 1700000000 + 500);
}

BOOST_AUTO_TEST_CASE(MinDifficultyBlocksFixUpdateTimeExactlyAtCap)
{
    const auto& consensus = Params().GetConsensus();
    CBlockIndex pindexPrev;
    pindexPrev.nHeight = 151201;
    pindexPrev.nTime = 1700000000;
    pindexPrev.nBits = UintToArith256(consensus.powLimit).GetCompact();

    // Wall-clock exactly at the cap: must be accepted verbatim.
    const int64_t cap_time{1700000000 + consensus.nPowTargetSpacing * 2};
    NodeClockContext clock_ctx{std::chrono::seconds{cap_time}};

    CBlockHeader header;
    header.nTime = 0;
    UpdateTime(&header, consensus, &pindexPrev);

    BOOST_CHECK_EQUAL(header.nTime, cap_time);
}

BOOST_AUTO_TEST_SUITE_END()
