// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>

#include <boost/test/unit_test.hpp>

#include <list>

BOOST_AUTO_TEST_SUITE(coinscachepair_tests)

static constexpr auto NUM_NODES{4};

std::list<CoinsCachePair> CreatePairs(CCoinsCacheEntry& head)
{
    std::list<CoinsCachePair> nodes;
    for (auto i{0}; i < NUM_NODES; ++i) {
        nodes.emplace_front();

        auto node{nodes.begin()};
        node->second.AddFlags(CCoinsCacheEntry::DIRTY, *node, head);

        BOOST_CHECK_EQUAL(node->second.GetFlags(), CCoinsCacheEntry::DIRTY);
        BOOST_CHECK_EQUAL(head.Next(), &(*node));

        if (i > 0) {
            BOOST_CHECK_EQUAL(&(*std::next(node)), node->second.Next());
        } else {
            BOOST_CHECK_EQUAL(nullptr, node->second.Next());
        }
    }
    return nodes;
}

BOOST_AUTO_TEST_CASE(linked_list_iteration)
{
    CCoinsCacheEntry head;
    auto nodes{CreatePairs(head)};

    // Check iterating through pairs is identical to iterating through a list
    auto node{head.Next()};
    for (const auto& expected : nodes) {
        BOOST_CHECK_EQUAL(&expected, node);
        node = node->second.Next();
    }
    BOOST_CHECK_EQUAL(node, nullptr);

    // Check iterating through pairs is identical to iterating through a list
    // Clear the flags during iteration
    node = head.Next();
    for (const auto& expected : nodes) {
        BOOST_CHECK_EQUAL(&expected, node);
        node = node->second.Next(/*clear_flags=*/true);
    }
    BOOST_CHECK_EQUAL(node, nullptr);
    // Check that head's next is empty
    BOOST_CHECK_EQUAL(head.Next(), nullptr);

    // Delete the nodes from the list to make sure there are no dangling pointers
    for (auto it{nodes.begin()}; it != nodes.end(); it = nodes.erase(it)) {
        BOOST_CHECK_EQUAL(it->second.GetFlags(), 0);
        BOOST_CHECK_EQUAL(it->second.Next(), nullptr);
    }
}

BOOST_AUTO_TEST_CASE(linked_list_iterate_erase)
{
    CCoinsCacheEntry head;
    auto nodes{CreatePairs(head)};

    // Check iterating through pairs is identical to iterating through a list
    // Erase the nodes as we iterate through, but don't clear flags
    // The flags will be cleared by the CCoinsCacheEntry's destructor
    auto node{head.Next()};
    for (auto expected{nodes.begin()}; expected != nodes.end(); expected = nodes.erase(expected)) {
        BOOST_CHECK_EQUAL(&(*expected), node);
        node = node->second.Next(/*clear_flags=*/false);
    }
    BOOST_CHECK_EQUAL(node, nullptr);

    // Check that head's next is empty
    BOOST_CHECK_EQUAL(head.Next(), nullptr);
}

BOOST_AUTO_TEST_CASE(linked_list_random_deletion)
{
    CCoinsCacheEntry head;
    auto nodes{CreatePairs(head)};

    // Create linked list head->n1->n2->n3->n4->nullptr
    auto n1{nodes.begin()};
    auto n2{std::next(n1)};
    auto n3{std::next(n2)};
    auto n4{std::next(n3)};

    // Delete n2
    // head->n1->n3->n4->nullptr
    nodes.erase(n2);
    // Check that n1 now points to n3, and n3 still points to n4
    // Also check that flags were not altered
    BOOST_CHECK_EQUAL(n1->second.GetFlags(), CCoinsCacheEntry::DIRTY);
    BOOST_CHECK_EQUAL(n1->second.Next(), &(*n3));
    BOOST_CHECK_EQUAL(n3->second.GetFlags(), CCoinsCacheEntry::DIRTY);
    BOOST_CHECK_EQUAL(n3->second.Next(), &(*n4));

    // Delete n1
    // head->n3->n4->nullptr
    nodes.erase(n1);
    // Check that head now points to n3, and n3 still points to n4
    // Also check that flags were not altered
    BOOST_CHECK_EQUAL(n3->second.GetFlags(), CCoinsCacheEntry::DIRTY);
    BOOST_CHECK_EQUAL(head.Next(), &(*n3));
    BOOST_CHECK_EQUAL(n3->second.Next(), &(*n4));

    // Delete n4
    // head->n3->nullptr
    nodes.erase(n4);
    // Check that head still points to n3, and n3 points to nullptr
    // Also check that flags were not altered
    BOOST_CHECK_EQUAL(n3->second.GetFlags(), CCoinsCacheEntry::DIRTY);
    BOOST_CHECK_EQUAL(head.Next(), &(*n3));
    BOOST_CHECK_EQUAL(n3->second.Next(), nullptr);

    // Delete n3
    // head->nullptr
    nodes.erase(n3);
    // Check that head's next now points to nullptr
    BOOST_CHECK_EQUAL(head.Next(), nullptr);
}

