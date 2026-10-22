// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/privbcast.h>
#include <privbcast/params.h>

#include <boost/test/unit_test.hpp>

#include <cstddef>

using node::PrivbcastQueue;

BOOST_AUTO_TEST_SUITE(privbcast_queue_tests)

BOOST_AUTO_TEST_CASE(file_descriptor_budget)
{
    // N3: a start waits for a running job only if that job outlives its cap. With starts at least
    // START_SPACING_MIN apart, the jobs started within one JOB_CAP before a start, and that start,
    // must all fit.
    BOOST_CHECK_GE(PrivbcastQueue::MAX_CONCURRENT_JOBS, size_t(privbcast::JOB_CAP / privbcast::START_SPACING_MIN) + 1);
    // N10: the recipient connections of every job that can run (D1), plus one discovery's (N4).
    for (const size_t seeds : {0, 1, 5}) {
        BOOST_CHECK_EQUAL(PrivbcastQueue::FileDescriptorBudget(seeds),
                          PrivbcastQueue::MAX_CONCURRENT_JOBS * privbcast::SLOTS + size_t{privbcast::QUERIES_PER_SEED} * seeds);
    }
    BOOST_CHECK_EQUAL(PrivbcastQueue::FileDescriptorBudget(3, /*max_concurrent=*/2), 2 * privbcast::SLOTS + size_t{privbcast::QUERIES_PER_SEED} * 3);
}

BOOST_AUTO_TEST_SUITE_END()
