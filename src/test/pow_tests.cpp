// Copyright (c) 2015-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

/* Test calculation of next difficulty target with no constraints applying */
BOOST_AUTO_TEST_CASE(get_next_work)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1261130161; // Block #30240
    CBlockIndex pindexLast;
    pindexLast.nHeight = 32255;
    pindexLast.nTime = 1262152739;  // Block #32255
    pindexLast.nBits = 0x1d00ffff;

    // Here (and below): expected_nbits is calculated in
    // CalculateNextWorkRequired(); redoing the calculation here would be just
    // reimplementing the same code that is written in pow.cpp. Rather than
    // copy that code, we just hardcode the expected result.
    unsigned int expected_nbits = 0x1d00d86aU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
}

/* Test the constraint on the upper bound for next work */
BOOST_AUTO_TEST_CASE(get_next_work_pow_limit)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1231006505; // Block #0
    CBlockIndex pindexLast;
    pindexLast.nHeight = 2015;
    pindexLast.nTime = 1233061996;  // Block #2015
    pindexLast.nBits = 0x1d00ffff;
    unsigned int expected_nbits = 0x1d00ffffU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
}

/* Test the constraint on the lower bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1279008237; // Block #66528
    CBlockIndex pindexLast;
    pindexLast.nHeight = 68543;
    pindexLast.nTime = 1279297671;  // Block #68543
    pindexLast.nBits = 0x1c05a3f4;
    unsigned int expected_nbits = 0x1c0168fdU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
    // Test that reducing nbits further would not be a PermittedDifficultyTransition.
    unsigned int invalid_nbits = expected_nbits-1;
    BOOST_CHECK(!PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, invalid_nbits));
}

/* Test the constraint on the upper bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_upper_limit_actual)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1263163443; // NOTE: Not an actual block time
    CBlockIndex pindexLast;
    pindexLast.nHeight = 46367;
    pindexLast.nTime = 1269211443;  // Block #46367
    pindexLast.nBits = 0x1c387f6f;
    unsigned int expected_nbits = 0x1d00e1fdU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
    // Test that increasing nbits further would not be a PermittedDifficultyTransition.
    unsigned int invalid_nbits = expected_nbits+1;
    BOOST_CHECK(!PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, invalid_nbits));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_negative_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    nBits = UintToArith256(consensus.powLimit).GetCompact(true);
    hash = uint256{1};
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_overflow_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits{~0x00800000U};
    hash = uint256{1};
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_too_easy_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 nBits_arith = UintToArith256(consensus.powLimit);
    nBits_arith *= 2;
    nBits = nBits_arith.GetCompact();
    hash = uint256{1};
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_biger_hash_than_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith = UintToArith256(consensus.powLimit);
    nBits = hash_arith.GetCompact();
    hash_arith *= 2; // hash > nBits
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_zero_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith{0};
    nBits = hash_arith.GetCompact();
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1]) : arith_uint256(0);
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[m_rng.randrange(10000)];
        CBlockIndex *p2 = &blocks[m_rng.randrange(10000)];
        CBlockIndex *p3 = &blocks[m_rng.randrange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }
}

void sanity_check_chainparams(const ArgsManager& args, ChainType chain_type)
{
    const auto chainParams = CreateChainParams(args, chain_type);
    const auto consensus = chainParams->GetConsensus();

    // hash genesis is correct
    BOOST_CHECK_EQUAL(consensus.hashGenesisBlock, chainParams->GenesisBlock().GetHash());

    // target timespan is an even multiple of spacing
    BOOST_CHECK_EQUAL(consensus.nPowTargetTimespan % consensus.nPowTargetSpacing, 0);

    // genesis nBits is positive, doesn't overflow and is lower than powLimit
    arith_uint256 pow_compact;
    bool neg, over;
    pow_compact.SetCompact(chainParams->GenesisBlock().nBits, &neg, &over);
    BOOST_CHECK(!neg && pow_compact != 0);
    BOOST_CHECK(!over);
    BOOST_CHECK(UintToArith256(consensus.powLimit) >= pow_compact);

    // check max target * 4*nPowTargetTimespan doesn't overflow -- see pow.cpp:CalculateNextWorkRequired()
    if (!consensus.fPowNoRetargeting) {
        arith_uint256 targ_max{UintToArith256(uint256{"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"})};
        targ_max /= consensus.nPowTargetTimespan*4;
        BOOST_CHECK(UintToArith256(consensus.powLimit) < targ_max);
    }
}

BOOST_AUTO_TEST_CASE(ChainParams_MAIN_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::MAIN);
}

BOOST_AUTO_TEST_CASE(ChainParams_REGTEST_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::REGTEST);
}

BOOST_AUTO_TEST_CASE(ChainParams_TESTNET_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::TESTNET);
}

BOOST_AUTO_TEST_CASE(ChainParams_TESTNET4_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::TESTNET4);
}

BOOST_AUTO_TEST_CASE(ChainParams_SIGNET_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::SIGNET);
}

/* Test that testnet4 min-difficulty blocks are allowed before the fork height */
BOOST_AUTO_TEST_CASE(testnet4_min_difficulty_pre_fork)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::TESTNET4);
    const auto& consensus = chainParams->GetConsensus();

    // Verify testnet4 has the expected fork height (epoch 75 boundary)
    const int fork_height = consensus.min_difficulty_fork_height;
    BOOST_CHECK_EQUAL(fork_height, 151200);
    BOOST_CHECK(consensus.fPowAllowMinDifficultyBlocks);

    const unsigned int pow_limit = UintToArith256(consensus.powLimit).GetCompact();

    // Create a mock chain so next block is fork_height - 1 (before fork)
    CBlockIndex pindexPrev;
    pindexPrev.nHeight = fork_height - 2;
    pindexPrev.nTime = 1714777860;  // Some base time
    pindexPrev.nBits = 0x1c00ffff;  // Some non-trivial difficulty

    // Create a block header with timestamp >20 minutes after previous block
    // This should trigger the min-difficulty rule before the fork
    CBlockHeader block;
    block.nTime = pindexPrev.nTime + consensus.nPowTargetSpacing * 2 + 1;  // >20 minutes later

    unsigned int bits = GetNextWorkRequired(&pindexPrev, &block, consensus);

    // Before fork: min-difficulty should be allowed
    BOOST_CHECK_EQUAL(bits, pow_limit);
}

