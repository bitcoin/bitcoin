// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mruset.h"

#include "random.h"
#include "util.h"
#include "test/test_bitcoin.h"

#include <set>

#include <boost/test/unit_test.hpp>

#define NUM_TESTS 16
#define MAX_SIZE 100

using namespace std;

BOOST_FIXTURE_TEST_SUITE(mruset_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(mruset_test)
{
    // The mruset being tested.
    mruset<int> mru(5000);

    // Run the test 10 times.
    for (int test = 0; test < 10; test++) {
        // Reset mru.
        mru.clear();

        // A deque + set to simulate the mruset.
        std::deque<int> rep;
        std::set<int> all;

        // Insert 10000 random integers below 15000.
        for (int j=0; j<10000; j++) {
            int add = GetRandInt(15000);
            mru.insert(add);

            // Add the number to rep/all as well.
            if (all.count(add) == 0) {
               all.insert(add);
               rep.push_back(add);
               if (all.size() == 5001) {
                   all.erase(rep.front());
                   rep.pop_front();
               }
            }

            // Do a full comparison between mru and the simulated mru every 1000 and every 5001 elements.
            if (j % 1000 == 0 || j % 5001 == 0) {
                mruset<int> mru2 = mru; // Also try making a copy

                // Check that all elements that should be in there, are in there.
                BOOST_FOREACH(int x, rep) {
                    BOOST_CHECK(mru.count(x));
                    BOOST_CHECK(mru2.count(x));
                }

                // Check that all elements that are in there, should be in there.
                BOOST_FOREACH(int x, mru) {
                    BOOST_CHECK(all.count(x));
                }

                // Check that all elements that are in there, should be in there.
                BOOST_FOREACH(int x, mru2) {
                    BOOST_CHECK(all.count(x));
                }

                for (int t = 0; t < 10; t++) {
                    int r = GetRandInt(15000);
                    BOOST_CHECK(all.count(r) == mru.count(r));
                    BOOST_CHECK(all.count(r) == mru2.count(r));
                }
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
