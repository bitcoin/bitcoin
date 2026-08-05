// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httpserver.h>
#include <rest.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <string>

using http_bitcoin::HTTPRequest;

BOOST_FIXTURE_TEST_SUITE(rest_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_query_string)
{
    std::string param;
    RESTResponseFormat rf;
    // No query string
    rf = ParseDataFormat(param, "/rest/endpoint/someresource.json");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource");
    BOOST_CHECK_EQUAL(rf, RESTResponseFormat::JSON);

    // Query string with single parameter
    rf = ParseDataFormat(param, "/rest/endpoint/someresource.bin?p1=v1");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource");
    BOOST_CHECK_EQUAL(rf, RESTResponseFormat::BINARY);

    // Query string with multiple parameters
    rf = ParseDataFormat(param, "/rest/endpoint/someresource.hex?p1=v1&p2=v2");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource");
    BOOST_CHECK_EQUAL(rf, RESTResponseFormat::HEX);

    // Incorrectly formed query string will not be handled
    rf = ParseDataFormat(param, "/rest/endpoint/someresource.json&p1=v1");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource.json&p1=v1");
    BOOST_CHECK_EQUAL(rf, RESTResponseFormat::UNDEF);

    // Omitted data format with query string should return UNDEF and hide query string
    rf = ParseDataFormat(param, "/rest/endpoint/someresource?p1=v1");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource");
    BOOST_CHECK_EQUAL(rf, RESTResponseFormat::UNDEF);

    // Data format specified after query string
    rf = ParseDataFormat(param, "/rest/endpoint/someresource?p1=v1.json");
    BOOST_CHECK_EQUAL(param, "/rest/endpoint/someresource");
    BOOST_CHECK_EQUAL(rf, RESTResponseFormat::UNDEF);
}

BOOST_AUTO_TEST_CASE(test_bool_parsing)
{
    HTTPRequest req;
    util::Expected<bool, std::string> ret;
    constexpr auto param = "verbose";

    // If no value is set, the default is used.
    req.m_target = "/rest/endpoint/resource?someotherparam=false";
    ret = RESTParseBoolParam(&req, param, /*default_val=*/true);
    BOOST_CHECK(ret.has_value());
    BOOST_CHECK_EQUAL(*ret, true);

    ret = RESTParseBoolParam(&req, param, /*default_val=*/false);
    BOOST_CHECK(ret.has_value());
    BOOST_CHECK_EQUAL(*ret, false);

    // All the checks will use this format string strprintf'ing in the param
    // and val.
    static constexpr char fmt_str[] = "/rest/endpoint/resource?%s=%s";

    // Happy case, true
    req.m_target = strprintf(fmt_str, param, "true");
    ret = RESTParseBoolParam(&req, param, /*default_val=*/true);
    BOOST_CHECK(ret.has_value());
    BOOST_CHECK_EQUAL(*ret, true);

    // Happy case, false
    req.m_target = strprintf(fmt_str, param, "false");
    ret = RESTParseBoolParam(&req, param, /*default_val=*/true);
    BOOST_CHECK(ret.has_value());
    BOOST_CHECK_EQUAL(*ret, false);

    // Error when some other value is used
    req.m_target = strprintf(fmt_str, param, "happiness");
    ret = RESTParseBoolParam(&req, param, /*default_val=*/true);
    BOOST_CHECK(!ret.has_value());
    auto err_str = strprintf("The \"%s\" query parameter must be either \"true\" or \"false\".", param);
    BOOST_CHECK_EQUAL(ret.error(), err_str);
}

BOOST_AUTO_TEST_SUITE_END()