/* Test that at the fork height, difficulty resets to 1,000,000 */
BOOST_AUTO_TEST_CASE(testnet4_difficulty_reset_at_fork)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::TESTNET4);
    const auto& consensus = chainParams->GetConsensus();
    const int fork_height = consensus.min_difficulty_fork_height;

    // Calculate expected difficulty: powLimit / 1,000,000
    arith_uint256 expected_target = UintToArith256(consensus.powLimit);
    expected_target /= 1000000;
    const unsigned int expected_bits = expected_target.GetCompact();

    // Create a mock chain so next block is at fork_height
    CBlockIndex pindexPrev;
    pindexPrev.nHeight = fork_height - 1;
    pindexPrev.nTime = 1714777860;
    pindexPrev.nBits = 0x1c00ffff;  // Some non-trivial (high) difficulty

    // Block header doesn't matter for the reset - it happens unconditionally at fork height
    CBlockHeader block;
    block.nTime = pindexPrev.nTime + consensus.nPowTargetSpacing;

    unsigned int bits = GetNextWorkRequired(&pindexPrev, &block, consensus);

    // At fork height: difficulty should reset to 1,000,000
    BOOST_CHECK_EQUAL(bits, expected_bits);
}

/* Test that testnet4 min-difficulty blocks are NOT allowed after the fork height */
BOOST_AUTO_TEST_CASE(testnet4_min_difficulty_post_fork)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::TESTNET4);
    const auto& consensus = chainParams->GetConsensus();
    const int fork_height = consensus.min_difficulty_fork_height;

    const unsigned int pow_limit = UintToArith256(consensus.powLimit).GetCompact();

    // Create a mock chain so next block is fork_height + 1 (after fork)
    CBlockIndex pindexPrev;
    pindexPrev.nHeight = fork_height;
    pindexPrev.nTime = 1714777860;
    pindexPrev.nBits = 0x1c00ffff;

    CBlockHeader block;
    block.nTime = pindexPrev.nTime + consensus.nPowTargetSpacing * 2 + 1;

    unsigned int bits = GetNextWorkRequired(&pindexPrev, &block, consensus);

    // After fork: min-difficulty should NOT be allowed
    // Even with >20 min delay, should return previous block's difficulty
    BOOST_CHECK_EQUAL(bits, pindexPrev.nBits);
    BOOST_CHECK(bits != pow_limit);
}

