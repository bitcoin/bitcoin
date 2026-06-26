// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httpserver.h>
#include <rpc/protocol.h>
#include <test/util/common.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <util/string.h>
#include <util/threadpool.h>

#include <boost/test/unit_test.hpp>

using http_bitcoin::GetQueryParameterFromUri;
using http_bitcoin::HTTPHeaders;
using http_bitcoin::HTTPRemoteClient;
using http_bitcoin::HTTPRequest;
using http_bitcoin::HTTPResponse;
using http_bitcoin::HTTPServer;
using http_bitcoin::MAX_BODY_SIZE;
using http_bitcoin::MAX_HEADERS_SIZE;
using util::LineReader;

// HTTP request captured from bitcoin-cli
constexpr std::string_view full_request = "POST / HTTP/1.1\r\n"
                                          "Host: 127.0.0.1\r\n"
                                          "Connection: close\r\n"
                                          "Content-Type: application/json\r\n"
                                          "Authorization: Basic X19jb29raWVfXzo5OGQ5ODQ3MWNmNjg0NzAzYTkzN2EzNzk0ZDFlODQ1NjZmYTRkZjJiMzFkYjhhODI4ZGY4MjVjOTg5ZGI4OTVl\r\n"
                                          "Content-Length: 46\r\n"
                                          "\r\n"
                                          R"({"method":"getblockcount","params":[],"id":1})""\n";

BOOST_FIXTURE_TEST_SUITE(httpserver_tests, SocketTestingSetup)

