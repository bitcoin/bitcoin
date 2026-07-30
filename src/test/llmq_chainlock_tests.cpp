// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/llmq_tests.h>
#include <test/util/setup_common.h>

#include <streams.h>
#include <util/strencodings.h>

#include <chainlock/chainlock.h>
#include <chainlock/clsig.h>
#include <chainlock/handler.h>
#include <llmq/context.h>
#include <msg_result.h>
#include <protocol.h>

#include <boost/test/unit_test.hpp>

using chainlock::ChainLockSig;
using namespace llmq;
using namespace llmq::testutils;

BOOST_AUTO_TEST_SUITE(llmq_chainlock_tests)

BOOST_AUTO_TEST_CASE(chainlock_construction_test)
{
    // Test default constructor
    ChainLockSig clsig1;
    BOOST_CHECK(clsig1.IsNull());
    BOOST_CHECK_EQUAL(clsig1.getHeight(), -1);
    BOOST_CHECK(clsig1.getBlockHash().IsNull());
    BOOST_CHECK(!clsig1.getSig().IsValid());

    // Test parameterized constructor
    int32_t height = 12345;
    uint256 blockHash = GetTestBlockHash(1);
    CBLSSignature sig = CreateRandomBLSSignature();

    ChainLockSig clsig2(height, blockHash, sig);
    BOOST_CHECK(!clsig2.IsNull());
    BOOST_CHECK_EQUAL(clsig2.getHeight(), height);
    BOOST_CHECK(clsig2.getBlockHash() == blockHash);
    BOOST_CHECK(clsig2.getSig() == sig);
}

BOOST_AUTO_TEST_CASE(chainlock_null_test)
{
    ChainLockSig clsig;

    // Default constructed should be null
    BOOST_CHECK(clsig.IsNull());

    // With height set but null hash, should not be null
    clsig = ChainLockSig(100, uint256(), CBLSSignature());
    BOOST_CHECK(!clsig.IsNull());

    // With valid data should not be null
    clsig = CreateChainLock(100, GetTestBlockHash(1));
    BOOST_CHECK(!clsig.IsNull());
}

BOOST_AUTO_TEST_CASE(chainlock_serialization_test)
{
    // Test with valid chainlock
    ChainLockSig clsig = CreateChainLock(67890, GetTestBlockHash(42));

    // Test serialization preserves all fields
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << clsig;

    ChainLockSig deserialized;
    ss >> deserialized;

    BOOST_CHECK_EQUAL(clsig.getHeight(), deserialized.getHeight());
    BOOST_CHECK(clsig.getBlockHash() == deserialized.getBlockHash());
    BOOST_CHECK(clsig.getSig() == deserialized.getSig());
    BOOST_CHECK_EQUAL(clsig.IsNull(), deserialized.IsNull());
}

BOOST_AUTO_TEST_CASE(chainlock_tostring_test)
{
    // Test null chainlock
    ChainLockSig nullClsig;
    std::string nullStr = nullClsig.ToString();
    BOOST_CHECK(!nullStr.empty());

    // Test valid chainlock
    int32_t height = 123456;
    uint256 blockHash = GetTestBlockHash(789);
    ChainLockSig clsig = CreateChainLock(height, blockHash);

    std::string str = clsig.ToString();
    BOOST_CHECK(!str.empty());

    // ToString should contain height and hash info
    BOOST_CHECK(str.find(strprintf("%d", height)) != std::string::npos);
    BOOST_CHECK(str.find(blockHash.ToString().substr(0, 10)) != std::string::npos);
}

BOOST_AUTO_TEST_CASE(chainlock_edge_cases_test)
{
    // Test with edge case heights
    ChainLockSig clsig1 = CreateChainLock(0, GetTestBlockHash(1));
    BOOST_CHECK_EQUAL(clsig1.getHeight(), 0);
    BOOST_CHECK(!clsig1.IsNull());

    ChainLockSig clsig2 = CreateChainLock(std::numeric_limits<int32_t>::max(), GetTestBlockHash(2));
    BOOST_CHECK_EQUAL(clsig2.getHeight(), std::numeric_limits<int32_t>::max());

    // Test serialization with extreme values
    CDataStream ss1(SER_NETWORK, PROTOCOL_VERSION);
    ss1 << clsig1;
    ChainLockSig clsig1_deserialized;
    ss1 >> clsig1_deserialized;
    BOOST_CHECK_EQUAL(clsig1.getHeight(), clsig1_deserialized.getHeight());

    CDataStream ss2(SER_NETWORK, PROTOCOL_VERSION);
    ss2 << clsig2;
    ChainLockSig clsig2_deserialized;
    ss2 >> clsig2_deserialized;
    BOOST_CHECK_EQUAL(clsig2.getHeight(), clsig2_deserialized.getHeight());
}

