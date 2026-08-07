// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(validation_header_tests)

BOOST_AUTO_TEST_CASE(valid_intrinsic)
{
    CBlockHeader header;
    header.nVersion = 1;
    header.hashPrevBlock = uint256{};
    header.hashMerkleRoot = uint256{"4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"};
    header.nTime = 1231006505;
    header.nBits = 0x1d00ffff;
    header.nNonce = 2083236893;

    BlockValidationState state;
    Consensus::Params consensusParams = CChainParams::Main()->GetConsensus();

    BOOST_CHECK(CheckBlockHeader(header, state, consensusParams));
    BOOST_CHECK(state.IsValid());
}

BOOST_AUTO_TEST_CASE(invalid_proof_of_work)
{
    CBlockHeader header;
    header.nVersion = 1;
    header.hashPrevBlock = uint256{};
    header.hashMerkleRoot = uint256{"4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"};
    header.nTime = 1231006505;
    header.nBits = 0x1d00ffff;
    header.nNonce = 2083236892;

    BlockValidationState state;
    Consensus::Params consensusParams = CChainParams::Main()->GetConsensus();

    BOOST_CHECK(!CheckBlockHeader(header, state, consensusParams));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "high-hash");
}

BOOST_AUTO_TEST_SUITE_END()