/* Test PermittedDifficultyTransition for testnet4 before and after fork */
BOOST_AUTO_TEST_CASE(testnet4_permitted_difficulty_transition)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::TESTNET4);
    const auto& consensus = chainParams->GetConsensus();
    const int fork_height = consensus.min_difficulty_fork_height;

    const unsigned int pow_limit = UintToArith256(consensus.powLimit).GetCompact();
    const unsigned int some_difficulty = 0x1c00ffff;

    // Calculate the fork reset difficulty (1,000,000)
    arith_uint256 fork_target = UintToArith256(consensus.powLimit);
    fork_target /= 1000000;
    const unsigned int fork_difficulty = fork_target.GetCompact();

    // Before fork: any transition should be permitted
    // because fPowAllowMinDifficultyBlocks is true and we're before fork
    BOOST_CHECK(PermittedDifficultyTransition(consensus, fork_height - 1, some_difficulty, pow_limit));
    BOOST_CHECK(PermittedDifficultyTransition(consensus, fork_height - 1, some_difficulty, some_difficulty));
    BOOST_CHECK(PermittedDifficultyTransition(consensus, fork_height - 1, pow_limit, some_difficulty));

    // At fork: only transition to difficulty 1,000,000 is permitted
    BOOST_CHECK(PermittedDifficultyTransition(consensus, fork_height, some_difficulty, fork_difficulty));
    BOOST_CHECK(!PermittedDifficultyTransition(consensus, fork_height, some_difficulty, pow_limit));
    BOOST_CHECK(!PermittedDifficultyTransition(consensus, fork_height, some_difficulty, some_difficulty));

    // After fork: strict rules apply
    // For non-retarget heights, difficulty must stay the same
    BOOST_CHECK(!PermittedDifficultyTransition(consensus, fork_height + 1, some_difficulty, pow_limit));
    BOOST_CHECK(PermittedDifficultyTransition(consensus, fork_height + 1, some_difficulty, some_difficulty));

    // Further after fork: same strict rules
    int non_retarget_height = fork_height + 800;
    BOOST_CHECK(!PermittedDifficultyTransition(consensus, non_retarget_height, some_difficulty, pow_limit));
    BOOST_CHECK(PermittedDifficultyTransition(consensus, non_retarget_height, some_difficulty, some_difficulty));

    // At the next retarget after the fork, the transition must be based on the
    // fork reset difficulty (1,000,000), not the pre-fork difficulty.
    const int next_retarget = fork_height + consensus.DifficultyAdjustmentInterval();
    // Retarget from fork_difficulty: within factor of 4 is permitted
    BOOST_CHECK(PermittedDifficultyTransition(consensus, next_retarget, fork_difficulty, fork_difficulty));
    // Transitioning from some unrelated pre-fork difficulty to fork_difficulty is not valid
    // if it exceeds the 4x adjustment limit
    BOOST_CHECK(!PermittedDifficultyTransition(consensus, next_retarget, fork_difficulty, pow_limit));
}

/* Test that regtest is unaffected by the fork (min_difficulty_fork_height = 0) */
BOOST_AUTO_TEST_CASE(regtest_min_difficulty_unaffected)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::REGTEST);
    const auto& consensus = chainParams->GetConsensus();

    // Verify regtest has no fork (min_difficulty_fork_height = 0 means disabled)
    BOOST_CHECK_EQUAL(consensus.min_difficulty_fork_height, 0);
    BOOST_CHECK(consensus.fPowAllowMinDifficultyBlocks);

    // Use testnet4's fork height to verify regtest is unaffected at those same heights
    const auto testnet4Params = CreateChainParams(*m_node.args, ChainType::TESTNET4);
    const int testnet4_fork_height = testnet4Params->GetConsensus().min_difficulty_fork_height;

    // PermittedDifficultyTransition should always return true for regtest
    // regardless of height (even at heights around the testnet4 fork)
    const unsigned int pow_limit = UintToArith256(consensus.powLimit).GetCompact();
    const unsigned int some_difficulty = 0x1c00ffff;

    BOOST_CHECK(PermittedDifficultyTransition(consensus, testnet4_fork_height - 1, some_difficulty, pow_limit));
    BOOST_CHECK(PermittedDifficultyTransition(consensus, testnet4_fork_height, some_difficulty, pow_limit));
    BOOST_CHECK(PermittedDifficultyTransition(consensus, testnet4_fork_height + 1, some_difficulty, pow_limit));
    BOOST_CHECK(PermittedDifficultyTransition(consensus, 200000, some_difficulty, pow_limit));
}

/* Test that mainnet is unaffected (fPowAllowMinDifficultyBlocks = false) */
BOOST_AUTO_TEST_CASE(mainnet_min_difficulty_unaffected)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    const auto& consensus = chainParams->GetConsensus();

    // Mainnet doesn't allow min-difficulty blocks at all
    BOOST_CHECK(!consensus.fPowAllowMinDifficultyBlocks);
    BOOST_CHECK_EQUAL(consensus.min_difficulty_fork_height, 0);

    // PermittedDifficultyTransition enforces strict rules on mainnet
    const unsigned int some_difficulty = 0x1c00ffff;
    const unsigned int pow_limit = UintToArith256(consensus.powLimit).GetCompact();

    // Non-retarget height: difficulty must stay same
    // (chose an arbitrary non-divisible by 2016 block height)
    int non_retarget_height = 150001;
    BOOST_CHECK(!PermittedDifficultyTransition(consensus, non_retarget_height, some_difficulty, pow_limit));
    BOOST_CHECK(PermittedDifficultyTransition(consensus, non_retarget_height, some_difficulty, some_difficulty));
}

BOOST_AUTO_TEST_SUITE_END()
