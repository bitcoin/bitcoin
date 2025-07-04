// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <headerssync.h>
#include <net_processing.h>
#include <pow.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <cstddef>
#include <vector>

#include <boost/test/unit_test.hpp>

using State = HeadersSyncState::State;

constexpr size_t TARGET_BLOCKS{15'000};
constexpr arith_uint256 CHAIN_WORK{TARGET_BLOCKS * 2};
constexpr size_t REDOWNLOAD_BUFFER_SIZE{TARGET_BLOCKS - (MAX_HEADERS_RESULTS + 123)};
constexpr size_t COMMITMENT_PERIOD{600};
constexpr char BUFFER_SIZE_WARNING[]{"Unexpectedly reached minimum required work before filling the headers sync redownload buffer, even though we started all the way back from genesis"};

// Standard set of checks common to all scenarios. Macro keeps failure lines at the call-site.
#define CHECK_RESULT(result_expression, hss, exp_state, exp_success, exp_request_more,                   \
                     exp_headers_size, exp_pow_validated_prev, exp_locator_hash)                         \
    do {                                                                                                 \
        const auto result = result_expression;                                                           \
        BOOST_REQUIRE_EQUAL(hss.GetState(), exp_state);                                                  \
        BOOST_CHECK_EQUAL(result.success, exp_success);                                                  \
        BOOST_CHECK_EQUAL(result.request_more, exp_request_more);                                        \
        BOOST_CHECK_EQUAL(result.pow_validated_headers.size(), exp_headers_size);                        \
        std::optional<uint256> locator_hash_opt{exp_locator_hash};                                       \
        if (locator_hash_opt == std::nullopt) {                                                          \
            BOOST_CHECK_EQUAL(exp_state, State::FINAL);                                                  \
        } else {                                                                                         \
            BOOST_CHECK_EQUAL(hss.NextHeadersRequestLocator().vHave.at(0), locator_hash_opt);            \
        }                                                                                                \
        std::optional<uint256> pow_validated_prev_opt{exp_pow_validated_prev};                           \
        if (pow_validated_prev_opt == std::nullopt) {                                                    \
            BOOST_CHECK_EQUAL(exp_headers_size, 0);                                                      \
        } else {                                                                                         \
            BOOST_CHECK_EQUAL(result.pow_validated_headers.at(0).hashPrevBlock, pow_validated_prev_opt); \
        }                                                                                                \
    } while (false)

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

static HeadersSyncState CreateState(const CBlockIndex* chain_start,
        const HeadersSyncParams& params = {
            .commitment_period = COMMITMENT_PERIOD,
            .redownload_buffer_size = REDOWNLOAD_BUFFER_SIZE,
        })
{
    return {/*id=*/0,
            Params().GetConsensus(),
            params,
            chain_start,
            /*minimum_required_work=*/CHAIN_WORK};
}

BOOST_FIXTURE_TEST_SUITE(headers_sync_chainwork_tests, RegTestingSetup)

// In this test, we construct two sets of headers from genesis, one with
// sufficient proof of work and one without.
// 1. We deliver the first set of headers and verify that the headers sync state
//    updates to the REDOWNLOAD phase successfully.
//    Then we deliver the second set of headers and verify that they fail
//    processing (presumably due to commitments not matching).
static void SneakyRedownload(const CBlockIndex*, const std::vector<CBlockHeader>&, const std::vector<CBlockHeader>&);
// 2. Verify that repeating with the first set of headers in both phases is
//    successful.
static void HappyPath(const CBlockIndex*, const std::vector<CBlockHeader>&);
// 3. Repeat the second set of headers in both phases to demonstrate behavior
//    when the chain a peer provides has too little work.
static void TooLittleWork(const CBlockIndex*, const std::vector<CBlockHeader>&);
// 4. Sets the redownload buffer size to be large enough that we reach the PoW
//    threshold before returning any headers, resulting in a warning.
static void TooBigBuffer(const CBlockIndex*, const std::vector<CBlockHeader>&);

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
    TooBigBuffer(chain_start, first_chain);
}

static void SneakyRedownload(const CBlockIndex* chain_start,
        const std::vector<CBlockHeader>& first_chain,
        const std::vector<CBlockHeader>& second_chain)
{
    // Feed the first chain to HeadersSyncState, by delivering 1 header
    // initially and then the rest.
    HeadersSyncState hss{CreateState(chain_start)};

    // Just feed one header and check state.
    CHECK_RESULT(hss.ProcessNextHeaders({{first_chain.front()}}, /*full_headers_message=*/true),
        hss, /*exp_state=*/State::PRESYNC,
        /*exp_success*/true, /*exp_request_more=*/true,
        /*exp_headers_size=*/0, /*exp_pow_validated_prev=*/std::nullopt,
        /*exp_locator_hash=*/first_chain.front().GetHash());

    // Pretend the message is still "full", so we don't abort.
    // This chain should look valid, and we should have met the proof-of-work
    // requirement during PRESYNC and transitioned to REDOWNLOAD.
    CHECK_RESULT(hss.ProcessNextHeaders(std::span{first_chain}.last(first_chain.size() - 1), true),
        hss, /*exp_state=*/State::REDOWNLOAD,
        /*exp_success*/true, /*exp_request_more=*/true,
        /*exp_headers_size=*/0, /*exp_pow_validated_prev=*/std::nullopt,
        /*exp_locator_hash=*/Params().GenesisBlock().GetHash());

    // Try to sneakily feed back the second chain during REDOWNLOAD.
    CHECK_RESULT(hss.ProcessNextHeaders(second_chain, true),
        hss, /*exp_state=*/State::FINAL,
        /*exp_success*/false, // Foiled! We detected mismatching headers.
        /*exp_request_more=*/false,
        /*exp_headers_size=*/0, /*exp_pow_validated_prev=*/std::nullopt,
        /*exp_locator_hash=*/std::nullopt);
}

