// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>

#include <boost/test/unit_test.hpp>

#include <list>

BOOST_AUTO_TEST_SUITE(coinscachepair_tests)

static constexpr auto NUM_NODES{4};

std::list<CoinsCachePair> CreatePairs(CoinsCachePair& sentinel)
{
    std::list<CoinsCachePair> nodes;
    for (auto i{0}; i < NUM_NODES; ++i) {
        nodes.emplace_back();

        auto node{std::prev(nodes.end())};
        CCoinsCacheEntry::SetDirty(*node, sentinel);

        BOOST_CHECK(node->second.IsDirty() && !node->second.IsFresh());
        BOOST_CHECK_EQUAL(node->second.Next(), &sentinel);
        BOOST_CHECK_EQUAL(sentinel.second.Prev(), &(*node));

        if (i > 0) {
            BOOST_CHECK_EQUAL(std::prev(node)->second.Next(), &(*node));
            BOOST_CHECK_EQUAL(node->second.Prev(), &(*std::prev(node)));
        }
    }
    return nodes;
}

BOOST_AUTO_TEST_CASE(linked_list_iteration)
{
    CoinsCachePair sentinel;
    sentinel.second.SelfRef(sentinel);
    auto nodes{CreatePairs(sentinel)};

    // Check iterating through pairs is identical to iterating through a list
    auto node{sentinel.second.Next()};
    for (const auto& expected : nodes) {
        BOOST_CHECK_EQUAL(&expected, node);
        node = node->second.Next();
    }
    BOOST_CHECK_EQUAL(node, &sentinel);

    // Check iterating through pairs is identical to iterating through a list
    // Clear the state during iteration
    node = sentinel.second.Next();
    for (const auto& expected : nodes) {
        BOOST_CHECK_EQUAL(&expected, node);
        auto next = node->second.Next();
        node->second.SetClean();
        node = next;
    }
    BOOST_CHECK_EQUAL(node, &sentinel);
    // Check that sentinel's next and prev are itself
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &sentinel);
    BOOST_CHECK_EQUAL(sentinel.second.Prev(), &sentinel);

    // Delete the nodes from the list to make sure there are no dangling pointers
    for (auto it{nodes.begin()}; it != nodes.end(); it = nodes.erase(it)) {
        BOOST_CHECK(!it->second.IsDirty() && !it->second.IsFresh());
    }
}

BOOST_AUTO_TEST_CASE(linked_list_iterate_erase)
{
    CoinsCachePair sentinel;
    sentinel.second.SelfRef(sentinel);
    auto nodes{CreatePairs(sentinel)};

    // Check iterating through pairs is identical to iterating through a list
    // Erase the nodes as we iterate through, but don't clear state
    // The state will be cleared by the CCoinsCacheEntry's destructor
    auto node{sentinel.second.Next()};
    for (auto expected{nodes.begin()}; expected != nodes.end(); expected = nodes.erase(expected)) {
        BOOST_CHECK_EQUAL(&(*expected), node);
        node = node->second.Next();
    }
    BOOST_CHECK_EQUAL(node, &sentinel);

    // Check that sentinel's next and prev are itself
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &sentinel);
    BOOST_CHECK_EQUAL(sentinel.second.Prev(), &sentinel);
}

