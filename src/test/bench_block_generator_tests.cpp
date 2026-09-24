// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <bench/block_generator.h>

#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <kernel/chainparams.h>
#include <script/script.h>
#include <streams.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <algorithm>
#include <cstdint>
#include <memory>

BOOST_AUTO_TEST_SUITE(bench_block_generator_tests)

BOOST_AUTO_TEST_CASE(block_generator_deterministic_seeded_output)
{
    const auto block{benchmark::GenerateBlock()};
    BOOST_CHECK_EQUAL(block.GetHash().GetHex(), "3e11f86e1716c022e29ad2aab042dc8610faed5f9020681b507420a019d50773");
    BOOST_CHECK_EQUAL(block.vtx.size(), benchmark::WITNESS_RECIPE.tx_count + 1);
}

BOOST_AUTO_TEST_CASE(block_generator_serialization_roundtrip)
{
    const auto params{CChainParams::RegTest()};
    const auto block{benchmark::GenerateBlock(*params, benchmark::LEGACY_RECIPE, uint256::ONE)};
    DataStream stream{benchmark::GenerateBlockData(*params, benchmark::LEGACY_RECIPE, uint256::ONE)};

    CBlock parsed;
    stream >> TX_WITH_WITNESS(parsed);
    BOOST_CHECK(parsed.GetHash() == block.GetHash());

    BlockValidationState validation_state;
    BOOST_CHECK(CheckBlock(parsed, validation_state, params->GetConsensus()));
}

BOOST_AUTO_TEST_CASE(block_generator_seed_perturbation)
{
    const auto params{CChainParams::RegTest()};
    const auto block_zero{benchmark::GenerateBlock(*params, benchmark::LEGACY_RECIPE, uint256::ZERO)};
    const auto block_one{benchmark::GenerateBlock(*params, benchmark::LEGACY_RECIPE, uint256::ONE)};
    BOOST_CHECK(block_zero.GetHash() != block_one.GetHash());
}

BOOST_AUTO_TEST_CASE(block_generator_multiple_seed_sanity)
{
    const auto params{CChainParams::RegTest()};
    for (uint8_t i{0}; i < 10; ++i) {
        const auto block{benchmark::GenerateBlock(*params, benchmark::WITNESS_RECIPE, uint256{i})};
        BlockValidationState validation_state;
        BOOST_CHECK(CheckBlock(block, validation_state, params->GetConsensus()));
        BOOST_CHECK(!IsBlockMutated(block, /*check_witness_root=*/true));
    }
}

BOOST_AUTO_TEST_CASE(block_generator_undo_matches_inputs)
{
    const auto params{CChainParams::RegTest()};
    auto recipe{benchmark::WITNESS_RECIPE};
    recipe.tx_count = 20;
    const auto block{benchmark::GenerateBlock(*params, recipe)};
    const auto undo{benchmark::GenerateBlockUndo(block)};
    BOOST_REQUIRE_EQUAL(undo.vtxundo.size(), block.vtx.size() - 1);

    for (size_t i{1}; i < block.vtx.size(); ++i) {
        const auto& tx{*block.vtx[i]};
        const auto& tx_undo{undo.vtxundo[i - 1]};
        BOOST_REQUIRE_EQUAL(tx_undo.vprevout.size(), tx.vin.size());

        CAmount input_total{};
        for (const auto& coin : tx_undo.vprevout) {
            BOOST_CHECK(!coin.out.scriptPubKey.IsUnspendable());
            input_total += coin.out.nValue;
        }
        BOOST_CHECK_EQUAL(input_total - tx.GetValueOut(), 1000);
    }

    DataStream first;
    first << undo;
    DataStream second;
    second << benchmark::GenerateBlockUndo(block);
    BOOST_CHECK(first.str() == second.str());

    CBlockUndo restored;
    first >> restored;
    BOOST_CHECK_EQUAL(restored.vtxundo.size(), undo.vtxundo.size());
}

BOOST_AUTO_TEST_CASE(block_generator_returns_unchecked_block)
{
    const auto block{benchmark::GenerateBlock()};
    BOOST_CHECK(!block.fChecked);
    BOOST_CHECK(!block.m_checked_merkle_root);
    BOOST_CHECK(!block.m_checked_witness_commitment);

    CBlock mutated{block};
    mutated.hashMerkleRoot = uint256::ONE;
    BlockValidationState state;
    BOOST_CHECK(!CheckBlock(mutated, state, CChainParams::RegTest()->GetConsensus()));
}

BOOST_FIXTURE_TEST_CASE(block_generator_passes_contextual_checks, RegTestingSetup)
{
    for (const auto& recipe : {benchmark::LEGACY_RECIPE, benchmark::WITNESS_RECIPE}) {
        const auto block{std::make_shared<const CBlock>(benchmark::GenerateBlock(m_node.chainman->GetParams(), recipe))};
        BlockValidationState state;
        LOCK(cs_main);
        BOOST_CHECK_MESSAGE(m_node.chainman->AcceptBlock(block, state, /*ppindex=*/nullptr, /*fRequested=*/true, /*dbp=*/nullptr, /*fNewBlock=*/nullptr, /*min_pow_checked=*/true),
                            state.ToString());
    }
}

BOOST_AUTO_TEST_CASE(block_generator_legacy_has_no_witness)
{
    const auto params{CChainParams::RegTest()};
    const auto block{benchmark::GenerateBlock(*params, benchmark::LEGACY_RECIPE)};
    BOOST_CHECK(std::ranges::none_of(block.vtx, [](const auto& tx) { return tx->HasWitness(); }));
    BOOST_CHECK_EQUAL(GetWitnessCommitmentIndex(block), NO_WITNESS_COMMITMENT);
}

BOOST_AUTO_TEST_CASE(block_generator_auto_count_respects_weight)
{
    const auto params{CChainParams::RegTest()};
    for (auto recipe : {benchmark::LEGACY_RECIPE, benchmark::WITNESS_RECIPE}) {
        recipe.tx_count = 0;
        for (uint8_t seed{0}; seed < 10; ++seed) {
            const auto block{benchmark::GenerateBlock(*params, recipe, uint256{seed})};
            BOOST_CHECK_LE(GetBlockWeight(block), MAX_BLOCK_WEIGHT);
            BOOST_CHECK(!IsBlockMutated(block, /*check_witness_root=*/true));
        }
    }

    auto recipe{benchmark::WITNESS_RECIPE};
    recipe.tx_count = 5000;
    const auto block{benchmark::GenerateBlock(*params, recipe)};
    BOOST_CHECK_LT(block.vtx.size(), recipe.tx_count + 1);
    BOOST_CHECK_LE(GetBlockWeight(block), MAX_BLOCK_WEIGHT);
    BOOST_CHECK(!IsBlockMutated(block, /*check_witness_root=*/true));
}

BOOST_AUTO_TEST_SUITE_END()
