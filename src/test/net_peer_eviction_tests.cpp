// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <net.h>
#include <test/util/net.h>
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
            /* m_network */ ALL_NETWORKS[random_context.randrange(ALL_NETWORKS.size())],
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
            c.m_is_local = false;
            c.m_network = NET_IPV4;
        },
        /* protected_peer_ids */ {0, 1, 2, 3, 4, 5},
        /* unprotected_peer_ids */ {6, 7, 8, 9, 10, 11},
        random_context));

    // Verify in the opposite direction.
    BOOST_CHECK(IsProtected(
        num_peers, [num_peers](NodeEvictionCandidate& c) {
            c.nTimeConnected = num_peers - c.id;
            c.m_is_local = false;
            c.m_network = NET_IPV6;
        },
        /* protected_peer_ids */ {6, 7, 8, 9, 10, 11},
        /* unprotected_peer_ids */ {0, 1, 2, 3, 4, 5},
        random_context));

    // Test protection of onion, localhost, and I2P peers...

    // Expect 1/4 onion peers to be protected from eviction,
    // if no localhost or I2P peers.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.m_is_local = false;
            c.m_network = (c.id == 3 || c.id == 8 || c.id == 9) ? NET_ONION : NET_IPV4;
        },
        /* protected_peer_ids */ {3, 8, 9},
        /* unprotected_peer_ids */ {},
        random_context));

    // Expect 1/4 onion peers and 1/4 of the other peers to be protected,
    // sorted by longest uptime (lowest nTimeConnected), if no localhost or I2P peers.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = false;
            c.m_network = (c.id == 3 || c.id > 7) ? NET_ONION : NET_IPV6;
        },
        /* protected_peer_ids */ {0, 1, 2, 3, 8, 9},
        /* unprotected_peer_ids */ {4, 5, 6, 7, 10, 11},
        random_context));

    // Expect 1/4 localhost peers to be protected from eviction,
    // if no onion or I2P peers.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.m_is_local = (c.id == 1 || c.id == 9 || c.id == 11);
            c.m_network = NET_IPV4;
        },
        /* protected_peer_ids */ {1, 9, 11},
        /* unprotected_peer_ids */ {},
        random_context));

    // Expect 1/4 localhost peers and 1/4 of the other peers to be protected,
    // sorted by longest uptime (lowest nTimeConnected), if no onion or I2P peers.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id > 6);
            c.m_network = NET_IPV6;
        },
        /* protected_peer_ids */ {0, 1, 2, 7, 8, 9},
        /* unprotected_peer_ids */ {3, 4, 5, 6, 10, 11},
        random_context));

    // Expect 1/4 I2P peers to be protected from eviction,
    // if no onion or localhost peers.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.m_is_local = false;
            c.m_network = (c.id == 2 || c.id == 7 || c.id == 10) ? NET_I2P : NET_IPV4;
        },
        /* protected_peer_ids */ {2, 7, 10},
        /* unprotected_peer_ids */ {},
        random_context));

    // Expect 1/4 I2P peers and 1/4 of the other peers to be protected,
    // sorted by longest uptime (lowest nTimeConnected), if no onion or localhost peers.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = false;
            c.m_network = (c.id == 4 || c.id > 8) ? NET_I2P : NET_IPV6;
        },
        /* protected_peer_ids */ {0, 1, 2, 4, 9, 10},
        /* unprotected_peer_ids */ {3, 5, 6, 7, 8, 11},
        random_context));

    // Tests with 2 networks...

    // Combined test: expect having 1 localhost and 1 onion peer out of 4 to
    // protect 1 localhost, 0 onion and 1 other peer, sorted by longest uptime;
    // stable sort breaks tie with array order of localhost first.
    BOOST_CHECK(IsProtected(
        4, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id == 4);
            c.m_network = (c.id == 3) ? NET_ONION : NET_IPV4;
        },
        /* protected_peer_ids */ {0, 4},
        /* unprotected_peer_ids */ {1, 2},
        random_context));

    // Combined test: expect having 1 localhost and 1 onion peer out of 7 to
    // protect 1 localhost, 0 onion, and 2 other peers (3 total), sorted by
    // uptime; stable sort breaks tie with array order of localhost first.
    BOOST_CHECK(IsProtected(
        7, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id == 6);
            c.m_network = (c.id == 5) ? NET_ONION : NET_IPV4;
        },
        /* protected_peer_ids */ {0, 1, 6},
        /* unprotected_peer_ids */ {2, 3, 4, 5},
        random_context));

    // Combined test: expect having 1 localhost and 1 onion peer out of 8 to
    // protect protect 1 localhost, 1 onion and 2 other peers (4 total), sorted
    // by uptime; stable sort breaks tie with array order of localhost first.
    BOOST_CHECK(IsProtected(
        8, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id == 6);
            c.m_network = (c.id == 5) ? NET_ONION : NET_IPV4;
        },
        /* protected_peer_ids */ {0, 1, 5, 6},
        /* unprotected_peer_ids */ {2, 3, 4, 7},
        random_context));

    // Combined test: expect having 3 localhost and 3 onion peers out of 12 to
    // protect 2 localhost and 1 onion, plus 3 other peers, sorted by longest
    // uptime; stable sort breaks ties with the array order of localhost first.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id == 6 || c.id == 9 || c.id == 11);
            c.m_network = (c.id == 7 || c.id == 8 || c.id == 10) ? NET_ONION : NET_IPV6;
        },
        /* protected_peer_ids */ {0, 1, 2, 6, 7, 9},
        /* unprotected_peer_ids */ {3, 4, 5, 8, 10, 11},
        random_context));

    // Combined test: expect having 4 localhost and 1 onion peer out of 12 to
    // protect 2 localhost and 1 onion, plus 3 other peers, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id > 4 && c.id < 9);
            c.m_network = (c.id == 10) ? NET_ONION : NET_IPV4;
        },
        /* protected_peer_ids */ {0, 1, 2, 5, 6, 10},
        /* unprotected_peer_ids */ {3, 4, 7, 8, 9, 11},
        random_context));

    // Combined test: expect having 4 localhost and 2 onion peers out of 16 to
    // protect 2 localhost and 2 onions, plus 4 other peers, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        16, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id == 6 || c.id == 9 || c.id == 11 || c.id == 12);
            c.m_network = (c.id == 8 || c.id == 10) ? NET_ONION : NET_IPV6;
        },
        /* protected_peer_ids */ {0, 1, 2, 3, 6, 8, 9, 10},
        /* unprotected_peer_ids */ {4, 5, 7, 11, 12, 13, 14, 15},
        random_context));

    // Combined test: expect having 5 localhost and 1 onion peer out of 16 to
    // protect 3 localhost (recovering the unused onion slot), 1 onion, and 4
    // others, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        16, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id > 10);
            c.m_network = (c.id == 10) ? NET_ONION : NET_IPV4;
        },
        /* protected_peer_ids */ {0, 1, 2, 3, 10, 11, 12, 13},
        /* unprotected_peer_ids */ {4, 5, 6, 7, 8, 9, 14, 15},
        random_context));

    // Combined test: expect having 1 localhost and 4 onion peers out of 16 to
    // protect 1 localhost and 3 onions (recovering the unused localhost slot),
    // plus 4 others, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        16, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id == 15);
            c.m_network = (c.id > 6 && c.id < 11) ? NET_ONION : NET_IPV6;
        },
        /* protected_peer_ids */ {0, 1, 2, 3, 7, 8, 9, 15},
        /* unprotected_peer_ids */ {5, 6, 10, 11, 12, 13, 14},
        random_context));

    // Combined test: expect having 2 onion and 4 I2P out of 12 peers to protect
    // 2 onion (prioritized for having fewer candidates) and 1 I2P, plus 3
    // others, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        num_peers, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = false;
            if (c.id == 8 || c.id == 10) {
                c.m_network = NET_ONION;
            } else if (c.id == 6 || c.id == 9 || c.id == 11 || c.id == 12) {
                c.m_network = NET_I2P;
            } else {
                c.m_network = NET_IPV4;
            }
        },
        /* protected_peer_ids */ {0, 1, 2, 6, 8, 10},
        /* unprotected_peer_ids */ {3, 4, 5, 7, 9, 11},
        random_context));

    // Tests with 3 networks...

    // Combined test: expect having 1 localhost, 1 I2P and 1 onion peer out of 4
    // to protect 1 I2P, 0 localhost, 0 onion and 1 other peer (2 total), sorted
    // by longest uptime; stable sort breaks tie with array order of I2P first.
    BOOST_CHECK(IsProtected(
        4, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id == 3);
            if (c.id == 4) {
                c.m_network = NET_I2P;
            } else if (c.id == 2) {
                c.m_network = NET_ONION;
            } else {
                c.m_network = NET_IPV6;
            }
        },
        /* protected_peer_ids */ {0, 4},
        /* unprotected_peer_ids */ {1, 2},
        random_context));

    // Combined test: expect having 1 localhost, 1 I2P and 1 onion peer out of 7
    // to protect 1 I2P, 0 localhost, 0 onion and 2 other peers (3 total) sorted
    // by longest uptime; stable sort breaks tie with array order of I2P first.
    BOOST_CHECK(IsProtected(
        7, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id == 4);
            if (c.id == 6) {
                c.m_network = NET_I2P;
            } else if (c.id == 5) {
                c.m_network = NET_ONION;
            } else {
                c.m_network = NET_IPV6;
            }
        },
        /* protected_peer_ids */ {0, 1, 6},
        /* unprotected_peer_ids */ {2, 3, 4, 5},
        random_context));

    // Combined test: expect having 1 localhost, 1 I2P and 1 onion peer out of 8
    // to protect 1 I2P, 1 localhost, 0 onion and 2 other peers (4 total) sorted
    // by uptime; stable sort breaks tie with array order of I2P then localhost.
    BOOST_CHECK(IsProtected(
        8, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id == 6);
            if (c.id == 5) {
                c.m_network = NET_I2P;
            } else if (c.id == 4) {
                c.m_network = NET_ONION;
            } else {
                c.m_network = NET_IPV6;
            }
        },
        /* protected_peer_ids */ {0, 1, 5, 6},
        /* unprotected_peer_ids */ {2, 3, 4, 7},
        random_context));

    // Combined test: expect having 4 localhost, 2 I2P, and 2 onion peers out of
    // 16 to protect 1 localhost, 2 I2P, and 1 onion (4/16 total), plus 4 others
    // for 8 total, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        16, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id == 6 || c.id > 11);
            if (c.id == 7 || c.id == 11) {
                c.m_network = NET_I2P;
            } else if (c.id == 9 || c.id == 10) {
                c.m_network = NET_ONION;
            } else {
                c.m_network = NET_IPV4;
            }
        },
        /* protected_peer_ids */ {0, 1, 2, 3, 6, 7, 9, 11},
        /* unprotected_peer_ids */ {4, 5, 8, 10, 12, 13, 14, 15},
        random_context));

    // Combined test: expect having 1 localhost, 8 I2P and 1 onion peer out of
    // 24 to protect 1, 4, and 1 (6 total), plus 6 others for 12/24 total,
    // sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        24, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id == 12);
            if (c.id > 14 && c.id < 23) { // 4 protected instead of usual 2
                c.m_network = NET_I2P;
            } else if (c.id == 23) {
                c.m_network = NET_ONION;
            } else {
                c.m_network = NET_IPV6;
            }
        },
        /* protected_peer_ids */ {0, 1, 2, 3, 4, 5, 12, 15, 16, 17, 18, 23},
        /* unprotected_peer_ids */ {6, 7, 8, 9, 10, 11, 13, 14, 19, 20, 21, 22},
        random_context));

    // Combined test: expect having 1 localhost, 3 I2P and 6 onion peers out of
    // 24 to protect 1, 3, and 2 (6 total, I2P has fewer candidates and so gets the
    // unused localhost slot), plus 6 others for 12/24 total, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        24, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id == 15);
            if (c.id == 12 || c.id == 14 || c.id == 17) {
                c.m_network = NET_I2P;
            } else if (c.id > 17) { // 4 protected instead of usual 2
                c.m_network = NET_ONION;
            } else {
                c.m_network = NET_IPV4;
            }
        },
        /* protected_peer_ids */ {0, 1, 2, 3, 4, 5, 12, 14, 15, 17, 18, 19},
        /* unprotected_peer_ids */ {6, 7, 8, 9, 10, 11, 13, 16, 20, 21, 22, 23},
        random_context));

    // Combined test: expect having 1 localhost, 7 I2P and 4 onion peers out of
    // 24 to protect 1 localhost, 2 I2P, and 3 onions (6 total), plus 6 others
    // for 12/24 total, sorted by longest uptime.
    BOOST_CHECK(IsProtected(
        24, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id == 13);
            if (c.id > 16) {
                c.m_network = NET_I2P;
            } else if (c.id == 12 || c.id == 14 || c.id == 15 || c.id == 16) {
                c.m_network = NET_ONION;
            } else {
                c.m_network = NET_IPV6;
            }
        },
        /* protected_peer_ids */ {0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 17, 18},
        /* unprotected_peer_ids */ {6, 7, 8, 9, 10, 11, 16, 19, 20, 21, 22, 23},
        random_context));

    // Combined test: expect having 8 localhost, 4 I2P, and 3 onion peers out of
    // 24 to protect 2 of each (6 total), plus 6 others for 12/24 total, sorted
    // by longest uptime.
    BOOST_CHECK(IsProtected(
        24, [](NodeEvictionCandidate& c) {
            c.nTimeConnected = c.id;
            c.m_is_local = (c.id > 15);
            if (c.id > 10 && c.id < 15) {
                c.m_network = NET_I2P;
            } else if (c.id > 6 && c.id < 10) {
                c.m_network = NET_ONION;
            } else {
                c.m_network = NET_IPV4;
            }
        },
        /* protected_peer_ids */ {0, 1, 2, 3, 4, 5, 7, 8, 11, 12, 16, 17},
        /* unprotected_peer_ids */ {6, 9, 10, 13, 14, 15, 18, 19, 20, 21, 22, 23},
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
