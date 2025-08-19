// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <headerssync.h>
#include <pow.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <cstddef>
#include <vector>

#include <boost/test/unit_test.hpp>

constexpr size_t TARGET_BLOCKS{15'000};
constexpr arith_uint256 CHAIN_WORK{TARGET_BLOCKS * 2};

struct HeadersGeneratorSetup : public RegTestingSetup {
    const CBlock& genesis{Params().GenesisBlock()};
    const CBlockIndex* chain_start{WITH_LOCK(::cs_main, return m_node.chainman->m_blockman.LookupBlockIndex(genesis.GetHash()))};

    // Generate headers for two different chains (using differing merkle roots
    // to ensure the headers are different).
    const std::vector<CBlockHeader>& FirstChain()
    {
        static const auto first_chain{GenerateHeaders(/*count=*/TARGET_BLOCKS - 1, genesis.GetHash(),
                genesis.nVersion, genesis.nTime, /*merkle_root=*/uint256::ZERO, genesis.nBits)};
        return first_chain;
    }
    const std::vector<CBlockHeader>& SecondChain()
    {
        static const auto second_chain{GenerateHeaders(/*count=*/TARGET_BLOCKS - 2, genesis.GetHash(),
                genesis.nVersion, genesis.nTime, /*merkle_root=*/uint256::ONE, genesis.nBits)};
        return second_chain;
    }

private:
    /** Search for a nonce to meet (regtest) proof of work */
    void FindProofOfWork(CBlockHeader& starting_header);
    /**
     * Generate headers in a chain that build off a given starting hash, using
     * the given nVersion, advancing time by 1 second from the starting
     * prev_time, and with a fixed merkle root hash.
     */
    std::vector<CBlockHeader> GenerateHeaders(size_t count,
            uint256 prev_hash, int32_t nVersion, uint32_t prev_time,
            const uint256& merkle_root, uint32_t nBits);
};

void HeadersGeneratorSetup::FindProofOfWork(CBlockHeader& starting_header)
{
    while (!CheckProofOfWork(starting_header.GetHash(), starting_header.nBits, Params().GetConsensus())) {
        ++starting_header.nNonce;
    }
}

std::vector<CBlockHeader> HeadersGeneratorSetup::GenerateHeaders(
        const size_t count, uint256 prev_hash, const int32_t nVersion,
        uint32_t prev_time, const uint256& merkle_root, const uint32_t nBits)
{
    std::vector<CBlockHeader> headers(count);
    for (auto& next_header : headers) {
        next_header.nVersion = nVersion;
        next_header.hashPrevBlock = prev_hash;
        next_header.hashMerkleRoot = merkle_root;
        next_header.nTime = ++prev_time;
        next_header.nBits = nBits;

        FindProofOfWork(next_header);
        prev_hash = next_header.GetHash();
    }
    return headers;
}

// In this test, we construct two sets of headers from genesis, one with
// sufficient proof of work and one without.
// 1. We deliver the first set of headers and verify that the headers sync state
//    updates to the REDOWNLOAD phase successfully.
// 2. Then we deliver the second set of headers and verify that they fail
//    processing (presumably due to commitments not matching).
// 3. Finally, we verify that repeating with the first set of headers in both
//    phases is successful.
BOOST_FIXTURE_TEST_SUITE(headers_sync_chainwork_tests, HeadersGeneratorSetup)

BOOST_AUTO_TEST_CASE(headers_sync_state)
{
    const auto& first_chain{FirstChain()};
    const auto& second_chain{SecondChain()};

    std::unique_ptr<HeadersSyncState> hss;

    std::vector<CBlockHeader> headers_batch;

    // Feed the first chain to HeadersSyncState, by delivering 1 header
    // initially and then the rest.
    headers_batch.insert(headers_batch.end(), std::next(first_chain.begin()), first_chain.end());

    hss.reset(new HeadersSyncState(0, Params().GetConsensus(), chain_start, CHAIN_WORK));
    (void)hss->ProcessNextHeaders({first_chain.front()}, true);
    // Pretend the first header is still "full", so we don't abort.
    auto result = hss->ProcessNextHeaders(headers_batch, true);

    // This chain should look valid, and we should have met the proof-of-work
    // requirement.
    BOOST_CHECK(result.success);
    BOOST_CHECK(result.request_more);
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::REDOWNLOAD);

    // Try to sneakily feed back the second chain.
    result = hss->ProcessNextHeaders(second_chain, true);
    BOOST_CHECK(!result.success); // foiled!
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::FINAL);

    // Now try again, this time feeding the first chain twice.
    hss.reset(new HeadersSyncState(0, Params().GetConsensus(), chain_start, CHAIN_WORK));
    (void)hss->ProcessNextHeaders(first_chain, true);
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::REDOWNLOAD);

    result = hss->ProcessNextHeaders(first_chain, true);
    BOOST_CHECK(result.success);
    BOOST_CHECK(!result.request_more);
    // All headers should be ready for acceptance:
    BOOST_CHECK(result.pow_validated_headers.size() == first_chain.size());
    // Nothing left for the sync logic to do:
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::FINAL);

    // Finally, verify that just trying to process the second chain would not
    // succeed (too little work)
    hss.reset(new HeadersSyncState(0, Params().GetConsensus(), chain_start, CHAIN_WORK));
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::PRESYNC);
     // Pretend just the first message is "full", so we don't abort.
    (void)hss->ProcessNextHeaders({second_chain.front()}, true);
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::PRESYNC);

    headers_batch.clear();
    headers_batch.insert(headers_batch.end(), std::next(second_chain.begin(), 1), second_chain.end());
    // Tell the sync logic that the headers message was not full, implying no
    // more headers can be requested. For a low-work-chain, this should causes
    // the sync to end with no headers for acceptance.
    result = hss->ProcessNextHeaders(headers_batch, false);
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::FINAL);
    BOOST_CHECK(result.pow_validated_headers.empty());
    BOOST_CHECK(!result.request_more);
    // Nevertheless, no validation errors should have been detected with the
    // chain:
    BOOST_CHECK(result.success);
}

BOOST_AUTO_TEST_SUITE_END()
