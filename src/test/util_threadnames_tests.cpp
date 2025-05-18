// Copyright (c) 2018-2021 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/string.h>
#include <util/threadnames.h>

#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <boost/test/unit_test.hpp>

using util::ToString;

BOOST_AUTO_TEST_SUITE(util_threadnames_tests)

const std::string TEST_THREAD_NAME_BASE = "test_thread.";

/**
 * Run a bunch of threads to all call util::ThreadRename.
 *
 * @return the set of name each thread has after attempted renaming.
 */
std::set<std::string> RenameEnMasse(int num_threads)
{
    std::vector<std::thread> threads;
    std::set<std::string> names;
    std::mutex lock;

    auto RenameThisThread = [&](int i) {
        util::ThreadRename(TEST_THREAD_NAME_BASE + ToString(i));
        std::lock_guard<std::mutex> guard(lock);
        names.insert(util::ThreadGetInternalName());
    };

    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(RenameThisThread, i);
    }

    for (std::thread& thread : threads) thread.join();

    return names;
}

/**
 * Rename a bunch of threads with the same basename (expect_multiple=true), ensuring suffixes are
 * applied properly.
 */
BOOST_AUTO_TEST_CASE(util_threadnames_test_rename_threaded)
{
    std::set<std::string> names = RenameEnMasse(100);

    BOOST_CHECK_EQUAL(names.size(), 100U);

    // Names "test_thread.[n]" should exist for n = [0, 99]
    for (int i = 0; i < 100; ++i) {
        BOOST_CHECK(names.find(TEST_THREAD_NAME_BASE + ToString(i)) != names.end());
    }

}

BOOST_AUTO_TEST_SUITE_END()
