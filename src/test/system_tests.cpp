// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <common/run_command.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <util/string.h>

#ifdef ENABLE_EXTERNAL_SIGNER
#include <util/subprocess.h>
#endif // ENABLE_EXTERNAL_SIGNER

#include <boost/cstdlib.hpp>
#include <boost/test/unit_test.hpp>

#include <string>

BOOST_FIXTURE_TEST_SUITE(system_tests, BasicTestingSetup)

#ifdef ENABLE_EXTERNAL_SIGNER

static std::vector<std::string> mock_executable(std::string name)
{
    return {boost::unit_test::framework::master_test_suite().argv[0], "--log_level=nothing", "--report_level=no", "--run_test=mock_process/" + name};
}

BOOST_AUTO_TEST_CASE(run_command)
{
    {
        const UniValue result = RunCommandParseJSON({});
        BOOST_CHECK(result.isNull());
    }
    {
        const UniValue result = RunCommandParseJSON(mock_executable("valid_json"));
        BOOST_CHECK(result.isObject());
        const UniValue& success = result.find_value("success");
        BOOST_CHECK(!success.isNull());
        BOOST_CHECK_EQUAL(success.get_bool(), true);
    }
    {
        // An invalid command is handled by cpp-subprocess
#ifdef WIN32
        const std::string expected{"CreateProcess failed: "};
#else
        const std::string expected{"execve failed: "};
#endif
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON({"invalid_command"}), subprocess::CalledProcessError, HasReason(expected));
    }
    {
        // Return non-zero exit code, no output to stderr
        const std::vector<std::string> command = mock_executable("nonzeroexit_nooutput");
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON(command), std::runtime_error, [&](const std::runtime_error& e) {
            const std::string what{e.what()};
            BOOST_CHECK(what.find(strprintf("RunCommandParseJSON error: process(%s) returned %d: \n", util::Join(command, " "), boost::exit_test_failure)) != std::string::npos);
            return true;
        });
    }
    {
        // Return non-zero exit code, with error message for stderr
        const std::vector<std::string> command = mock_executable("nonzeroexit_stderroutput");
        const std::string expected{"err"};
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON(command), std::runtime_error, [&](const std::runtime_error& e) {
            const std::string what(e.what());
            BOOST_CHECK(what.find(strprintf("RunCommandParseJSON error: process(%s) returned %s: %s", util::Join(command, " "), boost::exit_test_failure, "err")) != std::string::npos);
            BOOST_CHECK(what.find(expected) != std::string::npos);
            return true;
        });
    }
    {
        // Unable to parse JSON
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON(mock_executable("invalid_json")), std::runtime_error, HasReason("Unable to parse JSON: {"));
    }
    {
        // Test stdin
        const UniValue result = RunCommandParseJSON(mock_executable("pass_stdin_to_stdout"), "{\"success\": true}");
        BOOST_CHECK(result.isObject());
        const UniValue& success = result.find_value("success");
        BOOST_CHECK(!success.isNull());
        BOOST_CHECK_EQUAL(success.get_bool(), true);
    }
}
#endif // ENABLE_EXTERNAL_SIGNER

BOOST_AUTO_TEST_SUITE_END()
