// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <functional>
#include <optional>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(net_peer_eviction_tests, BasicTestingSetup)

std::vector<NodeEvictionCandidate> GetRandomNodeEvictionCandidates(const int n_candidates, FastRandomContext& random_context)
{
    std::vector<NodeEvictionCandidate> candidates;
    for (int id = 0; id < n_candidates; ++id) {
        candidates.push_back({
            /* id */ id,
            /* nTimeConnected */ static_cast<int64_t>(random_context.randrange(100)),
            /* m_min_ping_time */ std::chrono::microseconds{random_context.randrange(100)},
            /* nLastBlockTime */ static_cast<int64_t>(random_context.randrange(100)),
            /* nLastTXTime */ static_cast<int64_t>(random_context.randrange(100)),
            /* fRelevantServices */ random_context.randbool(),
            /* fRelayTxes */ random_context.randbool(),
            /* fBloomFilter */ random_context.randbool(),
            /* nKeyedNetGroup */ random_context.randrange(100),
            /* prefer_evict */ random_context.randbool(),
            /* m_is_local */ random_context.randbool(),
        });
    }
    return candidates;
}

// Returns true if any of the node ids in node_ids are selected for eviction.
bool IsEvicted(std::vector<NodeEvictionCandidate> candidates, const std::vector<NodeId>& node_ids, FastRandomContext& random_context)
{
    Shuffle(candidates.begin(), candidates.end(), random_context);
    const std::optional<NodeId> evicted_node_id = SelectNodeToEvict(std::move(candidates));
    if (!evicted_node_id) {
        return false;
    }
    return std::find(node_ids.begin(), node_ids.end(), *evicted_node_id) != node_ids.end();
}

// Create number_of_nodes random nodes, apply setup function candidate_setup_fn,
// apply eviction logic and then return true if any of the node ids in node_ids
// are selected for eviction.
bool IsEvicted(const int number_of_nodes, std::function<void(NodeEvictionCandidate&)> candidate_setup_fn, const std::vector<NodeId>& node_ids, FastRandomContext& random_context)
{
    std::vector<NodeEvictionCandidate> candidates = GetRandomNodeEvictionCandidates(number_of_nodes, random_context);
    for (NodeEvictionCandidate& candidate : candidates) {
        candidate_setup_fn(candidate);
    }
    return IsEvicted(candidates, node_ids, random_context);
}

namespace {
constexpr int NODE_EVICTION_TEST_ROUNDS{10};
constexpr int NODE_EVICTION_TEST_UP_TO_N_NODES{200};
} // namespace

BOOST_AUTO_TEST_CASE(peer_eviction_test)
{
    FastRandomContext random_context{true};

    for (int i = 0; i < NODE_EVICTION_TEST_ROUNDS; ++i) {
        for (int number_of_nodes = 0; number_of_nodes < NODE_EVICTION_TEST_UP_TO_N_NODES; ++number_of_nodes) {
            // Four nodes with the highest keyed netgroup values should be
            // protected from eviction.
            BOOST_CHECK(!IsEvicted(
                number_of_nodes, [number_of_nodes](NodeEvictionCandidate& candidate) {
                    candidate.nKeyedNetGroup = number_of_nodes - candidate.id;
                },
                {0, 1, 2, 3}, random_context));

            // Eight nodes with the lowest minimum ping time should be protected
            // from eviction.
            BOOST_CHECK(!IsEvicted(
                number_of_nodes, [](NodeEvictionCandidate& candidate) {
                    candidate.m_min_ping_time = std::chrono::microseconds{candidate.id};
                },
                {0, 1, 2, 3, 4, 5, 6, 7}, random_context));

            // Four nodes that most recently sent us novel transactions accepted
            // into our mempool should be protected from eviction.
            BOOST_CHECK(!IsEvicted(
                number_of_nodes, [number_of_nodes](NodeEvictionCandidate& candidate) {
                    candidate.nLastTXTime = number_of_nodes - candidate.id;
                },
                {0, 1, 2, 3}, random_context));

            // Up to eight non-tx-relay peers that most recently sent us novel
            // blocks should be protected from eviction.
            BOOST_CHECK(!IsEvicted(
                number_of_nodes, [number_of_nodes](NodeEvictionCandidate& candidate) {
                    candidate.nLastBlockTime = number_of_nodes - candidate.id;
                    if (candidate.id <= 7) {
                        candidate.fRelayTxes = false;
                        candidate.fRelevantServices = true;
                    }
                },
                {0, 1, 2, 3, 4, 5, 6, 7}, random_context));

            // Four peers that most recently sent us novel blocks should be
            // protected from eviction.
            BOOST_CHECK(!IsEvicted(
                number_of_nodes, [number_of_nodes](NodeEvictionCandidate& candidate) {
                    candidate.nLastBlockTime = number_of_nodes - candidate.id;
                },
                {0, 1, 2, 3}, random_context));

            // Combination of the previous two tests.
            BOOST_CHECK(!IsEvicted(
                number_of_nodes, [number_of_nodes](NodeEvictionCandidate& candidate) {
                    candidate.nLastBlockTime = number_of_nodes - candidate.id;
                    if (candidate.id <= 7) {
                        candidate.fRelayTxes = false;
                        candidate.fRelevantServices = true;
                    }
                },
                {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, random_context));

            // Combination of all tests above.
            BOOST_CHECK(!IsEvicted(
                number_of_nodes, [number_of_nodes](NodeEvictionCandidate& candidate) {
                    candidate.nKeyedNetGroup = number_of_nodes - candidate.id;           // 4 protected
                    candidate.m_min_ping_time = std::chrono::microseconds{candidate.id}; // 8 protected
                    candidate.nLastTXTime = number_of_nodes - candidate.id;              // 4 protected
                    candidate.nLastBlockTime = number_of_nodes - candidate.id;           // 4 protected
                },
                {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19}, random_context));

            // An eviction is expected given >= 29 random eviction candidates. The eviction logic protects at most
            // four peers by net group, eight by lowest ping time, four by last time of novel tx, up to eight non-tx-relay
            // peers by last novel block time, and four more peers by last novel block time.
            if (number_of_nodes >= 29) {
                BOOST_CHECK(SelectNodeToEvict(GetRandomNodeEvictionCandidates(number_of_nodes, random_context)));
            }

            // No eviction is expected given <= 20 random eviction candidates. The eviction logic protects at least
            // four peers by net group, eight by lowest ping time, four by last time of novel tx and four peers by last
            // novel block time.
            if (number_of_nodes <= 20) {
                BOOST_CHECK(!SelectNodeToEvict(GetRandomNodeEvictionCandidates(number_of_nodes, random_context)));
            }

            // Cases left to test:
            // * "Protect the half of the remaining nodes which have been connected the longest. [...]"
            // * "Pick out up to 1/4 peers that are localhost, sorted by longest uptime. [...]"
            // * "If any remaining peers are preferred for eviction consider only them. [...]"
            // * "Identify the network group with the most connections and youngest member. [...]"
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
