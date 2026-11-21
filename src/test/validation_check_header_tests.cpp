// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <consensus/consensus.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <kernel/chainparams.h>
#include <primitives/block.h>
#include <sync.h>
#include <util/time.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <string>
#include <vector>

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

BOOST_AUTO_TEST_CASE(contextual_valid)
{
    LOCK(::cs_main);

    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};

    CBlockIndex prev;
    prev.nHeight = 100;
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header{chainparams->GenesisBlock()};
    header.nTime = prev.GetMedianTimePast() + 1;

    const NodeClock::time_point now{std::chrono::seconds{header.nTime}};

    BlockValidationState state;

    BOOST_CHECK(ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsValid());
}

BOOST_AUTO_TEST_CASE(contextual_bad_diffbits)
{
    LOCK(::cs_main);

    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};

    CBlockIndex prev;
    prev.nHeight = 100;
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header{chainparams->GenesisBlock()};
    header.nTime = prev.GetMedianTimePast() + 1;
    header.nBits = 0x1dffffff;

    const NodeClock::time_point now{std::chrono::seconds{header.nTime}};

    BlockValidationState state;

    BOOST_CHECK(!ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-diffbits");
}

BOOST_AUTO_TEST_CASE(contextual_time_too_old)
{
    LOCK(::cs_main);

    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};

    CBlockIndex prev;
    prev.nHeight = 100;
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header{chainparams->GenesisBlock()};
    header.nTime = prev.GetMedianTimePast();

    const NodeClock::time_point now{std::chrono::seconds{header.nTime}};

    BlockValidationState state;

    BOOST_CHECK(!ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "time-too-old");
}

BOOST_AUTO_TEST_CASE(contextual_time_timewarp_attack)
{
    LOCK(::cs_main);

    // The timewarp check is only enforced on chains with BIP94.
    CChainParams::RegTestOptions opts{.enforce_bip94 = true};
    const auto chainparams{CChainParams::RegTest(opts)};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};

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

    CBlockHeader header{chainparams->GenesisBlock()};
    header.nVersion = 4;
    header.nTime = prev.nTime - MAX_TIMEWARP - 1;
    header.nBits = prev.nBits;
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

    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};

    CBlockIndex prev;
    prev.nHeight = 100;
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header{chainparams->GenesisBlock()};
    header.nTime = prev.nTime + MAX_FUTURE_BLOCK_TIME + 1;

    // Keep the clock at the previous block's time, so the header is more than
    // MAX_FUTURE_BLOCK_TIME ahead of it.
    const NodeClock::time_point now{std::chrono::seconds{prev.nTime}};

    BlockValidationState state;

    BOOST_CHECK(!ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "time-too-new");
}

BOOST_AUTO_TEST_CASE(contextual_version_1_after_heightincb)
{
    LOCK(::cs_main);

    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};

    CBlockIndex prev;
    // The deployment height is mid difficulty-period; at a retarget boundary
    // GetNextWorkRequired() would walk the (absent) pprev chain.
    prev.nHeight = consensusParams.DeploymentHeight(Consensus::DEPLOYMENT_HEIGHTINCB);
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header{chainparams->GenesisBlock()};
    header.nVersion = 1;
    header.nTime = prev.GetMedianTimePast() + 1;

    const NodeClock::time_point now{std::chrono::seconds{header.nTime}};

    BlockValidationState state;

    BOOST_CHECK(!ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-version(0x00000001)");
}

BOOST_AUTO_TEST_CASE(contextual_version_2_after_dersig)
{
    LOCK(::cs_main);

    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};

    CBlockIndex prev;
    // The deployment height is mid difficulty-period; at a retarget boundary
    // GetNextWorkRequired() would walk the (absent) pprev chain.
    prev.nHeight = consensusParams.DeploymentHeight(Consensus::DEPLOYMENT_DERSIG);
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header{chainparams->GenesisBlock()};
    header.nVersion = 2;
    header.nTime = prev.GetMedianTimePast() + 1;

    const NodeClock::time_point now{std::chrono::seconds{header.nTime}};

    BlockValidationState state;

    BOOST_CHECK(!ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-version(0x00000002)");
}

BOOST_AUTO_TEST_CASE(contextual_version_3_after_cltv)
{
    LOCK(::cs_main);

    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};

    CBlockIndex prev;
    // The deployment height is mid difficulty-period; at a retarget boundary
    // GetNextWorkRequired() would walk the (absent) pprev chain.
    prev.nHeight = consensusParams.DeploymentHeight(Consensus::DEPLOYMENT_CLTV);
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header{chainparams->GenesisBlock()};
    header.nVersion = 3;
    header.nTime = prev.GetMedianTimePast() + 1;

    const NodeClock::time_point now{std::chrono::seconds{header.nTime}};

    BlockValidationState state;

    BOOST_CHECK(!ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsInvalid());
    BOOST_CHECK(state.GetRejectReason() == "bad-version(0x00000003)");
}

BOOST_AUTO_TEST_CASE(contextual_version_2_before_dersig)
{
    LOCK(::cs_main);

    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};

    CBlockIndex prev;
    prev.nHeight = 100;
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header{chainparams->GenesisBlock()};
    header.nVersion = 2;
    header.nTime = prev.GetMedianTimePast() + 1;

    const NodeClock::time_point now{std::chrono::seconds{header.nTime}};

    BlockValidationState state;

    // Version 2 is only outdated once DERSIG requires version 3.
    BOOST_CHECK(ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsValid());
}

BOOST_AUTO_TEST_CASE(contextual_version_3_before_cltv)
{
    LOCK(::cs_main);

    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};

    CBlockIndex prev;
    prev.nHeight = 100;
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header{chainparams->GenesisBlock()};
    header.nVersion = 3;
    header.nTime = prev.GetMedianTimePast() + 1;

    const NodeClock::time_point now{std::chrono::seconds{header.nTime}};

    BlockValidationState state;

    // Version 3 is only outdated once CLTV requires version 4.
    BOOST_CHECK(ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsValid());
}

BOOST_AUTO_TEST_CASE(contextual_version_4_after_cltv)
{
    LOCK(::cs_main);

    const auto chainparams{CChainParams::Main()};
    const Consensus::Params& consensusParams{chainparams->GetConsensus()};

    CBlockIndex prev;
    // The deployment height is mid difficulty-period; at a retarget boundary
    // GetNextWorkRequired() would walk the (absent) pprev chain.
    prev.nHeight = consensusParams.DeploymentHeight(Consensus::DEPLOYMENT_CLTV);
    prev.nTime = 1231006505;
    prev.nBits = 0x1d00ffff;

    CBlockHeader header{chainparams->GenesisBlock()};
    header.nVersion = 4;
    header.nTime = prev.GetMedianTimePast() + 1;

    const NodeClock::time_point now{std::chrono::seconds{header.nTime}};

    BlockValidationState state;

    // Version 4 is accepted with every version deployment active.
    BOOST_CHECK(ContextualCheckBlockHeader(header, state, consensusParams, &prev, now));
    BOOST_CHECK(state.IsValid());
}

BOOST_AUTO_TEST_SUITE_END()