BOOST_AUTO_TEST_CASE(test_query_parameters)
{
    std::string uri {};

    // Tolerate a URI with invalid characters (% not followed by hex digits)
    uri = "/rest/endpoint/someresource.json?p1=v1&p2=v2%";
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri, "p1"), "v1");
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri, "p2"), "v2%");

    // No parameters
    uri = "localhost:8080/rest/headers/someresource.json";
    BOOST_CHECK(!GetQueryParameterFromUri(uri, "p1"));

    // Single parameter
    uri = "localhost:8080/rest/endpoint/someresource.json?p1=v1";
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri, "p1"), "v1");
    BOOST_CHECK(!GetQueryParameterFromUri(uri, "p2"));

    // Multiple parameters
    uri = "/rest/endpoint/someresource.json?p1=v1&p2=v2";
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri, "p1"), "v1");
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri, "p2"), "v2");

    // If the query string contains duplicate keys, the first value is returned
    uri = "/rest/endpoint/someresource.json?p1=v1&p1=v2";
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri, "p1"), "v1");

    // Invalid query string syntax is the same as not having parameters
    uri = "/rest/endpoint/someresource.json&p1=v1&p2=v2";
    BOOST_CHECK(!GetQueryParameterFromUri(uri, "p1"));

    // Multiple parameters, some characters encoded
    uri = "/rest/endpoint/someresource.json?p1=v1%20&p2=100%25";
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri, "p1"), "v1 ");
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri, "p2"), "100%");

    // Encoded query delimiters are part of the parameter value, not structure.
    uri = "/rest/endpoint/someresource.json?p=a%26b%3Dc%23frag&other=x";
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri, "p"), "a&b=c#frag");
    BOOST_CHECK_EQUAL(GetQueryParameterFromUri(uri, "other"), "x");

    // An encoded question mark in the path does not introduce a query section.
    uri = "/rest/endpoint/someresource.json%3Fp1%3Dv1%26p2%3D100%25";
    BOOST_CHECK(!GetQueryParameterFromUri(uri, "p1"));
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
        HTTPRequest req;
        LineReader reader(full_request, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(req.LoadBody(reader));
        BOOST_CHECK_EQUAL(req.m_method, HTTPRequestMethod::POST);
        BOOST_CHECK_EQUAL(req.GetRequestMethod(), HTTPRequestMethod::POST);
        BOOST_CHECK_EQUAL(req.m_target, "/");
        BOOST_CHECK_EQUAL(req.GetURI(), "/");
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
        BOOST_CHECK_EQUAL(req.m_method, HTTPRequestMethod::GET);
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
    {
        // Support "chunked" transfer. Chunk lengths are ascii-encoded hex integers, whitespace ignored
        HTTPRequest req;
        std::string_view ok_chunked = "GET / HTTP/1.0\n"
                                      "Transfer-Encoding: chunked\n"
                                      "\n"
                                      "10\n"
                                      R"({"method":"getbl)""\n"
                                      "   a    \n"
                                      R"(ockcount"})""\n"
                                      "0\n"
                                      "\n";
        LineReader reader(ok_chunked, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(req.LoadBody(reader));
        BOOST_CHECK_EQUAL(req.m_body, R"({"method":"getblockcount"})");
    }
    {
        // Prevent "chunked" transfer from exceeding size limit
        HTTPRequest req;
        std::string_view excessive_chunk_size = "GET / HTTP/1.0\n"
                                                "Transfer-Encoding: chunked\n"
                                                "\n"
                                                "10\n"
                                                R"({"method":"getbl)""\n"
                                                "20000000\n"
                                                R"(ockcount"})""\n"
                                                "0\n"
                                                "\n";
        LineReader reader(excessive_chunk_size, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK_EXCEPTION(req.LoadBody(reader), http_bitcoin::ContentTooLargeError, HasReason{"Chunk will exceed max body size"});
    }
    {
        // Allow (but ignore) Chunk Extensions
        HTTPRequest req;
        std::string_view ok_chunked = "GET / HTTP/1.0\n"
                                      "Transfer-Encoding: chunked\n"
                                      "\n"
                                      "10;sha256=715790e8a3b09d704ac9641f42d183a5ebc5fd939663de23da548519ac2165e5\n"
                                      R"({"method":"getbl)""\n"
                                      "   a   ;    compressed\n"
                                      R"(ockcount"})""\n"
                                      "0;why;would;anyone;do;this;\n"
                                      "Expires: Wed, 21 Oct 2026 07:28:00 GMT\n"
                                      "\n";
        LineReader reader(ok_chunked, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(req.LoadBody(reader));
        BOOST_CHECK_EQUAL(req.m_body, R"({"method":"getblockcount"})");
        // Chunk Trailer was cleared
        BOOST_CHECK_EQUAL(reader.Remaining(), 0);
    }
    {
        // Invalid "chunked" transfer, using roman numerals instead of hex for chunk length
        HTTPRequest req;
        std::string_view invalid_chunked = "GET / HTTP/1.0\n"
                                           "Transfer-Encoding: chunked\n"
                                           "\n"
                                           "XVI\n"
                                           R"({"method":"getbl)""\n"
                                           "X\n"
                                           R"(ockcount"})""\n"
                                           "0\n"
                                           "\n";
        LineReader reader(invalid_chunked, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK_EXCEPTION(req.LoadBody(reader), std::runtime_error, HasReason{"Cannot parse chunk length value"});
    }
    {
        // Invalid "chunked" transfer, missing chunk termination \n
        HTTPRequest req;
        std::string_view invalid_chunked = "GET / HTTP/1.0\n"
                                           "Transfer-Encoding: chunked\n"
                                           "\n"
                                           "10\n"
                                           R"({"method":"getbl)"
                                           "a\n" // interpreted as extra data at the end of `0x10`-sized chunk
                                           R"(ockcount"})"
                                           "0\n"
                                           "\n";
        LineReader reader(invalid_chunked, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK_EXCEPTION(req.LoadBody(reader), std::runtime_error, HasReason{"Improperly terminated chunk"});
    }
    {
        // End of buffer reached without chunk termination, caller must wait for more data to arrive
        HTTPRequest req;
        std::string delayed_chunked = "GET / HTTP/1.0\n"
                                      "Transfer-Encoding: chunked\n"
                                      "\n"
                                      "10\n"
                                      R"({"method":"getbl)""\n"
                                      "a\n"
                                      R"(ockcount"})";
        LineReader reader1(delayed_chunked, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader1));
        BOOST_CHECK(req.LoadHeaders(reader1));
        BOOST_CHECK(!req.LoadBody(reader1));
        // more data arrives!
        delayed_chunked += "\n0\n\n";
        LineReader reader2(delayed_chunked, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader2));
        BOOST_CHECK(req.LoadHeaders(reader2));
        BOOST_CHECK(req.LoadBody(reader2));
    }
}