BOOST_AUTO_TEST_CASE(linked_list_add_flags)
{
    CCoinsCacheEntry head;
    CoinsCachePair n1;
    CoinsCachePair n2;

    // Check that adding 0 flag has no effect
    n1.second.AddFlags(0, n1, head);
    BOOST_CHECK_EQUAL(n1.second.GetFlags(), 0);
    BOOST_CHECK_EQUAL(n1.second.Next(), nullptr);
    BOOST_CHECK_EQUAL(head.Next(), nullptr);

    // Check that adding DIRTY flag inserts it into linked list and sets flags
    n1.second.AddFlags(CCoinsCacheEntry::DIRTY, n1, head);
    BOOST_CHECK_EQUAL(n1.second.GetFlags(), CCoinsCacheEntry::DIRTY);
    BOOST_CHECK_EQUAL(n1.second.Next(), nullptr);
    BOOST_CHECK_EQUAL(head.Next(), &n1);

    // Check that adding FRESH flag on new node inserts it after head
    n2.second.AddFlags(CCoinsCacheEntry::FRESH, n2, head);
    BOOST_CHECK_EQUAL(n2.second.GetFlags(), CCoinsCacheEntry::FRESH);
    BOOST_CHECK_EQUAL(n2.second.Next(), &n1);
    BOOST_CHECK_EQUAL(head.Next(), &n2);

    // Check that adding 0 flag has no effect, and doesn't change position
    n1.second.AddFlags(0, n1, head);
    BOOST_CHECK_EQUAL(n1.second.GetFlags(), CCoinsCacheEntry::DIRTY);
    BOOST_CHECK_EQUAL(n1.second.Next(), nullptr);
    BOOST_CHECK_EQUAL(head.Next(), &n2);
    BOOST_CHECK_EQUAL(n2.second.Next(), &n1);

    // Check that we can add extra flags, but they don't change our position
    n1.second.AddFlags(CCoinsCacheEntry::FRESH, n1, head);
    BOOST_CHECK_EQUAL(n1.second.GetFlags(), CCoinsCacheEntry::DIRTY | CCoinsCacheEntry::FRESH);
    BOOST_CHECK_EQUAL(n1.second.Next(), nullptr);
    BOOST_CHECK_EQUAL(head.Next(), &n2);
    BOOST_CHECK_EQUAL(n2.second.Next(), &n1);

    // Check that we can clear flags then re-add them
    n1.second.ClearFlags();
    BOOST_CHECK_EQUAL(n1.second.GetFlags(), 0);
    BOOST_CHECK_EQUAL(n2.second.Next(), nullptr);

    // Check that calling `ClearFlags` with 0 flags has no effect
    n1.second.ClearFlags();
    BOOST_CHECK_EQUAL(n1.second.GetFlags(), 0);
    BOOST_CHECK_EQUAL(n2.second.Next(), nullptr);

    // Adding 0 still has no effect
    n1.second.AddFlags(0, n1, head);
    BOOST_CHECK_EQUAL(n1.second.GetFlags(), 0);
    BOOST_CHECK_EQUAL(n1.second.Next(), nullptr);
    BOOST_CHECK_EQUAL(head.Next(), &n2);

    // But adding DIRTY re-inserts it after head
    n1.second.AddFlags(CCoinsCacheEntry::DIRTY, n1, head);
    BOOST_CHECK_EQUAL(n1.second.GetFlags(), CCoinsCacheEntry::DIRTY);
    BOOST_CHECK_EQUAL(head.Next(), &n1);
    BOOST_CHECK_EQUAL(n1.second.Next(), &n2);
}

BOOST_AUTO_TEST_SUITE_END()
