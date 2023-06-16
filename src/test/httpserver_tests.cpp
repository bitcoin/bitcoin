// Copyright (c) 2012-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httpserver.h>
#include <test/util/setup_common.h>

#include <event2/http.h>

#include <string>
#include <string_view>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(httpserver_tests, BasicTestingSetup)

std::optional<std::string> GetParameterFromURI(const std::string_view uri, const std::string& key){
    auto uri_parsed{evhttp_uri_parse(uri.data())};
    if (uri_parsed == nullptr) return std::nullopt;
    auto param_value{GetQueryParameterFromUri(*uri_parsed, key)};
    evhttp_uri_free(uri_parsed);
    return param_value;
}

/** Test if query parameter exists in the provided uri. */
bool QueryParameterExists(const std::string_view uri, const std::string& key)
{
    return GetParameterFromURI(uri, key).has_value();
}

/** Test if query parameter exists in the provided uri and if its value matches the expected_value. */
bool QueryParameterEquals(const std::string_view uri, const std::string& key, const std::string_view expected_value)
{
    auto param_value{GetParameterFromURI(uri, key)};
    return param_value.has_value() && *param_value == expected_value;
}

BOOST_AUTO_TEST_CASE(test_query_parameters)
{
    std::string uri {};

    // No parameters
    BOOST_CHECK(!QueryParameterExists("localhost:8080/rest/headers/someresource.json", "p1"));

    // Single parameter
    BOOST_CHECK(QueryParameterEquals("localhost:8080/rest/endpoint/someresource.json?p1=v1", "p1", "v1"));
    BOOST_CHECK(!QueryParameterExists("localhost:8080/rest/endpoint/someresource.json?p1=v1", "p2"));

    // Multiple parameters
    BOOST_CHECK(QueryParameterEquals("/rest/endpoint/someresource.json?p1=v1&p2=v2", "p1", "v1"));
    BOOST_CHECK(QueryParameterEquals("/rest/endpoint/someresource.json?p1=v1&p2=v2", "p2", "v2"));

    // If the query string contains duplicate keys, the first value is returned
    BOOST_CHECK(QueryParameterEquals("/rest/endpoint/someresource.json?p1=v1&p1=v2", "p1", "v1"));

    // Invalid query string syntax is the same as not having parameters
    BOOST_CHECK(!QueryParameterExists("/rest/endpoint/someresource.json&p1=v1&p2=v2", "p1"));

    // URI with invalid characters (%) won't return any values regardless of which query parameter is queried
    BOOST_CHECK(!QueryParameterExists("/rest/endpoint/someresource.json&p1=v1&p2=v2%", "p1"));
}
BOOST_AUTO_TEST_SUITE_END()
