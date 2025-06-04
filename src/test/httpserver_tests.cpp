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
using http_bitcoin::HTTPRequest;
using http_bitcoin::HTTPResponse;
using http_bitcoin::HTTPServer;
using http_bitcoin::MAX_BODY_SIZE;
using http_bitcoin::MAX_HEADERS_SIZE;
using util::LineReader;

BOOST_FIXTURE_TEST_SUITE(httpserver_tests, SocketTestingSetup)

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

BOOST_AUTO_TEST_CASE(http_request_tests)
{
    {
        // Reading request captured from bitcoin-cli
        constexpr std::string_view full_request = "POST / HTTP/1.1\r\n"
                                                  "Host: 127.0.0.1\r\n"
                                                  "Connection: close\r\n"
                                                  "Content-Type: application/json\r\n"
                                                  "Authorization: Basic X19jb29raWVfXzo5OGQ5ODQ3MWNmNjg0NzAzYTkzN2EzNzk0ZDFlODQ1NjZmYTRkZjJiMzFkYjhhODI4ZGY4MjVjOTg5ZGI4OTVl\r\n"
                                                  "Content-Length: 46\r\n"
                                                  "\r\n"
                                                  R"({"method":"getblockcount","params":[],"id":1})""\n";
        HTTPRequest req;
        LineReader reader(full_request, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(req.LoadBody(reader));
        BOOST_CHECK_EQUAL(req.m_method, "POST");
        BOOST_CHECK_EQUAL(req.m_target, "/");
        BOOST_CHECK_EQUAL(req.m_version.major, 1);
        BOOST_CHECK_EQUAL(req.m_version.minor, 1);
        BOOST_CHECK_EQUAL(req.m_headers.FindFirst("Host"), "127.0.0.1");
        BOOST_CHECK_EQUAL(req.m_headers.FindFirst("Connection"), "close");
        BOOST_CHECK_EQUAL(req.m_headers.FindFirst("Content-Type"), "application/json");
        BOOST_CHECK_EQUAL(req.m_headers.FindFirst("Authorization"), "Basic X19jb29raWVfXzo5OGQ5ODQ3MWNmNjg0NzAzYTkzN2EzNzk0ZDFlODQ1NjZmYTRkZjJiMzFkYjhhODI4ZGY4MjVjOTg5ZGI4OTVl");
        BOOST_CHECK_EQUAL(req.m_headers.FindFirst("Content-Length"), "46");
        BOOST_CHECK_EQUAL(req.m_body.size(), 46);
        BOOST_CHECK_EQUAL(req.m_body, R"({"method":"getblockcount","params":[],"id":1})""\n");
    }
    {
        // Malformed: no spaces between data
        HTTPRequest req;
        LineReader reader("GET/HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP request line too short"});
    }
    {
        // Malformed: too many spaces
        HTTPRequest req;
        LineReader reader("GET / HTTP / 1.0\r\nHost: 127.0.0.1\r\n\r\n", MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP request line malformed"});
    }
    {
        // Malformed: slash missing before version
        HTTPRequest req;
        LineReader reader("GET / HTTP1.0\r\nHost: 127.0.0.1\r\n\r\n", MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP request line too short"});
    }
    {
        // Malformed: no decimal in version
        HTTPRequest req;
        LineReader reader("GET / HTTP/11\r\nHost: 127.0.0.1\r\n\r\n", MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP request line too short"});
    }
    {
        // Malformed: version is not a number
        HTTPRequest req;
        LineReader reader("GET / HTTP/1.x\r\nHost: 127.0.0.1\r\n\r\n", MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP bad version"});
    }
    {
        // Malformed: version is out of range
        HTTPRequest req;
        LineReader reader("GET / HTTP/2.0\r\nHost: 127.0.0.1\r\n\r\n", MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP bad version"});
    }
    {
        // Malformed: version is out of range
        HTTPRequest req;
        LineReader reader("GET / HTTP/0.9\r\nHost: 127.0.0.1\r\n\r\n", MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP bad version"});
    }
    {
        // Malformed: version is out of range
        HTTPRequest req;
        LineReader reader("GET / HTTP/-1.0\r\nHost: 127.0.0.1\r\n\r\n", MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP bad version"});
    }
    {
        // Malformed: version is not exactly two integers and a dot
        HTTPRequest req;
        LineReader reader("GET / HTTP/1.00\r\nHost: 127.0.0.1\r\n\r\n", MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP bad version"});
    }
    {
        // Malformed: contains NUL
        HTTPRequest req;
        LineReader reader{std::string_view{"GET /safe\0/etc/passwd HTTP/1.00\r\nHost: 127.0.0.1\r\n\r\n", 50}, MAX_HEADERS_SIZE};
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"Invalid request line contains NUL"});
    }
    {
        // Malformed: differing Content-Length values, case insensitive
        constexpr std::string_view differing_length = "GET / HTTP/1.1\n"
                                                      "Host: 127.0.0.1\n"
                                                      "Content-Length: 8\n"
                                                      "content-length: 9\n\n"
                                                      "12345678";
        HTTPRequest req;
        util::LineReader reader{differing_length, /*max_line_length=*/MAX_HEADERS_SIZE};
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK_EXCEPTION(req.LoadBody(reader), std::runtime_error, HasReason{"Differing Content-Length values"});
    }
    {
        // Ok: multiple same Content-Length values
        constexpr std::string_view differing_length = "GET / HTTP/1.1\n"
                                                      "Host: 127.0.0.1\n"
                                                      "Content-Length: 8\n"
                                                      "content-length: 8\n\n"
                                                      "12345678";
        HTTPRequest req;
        util::LineReader reader{differing_length, /*max_line_length=*/MAX_HEADERS_SIZE};
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(req.LoadBody(reader));
    }
    {
        // Ok
        HTTPRequest req;
        LineReader reader("GET / HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n", MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(req.LoadBody(reader));
        BOOST_CHECK_EQUAL(req.m_method, "GET");
        BOOST_CHECK_EQUAL(req.m_target, "/");
        BOOST_CHECK_EQUAL(req.m_version.major, 1);
        BOOST_CHECK_EQUAL(req.m_version.minor, 0);
        BOOST_CHECK_EQUAL(req.m_headers.FindFirst("Host"), "127.0.0.1");
        // no body is OK
        BOOST_CHECK_EQUAL(req.m_body.size(), 0);
    }
    {
        // Malformed: missing colon
        HTTPRequest req;
        LineReader reader("GET / HTTP/1.0\r\nHost=127.0.0.1\r\n\r\n", MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK_EXCEPTION(req.LoadHeaders(reader), std::runtime_error, HasReason{"HTTP header missing colon (:)"});
    }
    {
        // We might not have received enough data from the client which is not
        // an error. We return false so the caller can try again later when the
        // buffer has more data.
        HTTPRequest req;
        LineReader reader("GET / HTTP/1.0\r\nHost: ", MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(!req.LoadHeaders(reader));
    }
    {
        // No Content-Length: body is not read
        HTTPRequest req;
        LineReader reader("GET / HTTP/1.0\r\n\r\n" R"({"method":"getblockcount"})", MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(req.LoadBody(reader));
        // Don't try to read request body if Content-Length is missing
        BOOST_CHECK_EQUAL(req.m_body.size(), 0);
    }
    {
        // Malformed: Content-Length is not a number
        HTTPRequest req;
        LineReader reader("GET / HTTP/1.0\r\nContent-Length: eleven\r\n\r\n" R"({"method":"getblockcount"})", MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK_EXCEPTION(req.LoadBody(reader), std::runtime_error, HasReason{"Cannot parse Content-Length value"});
    }
    {
        // Malformed: Content-Length is negative
        HTTPRequest req;
        LineReader reader("GET / HTTP/1.0\r\nContent-Length: -8\r\n\r\n" R"({"method":"getblockcount"})", MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK_EXCEPTION(req.LoadBody(reader), std::runtime_error, HasReason{"Cannot parse Content-Length value"});
    }
    {
        // Content-Length exceeds limit
        constexpr auto excessive_size{MAX_BODY_SIZE + 1};
        std::string huge_body(excessive_size, 'x');
        const std::string request{"GET / HTTP/1.0\r\nContent-Length: " + util::ToString(excessive_size) + "\r\n\r\n" + std::move(huge_body)};
        HTTPRequest req;
        LineReader reader(request, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK_EXCEPTION(req.LoadBody(reader), http_bitcoin::ContentTooLargeError, HasReason{"Max body size exceeded"});
    }
    {
        // Content-Length exactly on the limit
        std::string max_body(MAX_BODY_SIZE, 'x');
        const std::string request{"GET / HTTP/1.0\r\nContent-Length: " + util::ToString(MAX_BODY_SIZE) + "\r\n\r\n" + std::move(max_body)};
        HTTPRequest req;
        LineReader reader(request, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(req.LoadBody(reader));
    }
    {
        // Content-Length indicates more data than we have in the buffer.
        // Not an error; we wait for more data before completing the body.
        HTTPRequest req;
        LineReader reader("GET / HTTP/1.0\r\nContent-Length: 1024\r\n\r\n" R"({"method":"getblockcount"})", MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(!req.LoadBody(reader));
    }
}

BOOST_AUTO_TEST_CASE(http_server_socket_tests)
{
    HTTPServer server;

    {
        // We can only bind to NET_IPV4 and NET_IPV6
        CService onion_address{Lookup("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaam2dqd.onion", /*portDefault=*/0, /*fAllowLookup=*/false).value()};
        auto result{server.BindAndStartListening(onion_address)};
        BOOST_REQUIRE(!result);
        BOOST_CHECK_EQUAL(result.error(), "Bind address family for aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaam2dqd.onion:0 not supported");
    }

    // This VALID address won't actually get used because we stubbed CreateSock()
    CService addr_bind{Lookup("0.0.0.0", /*portDefault=*/0, /*fAllowLookup=*/false).value()};

    // Init state
    BOOST_REQUIRE_EQUAL(server.GetListeningSocketCount(), 0);
    // Bind to mock Listening Socket
    BOOST_REQUIRE(server.BindAndStartListening(addr_bind));
    // We are bound and listening
    BOOST_REQUIRE_EQUAL(server.GetListeningSocketCount(), 1);

    // Pick up the phone, there's no one there
    CService addr_connection;
    BOOST_REQUIRE(!server.AcceptConnectionFromListeningSocket(addr_connection));

    // Create a mock client and add it to the local CreateSock queue
    ConnectClient();
    // Accept the connection
    BOOST_REQUIRE(server.AcceptConnectionFromListeningSocket(addr_connection));
    BOOST_CHECK_EQUAL(addr_connection.ToStringAddrPort(), "5.5.5.5:6789");
}
BOOST_AUTO_TEST_SUITE_END()
