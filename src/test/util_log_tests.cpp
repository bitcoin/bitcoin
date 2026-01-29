// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <util/log.h>

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <vector>

using namespace util::log;

BOOST_AUTO_TEST_SUITE(util_log_tests)

BOOST_AUTO_TEST_CASE(dispatcher_enabled)
{
    Dispatcher dispatcher{};

    // No callbacks registered - should not be enabled
    BOOST_CHECK(!dispatcher.Enabled());

    // Register a callback - should now be enabled
    auto handle = dispatcher.RegisterCallback([](const Entry&) {});
    BOOST_CHECK(dispatcher.Enabled());

    // Unregister the callback - should no longer be enabled
    dispatcher.UnregisterCallback(handle);
    BOOST_CHECK(!dispatcher.Enabled());
}

BOOST_AUTO_TEST_CASE(dispatcher_callback_receives_entry)
{
    Dispatcher dispatcher{};

    std::vector<Entry> received_entries;
    auto handle = dispatcher.RegisterCallback([&](const Entry& entry) {
        received_entries.push_back(entry);
    });

    const uint64_t test_category{42};
    auto before = std::chrono::system_clock::now();
    auto line = __LINE__ + 1; // capture the line number of the log statement below
    dispatcher.Log(Level::Info, /*category=*/test_category, {__func__}, /*should_ratelimit=*/true, "test message %d", 123);
    auto after = std::chrono::system_clock::now();

    BOOST_REQUIRE_EQUAL(received_entries.size(), 1);
    const auto& entry = received_entries[0];

    BOOST_CHECK_EQUAL(static_cast<int>(entry.level), static_cast<int>(Level::Info));
    BOOST_CHECK_EQUAL(entry.category, test_category);
    BOOST_CHECK_EQUAL(entry.message, "test message 123");
    BOOST_CHECK(entry.source_loc.file_name().ends_with(__FILE__));
    BOOST_CHECK_EQUAL(entry.source_loc.function_name_short(), __func__);
    BOOST_CHECK_EQUAL(entry.source_loc.line(), line);
    BOOST_CHECK(entry.timestamp >= before && entry.timestamp <= after);
    BOOST_CHECK(entry.should_ratelimit);

    dispatcher.UnregisterCallback(handle);
}

BOOST_AUTO_TEST_CASE(dispatcher_multiple_callbacks)
{
    Dispatcher dispatcher{};

    int callback1_count{0};
    int callback2_count{0};

    BOOST_CHECK(!dispatcher.Enabled());
    auto handle1 = dispatcher.RegisterCallback([&](const Entry&) {
        ++callback1_count;
    });
    auto handle2 = dispatcher.RegisterCallback([&](const Entry&) {
        ++callback2_count;
    });
    BOOST_CHECK(dispatcher.Enabled());

    dispatcher.Log(Level::Info, /*category=*/0, __func__, /*should_ratelimit=*/false, "test");

    // Both callbacks should have been called
    BOOST_CHECK_EQUAL(callback1_count, 1);
    BOOST_CHECK_EQUAL(callback2_count, 1);

    // Unregister first callback
    dispatcher.UnregisterCallback(handle1);

    dispatcher.Log(Level::Info, /*category=*/0, __func__, /*should_ratelimit=*/false, "test");

    // Only second callback should have been called
    BOOST_CHECK_EQUAL(callback1_count, 1);
    BOOST_CHECK_EQUAL(callback2_count, 2);

    dispatcher.UnregisterCallback(handle2);
    BOOST_CHECK(!dispatcher.Enabled());
}

BOOST_AUTO_TEST_CASE(dispatcher_skips_when_not_enabled)
{
    Dispatcher dispatcher{};
    BOOST_CHECK(!dispatcher.Enabled());

    // No callbacks registered, Log should be a no-op (and not crash)
    dispatcher.Log(Level::Info, /*category=*/0, __func__, /*should_ratelimit=*/false, "this should not crash");
}

BOOST_AUTO_TEST_SUITE_END()
