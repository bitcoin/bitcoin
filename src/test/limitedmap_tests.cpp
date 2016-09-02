// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "limitedmap.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(limitedmap_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(limitedmap_test)
{
    // create a limitedmap capped at 10 items
    limitedmap<int, int> map(10);

    // check that the max size is 10
    FAST_CHECK(map.max_size() == 10);

    // check that it's empty
    FAST_CHECK(map.size() == 0);

    // insert (-1, -1)
    map.insert(std::pair<int, int>(-1, -1));

    // make sure that the size is updated
    FAST_CHECK(map.size() == 1);

    // make sure that the new items is in the map
    FAST_CHECK(map.count(-1) == 1);

    // insert 10 new items
    for (int i = 0; i < 10; i++) {
        map.insert(std::pair<int, int>(i, i + 1));
    }

    // make sure that the map now contains 10 items...
    FAST_CHECK(map.size() == 10);

    // ...and that the first item has been discarded
    FAST_CHECK(map.count(-1) == 0);

    // iterate over the map, both with an index and an iterator
    limitedmap<int, int>::const_iterator it = map.begin();
    for (int i = 0; i < 10; i++) {
        // make sure the item is present
        FAST_CHECK(map.count(i) == 1);

        // use the iterator to check for the expected key adn value
        FAST_CHECK(it->first == i);
        FAST_CHECK(it->second == i + 1);
        
        // use find to check for the value
        FAST_CHECK(map.find(i)->second == i + 1);
        
        // update and recheck
        map.update(it, i + 2);
        FAST_CHECK(map.find(i)->second == i + 2);

        it++;
    }

    // check that we've exhausted the iterator
    FAST_CHECK(it == map.end());

    // resize the map to 5 items
    map.max_size(5);

    // check that the max size and size are now 5
    FAST_CHECK(map.max_size() == 5);
    FAST_CHECK(map.size() == 5);

    // check that items less than 5 have been discarded
    // and items greater than 5 are retained
    for (int i = 0; i < 10; i++) {
        if (i < 5) {
            FAST_CHECK(map.count(i) == 0);
        } else {
            FAST_CHECK(map.count(i) == 1);
        }
    }

    // erase some items not in the map
    for (int i = 100; i < 1000; i += 100) {
        map.erase(i);
    }

    // check that the size is unaffected
    FAST_CHECK(map.size() == 5);

    // erase the remaining elements
    for (int i = 5; i < 10; i++) {
        map.erase(i);
    }

    // check that the map is now empty
    FAST_CHECK(map.empty());
}

BOOST_AUTO_TEST_SUITE_END()
