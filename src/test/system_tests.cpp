// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <test/util/setup_common.h>
#include <util/system.h>
#include <univalue.h>

#ifdef HAVE_BOOST_PROCESS
#include <boost/process.hpp>
#endif // HAVE_BOOST_PROCESS

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(system_tests, BasicTestingSetup)

// At least one test is required (in case HAVE_BOOST_PROCESS is not defined).
// Workaround for https://github.com/bitcoin/bitcoin/issues/19128
BOOST_AUTO_TEST_CASE(dummy)
{
    BOOST_CHECK(true);
}

#ifdef HAVE_BOOST_PROCESS

bool checkMessageFalse(const std::runtime_error& ex)
{
    BOOST_CHECK_EQUAL(ex.what(), std::string("RunCommandParseJSON error: process(false) returned 1: \n"));
    return true;
}

bool checkMessageFileNotFound(const std::runtime_error& ex)
{
    const std::string what(ex.what());
    BOOST_CHECK(what.find("RunCommandParseJSON error:") != std::string::npos);
    // "ls nosuchfile" should result in:
    // On Linux & Mac: "No such file or directory"
    // On Windows: "The system cannot find the file specified."
    BOOST_CHECK(what.find("file") != std::string::npos);
    return true;
}

bool checkCommandNotFound(const std::runtime_error& ex)
{
    const std::string what(ex.what());
    BOOST_CHECK(what.find("127") != std::string::npos);
    return true;
}

BOOST_AUTO_TEST_CASE(run_command)
{
    {
        const UniValue result = RunCommandParseJSON("");
        BOOST_CHECK(result.isNull());
    }
    {
        const UniValue result = RunCommandParseJSON("echo '{\"success\": true}'");
        BOOST_CHECK(result.isObject());
        const UniValue& success = find_value(result, "success");
        BOOST_CHECK(!success.isNull());
        BOOST_CHECK_EQUAL(success.getBool(), true);
    }
    {
        // An invalid command results in exit code 127
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON("invalid_command"), std::runtime_error, checkCommandNotFound);
    }
    {
        // Return non-zero exit code, no output to stderr
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON("false"), std::runtime_error, checkMessageFalse);
    }
    {
        // Return non-zero exit code, with error message for stderr
        BOOST_CHECK_EXCEPTION(RunCommandParseJSON("ls nosuchfile"), std::runtime_error, checkMessageFileNotFound);
    }
    {
        BOOST_REQUIRE_THROW(RunCommandParseJSON("echo '{\"'"), std::runtime_error); // Unable to parse JSON
    }
    // Test std::in, except for Windows
#ifndef WIN32
    {
        const UniValue result = RunCommandParseJSON("cat", "{\"success\": true}");
        BOOST_CHECK(result.isObject());
        const UniValue& success = find_value(result, "success");
        BOOST_CHECK(!success.isNull());
        BOOST_CHECK_EQUAL(success.getBool(), true);
    }
#endif
}
#endif // HAVE_BOOST_PROCESS

BOOST_AUTO_TEST_SUITE_END()