BOOST_AUTO_TEST_CASE(linked_list_random_deletion)
{
    CoinsCachePair sentinel;
    sentinel.second.SelfRef(sentinel);
    auto nodes{CreatePairs(sentinel)};

    // Create linked list sentinel->n1->n2->n3->n4->sentinel
    auto n1{nodes.begin()};
    auto n2{std::next(n1)};
    auto n3{std::next(n2)};
    auto n4{std::next(n3)};

    // Delete n2
    // sentinel->n1->n3->n4->sentinel
    nodes.erase(n2);
    // Check that n1 now points to n3, and n3 still points to n4
    // Also check that state was not altered
    BOOST_CHECK(n1->second.IsDirty() && !n1->second.IsFresh());
    BOOST_CHECK_EQUAL(n1->second.Next(), &(*n3));
    BOOST_CHECK(n3->second.IsDirty() && !n3->second.IsFresh());
    BOOST_CHECK_EQUAL(n3->second.Next(), &(*n4));
    BOOST_CHECK_EQUAL(n3->second.Prev(), &(*n1));

    // Delete n1
    // sentinel->n3->n4->sentinel
    nodes.erase(n1);
    // Check that sentinel now points to n3, and n3 still points to n4
    // Also check that state was not altered
    BOOST_CHECK(n3->second.IsDirty() && !n3->second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &(*n3));
    BOOST_CHECK_EQUAL(n3->second.Next(), &(*n4));
    BOOST_CHECK_EQUAL(n3->second.Prev(), &sentinel);

    // Delete n4
    // sentinel->n3->sentinel
    nodes.erase(n4);
    // Check that sentinel still points to n3, and n3 points to sentinel
    // Also check that state was not altered
    BOOST_CHECK(n3->second.IsDirty() && !n3->second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &(*n3));
    BOOST_CHECK_EQUAL(n3->second.Next(), &sentinel);
    BOOST_CHECK_EQUAL(sentinel.second.Prev(), &(*n3));

    // Delete n3
    // sentinel->sentinel
    nodes.erase(n3);
    // Check that sentinel's next and prev are itself
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &sentinel);
    BOOST_CHECK_EQUAL(sentinel.second.Prev(), &sentinel);
}

BOOST_AUTO_TEST_CASE(linked_list_set_state)
{
    CoinsCachePair sentinel;
    sentinel.second.SelfRef(sentinel);
    CoinsCachePair n1;
    CoinsCachePair n2;

    // Check that setting DIRTY inserts it into linked list and sets state
    CCoinsCacheEntry::SetDirty(n1, sentinel);
    BOOST_CHECK(n1.second.IsDirty() && !n1.second.IsFresh());
    BOOST_CHECK_EQUAL(n1.second.Next(), &sentinel);
    BOOST_CHECK_EQUAL(n1.second.Prev(), &sentinel);
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n1);
    BOOST_CHECK_EQUAL(sentinel.second.Prev(), &n1);

    // Check that setting FRESH on new node inserts it after n1
    CCoinsCacheEntry::SetFresh(n2, sentinel);
    BOOST_CHECK(n2.second.IsFresh() && !n2.second.IsDirty());
    BOOST_CHECK_EQUAL(n2.second.Next(), &sentinel);
    BOOST_CHECK_EQUAL(n2.second.Prev(), &n1);
    BOOST_CHECK_EQUAL(n1.second.Next(), &n2);
    BOOST_CHECK_EQUAL(sentinel.second.Prev(), &n2);

    // Check that we can set extra state, but they don't change our position
    CCoinsCacheEntry::SetFresh(n1, sentinel);
    BOOST_CHECK(n1.second.IsDirty() && n1.second.IsFresh());
    BOOST_CHECK_EQUAL(n1.second.Next(), &n2);
    BOOST_CHECK_EQUAL(n1.second.Prev(), &sentinel);
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n1);
    BOOST_CHECK_EQUAL(n2.second.Prev(), &n1);

    // Check that we can clear state then re-set it
    n1.second.SetClean();
    BOOST_CHECK(!n1.second.IsDirty() && !n1.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n2);
    BOOST_CHECK_EQUAL(sentinel.second.Prev(), &n2);
    BOOST_CHECK_EQUAL(n2.second.Next(), &sentinel);
    BOOST_CHECK_EQUAL(n2.second.Prev(), &sentinel);

    // Calling `SetClean` a second time has no effect
    n1.second.SetClean();
    BOOST_CHECK(!n1.second.IsDirty() && !n1.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n2);
    BOOST_CHECK_EQUAL(sentinel.second.Prev(), &n2);
    BOOST_CHECK_EQUAL(n2.second.Next(), &sentinel);
    BOOST_CHECK_EQUAL(n2.second.Prev(), &sentinel);

    // Adding DIRTY re-inserts it after n2
    CCoinsCacheEntry::SetDirty(n1, sentinel);
    BOOST_CHECK(n1.second.IsDirty() && !n1.second.IsFresh());
    BOOST_CHECK_EQUAL(n2.second.Next(), &n1);
    BOOST_CHECK_EQUAL(n1.second.Prev(), &n2);
    BOOST_CHECK_EQUAL(n1.second.Next(), &sentinel);
    BOOST_CHECK_EQUAL(sentinel.second.Prev(), &n1);
}

BOOST_AUTO_TEST_SUITE_END()
