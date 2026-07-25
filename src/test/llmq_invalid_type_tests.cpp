// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <chainparams.h>
#include <llmq/blockprocessor.h>
#include <llmq/context.h>
#include <llmq/params.h>
#include <llmq/quorumsman.h>
#include <llmq/utils.h>
#include <saltedhasher.h>
#include <sync.h>
#include <uint256.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

#include <map>

// An LLMQ type arriving in a P2P QSIGSHARE message is an unvalidated uint8_t. It used to reach
// CQuorumManager::GetQuorum() -> CQuorumBlockProcessor::HasMinedCommitment(), where indexing
// mapHasMinedCommitmentCache with std::map::operator[] inserted a default-constructed
// Uint256LruHashMap. Its default MaxSize is 0, and unordered_lru_cache's constructor asserts
// maxSize != 0, so any of the ~250 unregistered type values aborted the node.
//
// These caches are only ever seeded by InitQuorumsCache() with the LLMQ types in the active
// chain's consensus params, so every lookup keyed by untrusted input must use find() rather than
// operator[]. The message handler now rejects unregistered types up front as well.

BOOST_AUTO_TEST_SUITE(llmq_invalid_type_tests)

// Not a named enumerator, and never present in any chain's consensus params.
static constexpr Consensus::LLMQType UNKNOWN_LLMQ_TYPE{static_cast<Consensus::LLMQType>(0)};
// LLMQ_25_67. A real enumerator, but registered on testnet only, so it is unknown under regtest.
static constexpr Consensus::LLMQType UNREGISTERED_LLMQ_TYPE{Consensus::LLMQType::LLMQ_25_67};

BOOST_FIXTURE_TEST_CASE(init_quorums_cache_does_not_seed_unknown_types, BasicTestingSetup)
{
    std::map<Consensus::LLMQType, Uint256LruHashMap<bool>> cache;
    llmq::utils::InitQuorumsCache(cache, Params().GetConsensus());
    BOOST_REQUIRE(!cache.empty());

    BOOST_REQUIRE(!Params().GetLLMQ(UNKNOWN_LLMQ_TYPE).has_value());
    BOOST_CHECK(cache.find(UNKNOWN_LLMQ_TYPE) == cache.end());

    // Every seeded type has a non-zero capacity; anything else would have to be
    // default-constructed, which is exactly what aborts.
    for (const auto& llmq : Params().GetConsensus().llmqs) {
        auto it = cache.find(llmq.type);
        BOOST_REQUIRE(it != cache.end());
        BOOST_CHECK_GT(it->second.max_size(), 0U);
    }
}

// Crash site. Pre-fix this aborted with `Assertion failed: (_maxSize != 0)`; it must report that
// there is no mined commitment instead.
BOOST_FIXTURE_TEST_CASE(has_mined_commitment_unknown_llmq_type_is_safe, RegTestingSetup)
{
    const auto& qbp = *Assert(Assert(m_node.llmq_ctx)->quorum_block_processor);
    const uint256 tip_hash = WITH_LOCK(::cs_main, return Assert(m_node.chainman->ActiveTip())->GetBlockHash());

    BOOST_REQUIRE(!Params().GetLLMQ(UNKNOWN_LLMQ_TYPE).has_value());
    BOOST_REQUIRE(!Params().GetLLMQ(UNREGISTERED_LLMQ_TYPE).has_value());

    BOOST_CHECK(!qbp.HasMinedCommitment(UNKNOWN_LLMQ_TYPE, tip_hash));
    BOOST_CHECK(!qbp.HasMinedCommitment(UNREGISTERED_LLMQ_TYPE, tip_hash));
}

// The exact call ProcessMessageSigShare() makes with wire-supplied values: LookupBlockIndex()
// succeeds for a real block hash, so HasQuorum() -> HasMinedCommitment() runs with the
// attacker-chosen LLMQ type. Pre-fix this aborted; it must return nullptr instead.
BOOST_FIXTURE_TEST_CASE(get_quorum_unknown_llmq_type_is_safe, RegTestingSetup)
{
    const auto& qman = *Assert(Assert(m_node.llmq_ctx)->qman);
    const uint256 tip_hash = WITH_LOCK(::cs_main, return Assert(m_node.chainman->ActiveTip())->GetBlockHash());

    BOOST_REQUIRE(!Params().GetLLMQ(UNKNOWN_LLMQ_TYPE).has_value());
    BOOST_REQUIRE(!Params().GetLLMQ(UNREGISTERED_LLMQ_TYPE).has_value());

    BOOST_CHECK(qman.GetQuorum(UNKNOWN_LLMQ_TYPE, tip_hash) == nullptr);
    BOOST_CHECK(qman.GetQuorum(UNREGISTERED_LLMQ_TYPE, tip_hash) == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
