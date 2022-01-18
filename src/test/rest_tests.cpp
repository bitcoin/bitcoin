// Copyright (c) 2012-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rest.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_FIXTURE_TEST_SUITE(rest_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_query_string)
{
    std::string param;
    RetFormat rf;
    // No query string
    rf = ParseDataFormat(param, "/rest/endpoint/someresource.json");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource");
    BOOST_CHECK_EQUAL(rf, RetFormat::JSON);

    // Query string with single parameter
    rf = ParseDataFormat(param, "/rest/endpoint/someresource.json?p1=v1");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource");
    BOOST_CHECK_EQUAL(rf, RetFormat::JSON);

    // Query string with multiple parameters
    rf = ParseDataFormat(param, "/rest/endpoint/someresource.json?p1=v1&p2=v2");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource");
    BOOST_CHECK_EQUAL(rf, RetFormat::JSON);

    // Incorrectly formed query string will not be handled
    rf = ParseDataFormat(param, "/rest/endpoint/someresource.json&p1=v1");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource.json&p1=v1");
    BOOST_CHECK_EQUAL(rf, RetFormat::UNDEF);
}
BOOST_AUTO_TEST_SUITE_END()