static void HappyPath(const CBlockIndex* chain_start,
        const std::vector<CBlockHeader>& first_chain)
{
    // During normal operation we shouldn't get the redownload buffer size warning.
    ASSERT_NO_DEBUG_LOG(BUFFER_SIZE_WARNING);

    // This time we feed the first chain twice.
    HeadersSyncState hss{CreateState(chain_start)};

    // Sufficient work transitions us from PRESYNC to REDOWNLOAD:
    const auto genesis_hash{Params().GenesisBlock().GetHash()};
    CHECK_RESULT(hss.ProcessNextHeaders(first_chain, /*full_headers_message=*/true),
        hss, /*exp_state=*/State::REDOWNLOAD,
        /*exp_success*/true, /*exp_request_more=*/true,
        /*exp_headers_size=*/0, /*exp_pow_validated_prev=*/std::nullopt,
        /*exp_locator_hash=*/genesis_hash);

    // Process only so that the internal threshold isn't met, meaning validated
    // headers shouldn't be returned yet:
    CHECK_RESULT(hss.ProcessNextHeaders({first_chain.begin(), REDOWNLOAD_BUFFER_SIZE}, /*full_headers_message=*/true),
        // Not done:
        hss, /*exp_state=*/State::REDOWNLOAD,
        /*exp_success*/true, /*exp_request_more=*/true,
        /*exp_headers_size=*/0, /*exp_pow_validated_prev=*/std::nullopt,
        /*exp_locator_hash=*/first_chain[REDOWNLOAD_BUFFER_SIZE - 1].GetHash());

    CHECK_RESULT(hss.ProcessNextHeaders({first_chain.begin() + REDOWNLOAD_BUFFER_SIZE, 1}, true),
        hss, /*exp_state=*/State::REDOWNLOAD,
        /*exp_success*/true, /*exp_request_more=*/true,
        /*exp_headers_size=*/1, /*exp_pow_validated_prev=*/genesis_hash,
        /*exp_locator_hash=*/first_chain[REDOWNLOAD_BUFFER_SIZE].GetHash());

    // Feed in remaining headers, meeting the work threshold again and
    // completing the REDOWNLOAD phase.
    CHECK_RESULT(hss.ProcessNextHeaders({first_chain.begin() + REDOWNLOAD_BUFFER_SIZE + 1, first_chain.end()}, true),
        hss, /*exp_state=*/State::FINAL,
        /*exp_success*/true, /*exp_request_more=*/false,
        // All headers except the one already returned above:
        /*exp_headers_size=*/first_chain.size() - 1, /*exp_pow_validated_prev=*/first_chain.front().GetHash(),
        /*exp_locator_hash=*/std::nullopt);
}

static void TooLittleWork(const CBlockIndex* chain_start,
        const std::vector<CBlockHeader>& second_chain)
{
    // Verify that just trying to process the second chain would not succeed
    // (too little work).
    HeadersSyncState hss{CreateState(chain_start)};
    BOOST_REQUIRE_EQUAL(hss.GetState(), State::PRESYNC);

    // Pretend just the first message is "full", so we don't abort.
    CHECK_RESULT(hss.ProcessNextHeaders({{second_chain.front()}}, true),
        hss, /*exp_state=*/State::PRESYNC,
        /*exp_success*/true, /*exp_request_more=*/true,
        /*exp_headers_size=*/0, /*exp_pow_validated_prev=*/std::nullopt,
        /*exp_locator_hash=*/second_chain.front().GetHash());

    // Tell the sync logic that the headers message was not full, implying no
    // more headers can be requested. For a low-work-chain, this should cause
    // the sync to end with no headers for acceptance.
    CHECK_RESULT(hss.ProcessNextHeaders(std::span{second_chain}.last(second_chain.size() - 1), false),
        hss, /*exp_state=*/State::FINAL,
        // Nevertheless, no validation errors should have been detected with the
        // chain:
        /*exp_success*/true,
        /*exp_request_more=*/false,
        /*exp_headers_size=*/0, /*exp_pow_validated_prev=*/std::nullopt,
        /*exp_locator_hash=*/std::nullopt);
}

static void TooBigBuffer(const CBlockIndex* chain_start,
        const std::vector<CBlockHeader>& first_chain)
{
    // Intentionally too big redownload buffer in order to trigger warning.
    HeadersSyncState hss{CreateState(chain_start,
                                     HeadersSyncParams{
                                         .commitment_period = COMMITMENT_PERIOD,
                                         .redownload_buffer_size = first_chain.size(),
                                     })};

    const auto genesis_hash{Params().GenesisBlock().GetHash()};
    CHECK_RESULT(hss.ProcessNextHeaders(first_chain, true),
        hss, /*exp_state=*/State::REDOWNLOAD,
        /*exp_success*/true, /*exp_request_more=*/true,
        /*exp_headers_size=*/0, /*exp_pow_validated_prev=*/std::nullopt,
        /*exp_locator_hash=*/genesis_hash);

    ASSERT_DEBUG_LOG(BUFFER_SIZE_WARNING);
    CHECK_RESULT(hss.ProcessNextHeaders(first_chain, true),
        hss, /*exp_state=*/State::FINAL,
        /*exp_success*/true, /*exp_request_more=*/false,
        /*exp_headers_size=*/first_chain.size(), /*exp_pow_validated_prev=*/genesis_hash,
        /*exp_locator_hash=*/std::nullopt);
}

BOOST_AUTO_TEST_SUITE_END()
