// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <test/util/setup_common.h>
#include <util/system.h>
#include <univalue.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(system_tests, BasicTestingSetup)

// At least one test is required (in case HAVE_BOOST_PROCESS is not defined)
BOOST_AUTO_TEST_CASE(dummy)
{
    BOOST_CHECK(true);
}

#ifdef HAVE_BOOST_PROCESS
BOOST_AUTO_TEST_CASE(run_command)
{
    {
        const UniValue result = runCommandParseJSON("");
        BOOST_CHECK(result.isNull());
    }
    {
#ifdef WIN32
        // Windows requires single quotes to prevent escaping double quotes from the JSON...
        const UniValue result = runCommandParseJSON("echo '{\"success\": true}'");
#else
        // ... but Linux and macOS echo a single quote if it's used
        const UniValue result = runCommandParseJSON("echo \"{\"success\": true}\"");
#endif
        BOOST_CHECK(result.isObject());
        const UniValue& success = find_value(result, "success");
        BOOST_CHECK(!success.isNull());
        BOOST_CHECK_EQUAL(success.getBool(), true);
    }
    {
        BOOST_REQUIRE_THROW(runCommandParseJSON("invalid_command"), std::runtime_error); // Command failed
    }
    {
        BOOST_REQUIRE_THROW(runCommandParseJSON("echo \"{\""), std::runtime_error); // Unable to parse JSON
    }
    // Test std::in, except for Windows
#ifndef WIN32
    {
        const UniValue result = runCommandParseJSON("cat", "{\"success\": true}");
        BOOST_CHECK(result.isObject());
        const UniValue& success = find_value(result, "success");
        BOOST_CHECK(!success.isNull());
        BOOST_CHECK_EQUAL(success.getBool(), true);
    }
#endif
}
#endif

BOOST_AUTO_TEST_SUITE_END()
