// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <vector>
#include "main.h"
#include "util.h"


#define SKIPLIST_LENGTH 300000

BOOST_AUTO_TEST_SUITE(skiplist_tests)

BOOST_AUTO_TEST_CASE(skiplist_test)
{
    std::vector<CBlockIndex> vIndex(SKIPLIST_LENGTH);

    for (int i=0; i<SKIPLIST_LENGTH; i++) {
        vIndex[i].nHeight = i;
        vIndex[i].pprev = (i == 0) ? NULL : &vIndex[i - 1];
        vIndex[i].BuildSkip();
    }

    for (int i=0; i<SKIPLIST_LENGTH; i++) {
        if (i > 0) {
            BOOST_CHECK(vIndex[i].pskip == &vIndex[vIndex[i].pskip->nHeight]);
            BOOST_CHECK(vIndex[i].pskip->nHeight < i);
        } else {
            BOOST_CHECK(vIndex[i].pskip == NULL);
        }
    }

    for (int i=0; i < 1000; i++) {
        int from = insecure_rand() % (SKIPLIST_LENGTH - 1);
        int to = insecure_rand() % (from + 1);

        BOOST_CHECK(vIndex[SKIPLIST_LENGTH - 1].GetAncestor(from) == &vIndex[from]);
        BOOST_CHECK(vIndex[from].GetAncestor(to) == &vIndex[to]);
        BOOST_CHECK(vIndex[from].GetAncestor(0) == &vIndex[0]);
    }
}

BOOST_AUTO_TEST_SUITE_END()

