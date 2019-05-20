// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/threadnames.h>
#include <test/setup_common.h>

#include <thread>
#include <vector>
#include <set>
#include <mutex>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(util_threadnames_tests, BasicTestingSetup)

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
        util::ThreadRename(TEST_THREAD_NAME_BASE + std::to_string(i));
        std::lock_guard<std::mutex> guard(lock);
        names.insert(util::ThreadGetInternalName());
    };

    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread(RenameThisThread, i));
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
    BOOST_CHECK_EQUAL(util::ThreadGetInternalName(), "");

#if !defined(HAVE_THREAD_LOCAL)
    // This test doesn't apply to platforms where we don't have thread_local.
    return;
#endif

    std::set<std::string> names = RenameEnMasse(100);

    BOOST_CHECK_EQUAL(names.size(), 100);

    // Names "test_thread.[n]" should exist for n = [0, 99]
    for (int i = 0; i < 100; ++i) {
        BOOST_CHECK(names.find(TEST_THREAD_NAME_BASE + std::to_string(i)) != names.end());
    }

}

BOOST_AUTO_TEST_SUITE_END()
