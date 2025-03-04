// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * See https://www.boost.org/doc/libs/1_78_0/libs/test/doc/html/boost_test/adv_scenarios/single_header_customizations/multiple_translation_units.html
 */
#define BOOST_TEST_MODULE Bitcoin Core Test Suite

#include <gtest/gtest.h>

#include <test/util/setup_common.h>

#include <functional>
#include <iostream>

std::vector<const char*> COMMAND_LINE_ARGUMENTS;

/** Redirect debug log to unit_test.log files */
const std::function<void(const std::string&)> G_TEST_LOG_FUN = [](const std::string& s) {
    static const bool should_log{std::any_of(
        &COMMAND_LINE_ARGUMENTS[1],
        &COMMAND_LINE_ARGUMENTS.back(),
        [](const char* arg) {
            return std::string{"DEBUG_LOG_OUT"} == arg;
        })};
    if (!should_log) return;
    std::cout << s;
};

/**
 * Retrieve the command line arguments from boost.
 * Allows usage like:
 * `test_bitcoin --run_test="net_tests/cnode_listen_port" -- -checkaddrman=1 -printtoconsole=1`
 * which would return `["-checkaddrman=1", "-printtoconsole=1"]`.
 */
const std::function<std::vector<const char*>()> G_TEST_COMMAND_LINE_ARGUMENTS = []() {
    return COMMAND_LINE_ARGUMENTS;
};

/**
 * Retrieve the boost unit test name.
 */
const std::function<std::string()> G_TEST_GET_FULL_NAME = []() {
    const testing::TestInfo* test_info = testing::UnitTest::GetInstance()->current_test_info();
    return std::string(test_info->name()) + "_" + test_info->test_suite_name();
};

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    for (int i = 1; i < argc; ++i) {
        COMMAND_LINE_ARGUMENTS.push_back(argv[i]);
    }
    return RUN_ALL_TESTS();
}
