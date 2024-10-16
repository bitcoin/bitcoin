// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httpserver.h>
#include <rpc/protocol.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <util/string.h>

#include <boost/test/unit_test.hpp>

using http_bitcoin::HTTPHeaders;
using http_bitcoin::HTTPResponse;
using http_bitcoin::MAX_HEADERS_SIZE;

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
        BOOST_CHECK(!headers.FindFirst("Cache-Control"));
        headers.Write("Cache-Control", "no-cache");
        // Check case-insensitive key matching
        BOOST_CHECK_EQUAL(headers.FindFirst("Cache-Control"), "no-cache");
        BOOST_CHECK_EQUAL(headers.FindFirst("cache-control"), "no-cache");
        // Additional values are appended, compared case-insensitive
        headers.Write("cache-control", "max-age=60");
        BOOST_CHECK_EQUAL(headers.FindFirst("Cache-Control"), "no-cache");
        BOOST_CHECK((headers.FindAll("Cache-Control") == std::vector<std::string_view>{"no-cache", "max-age=60"}));
        // Add a few more
        headers.Write("Pie", "apple");
        headers.Write("Sandwich", "ham");
        headers.Write("Coffee", "black");
        BOOST_CHECK_EQUAL(headers.FindFirst("Pie"), "apple");
        // Remove
        headers.RemoveAll("Pie");
        BOOST_CHECK(!headers.FindFirst("Pie"));
        // Combine for transmission
        std::string headers_string{headers.Stringify()};
        BOOST_CHECK_EQUAL(headers_string, "Cache-Control: no-cache\r\n"
                                          "cache-control: max-age=60\r\n"
                                          "Sandwich: ham\r\n"
                                          "Coffee: black\r\n"
                                          "\r\n");
    }
    {
        // Reading request headers captured from bitcoin-cli
        constexpr std::string_view bitcoin_cli_headers = "Host: 127.0.0.1\r\n"
                                                         "Connection: close\r\n"
                                                         "Content-Type: application/json\r\n"
                                                         "Authorization: Basic X19jb29raWVfXzozYzJkNTAxNDFlMGJiYmVhMTI5ODg3NzI5MTM3NTRmNThkNjc2OWMwZTYxZjgzNTgyNzEwYTY1OGRkYjVmZGQ3\r\n"
                                                         "Content-Length: 46\r\n";
        util::LineReader reader(bitcoin_cli_headers, /*max_line_length=*/MAX_HEADERS_SIZE);
        HTTPHeaders headers{};
        headers.Read(reader);
        BOOST_CHECK_EQUAL(headers.FindFirst("Host"), "127.0.0.1");
        BOOST_CHECK_EQUAL(headers.FindFirst("Connection"), "close");
        BOOST_CHECK_EQUAL(headers.FindFirst("Content-Type"), "application/json");
        BOOST_CHECK_EQUAL(headers.FindFirst("Authorization"), "Basic X19jb29raWVfXzozYzJkNTAxNDFlMGJiYmVhMTI5ODg3NzI5MTM3NTRmNThkNjc2OWMwZTYxZjgzNTgyNzEwYTY1OGRkYjVmZGQ3");
        BOOST_CHECK_EQUAL(headers.FindFirst("Content-Length"), "46");
        BOOST_CHECK(!headers.FindFirst("Pizza"));
    }
    // Ensure invalid headers are rejected
    {
        // missing a colon
        util::LineReader reader{"key value\n", /*max_line_length=*/MAX_HEADERS_SIZE};
        BOOST_CHECK_EXCEPTION(HTTPHeaders{}.Read(reader), std::runtime_error, HasReason{"HTTP header missing colon (:)"});
    }
    {
        // missing a key
        util::LineReader reader{":value\n", /*max_line_length=*/MAX_HEADERS_SIZE};
        BOOST_CHECK_EXCEPTION(HTTPHeaders{}.Read(reader), std::runtime_error, HasReason{"Empty HTTP header name"});
    }
    {
        // contains NUL
        util::LineReader reader{std::string_view{"X-Custom: foo\0bar\n", 18}, /*max_line_length=*/MAX_HEADERS_SIZE};
        BOOST_CHECK_EXCEPTION(HTTPHeaders{}.Read(reader), std::runtime_error, HasReason{"Header contains invalid character"});
    }
    {
        // contains bare \r (not followed by \n)
        util::LineReader reader{std::string_view{"X-Custom: foo\rbar\n"}, /*max_line_length=*/MAX_HEADERS_SIZE};
        BOOST_CHECK_EXCEPTION(HTTPHeaders{}.Read(reader), std::runtime_error, HasReason{"Header contains invalid character"});
    }
    {
        // contains odd \r preceding the expected CRLF
        util::LineReader reader{"X-Custom: foo\r\r\n", /*max_line_length=*/MAX_HEADERS_SIZE};
        BOOST_CHECK_EXCEPTION(HTTPHeaders{}.Read(reader), std::runtime_error, HasReason{"Header contains invalid character"});
    }
    {
        // key contains whitespace
        util::LineReader reader{"key : value\n", /*max_line_length=*/MAX_HEADERS_SIZE};
        BOOST_CHECK_EXCEPTION(HTTPHeaders{}.Read(reader), std::runtime_error, HasReason{"Invalid header field-name contains whitespace"});
    }
    {
        // Individual lines are below MAX_HEADERS_SIZE but the total is excessive
        std::string lines;
        lines.reserve(820 * 10);
        for (int i = 0; i < 820; ++i) {
            lines.append("key:value\n");
        }
        std::string_view excessive_headers{lines};
        BOOST_CHECK_GT(excessive_headers.size(), MAX_HEADERS_SIZE);
        util::LineReader reader{excessive_headers, /*max_line_length=*/MAX_HEADERS_SIZE};
        BOOST_CHECK_EXCEPTION(HTTPHeaders{}.Read(reader), std::runtime_error, HasReason{"HTTP headers exceed size limit"});
    }
    {
        // Ok
        util::LineReader reader{"key: value\n", /*max_line_length=*/MAX_HEADERS_SIZE};
        HTTPHeaders headers{};
        headers.Read(reader);
        BOOST_CHECK_EQUAL(headers.FindFirst("key"), "value");
    }
}

BOOST_AUTO_TEST_CASE(http_response_tests)
{
    // Typical HTTP 1.1 response headers
    HTTPHeaders headers{};
    headers.Write("Content-Length", "41");

    // Response points to headers which already exist because some of them
    // are set before we even know what the response will be.
    HTTPResponse res;
    res.m_version = {.major = 1, .minor = 1};
    res.m_status = HTTP_OK;
    res.m_headers = std::move(headers);
    BOOST_CHECK_EQUAL(
        res.StringifyHeaders(),
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 41\r\n"
        "\r\n");
}
BOOST_AUTO_TEST_SUITE_END()
