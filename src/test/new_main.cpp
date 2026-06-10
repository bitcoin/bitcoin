// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BITCOIN_TEST_MAIN

#include <test/util/framework.h>

#include <test/util/setup_common.h>

#include <functional>
#include <string>
#include <vector>

/**
 * Retrieve the user-supplied command line arguments — everything after the
 * `--` separator on the command line. Allows usage like:
 * `test_bitcoin --run_test="net_tests/cnode_listen_port" -- -checkaddrman=1 -printtoconsole=1`
 * which would return `["-checkaddrman=1", "-printtoconsole=1"]`.
 */
const std::function<std::vector<const char*>()> G_TEST_COMMAND_LINE_ARGUMENTS = []() {
    const auto ua = framework::user_args();
    return ua;
};

/**
 * Retrieve the full name (`suite/case`) of the currently-running test.
 */
const std::function<std::string()> G_TEST_GET_FULL_NAME = []() {
    return framework::current_test_full_name();
};
