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

BOOST_AUTO_TEST_CASE(contextual_valid)
{
    LOCK(::cs_main);

    CBlockIndex prev;
    prev.nHeight = 100;
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header;
    header.nVersion = 1;
    header.hashPrevBlock = uint256{};
    header.hashMerkleRoot = uint256{"4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"};
    header.nTime = prev.GetMedianTimePast() + 1;
    header.nBits = 0x1d00ffff;
    header.nNonce = 2083236893;

    const NodeClock::time_point now{std::chrono::seconds{header.nTime}};

    BlockValidationState state;
    Consensus::Params consensusParams = CChainParams::Main()->GetConsensus();

    BOOST_CHECK(ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsValid());
}

BOOST_AUTO_TEST_CASE(contextual_bad_diffbits)
{
    LOCK(::cs_main);

    CBlockIndex prev;
    prev.nHeight = 100;
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header;
    header.nVersion = 1;
    header.hashPrevBlock = uint256{};
    header.hashMerkleRoot = uint256{"4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"};
    header.nTime = prev.GetMedianTimePast() + 1;
    header.nBits = 0x1dffffff;
    header.nNonce = 2083236893;

    const NodeClock::time_point now{std::chrono::seconds{header.nTime}};

    BlockValidationState state;
    Consensus::Params consensusParams = CChainParams::Main()->GetConsensus();

    BOOST_CHECK(!ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-diffbits");
}

BOOST_AUTO_TEST_CASE(contextual_time_too_old)
{
    LOCK(::cs_main);

    CBlockIndex prev;
    prev.nHeight = 100;
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header;
    header.nVersion = 1;
    header.hashPrevBlock = uint256{};
    header.hashMerkleRoot = uint256{"4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"};
    header.nTime = prev.GetMedianTimePast();
    header.nBits = 0x1d00ffff;
    header.nNonce = 2083236893;

    const NodeClock::time_point now{std::chrono::seconds{prev.nTime}};

    BlockValidationState state;
    Consensus::Params consensusParams = CChainParams::Main()->GetConsensus();

    BOOST_CHECK(!ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "time-too-old");
}

BOOST_AUTO_TEST_CASE(contextual_time_timewarp_attack)
{
    LOCK(::cs_main);

    // The timewarp check is only enforced on chains with BIP94.
    CChainParams::RegTestOptions opts{.enforce_bip94 = true};
    Consensus::Params consensusParams = CChainParams::RegTest(opts)->GetConsensus();

    const int interval{static_cast<int>(consensusParams.DifficultyAdjustmentInterval())};
    std::vector<CBlockIndex> chain(interval);
    for (int i = 0; i < interval; ++i) {
        chain[i].nHeight = i;
        chain[i].nTime = 1231006505 + i * 600;
        chain[i].nBits = 0x207fffff;
        chain[i].pprev = i > 0 ? &chain[i - 1] : nullptr;
    }

    CBlockIndex& prev{chain.back()};
    prev.nTime += 10000;

    CBlockHeader header;
    header.nVersion = 4;
    header.hashPrevBlock = uint256{};
    header.hashMerkleRoot = uint256{"4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"};
    header.nTime = prev.nTime - MAX_TIMEWARP - 1;
    header.nBits = prev.nBits;
    header.nNonce = 2083236893;
    BOOST_REQUIRE(header.GetBlockTime() > prev.GetMedianTimePast());

    const NodeClock::time_point now{std::chrono::seconds{header.nTime}};

    BlockValidationState state;

    BOOST_CHECK(!ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "time-timewarp-attack");
}

BOOST_AUTO_TEST_CASE(contextual_time_too_new)
{
    LOCK(::cs_main);

    CBlockIndex prev;
    prev.nHeight = 100;
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header;
    header.nVersion = 1;
    header.hashPrevBlock = uint256{};
    header.hashMerkleRoot = uint256{"4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"};
    header.nTime = prev.nTime + MAX_FUTURE_BLOCK_TIME + 1;
    header.nBits = 0x1d00ffff;
    header.nNonce = 2083236893;

    const NodeClock::time_point now{std::chrono::seconds{prev.nTime}};

    BlockValidationState state;
    Consensus::Params consensusParams = CChainParams::Main()->GetConsensus();

    BOOST_CHECK(!ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "time-too-new");
}

BOOST_AUTO_TEST_CASE(contextual_bad_version_1)
{
    LOCK(::cs_main);

    Consensus::Params consensusParams = CChainParams::Main()->GetConsensus();

    CBlockIndex prev;
    prev.nHeight = consensusParams.DeploymentHeight(Consensus::DEPLOYMENT_HEIGHTINCB);
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header;
    header.nVersion = 1;
    header.hashPrevBlock = uint256{};
    header.hashMerkleRoot = uint256{"4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"};
    header.nTime = prev.GetMedianTimePast() + 1;
    header.nBits = 0x1d00ffff;
    header.nNonce = 2083236893;

    const NodeClock::time_point now{std::chrono::seconds{prev.nTime}};

    BlockValidationState state;

    BOOST_CHECK(!ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-version(0x00000001)");
}

BOOST_AUTO_TEST_CASE(contextual_bad_version_2)
{
    LOCK(::cs_main);

    Consensus::Params consensusParams = CChainParams::Main()->GetConsensus();

    CBlockIndex prev;
    prev.nHeight = consensusParams.DeploymentHeight(Consensus::DEPLOYMENT_DERSIG);
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header;
    header.nVersion = 2;
    header.hashPrevBlock = uint256{};
    header.hashMerkleRoot = uint256{"4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"};
    header.nTime = prev.GetMedianTimePast() + 1;
    header.nBits = 0x1d00ffff;
    header.nNonce = 2083236893;

    const NodeClock::time_point now{std::chrono::seconds{prev.nTime}};

    BlockValidationState state;

    BOOST_CHECK(!ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-version(0x00000002)");
}

BOOST_AUTO_TEST_CASE(contextual_bad_version_3)
{
    LOCK(::cs_main);

    Consensus::Params consensusParams = CChainParams::Main()->GetConsensus();

    CBlockIndex prev;
    prev.nHeight = consensusParams.DeploymentHeight(Consensus::DEPLOYMENT_CLTV);
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header;
    header.nVersion = 3;
    header.hashPrevBlock = uint256{};
    header.hashMerkleRoot = uint256{"4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"};
    header.nTime = prev.GetMedianTimePast() + 1;
    header.nBits = 0x1d00ffff;
    header.nNonce = 2083236893;

    const NodeClock::time_point now{std::chrono::seconds{prev.nTime}};

    BlockValidationState state;

    BOOST_CHECK(!ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-version(0x00000003)");
}

BOOST_AUTO_TEST_SUITE_END()
