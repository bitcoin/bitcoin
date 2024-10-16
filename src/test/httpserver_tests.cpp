// Copyright (c) 2012-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httpserver.h>
#include <rpc/protocol.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

using http_bitcoin::HTTPHeaders;
using http_bitcoin::HTTPResponse;

BOOST_FIXTURE_TEST_SUITE(httpserver_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_query_parameters)
{
    using http_libevent::GetQueryParameterFromUri;
    std::string uri {};

    // No parameters
    uri = "localhost:8080/rest/headers/someresource.json";
    BOOST_CHECK(!GetQueryParameterFromUri(uri.c_str(), "p1").has_value());

    // Single parameter
    uri = "localhost:8080/rest/endpoint/someresource.json?p1=v1";
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri.c_str(), "p1").value(), "v1");
    BOOST_CHECK(!GetQueryParameterFromUri(uri.c_str(), "p2").has_value());

    // Multiple parameters
    uri = "/rest/endpoint/someresource.json?p1=v1&p2=v2";
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri.c_str(), "p1").value(), "v1");
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri.c_str(), "p2").value(), "v2");

    // If the query string contains duplicate keys, the first value is returned
    uri = "/rest/endpoint/someresource.json?p1=v1&p1=v2";
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri.c_str(), "p1").value(), "v1");

    // Invalid query string syntax is the same as not having parameters
    uri = "/rest/endpoint/someresource.json&p1=v1&p2=v2";
    BOOST_CHECK(!GetQueryParameterFromUri(uri.c_str(), "p1").has_value());

    // URI with invalid characters (%) raises a runtime error regardless of which query parameter is queried
    uri = "/rest/endpoint/someresource.json&p1=v1&p2=v2%";
    BOOST_CHECK_EXCEPTION(GetQueryParameterFromUri(uri.c_str(), "p1"), std::runtime_error, HasReason("URI parsing failed, it likely contained RFC 3986 invalid characters"));
}

BOOST_AUTO_TEST_CASE(http_headers_tests)
{
    {
        // Writing response headers
        HTTPHeaders headers{};
        BOOST_CHECK(!headers.Find("Cache-Control"));
        headers.Write("Cache-Control", "no-cache");
        // Check case-insensitive key matching
        BOOST_CHECK_EQUAL(headers.Find("Cache-Control").value(), "no-cache");
        BOOST_CHECK_EQUAL(headers.Find("cache-control").value(), "no-cache");
        // Additional values are comma-separated and appended
        headers.Write("Cache-Control", "no-store");
        BOOST_CHECK_EQUAL(headers.Find("Cache-Control").value(), "no-cache, no-store");
        // Add a few more
        headers.Write("Pie", "apple");
        headers.Write("Sandwich", "ham");
        headers.Write("Coffee", "black");
        BOOST_CHECK_EQUAL(headers.Find("Pie").value(), "apple");
        // Remove
        headers.Remove("Pie");
        BOOST_CHECK(!headers.Find("Pie"));
        // Combine for transmission
        // std::map sorts alphabetically by key, no order is specified for HTTP
        BOOST_CHECK_EQUAL(
            headers.Stringify(),
            "Cache-Control: no-cache, no-store\r\n"
            "Coffee: black\r\n"
            "Sandwich: ham\r\n\r\n");
    }
    {
        // Reading request headers captured from bitcoin-cli
        std::vector<std::byte> buffer{TryParseHex<std::byte>(
            "486f73743a203132372e302e302e310d0a436f6e6e656374696f6e3a20636c6f73"
            "650d0a436f6e74656e742d547970653a206170706c69636174696f6e2f6a736f6e"
            "0d0a417574686f72697a6174696f6e3a204261736963205831396a623239726157"
            "5666587a6f7a597a4a6b4e5441784e44466c4d474a69596d56684d5449354f4467"
            "334e7a49354d544d334e54526d4e54686b4e6a63324f574d775a5459785a6a677a"
            "4e5467794e7a4577595459314f47526b596a566d5a4751330d0a436f6e74656e74"
            "2d4c656e6774683a2034360d0a0d0a").value()};
        util::LineReader reader(buffer, /*max_read=*/1028);
        HTTPHeaders headers{};
        headers.Read(reader);
        BOOST_CHECK_EQUAL(headers.Find("Host").value(), "127.0.0.1");
        BOOST_CHECK_EQUAL(headers.Find("Connection").value(), "close");
        BOOST_CHECK_EQUAL(headers.Find("Content-Type").value(), "application/json");
        BOOST_CHECK_EQUAL(headers.Find("Authorization").value(), "Basic X19jb29raWVfXzozYzJkNTAxNDFlMGJiYmVhMTI5ODg3NzI5MTM3NTRmNThkNjc2OWMwZTYxZjgzNTgyNzEwYTY1OGRkYjVmZGQ3");
        BOOST_CHECK_EQUAL(headers.Find("Content-Length").value(), "46");
        BOOST_CHECK(!headers.Find("Pizza"));
    }
}

BOOST_AUTO_TEST_CASE(http_response_tests)
{
    // Typical HTTP 1.1 response headers
    HTTPHeaders headers{};
    headers.Write("Content-Type", "application/json");
    headers.Write("Date", "Tue, 15 Oct 2024 17:54:12 GMT");
    headers.Write("Content-Length", "41");
    // Response points to headers which already exist because some of them
    // are set before we even know what the response will be.
    HTTPResponse res;
    res.m_version_major = 1;
    res.m_version_minor = 1;
    res.m_status = HTTP_OK;
    res.m_reason = HTTPReason.find(res.m_status)->second;
    res.m_body = StringToBuffer("{\"result\":865793,\"error\":null,\"id\":null\"}");
    // Everything except the body, which might be raw bytes instead of a string
    res.m_headers = std::move(headers);
    BOOST_CHECK_EQUAL(
        res.StringifyHeaders(),
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 41\r\n"
        "Content-Type: application/json\r\n"
        "Date: Tue, 15 Oct 2024 17:54:12 GMT\r\n"
        "\r\n");
}
BOOST_AUTO_TEST_SUITE_END()