BOOST_AUTO_TEST_CASE(http_server_socket_tests)
{
    // Hard code the timestamp for the Date header in the HTTP response
    // Wed Dec 11 00:47:09 2024 UTC
    SetMockTime(1733878029);

    // Prepare a request handler that just stores received requests so we can examine them.
    // Mutex is required to prevent a race between this test's main thread and the server's I/O loop.
    Mutex requests_mutex;
    std::deque<std::unique_ptr<HTTPRequest>> requests;
    auto StoreRequest = [&](std::unique_ptr<HTTPRequest>&& req) {
        LOCK(requests_mutex);
        requests.push_back(std::move(req));
    };

    HTTPServer server{StoreRequest};

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

    // Start the I/O loop
    server.StartSocketsThreads();

    // No connections yet
    BOOST_CHECK_EQUAL(server.GetConnectionsCount(), 0);

    // Create a mock client with pre-loaded request data and add it to the local CreateSock queue.
    // Keep a handle for the mock client's send and receive pipes so we can examine
    // the data it "receives".
    std::shared_ptr<DynSock::Pipes> mock_client_socket_pipes{ConnectClient(std::as_bytes(std::span(full_request)))};

    // Wait up to a minute to find and connect the client in the I/O loop
    int attempts{6000};
    while (server.GetConnectionsCount() < 1) {
        std::this_thread::sleep_for(10ms);
        BOOST_REQUIRE(--attempts > 0);
    }

    // Prepare a pointer to the client, we'll assign it from the request itself.
    std::shared_ptr<HTTPRemoteClient> client;

    // Wait up to a minute to read the request from the client.
    // Given that the mock client is itself a mock socket
    // with hard-coded data it should only take a fraction of that.
    attempts = 6000;
    while (true) {
        {
            LOCK(requests_mutex);
            // Connected client should have one request already from the static content.
            if (requests.size() == 1) {
                // Check the received request
                BOOST_CHECK_EQUAL(requests.front()->m_body, R"({"method":"getblockcount","params":[],"id":1})""\n");
                BOOST_CHECK_EQUAL(requests.front()->GetPeer().ToStringAddrPort(), "5.5.5.5:6789");

                // Inspect the connection pointed to from the request
                client = requests.front()->m_client;
                BOOST_CHECK_EQUAL(client->m_origin, "5.5.5.5:6789");

                // Respond to request
                requests.front()->WriteReply(HTTP_OK, "874140\n");

                break;
            }
        }
        std::this_thread::sleep_for(10ms);
        BOOST_REQUIRE(--attempts > 0);
    }

    // Check the sent response from the mock client at the other end of the mock socket
    std::string actual;
    // Wait up to one minute for all the bytes to appear in the "send" pipe.
    char buf[0x10000] = {};
    attempts = 6000;
    while (attempts > 0)
    {
        ssize_t bytes_read = mock_client_socket_pipes->send.GetBytes(buf, sizeof(buf), 0);
        if (bytes_read > 0) {
            actual.append(buf, bytes_read);
            if (actual.length() == 146) {
                break;
            }
        }
        std::this_thread::sleep_for(10ms);
        --attempts;
    }
    BOOST_CHECK(actual.starts_with("HTTP/1.1 200 OK\r\n"));
    BOOST_CHECK(actual.ends_with("\r\n874140\n"));
    // Headers can be sorted in any order, and will be, since we use unordered_map
    BOOST_CHECK(actual.find("Connection: close\r\n") != std::string::npos);
    BOOST_CHECK(actual.find("Content-Length: 7\r\n") != std::string::npos);
    BOOST_CHECK(actual.find("Content-Type: text/html; charset=ISO-8859-1\r\n") != std::string::npos);
    BOOST_CHECK(actual.find("Date: Wed, 11 Dec 2024 00:47:09 GMT\r\n") != std::string::npos);

    // Wait up to one minute for connection to be automatically closed, because
    // keep-alive was not set by the client and we are done responding to their request.
    attempts = 6000;
    while (server.GetConnectionsCount() != 0) {
        std::this_thread::sleep_for(10ms);
        BOOST_REQUIRE(--attempts > 0);
    }

    // Stop the I/O loop and shutdown
    server.InterruptNet();
    // Wait for I/O loop to finish, after all connected sockets are closed
    server.JoinSocketsThreads();
    // Close all listening sockets
    server.StopListening();
}

