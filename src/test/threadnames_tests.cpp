// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <threadnames.h>
#include <test/test_bitcoin.h>

#include <thread>
#include <vector>
#include <set>
#include <mutex>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(threadnames_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(threadnames_test_process_rename_serial)
{
    ThreadNameRegistry reg;

    std::string original_process_name = reg.GetProcessName();

    reg.RenameProcess("bazzz");
#ifdef CAN_READ_PROCESS_NAME
    BOOST_CHECK_EQUAL(reg.GetProcessName(), "bazzz");
#else
    // Special case for platforms which don't support reading of process name.
    BOOST_CHECK_EQUAL(reg.GetProcessName(), "<unknown>");
#endif

    reg.RenameProcess("barrr");
#ifdef CAN_READ_PROCESS_NAME
    BOOST_CHECK_EQUAL(reg.GetProcessName(), "barrr");
#else
    // Special case for platforms which don't support reading of process name.
    BOOST_CHECK_EQUAL(reg.GetProcessName(), "<unknown>");
#endif

    reg.RenameProcess(original_process_name.c_str());
    BOOST_CHECK_EQUAL(reg.GetProcessName(), original_process_name);
}

/**
 * Ensure that expect_multiple prevents collisions by appending a numeric suffix.
 */
BOOST_AUTO_TEST_CASE(threadnames_test_rename_multiple_serial)
{
    ThreadNameRegistry reg;

    std::string original_process_name = reg.GetProcessName();

    BOOST_CHECK(reg.Rename("foo", /*expect_multiple=*/ true));
    BOOST_CHECK_EQUAL(reg.GetName(), "foo.0");

    // Can't rename to "foo" as that would be a collision.
    BOOST_CHECK_EQUAL(reg.Rename("foo", /*expect_multiple=*/ false), false);
    BOOST_CHECK_EQUAL(reg.GetName(), "foo.0");

    BOOST_CHECK(reg.Rename("foo", /*expect_multiple=*/ true));
    BOOST_CHECK_EQUAL(reg.GetName(), "foo.1");

    BOOST_CHECK(reg.Rename("foo", /*expect_multiple=*/ true));
    BOOST_CHECK_EQUAL(reg.GetName(), "foo.2");

    reg.RenameProcess(original_process_name.c_str());
    BOOST_CHECK_EQUAL(reg.GetProcessName(), original_process_name);
}

/**
 * Run a bunch of threads to all call Rename with some parameters.
 *
 * @return the set of name each thread has after attempted renaming.
 */
std::set<std::string> RenameEnMasse(int num_threads, bool expect_multiple)
{
    ThreadNameRegistry reg;
    std::vector<std::thread> threads;
    std::set<std::string> names;
    std::mutex lock;

    auto RenameThisThread = [&]() {
        reg.Rename("test_thread", /*expect_multiple=*/ expect_multiple);
        std::lock_guard<std::mutex> guard(lock);
        names.insert(reg.GetName());
    };

    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread(RenameThisThread));
    }

    for (std::thread& thread : threads) thread.join();

    return names;
}

/**
 * Rename a bunch of threads with the same basename (expect_multiple=true), ensuring suffixes are
 * applied properly.
 */
BOOST_AUTO_TEST_CASE(threadnames_test_rename_multiple_threaded)
{
    std::set<std::string> names = RenameEnMasse(100, true);

    BOOST_CHECK_EQUAL(names.size(), 100);

    // Names "test_thread.[n]" should exist for n = [0, 99]
    for (int i = 0; i < 100; ++i) {
        BOOST_CHECK(names.find("test_thread." + std::to_string(i)) != names.end());
    }

}

/**
 * Rename a bunch of threads with the same basename (expect_multiple=false), ensuring only one
 * rename succeeds.
 */
BOOST_AUTO_TEST_CASE(threadnames_test_rename_threaded)
{
    std::set<std::string> names = RenameEnMasse(100, false);

    // Only one thread's Rename should have succeeded for the same name.
    BOOST_CHECK_EQUAL(names.count("test_thread"), 1);
    BOOST_CHECK_EQUAL(names.size(), 2);  // test_thread and test_bitcoin (name of parent thread)

}

BOOST_AUTO_TEST_SUITE_END()
