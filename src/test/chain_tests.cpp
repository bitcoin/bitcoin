// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <chain.h>
#include <test/util/setup_common.h>
#include <vector>

BOOST_AUTO_TEST_SUITE(chain_tests)

BOOST_AUTO_TEST_CASE(get_median_time_past)
{
    std::vector<CBlockIndex> blocks(11);
    std::vector<CBlockIndex>::iterator curr, prev;
    blocks.begin()->nTime = 0;
    for (curr = blocks.begin() + 1, prev = blocks.begin(); curr != blocks.end(); prev = curr, ++curr) {
        curr->nTime = prev->nTime + 1;
        curr->pprev = &(*prev);
    }
    // block nTime already sorted in increasing order
    BOOST_CHECK(blocks[10].GetMedianTimePast() == blocks[5].nTime);
}


BOOST_AUTO_TEST_SUITE_END()