BOOST_AUTO_TEST_CASE(http_socket_error_tests)
{
    // Create a tiny threadpool for the HTTPRequest handler
    ThreadPool workers("http");
    workers.Start(1);

    // Hard-code the server's request handler to respond to each request with
    // an incremented block count. Handle the replies in the worker thread.
    std::atomic<int> height{0};
    HTTPServer server{[&](std::shared_ptr<HTTPRequest> req) {
        auto item = [req, &height]() {
            const int h = height.fetch_add(1);
            req->WriteReply(HTTP_OK, strprintf("height: %d\n", h));
        };
        // Can't call BOOST_REQUIRE from worker thread
        Assert(workers.Submit(std::move(item)));
    }};

    // All replies will be the same size
    static constexpr std::size_t reply_length = std::string_view{
        "HTTP/1.1 200 OK\r\n"
        "Date: Thu, 01 Jan 2026 00:00:00 GMT\r\n"           // All RFC1123 dates are 29 characters
        "Content-Length: 10\r\n"
        "Content-Type: text/html; charset=ISO-8859-1\r\n"
        "\r\n"
        "height: 0\n"
    }.size();

    /**
     * A mocked Sock derived from DynSock whose Send() only succeeds when there is more than
     * one reply being sent (send buffer length > reply_length). Otherwise it returns
     * a recoverable error (WSAEAGAIN).
     *
     * After it sends successfully once, it continues to always succeed.
     *
     * Useful for testing "try again" logic around non-blocking socket Send() failures.
     */
    class ErrorSock : public DynSock
    {
    public:
        explicit ErrorSock(std::shared_ptr<Pipes> pipes) : DynSock{std::move(pipes)} {}
        DynSock& operator=(Sock&&) override { assert(false); return *this; }

        ssize_t Send(const void* buf, size_t len, int flags) const override
        {
            if (len <= reply_length && !m_have_sent) {
                #ifdef WIN32
                WSASetLastError(WSAEWOULDBLOCK);
                #else
                errno = WSAEAGAIN;
                #endif
                return -1;
            } else {
                m_have_sent = true;
                return DynSock::Send(buf, len, flags);
            }
        }

        mutable bool m_have_sent{false};
    };

    // Simpler server startup than the last test
    CService addr_bind{Lookup("0.0.0.0", /*portDefault=*/0, /*fAllowLookup=*/false).value()};
    BOOST_REQUIRE(server.BindAndStartListening(addr_bind));
    server.StartSocketsThreads();

    // Prepare initial requests
    int num_requests = 2;
    // Use keep-alive so the server holds the connection open for all requests.
    std::string keepalive_request{full_request};
    keepalive_request.replace(keepalive_request.find("Connection: close"), 17, "Connection: keep-alive");
    // Combine all requests so they are read from the socket on a single iteration of the I/O loop
    std::string all_requests;
    for (int i = 0; i < num_requests; i++) {
        all_requests += keepalive_request;
    }

    // Watch the log messages to ensure that the first two replies were sent
    // together. This indicates the non-optimistic send path was used
    // because a reply was already sitting in the send buffer when a second reply
    // was added.
    DebugLogHelper find_two_replies{strprintf("Sent %d bytes to client", reply_length * 2),
                                    [&](const std::string* s) {
                                        return true;
                                    }};
    // Last reply should be sent on its own by optimistic send path, because
    // the send buffer was empty when the reply was written.
    DebugLogHelper find_one_reply{strprintf("Sent %d bytes to client", reply_length),
                                  [&](const std::string* s) {
                                      return true;
                                   }};

    // Connect the ErrorSock as mock client with the preloaded data and get a handle on the I/O pipes
    std::shared_ptr<ErrorSock::Pipes> mock_client_socket_pipes{
        ConnectClient<ErrorSock>(std::as_bytes(std::span(all_requests)))
    };

    // Wait up to one minute for the last reply from the server
    std::string actual;
    char buf[0x10000] = {};
    int attempts = 6000;
    while (attempts > 0)
    {
        ssize_t bytes_read = mock_client_socket_pipes->send.GetBytes(buf, sizeof(buf), 0);
        if (bytes_read > 0) {
            actual.append(buf, bytes_read);
            if (actual.find(strprintf("height: %d", num_requests - 1)) != std::string::npos) {
                break;
            }
        }
        std::this_thread::sleep_for(10ms);
        --attempts;
    }

    // Send the third request.
    // If there was a race between WriteReply() in the worker thread setting m_send_ready=true
    // and SocketHandlerConnected() in the I/O thread flushing the send buffer,
    // then the socket would be stuck in write mode with nothing to write,
    // the server would never read from the socket, and this request would time out.
    // Wait a second to ensure both the worker thread and I/O thread are idle.
    // If we send the next request too soon it might get accepted by the server before
    // it gets wedged shut.
    std::this_thread::sleep_for(1000ms);
    mock_client_socket_pipes->recv.PushBytes(keepalive_request.data(), keepalive_request.size());
    num_requests++;

    // Wait up to one minute for reply
    attempts = 6000;
    while (attempts > 0)
    {
        ssize_t bytes_read = mock_client_socket_pipes->send.GetBytes(buf, sizeof(buf), 0);
        if (bytes_read > 0) {
            actual.append(buf, bytes_read);
            if (actual.find(strprintf("height: %d", num_requests - 1)) != std::string::npos) {
                break;
            }
        }
        std::this_thread::sleep_for(10ms);
        --attempts;
    }

    // All replies were received
    for (int i = 0; i < num_requests; i++) {
        BOOST_REQUIRE(actual.find(strprintf("height: %d", i)) != std::string::npos);
    }

    // Close the keep-alive connection
    server.DisconnectAllClients();

    workers.Stop();

    server.InterruptNet();
    server.JoinSocketsThreads();
    server.StopListening();
}

BOOST_AUTO_TEST_SUITE_END()
