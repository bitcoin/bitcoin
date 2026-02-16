// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <bench/block_generator.h>

#include <chainparams.h>
#include <streams.h>
#include <validation.h>

#include <cstdint>

BOOST_AUTO_TEST_CASE(block_generator_deterministic_seeded_output)
{
    const auto params{CChainParams::RegTest(CChainParams::RegTestOptions{})};
    const auto& chain_params{*params};
    const auto block_a{benchmark::GenerateBlock(chain_params)};
    const auto block_b{benchmark::GenerateBlock(chain_params)};
    BOOST_CHECK(block_a.GetHash() == block_b.GetHash());
    BOOST_CHECK_EQUAL(block_a.vtx.size(), block_b.vtx.size());
    BOOST_CHECK_EQUAL(block_a.vtx.size(), benchmark::kWitness.tx_count + 1);
}

BOOST_AUTO_TEST_CASE(block_generator_serialization_roundtrip)
{
    const auto params{CChainParams::RegTest(CChainParams::RegTestOptions{})};
    const auto& chain_params{*params};
    const auto block{benchmark::GenerateBlock(chain_params, benchmark::kLegacy, uint256::ONE)};
    DataStream stream{benchmark::GenerateBlockData(chain_params, benchmark::kLegacy, uint256::ONE)};

    CBlock parsed;
    stream >> TX_WITH_WITNESS(parsed);
    BOOST_CHECK(parsed.GetHash() == block.GetHash());

    BlockValidationState validation_state;
    BOOST_CHECK(CheckBlock(parsed, validation_state, chain_params.GetConsensus()));
}

BOOST_AUTO_TEST_CASE(block_generator_seed_perturbation)
{
    const auto params{CChainParams::RegTest(CChainParams::RegTestOptions{})};
    const auto& chain_params{*params};
    const auto block_zero{benchmark::GenerateBlock(chain_params, benchmark::kLegacy, uint256::ZERO)};
    const auto block_one{benchmark::GenerateBlock(chain_params, benchmark::kLegacy, uint256::ONE)};
    BOOST_CHECK(block_zero.GetHash() != block_one.GetHash());
}

BOOST_AUTO_TEST_CASE(block_generator_multiple_seed_sanity)
{
    const auto params{CChainParams::RegTest(CChainParams::RegTestOptions{})};
    const auto& chain_params{*params};
    const bool segwit_active{chain_params.GetConsensus().SegwitHeight <= 1};

    for (uint8_t i{0}; i < 10; ++i) {
        const auto block{benchmark::GenerateBlock(chain_params, benchmark::kWitness, uint256{i})};
        BlockValidationState validation_state;
        BOOST_CHECK(CheckBlock(block, validation_state, chain_params.GetConsensus()));
        BOOST_CHECK(!IsBlockMutated(block, /*check_witness_root=*/segwit_active));
    }
}