BOOST_AUTO_TEST_CASE(chainlock_comparison_test)
{
    // Create identical chainlocks
    int32_t height = 5000;
    uint256 blockHash = GetTestBlockHash(10);
    CBLSSignature sig = CreateRandomBLSSignature();

    ChainLockSig clsig1(height, blockHash, sig);
    ChainLockSig clsig2(height, blockHash, sig);

    // Verify getters return same values
    BOOST_CHECK_EQUAL(clsig1.getHeight(), clsig2.getHeight());
    BOOST_CHECK(clsig1.getBlockHash() == clsig2.getBlockHash());
    BOOST_CHECK(clsig1.getSig() == clsig2.getSig());

    // Different chainlocks
    ChainLockSig clsig3(height + 1, blockHash, sig);
    BOOST_CHECK(clsig1.getHeight() != clsig3.getHeight());

    ChainLockSig clsig4(height, GetTestBlockHash(11), sig);
    BOOST_CHECK(clsig1.getBlockHash() != clsig4.getBlockHash());
}

BOOST_AUTO_TEST_CASE(chainlock_malformed_data_test)
{
    // Test deserialization of truncated data
    ChainLockSig clsig = CreateChainLock(1000, GetTestBlockHash(5));

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << clsig;

    // Truncate the stream
    std::string data = ss.str();
    for (size_t truncateAt = 1; truncateAt < data.size(); truncateAt += 10) {
        CDataStream truncated(std::vector<unsigned char>(data.begin(), data.begin() + truncateAt), SER_NETWORK,
                              PROTOCOL_VERSION);

        ChainLockSig deserialized;
        try {
            truncated >> deserialized;
            // If no exception, verify it's either complete or default
            if (truncateAt < sizeof(int32_t)) {
                BOOST_CHECK(deserialized.IsNull());
            }
        } catch (const std::exception&) {
            // Expected for most truncation points
        }
    }
}

BOOST_FIXTURE_TEST_CASE(stale_chainlocks_are_remembered_for_duplicate_suppression, TestingSetup)
{
    m_node.clhandler->CheckActiveState();

    auto best_clsig = CreateChainLock(100, GetTestBlockHash(1));
    BOOST_REQUIRE(m_node.chainlocks->UpdateBestChainlock(::SerializeHash(best_clsig), best_clsig, /*pindex=*/nullptr));

    BOOST_CHECK_EQUAL(m_node.clhandler->SeenChainLockCacheSizeForTesting(), 0U);

    for (uint32_t i = 0; i < 10; ++i) {
        auto stale_clsig = CreateChainLock(100, GetTestBlockHash(1000 + i));
        const auto hash = ::SerializeHash(stale_clsig);

        [[maybe_unused]] const auto result =
            m_node.clhandler->ProcessNewChainLock(/*from=*/0, stale_clsig, *m_node.llmq_ctx->qman, hash);

        BOOST_CHECK(m_node.clhandler->AlreadyHave(CInv{MSG_CLSIG, hash}));
        BOOST_CHECK_EQUAL(m_node.clhandler->SeenChainLockCacheSizeForTesting(), static_cast<size_t>(i) + 1U);
    }
}

BOOST_FIXTURE_TEST_CASE(seen_chainlock_cache_is_bounded, TestingSetup)
{
    m_node.clhandler->CheckActiveState();

    const size_t max_size = m_node.clhandler->SeenChainLockCacheMaxSizeForTesting();
    BOOST_REQUIRE_GT(max_size, 0U);

    for (size_t i = 0; i < max_size + 1; ++i) {
        auto clsig = CreateChainLock(static_cast<int32_t>(i), GetTestBlockHash(static_cast<uint32_t>(2000 + i)));
        [[maybe_unused]] const auto result =
            m_node.clhandler->ProcessNewChainLock(/*from=*/-1, clsig, *m_node.llmq_ctx->qman, ::SerializeHash(clsig));
        BOOST_CHECK_LE(m_node.clhandler->SeenChainLockCacheSizeForTesting(), max_size);
        if (i == 0) {
            BOOST_CHECK_GT(m_node.clhandler->SeenChainLockCacheSizeForTesting(), 0U);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(best_chainlock_is_already_have_after_seen_cache_eviction, TestingSetup)
{
    m_node.clhandler->CheckActiveState();

    auto best_clsig = CreateChainLock(100, GetTestBlockHash(1));
    const auto best_hash = ::SerializeHash(best_clsig);
    BOOST_REQUIRE(m_node.chainlocks->UpdateBestChainlock(best_hash, best_clsig, /*pindex=*/nullptr));
    BOOST_CHECK(m_node.clhandler->AlreadyHave(CInv{MSG_CLSIG, best_hash}));

    const size_t max_size = m_node.clhandler->SeenChainLockCacheMaxSizeForTesting();
    BOOST_REQUIRE_GT(max_size, 0U);

    for (size_t i = 0; i < max_size + 1; ++i) {
        auto clsig = CreateChainLock(static_cast<int32_t>(101 + i), GetTestBlockHash(static_cast<uint32_t>(3000 + i)));
        [[maybe_unused]] const auto result =
            m_node.clhandler->ProcessNewChainLock(/*from=*/-1, clsig, *m_node.llmq_ctx->qman, ::SerializeHash(clsig));
        BOOST_CHECK_LE(m_node.clhandler->SeenChainLockCacheSizeForTesting(), max_size);
    }

    BOOST_CHECK(m_node.clhandler->AlreadyHave(CInv{MSG_CLSIG, best_hash}));
}

BOOST_AUTO_TEST_SUITE_END()
