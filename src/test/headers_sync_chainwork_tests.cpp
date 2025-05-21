// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <headerssync.h>
#include <headerssync-params.h>
#include <net_processing.h>
#include <pow.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <cstddef>
#include <vector>

#include <boost/test/unit_test.hpp>

constexpr size_t TARGET_BLOCKS{REDOWNLOAD_BUFFER_SIZE + MAX_HEADERS_RESULTS + 123};
constexpr arith_uint256 CHAIN_WORK{TARGET_BLOCKS * 2};

/** Search for a nonce to meet (regtest) proof of work */
static void FindProofOfWork(CBlockHeader& starting_header)
{
    while (!CheckProofOfWork(starting_header.GetHash(), starting_header.nBits, Params().GetConsensus())) {
        ++starting_header.nNonce;
    }
}

/**
 * Generate headers in a chain that build off a given starting hash, using
 * the given nVersion, advancing time by 1 second from the starting
 * prev_time, and with a fixed merkle root hash.
 */
static void GenerateHeaders(std::vector<CBlockHeader>& headers,
        size_t count, const uint256& starting_hash, const int nVersion, int prev_time,
        const uint256& merkle_root, const uint32_t nBits)
{
    uint256 prev_hash = starting_hash;

    while (headers.size() < count) {
        headers.emplace_back();
        CBlockHeader& next_header = headers.back();
        next_header.nVersion = nVersion;
        next_header.hashPrevBlock = prev_hash;
        next_header.hashMerkleRoot = merkle_root;
        next_header.nTime = prev_time+1;
        next_header.nBits = nBits;

        FindProofOfWork(next_header);
        prev_hash = next_header.GetHash();
        prev_time = next_header.nTime;
    }
}

BOOST_FIXTURE_TEST_SUITE(headers_sync_chainwork_tests, RegTestingSetup)

// In this test, we construct two sets of headers from genesis, one with
// sufficient proof of work and one without.
// 1. We deliver the first set of headers and verify that the headers sync state
//    updates to the REDOWNLOAD phase successfully.
// 2. Then we deliver the second set of headers and verify that they fail
//    processing (presumably due to commitments not matching).
static void SneakyRedownload(const CBlockIndex* chain_start,
        const std::vector<CBlockHeader>& first_chain,
        const std::vector<CBlockHeader>& second_chain);
// 3. Verify that repeating with the first set of headers in both phases is
//    successful.
static void HappyPath(const CBlockIndex* chain_start,
        const std::vector<CBlockHeader>& first_chain);
// 4. Finally, repeat the second set of headers in both phases to demonstrate
//    behavior when the chain a peer provides has too little work.
static void TooLittleWork(const CBlockIndex* chain_start,
        const std::vector<CBlockHeader>& second_chain);

BOOST_AUTO_TEST_CASE(headers_sync_state)
{
    std::vector<CBlockHeader> first_chain;
    std::vector<CBlockHeader> second_chain;

    const auto genesis{Params().GenesisBlock()};

    // Generate headers for two different chains (using differing merkle roots
    // to ensure the headers are different).
    GenerateHeaders(first_chain, TARGET_BLOCKS - 1, genesis.GetHash(), genesis.nVersion,
                    genesis.nTime, /*merkle_root=*/uint256::ZERO, genesis.nBits);
    GenerateHeaders(second_chain, TARGET_BLOCKS - 2, genesis.GetHash(), genesis.nVersion,
                    genesis.nTime, /*merkle_root=*/uint256::ONE, genesis.nBits);

    const CBlockIndex* chain_start = WITH_LOCK(::cs_main, return m_node.chainman->m_blockman.LookupBlockIndex(genesis.GetHash()));

    SneakyRedownload(chain_start, first_chain, second_chain);
    HappyPath(chain_start, first_chain);
    TooLittleWork(chain_start, second_chain);
}

static void SneakyRedownload(const CBlockIndex* chain_start,
        const std::vector<CBlockHeader>& first_chain,
        const std::vector<CBlockHeader>& second_chain)
{
    // Feed the first chain to HeadersSyncState, by delivering 1 header
    // initially and then the rest.
    HeadersSyncState hss{0, Params().GetConsensus(), chain_start, CHAIN_WORK};
    {
        // Just feed one header and check state.
        auto result{hss.ProcessNextHeaders(std::span{first_chain.begin(), 1}, true)};
        BOOST_REQUIRE_EQUAL(hss.GetState(), HeadersSyncState::State::PRESYNC);
        BOOST_CHECK(result.success);
        BOOST_CHECK(result.request_more);
        BOOST_CHECK_EQUAL(result.pow_validated_headers.size(), 0);
        BOOST_CHECK_EQUAL(hss.NextHeadersRequestLocator().vHave.front(), first_chain.front().GetHash());
    }

    {
        // Pretend the message is still "full", so we don't abort.
        auto result{hss.ProcessNextHeaders(std::span{first_chain.begin() + 1, first_chain.end()}, true)};
        // This chain should look valid, and we should have met the proof-of-work
        // requirement during PRESYNC and transitioned to REDOWNLOAD.
        BOOST_REQUIRE_EQUAL(hss.GetState(), HeadersSyncState::State::REDOWNLOAD);
        BOOST_CHECK(result.success);
        BOOST_CHECK(result.request_more);
        BOOST_CHECK_EQUAL(result.pow_validated_headers.size(), 0);
        // The locator should reset to genesis.
        BOOST_CHECK_EQUAL(hss.NextHeadersRequestLocator().vHave.front(), Params().GenesisBlock().GetHash());
    }

    {
        // Try to sneakily feed back the second chain during REDOWNLOAD.
        auto result{hss.ProcessNextHeaders(second_chain, true)};
        BOOST_REQUIRE_EQUAL(hss.GetState(), HeadersSyncState::State::FINAL);
        BOOST_CHECK(!result.success); // Foiled! We detected mismatching headers.
        BOOST_CHECK(!result.request_more);
        BOOST_CHECK_EQUAL(result.pow_validated_headers.size(), 0);
    }
}

