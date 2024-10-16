// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httpserver.h>
#include <rpc/protocol.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <test/util/str.h>

#include <boost/test/unit_test.hpp>

using namespace util::hex_literals;
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
        // Additional values are appended but FindFirst() does not return them
        headers.Write("Cache-Control", "no-store");
        BOOST_CHECK_EQUAL(headers.FindFirst("Cache-Control"), "no-cache");
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
                                          "Cache-Control: no-store\r\n"
                                          "Sandwich: ham\r\n"
                                          "Coffee: black\r\n"
                                          "\r\n");
    }
    {
        // Reading request headers captured from bitcoin-cli
        constexpr auto buffer{
            "486f73743a203132372e302e302e310d0a436f6e6e656374696f6e3a20636c6f73"
            "650d0a436f6e74656e742d547970653a206170706c69636174696f6e2f6a736f6e"
            "0d0a417574686f72697a6174696f6e3a204261736963205831396a623239726157"
            "5666587a6f7a597a4a6b4e5441784e44466c4d474a69596d56684d5449354f4467"
            "334e7a49354d544d334e54526d4e54686b4e6a63324f574d775a5459785a6a677a"
            "4e5467794e7a4577595459314f47526b596a566d5a4751330d0a436f6e74656e74"
            "2d4c656e6774683a2034360d0a0d0a"_hex};
        util::LineReader reader(buffer, /*max_line_length=*/MAX_HEADERS_SIZE);
        HTTPHeaders headers{};
        headers.Read(reader);
        BOOST_CHECK_EQUAL(headers.FindFirst("Host"), "127.0.0.1");
        BOOST_CHECK_EQUAL(headers.FindFirst("Connection"), "close");
        BOOST_CHECK_EQUAL(headers.FindFirst("Content-Type"), "application/json");
        BOOST_CHECK_EQUAL(headers.FindFirst("Authorization"), "Basic X19jb29raWVfXzozYzJkNTAxNDFlMGJiYmVhMTI5ODg3NzI5MTM3NTRmNThkNjc2OWMwZTYxZjgzNTgyNzEwYTY1OGRkYjVmZGQ3");
        BOOST_CHECK_EQUAL(headers.FindFirst("Content-Length"), "46");
        BOOST_CHECK(!headers.FindFirst("Pizza"));
    }
    {
        // Ensure invalid headers are rejected
        std::span<const std::byte> missing_a_colon{StringToBytes("key value\n")};
        util::LineReader reader1(missing_a_colon, /*max_line_length=*/MAX_HEADERS_SIZE);
        HTTPHeaders headers1{};
        BOOST_CHECK_EXCEPTION(headers1.Read(reader1), std::runtime_error, HasReason{"HTTP header missing colon (:)"});

        std::span<const std::byte> missing_a_key{StringToBytes(":value\n")};
        util::LineReader reader2(missing_a_key, /*max_line_length=*/MAX_HEADERS_SIZE);
        HTTPHeaders headers2{};
        BOOST_CHECK_EXCEPTION(headers2.Read(reader2), std::runtime_error, HasReason{"Empty HTTP header name"});

        // Individual lines are below MAX_HEADERS_SIZE but the total is excessive
        std::string lines;
        for (int i = 0; i < 820; ++i) {
            lines.append("key: value\n");
        }
        std::span<const std::byte> excessive_size{StringToBytes(lines)};
        util::LineReader reader3(excessive_size, /*max_line_length=*/MAX_HEADERS_SIZE);
        HTTPHeaders headers3{};
        BOOST_CHECK_EXCEPTION(headers3.Read(reader3), std::runtime_error, HasReason{"HTTP headers exceed size limit"});

        // Fixed
        std::span<const std::byte> fixed_headers{StringToBytes("key:value\n")};
        util::LineReader reader4(fixed_headers, /*max_line_length=*/MAX_HEADERS_SIZE);
        HTTPHeaders headers4{};
        headers4.Read(reader4);
        BOOST_CHECK_EQUAL(headers4.FindFirst("key"), "value");
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
    res.m_version_major = 1;
    res.m_version_minor = 1;
    res.m_status = HTTP_OK;
    res.m_reason = HTTPStatusReasonString(res.m_status);
    std::span<const std::byte> result{StringToBytes(R"({"result":865793,"error":null,"id":null})")};
    res.m_body.assign(result.begin(), result.end());
    res.m_headers = std::move(headers);
    BOOST_CHECK_EQUAL(
        res.StringifyHeaders(),
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 41\r\n"
        "\r\n");
}
BOOST_AUTO_TEST_SUITE_END()
