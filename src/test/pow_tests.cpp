// Copyright (c) 2015-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

/* Test calculation of next difficulty target with DGW */
BOOST_AUTO_TEST_CASE(get_next_work)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);

    static const std::vector<std::pair<uint32_t, uint32_t>> mainnet_data = {
        { 1408728124, 0x1b104be1U }, { 1408728332, 0x1b10e09eU }, { 1408728479, 0x1b11a33cU },
        { 1408728495, 0x1b121cf3U }, { 1408728608, 0x1b11951eU }, { 1408728744, 0x1b11abacU },
        { 1408728756, 0x1b118d9cU }, { 1408728950, 0x1b1123f9U }, { 1408729116, 0x1b1141bfU },
        { 1408729179, 0x1b110764U }, { 1408729305, 0x1b107556U }, { 1408729474, 0x1b104297U },
        { 1408729576, 0x1b1063d0U }, { 1408729587, 0x1b10e878U }, { 1408729647, 0x1b0dfaffU },
        { 1408729678, 0x1b0c9ab8U }, { 1408730179, 0x1b0c03d6U }, { 1408730862, 0x1b0dd168U },
        { 1408730914, 0x1b10b864U }, { 1408731242, 0x1b0fed89U }, { 1408731256, 0x1b113ff1U },
        { 1408732229, 0x1b10460bU }, { 1408732257, 0x1b13b83fU }, { 1408732489, 0x1b1418d4U }
    };

    // Construct a chain of block index entries
    std::list<CBlockIndex> blockidx;
    CBlockIndex* blockIndexLast{nullptr};

    for (const auto& [nTime, nBits] : mainnet_data) {
        auto& entry = blockidx.emplace_back();
        entry.nTime = nTime;
        entry.nBits = nBits;
        entry.pprev = blockIndexLast;
        blockIndexLast = &entry;
    }
    blockIndexLast->nHeight = 123456;
    assert(mainnet_data.size() == blockidx.size());

    CBlockHeader blockHeader;
    blockHeader.nTime = 1408732505; // Block #123457
    BOOST_CHECK_EQUAL(GetNextWorkRequired(blockIndexLast, &blockHeader, chainParams->GetConsensus()), 0x1b1441deU); // Block #123457 has 0x1b1441de

    // test special rules for slow blocks on devnet/testnet
    gArgs.SoftSetBoolArg("-devnet", true);
    const auto chainParamsDev = CreateChainParams(*m_node.args, CBaseChainParams::DEVNET);
    gArgs.ForceRemoveArg("devnet");

    // make sure normal rules apply
    blockHeader.nTime = 1408732505; // Block #123457
    BOOST_CHECK_EQUAL(GetNextWorkRequired(blockIndexLast, &blockHeader, chainParamsDev->GetConsensus()), 0x1b1441deU); // Block #123457 has 0x1b1441de

    // 10x higher target
    blockHeader.nTime = 1408733090; // Block #123457 (10m+1sec)
    BOOST_CHECK_EQUAL(GetNextWorkRequired(blockIndexLast, &blockHeader, chainParamsDev->GetConsensus()), 0x1c00c8f8U); // Block #123457 has 0x1c00c8f8
    blockHeader.nTime = 1408733689; // Block #123457 (20m)
    BOOST_CHECK_EQUAL(GetNextWorkRequired(blockIndexLast, &blockHeader, chainParamsDev->GetConsensus()), 0x1c00c8f8U); // Block #123457 has 0x1c00c8f8
    // lowest diff possible
    blockHeader.nTime = 1408739690; // Block #123457 (2h+1sec)
    BOOST_CHECK_EQUAL(GetNextWorkRequired(blockIndexLast, &blockHeader, chainParamsDev->GetConsensus()), 0x207fffffU); // Block #123457 has 0x207fffff
    blockHeader.nTime = 1408743289; // Block #123457 (3h)
    BOOST_CHECK_EQUAL(GetNextWorkRequired(blockIndexLast, &blockHeader, chainParamsDev->GetConsensus()), 0x207fffffU); // Block #123457 has 0x207fffff
}

/* Test the constraint on the upper bound for next work */
// BOOST_AUTO_TEST_CASE(get_next_work_pow_limit)
// {
//     const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);

//     int64_t nLastRetargetTime = 1231006505; // Block #0
//     CBlockIndex pindexLast;
//     pindexLast.nHeight = 2015;
//     pindexLast.nTime = 1233061996;  // Block #2015
//     pindexLast.nBits = 0x1d00ffff;
//     BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00ffffU);
// }

/* Test the constraint on the lower bound for actual time taken */
// BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual)
// {
//     const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);

//     int64_t nLastRetargetTime = 1279008237; // Block #66528
//     CBlockIndex pindexLast;
//     pindexLast.nHeight = 68543;
//     pindexLast.nTime = 1279297671;  // Block #68543
//     pindexLast.nBits = 0x1c05a3f4;
//     BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1c0168fdU);
// }

/* Test the constraint on the upper bound for actual time taken */
// BOOST_AUTO_TEST_CASE(get_next_work_upper_limit_actual)
// {
//     const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);

//     int64_t nLastRetargetTime = 1263163443; // NOTE: Not an actual block time
//     CBlockIndex pindexLast;
//     pindexLast.nHeight = 46367;
//     pindexLast.nTime = 1269211443;  // Block #46367
//     pindexLast.nBits = 0x1c387f6f;
//     BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00e1fdU);
// }

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_negative_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    nBits = UintToArith256(consensus.powLimit).GetCompact(true);
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_overflow_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits = ~0x00800000;
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_too_easy_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 nBits_arith = UintToArith256(consensus.powLimit);
    nBits_arith *= 2;
    nBits = nBits_arith.GetCompact();
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_biger_hash_than_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
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
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith{0};
    nBits = hash_arith.GetCompact();
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1]) : arith_uint256(0);
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p2 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p3 = &blocks[InsecureRandRange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }
}

BOOST_AUTO_TEST_SUITE_END()