static void HappyPath(const CBlockIndex* chain_start,
        const std::vector<CBlockHeader>& first_chain)
{
    // This time we feed the first chain twice.
    HeadersSyncState hss{0, Params().GetConsensus(), chain_start, CHAIN_WORK};
    const auto genesis_hash{Params().GenesisBlock().GetHash()};
    {
        auto result{hss.ProcessNextHeaders(first_chain, true)};
        // Switched from PRESYNC to REDOWNLOAD after reaching sufficient work:
        BOOST_REQUIRE_EQUAL(hss.GetState(), HeadersSyncState::State::REDOWNLOAD);
        BOOST_CHECK(result.success);
        BOOST_CHECK(result.request_more);
        BOOST_CHECK_EQUAL(result.pow_validated_headers.size(), 0);
        // The locator should reset to genesis.
        BOOST_CHECK_EQUAL(hss.NextHeadersRequestLocator().vHave.front(), genesis_hash);
    }

    {
        // Process only so that the internal threshold isn't met, meaning validated
        // headers shouldn't be returned yet:
        auto result{hss.ProcessNextHeaders(std::span{first_chain.begin(), REDOWNLOAD_BUFFER_SIZE}, true)};
        // Not done:
        BOOST_REQUIRE_EQUAL(hss.GetState(), HeadersSyncState::State::REDOWNLOAD);
        BOOST_CHECK(result.success);
        BOOST_CHECK(result.request_more);
        BOOST_CHECK_EQUAL(result.pow_validated_headers.size(), 0);
        BOOST_CHECK_EQUAL(hss.NextHeadersRequestLocator().vHave.front(), first_chain[REDOWNLOAD_BUFFER_SIZE - 1].GetHash());
    }

    CBlockHeader first_after_genesis;
    {
        // Next header should make us exceed the threshold, but still not be done:
        auto result{hss.ProcessNextHeaders(std::span{first_chain.begin() + REDOWNLOAD_BUFFER_SIZE, 1}, true)};
        BOOST_REQUIRE_EQUAL(hss.GetState(), HeadersSyncState::State::REDOWNLOAD);
        BOOST_CHECK(result.success);
        BOOST_CHECK(result.request_more);
        BOOST_REQUIRE_EQUAL(result.pow_validated_headers.size(), 1);
        BOOST_CHECK_EQUAL(hss.NextHeadersRequestLocator().vHave.front(), first_chain[REDOWNLOAD_BUFFER_SIZE].GetHash());
        first_after_genesis = result.pow_validated_headers.front();
        BOOST_CHECK_EQUAL(first_after_genesis.hashPrevBlock, genesis_hash);
    }

    {
        // Feed in remaining headers, meeting the work threshold again and
        // completing the REDOWNLOAD phase.
        auto result{hss.ProcessNextHeaders(std::span{first_chain.begin() + REDOWNLOAD_BUFFER_SIZE + 1, first_chain.end()}, true)};
        // Nothing left for the sync logic to do:
        BOOST_REQUIRE_EQUAL(hss.GetState(), HeadersSyncState::State::FINAL);
        BOOST_CHECK(result.success);
        BOOST_CHECK(!result.request_more);
        // All headers except the one already returned above:
        BOOST_REQUIRE_EQUAL(result.pow_validated_headers.size(), first_chain.size() - 1);
        BOOST_CHECK_EQUAL(result.pow_validated_headers.front().hashPrevBlock, first_after_genesis.GetHash());
    }
}

static void TooLittleWork(const CBlockIndex* chain_start,
        const std::vector<CBlockHeader>& second_chain)
{
    // Verify that just trying to process the second chain would not succeed
    // (too little work).
    HeadersSyncState hss{0, Params().GetConsensus(), chain_start, CHAIN_WORK};
    BOOST_REQUIRE_EQUAL(hss.GetState(), HeadersSyncState::State::PRESYNC);
    {
        // Pretend just the first message is "full", so we don't abort.
        auto result{hss.ProcessNextHeaders(std::span{second_chain.begin(), 1}, true)};
        BOOST_REQUIRE_EQUAL(hss.GetState(), HeadersSyncState::State::PRESYNC);
        BOOST_CHECK(result.success);
        BOOST_CHECK(result.request_more);
        BOOST_CHECK_EQUAL(result.pow_validated_headers.size(), 0);
    }

    {
        // Tell the sync logic that the headers message was not full, implying no
        // more headers can be requested. For a low-work-chain, this should cause
        // the sync to end with no headers for acceptance.
        auto result{hss.ProcessNextHeaders(std::span{second_chain.begin() + 1, second_chain.end()}, false)};
        BOOST_REQUIRE_EQUAL(hss.GetState(), HeadersSyncState::State::FINAL);
        BOOST_CHECK(!result.request_more);
        BOOST_CHECK_EQUAL(result.pow_validated_headers.size(), 0);
        // Nevertheless, no validation errors should have been detected with the
        // chain:
        BOOST_CHECK(result.success);
    }
}

BOOST_AUTO_TEST_SUITE_END()
