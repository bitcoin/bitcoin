// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <functional>
#include <optional>
#include <unordered_set>
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
            /* m_is_onion */ random_context.randbool(),
        });
    }
    return candidates;
}

// Create `num_peers` random nodes, apply setup function `candidate_setup_fn`,
// call ProtectEvictionCandidatesByRatio() to apply protection logic, and then
// return true if all of `protected_peer_ids` and none of `unprotected_peer_ids`
// are protected from eviction, i.e. removed from the eviction candidates.
bool IsProtected(int num_peers,
                 std::function<void(NodeEvictionCandidate&)> candidate_setup_fn,
                 const std::unordered_set<NodeId>& protected_peer_ids,
                 const std::unordered_set<NodeId>& unprotected_peer_ids,
                 FastRandomContext& random_context)
{
    std::vector<NodeEvictionCandidate> candidates{GetRandomNodeEvictionCandidates(num_peers, random_context)};
    for (NodeEvictionCandidate& candidate : candidates) {
        candidate_setup_fn(candidate);
    }
    Shuffle(candidates.begin(), candidates.end(), random_context);

    const size_t size{candidates.size()};
    const size_t expected{size - size / 2}; // Expect half the candidates will be protected.
    ProtectEvictionCandidatesByRatio(candidates);
    BOOST_CHECK_EQUAL(candidates.size(), expected);

    size_t unprotected_count{0};
    for (const NodeEvictionCandidate& candidate : candidates) {
        if (protected_peer_ids.count(candidate.id)) {
            // this peer should have been removed from the eviction candidates
            BOOST_TEST_MESSAGE(strprintf("expected candidate to be protected: %d", candidate.id));
            return false;
        }
        if (unprotected_peer_ids.count(candidate.id)) {
            // this peer remains in the eviction candidates, as expected
            ++unprotected_count;
        }
    }

    const bool is_protected{unprotected_count == unprotected_peer_ids.size()};
    if (!is_protected) {
        BOOST_TEST_MESSAGE(strprintf("unprotected: expected %d, actual %d",
                                     unprotected_peer_ids.size(), unprotected_count));
    }
    return is_protected;
}

