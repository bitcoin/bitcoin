// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/params.h>
#include <consensus/validation.h>
#include <kernel/chainparams.h>
#include <primitives/block.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <string>

BOOST_AUTO_TEST_SUITE(validation_check_header_tests)

BOOST_AUTO_TEST_CASE(intrinsic_valid)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};

    // The genesis header has valid proof of work.
    const CBlockHeader header{chainparams->GenesisBlock()};

    BlockValidationState state;

    BOOST_CHECK(CheckBlockHeader(header, state, consensusParams));
    BOOST_CHECK(state.IsValid());
}

BOOST_AUTO_TEST_CASE(intrinsic_high_hash)
{
    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};

    CBlockHeader header{chainparams->GenesisBlock()};
    // Decrementing the nonce invalidates the proof of work.
    header.nNonce--;

    BlockValidationState state;

    BOOST_CHECK(!CheckBlockHeader(header, state, consensusParams));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "high-hash");
}

BOOST_AUTO_TEST_SUITE_END()
