// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <ipc/process.h>
#include <ipc/test/ipc_test.h>

#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(ipc_tests, BasicTestingSetup)
BOOST_AUTO_TEST_CASE(ipc_tests)
{
    IpcPipeTest();
    IpcSocketPairTest();
    IpcSocketTest(m_args.GetDataDirNet());
}

// Test address parsing.
BOOST_AUTO_TEST_CASE(parse_address_test)
{
    std::unique_ptr<ipc::Process> process{ipc::MakeProcess()};
    fs::path datadir{"/var/empty/notexist"};
    auto check_notexist{[](const std::system_error& e) { return e.code() == std::errc::no_such_file_or_directory; }};
    auto check_address{[&](std::string address, std::string expect_address, std::string expect_error) {
        if (expect_error.empty()) {
            BOOST_CHECK_EXCEPTION(process->connect(datadir, "test_bitcoin", address), std::system_error, check_notexist);
        } else {
            BOOST_CHECK_EXCEPTION(process->connect(datadir, "test_bitcoin", address), std::invalid_argument, HasReason(expect_error));
        }
        BOOST_CHECK_EQUAL(address, expect_address);
    }};
    check_address("unix", "unix:/var/empty/notexist/test_bitcoin.sock", "");
    check_address("unix:", "unix:/var/empty/notexist/test_bitcoin.sock", "");
    check_address("unix:path.sock", "unix:/var/empty/notexist/path.sock", "");
    check_address("unix:0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000.sock",
                  "unix:/var/empty/notexist/0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000.sock",
                  "Unix address path \"/var/empty/notexist/0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000.sock\" exceeded maximum socket path length");
    check_address("invalid", "invalid", "Unrecognized address 'invalid'");

    auto check_bind{[](const std::string& configured_address, const std::string& expected_address, std::optional<size_t> expected_max_connections, const std::string& expected_error) {
        std::string error;
        auto bind{ipc::ParseBindAddress(configured_address, error)};
        if (!expected_error.empty()) {
            BOOST_CHECK(!bind);
            BOOST_CHECK_EQUAL(error, expected_error);
            return;
        }
        BOOST_REQUIRE(bind);
        BOOST_CHECK_EQUAL(bind->address, expected_address);
        BOOST_CHECK_EQUAL(bind->max_connections.has_value(), expected_max_connections.has_value());
        if (bind->max_connections && expected_max_connections) {
            BOOST_CHECK_EQUAL(*bind->max_connections, *expected_max_connections);
        }
    }};
    check_bind("unix", "unix", std::nullopt, "");
    check_bind("unix:", "unix:", std::nullopt, "");
    check_bind("unix::max-connections=8", "unix:", 8, "");
    check_bind("unix:path.sock:max-connections=8", "unix:path.sock", 8, "");
    check_bind("unix:max-connections=8", "", std::nullopt, "Missing unix socket path before max-connections option; use unix::max-connections=<n> for the default path");
    check_bind("unix::max-connections=-1", "", std::nullopt, "Invalid max-connections value '-1'");
    check_bind("unix::max-connections=", "", std::nullopt, "Missing value for max-connections option");
}

BOOST_AUTO_TEST_SUITE_END()