BOOST_AUTO_TEST_CASE(peer_protection_test)
{
    FastRandomContext random_context{true};
    int num_peers{12};

    // Expect half of the peers with greatest uptime (the lowest nTimeConnected)
    // to be protected from eviction.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_onion = c.m_is_local = false;
        },
        /* protected_peer_ids */ {0, 1, 2, 3, 4, 5},
        /* unprotected_peer_ids */ {6, 7, 8, 9, 10, 11},
        random_context));

    // Verify in the opposite direction.
    BOOST_CHECK(IsProtected(
        num_peers, [num_peers](NodeEvictionCandidate& c) {
            c.nTimeConnected = num_peers - c.id;
            c.m_is_onion = c.m_is_local = false;
        },
        /* protected_peer_ids */ {6, 7, 8, 9, 10, 11},
        /* unprotected_peer_ids */ {0, 1, 2, 3, 4, 5},
        random_context));

    // Test protection of onion and localhost peers...

    // Expect 1/4 onion peers to be protected from eviction,
    // independently of other characteristics.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.m_is_onion = (c.id == 3 || c.id == 8 || c.id == 9);
        },
        /* protected_peer_ids */ {3, 8, 9},
        /* unprotected_peer_ids */ {},
        random_context));

    // Expect 1/4 onion peers and 1/4 of the others to be protected
    // from eviction, sorted by longest uptime (lowest nTimeConnected).
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = false;
            c.m_is_onion = (c.id == 3 || c.id > 7);
        },
        /* protected_peer_ids */ {0, 1, 2, 3, 8, 9},
        /* unprotected_peer_ids */ {4, 5, 6, 7, 10, 11},
        random_context));

    // Expect 1/4 localhost peers to be protected from eviction,
    // if no onion peers.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.m_is_onion = false;
            c.m_is_local = (c.id == 1 || c.id == 9 || c.id == 11);
        },
        /* protected_peer_ids */ {1, 9, 11},
        /* unprotected_peer_ids */ {},
        random_context));

    // Expect 1/4 localhost peers and 1/4 of the other peers to be protected,
    // sorted by longest uptime (lowest nTimeConnected), if no onion peers.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_onion = false;
            c.m_is_local = (c.id > 6);
        },
        /* protected_peer_ids */ {0, 1, 2, 7, 8, 9},
        /* unprotected_peer_ids */ {3, 4, 5, 6, 10, 11},
        random_context));

    // Combined test: expect 1/4 onion and 2 localhost peers to be protected
    // from eviction, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_onion = (c.id == 0 || c.id == 5 || c.id == 10);
            c.m_is_local = (c.id == 1 || c.id == 9 || c.id == 11);
        },
        /* protected_peer_ids */ {0, 1, 2, 5, 9, 10},
        /* unprotected_peer_ids */ {3, 4, 6, 7, 8, 11},
        random_context));

    // Combined test: expect having only 1 onion to allow allocating the
    // remaining 2 of the 1/4 to localhost peers, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        num_peers + 4, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_onion = (c.id == 15);
            c.m_is_local = (c.id > 6 && c.id < 11);
        },
        /* protected_peer_ids */ {0, 1, 2, 3, 7, 8, 9, 15},
        /* unprotected_peer_ids */ {4, 5, 6, 10, 11, 12, 13, 14},
        random_context));

    // Combined test: expect 2 onions (< 1/4) to allow allocating the minimum 2
    // localhost peers, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_onion = (c.id == 7 || c.id == 9);
            c.m_is_local = (c.id == 6 || c.id == 11);
        },
        /* protected_peer_ids */ {0, 1, 6, 7, 9, 11},
        /* unprotected_peer_ids */ {2, 3, 4, 5, 8, 10},
        random_context));

    // Combined test: when > 1/4, expect max 1/4 onion and 2 localhost peers
    // to be protected from eviction, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_onion = (c.id > 3 && c.id < 8);
            c.m_is_local = (c.id > 7);
        },
        /* protected_peer_ids */ {0, 4, 5, 6, 8, 9},
        /* unprotected_peer_ids */ {1, 2, 3, 7, 10, 11},
        random_context));

    // Combined test: idem > 1/4 with only 8 peers: expect 2 onion and 2
    // localhost peers (1/4 + 2) to be protected, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        8, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_onion = (c.id > 1 && c.id < 5);
            c.m_is_local = (c.id > 4);
        },
        /* protected_peer_ids */ {2, 3, 5, 6},
        /* unprotected_peer_ids */ {0, 1, 4, 7},
        random_context));

    // Combined test: idem > 1/4 with only 6 peers: expect 1 onion peer and no
    // localhost peers (1/4 + 0) to be protected, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        6, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_onion = (c.id == 4 || c.id == 5);
            c.m_is_local = (c.id == 3);
        },
        /* protected_peer_ids */ {0, 1, 4},
        /* unprotected_peer_ids */ {2, 3, 5},
        random_context));
}

// Returns true if any of the node ids in node_ids are selected for eviction.
bool IsEvicted(std::vector<NodeEvictionCandidate> candidates, const std::unordered_set<NodeId>& node_ids, FastRandomContext& random_context)
{
    Shuffle(candidates.begin(), candidates.end(), random_context);
    const std::optional<NodeId> evicted_node_id = SelectNodeToEvict(std::move(candidates));
    if (!evicted_node_id) {
        return false;
    }
    return node_ids.count(*evicted_node_id);
}

// Create number_of_nodes random nodes, apply setup function candidate_setup_fn,
// apply eviction logic and then return true if any of the node ids in node_ids
// are selected for eviction.
bool IsEvicted(const int number_of_nodes, std::function<void(NodeEvictionCandidate&)> candidate_setup_fn, const std::unordered_set<NodeId>& node_ids, FastRandomContext& random_context)
{
    std::vector<NodeEvictionCandidate> candidates = GetRandomNodeEvictionCandidates(number_of_nodes, random_context);
    for (NodeEvictionCandidate& candidate : candidates) {
        candidate_setup_fn(candidate);
    }
    return IsEvicted(candidates, node_ids, random_context);
}

BOOST_AUTO_TEST_CASE(peer_eviction_test)
{
    FastRandomContext random_context{true};

    for (int number_of_nodes = 0; number_of_nodes < 200; ++number_of_nodes) {
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
        // * "If any remaining peers are preferred for eviction consider only them. [...]"
        // * "Identify the network group with the most connections and youngest member. [...]"
    }
}

BOOST_AUTO_TEST_SUITE_END()
