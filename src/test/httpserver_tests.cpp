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
using http_bitcoin::HTTPClient;
using http_bitcoin::HTTPHeaders;
using http_bitcoin::HTTPRequest;
using http_bitcoin::HTTPResponse;
using http_bitcoin::HTTPServer;
using http_bitcoin::MAX_HEADERS_SIZE;
using util::LineReader;

// HTTP request captured from bitcoin-cli
constexpr auto full_request{
    "504f5354202f20485454502f312e310d0a486f73743a203132372e302e302e310d"
    "0a436f6e6e656374696f6e3a20636c6f73650d0a436f6e74656e742d547970653a"
    "206170706c69636174696f6e2f6a736f6e0d0a417574686f72697a6174696f6e3a"
    "204261736963205831396a6232397261575666587a6f354f4751354f4451334d57"
    "4e6d4e6a67304e7a417a59546b7a4e32457a4e7a6b305a44466c4f4451314e6a5a"
    "6d5954526b5a6a4a694d7a466b596a68684f4449345a4759344d6a566a4f546735"
    "5a4749344f54566c0d0a436f6e74656e742d4c656e6774683a2034360d0a0d0a7b"
    "226d6574686f64223a22676574626c6f636b636f756e74222c22706172616d7322"
    "3a5b5d2c226964223a317d0a"_hex};

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

BOOST_AUTO_TEST_CASE(http_request_tests)
{
    {
        HTTPRequest req;
        LineReader reader(full_request, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(req.LoadBody(reader));
        BOOST_CHECK_EQUAL(req.m_method, "POST");
        BOOST_CHECK_EQUAL(req.m_target, "/");
        BOOST_CHECK_EQUAL(req.m_version_major, 1);
        BOOST_CHECK_EQUAL(req.m_version_minor, 1);
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
        std::span<const std::byte> too_short_request_line{StringToBytes("GET/HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n")};
        LineReader reader(too_short_request_line, MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP request line too short"});
    }
    {
        // Malformed: too many spaces
        HTTPRequest req;
        std::span<const std::byte> malformed_request_line{StringToBytes("GET / HTTP / 1.0\r\nHost: 127.0.0.1\r\n\r\n")};
        LineReader reader(malformed_request_line, MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP request line malformed"});
    }
    {
        // Malformed: slash missing before version
        HTTPRequest req;
        std::span<const std::byte> malformed_request_line{StringToBytes("GET / HTTP1.0\r\nHost: 127.0.0.1\r\n\r\n")};
        LineReader reader(malformed_request_line, MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP request line too short"});
    }
    {
        // Malformed: no decimal in version
        HTTPRequest req;
        std::span<const std::byte> malformed_request_line{StringToBytes("GET / HTTP/11\r\nHost: 127.0.0.1\r\n\r\n")};
        LineReader reader(malformed_request_line, MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP request line too short"});
    }
    {
        // Malformed: version is not a number
        HTTPRequest req;
        std::span<const std::byte> malformed_request_line{StringToBytes("GET / HTTP/1.x\r\nHost: 127.0.0.1\r\n\r\n")};
        LineReader reader(malformed_request_line, MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP bad version"});
    }
    {
        // Malformed: version is out of range
        HTTPRequest req;
        std::span<const std::byte> malformed_request_line{StringToBytes("GET / HTTP/2.0\r\nHost: 127.0.0.1\r\n\r\n")};
        LineReader reader(malformed_request_line, MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP bad version"});
    }
    {
        // Malformed: version is out of range
        HTTPRequest req;
        std::span<const std::byte> malformed_request_line{StringToBytes("GET / HTTP/-1.0\r\nHost: 127.0.0.1\r\n\r\n")};
        LineReader reader(malformed_request_line, MAX_HEADERS_SIZE);
        BOOST_CHECK_EXCEPTION(req.LoadControlData(reader), std::runtime_error, HasReason{"HTTP bad version"});
    }
    {
        HTTPRequest req;
        std::span<const std::byte> ok_request_line{StringToBytes("GET / HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n")};
        LineReader reader(ok_request_line, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(req.LoadBody(reader));
        BOOST_CHECK_EQUAL(req.m_method, "GET");
        BOOST_CHECK_EQUAL(req.m_target, "/");
        BOOST_CHECK_EQUAL(req.m_version_major, 1);
        BOOST_CHECK_EQUAL(req.m_version_minor, 0);
        BOOST_CHECK_EQUAL(req.m_headers.FindFirst("Host"), "127.0.0.1");
        // no body is OK
        BOOST_CHECK_EQUAL(req.m_body.size(), 0);
    }
    {
        HTTPRequest req;
        std::span<const std::byte> malformed_headers{StringToBytes("GET / HTTP/1.0\r\nHost=127.0.0.1\r\n\r\n")};
        LineReader reader(malformed_headers, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK_EXCEPTION(req.LoadHeaders(reader), std::runtime_error, HasReason{"HTTP header missing colon (:)"});
    }
    {
        // We might not have received enough data from the client which is not
        // an error. We return false so the caller can try again later when the
        // buffer has more data.
        HTTPRequest req;
        std::span<const std::byte> incomplete_headers{StringToBytes("GET / HTTP/1.0\r\nHost: ")};
        LineReader reader(incomplete_headers, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(!req.LoadHeaders(reader));
    }
    {
        HTTPRequest req;
        std::span<const std::byte> no_content_length{StringToBytes("GET / HTTP/1.0\r\n\r\n" R"({"method":"getblockcount"})")};
        LineReader reader(no_content_length, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(req.LoadBody(reader));
        // Don't try to read request body if Content-Length is missing
        BOOST_CHECK_EQUAL(req.m_body.size(), 0);
    }
    {
        // Malformed: Content-Length is not a number
        HTTPRequest req;
        std::span<const std::byte> bad_content_length{StringToBytes("GET / HTTP/1.0\r\nContent-Length: eleven\r\n\r\n" R"({"method":"getblockcount"})")};
        LineReader reader(bad_content_length, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK_EXCEPTION(req.LoadBody(reader), std::runtime_error, HasReason{"Cannot parse Content-Length value"});
    }
    {
        // Malformed: Content-Length is negative
        HTTPRequest req;
        std::span<const std::byte> bad_content_length{StringToBytes("GET / HTTP/1.0\r\nContent-Length: -8\r\n\r\n" R"({"method":"getblockcount"})")};
        LineReader reader(bad_content_length, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK_EXCEPTION(req.LoadBody(reader), std::runtime_error, HasReason{"Cannot parse Content-Length value"});
    }
    {
        // Content-Length exceeds limit
        constexpr std::size_t excessive_size = 32 * 1024 * 1024 + 1;
        std::string huge_body;
        huge_body.reserve(excessive_size);
        huge_body.append(excessive_size, 'x');
        std::string request = "GET / HTTP/1.0\r\nContent-Length: 33554433\r\n\r\n" + huge_body;
        HTTPRequest req;
        std::span<const std::byte> excessive_content_length{StringToBytes(request)};
        LineReader reader(excessive_content_length, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK_EXCEPTION(req.LoadBody(reader), std::runtime_error, HasReason{"Max body size exceeded"});
    }
    {
        // Content-Length indicates more data than we have in the buffer.
        // Not an error, we just try again later.
        HTTPRequest req;
        std::span<const std::byte> excessive_content_length{StringToBytes("GET / HTTP/1.0\r\nContent-Length: 1024\r\n\r\n" R"({"method":"getblockcount"})")};
        LineReader reader(excessive_content_length, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(!req.LoadBody(reader));
    }
    {
        // Support "chunked" transfer. Chunk lengths are ascii-encoded hex integers, whitespace ignored
        HTTPRequest req;
        std::span<const std::byte> ok_chunked{StringToBytes("GET / HTTP/1.0\n"
                                                            "Transfer-Encoding: chunked\n"
                                                            "\n"
                                                            "10\n"
                                                            R"({"method":"getbl)""\n"
                                                            "   a    \n"
                                                            R"(ockcount"})""\n"
                                                            "0\n"
                                                            "\n")};
        LineReader reader(ok_chunked, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK(req.LoadBody(reader));
        BOOST_CHECK_EQUAL(req.m_body, R"({"method":"getblockcount"})");
    }
    {
        // Prevent "chunked" transfer from exceeding size limit
        HTTPRequest req;
        std::span<const std::byte> ok_chunked{StringToBytes("GET / HTTP/1.0\n"
                                                            "Transfer-Encoding: chunked\n"
                                                            "\n"
                                                            "10\n"
                                                            R"({"method":"getbl)""\n"
                                                            "20000000\n"
                                                            R"(ockcount"})""\n"
                                                            "0\n"
                                                            "\n")};
        LineReader reader(ok_chunked, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK_EXCEPTION(req.LoadBody(reader), std::runtime_error, HasReason{"Chunk will exceed max body size"});
    }
    {
        // Allow (but ignore) Chunk Extensions
        HTTPRequest req;
        std::span<const std::byte> ok_chunked{StringToBytes("GET / HTTP/1.0\n"
                                                            "Transfer-Encoding: chunked\n"
                                                            "\n"
                                                            "10;sha256=715790e8a3b09d704ac9641f42d183a5ebc5fd939663de23da548519ac2165e5\n"
                                                            R"({"method":"getbl)""\n"
                                                            "   a   ;    compressed\n"
                                                            R"(ockcount"})""\n"
                                                            "0;why;would;anyone;do;this;\n"
                                                            "Expires: Wed, 21 Oct 2026 07:28:00 GMT\n"
                                                            "\n")};
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
        std::span<const std::byte> invalid_chunked{StringToBytes("GET / HTTP/1.0\n"
                                                                 "Transfer-Encoding: chunked\n"
                                                                 "\n"
                                                                 "XVI\n"
                                                                 R"({"method":"getbl)""\n"
                                                                 "X\n"
                                                                 R"(ockcount"})""\n"
                                                                 "0\n"
                                                                 "\n")};
        LineReader reader(invalid_chunked, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK_EXCEPTION(req.LoadBody(reader), std::runtime_error, HasReason{"Cannot parse chunk length value"});
    }
    {
        // Invalid "chunked" transfer, missing chunk termination \n
        HTTPRequest req;
        std::span<const std::byte> invalid_chunked{StringToBytes("GET / HTTP/1.0\n"
                                                                 "Transfer-Encoding: chunked\n"
                                                                 "\n"
                                                                 "10\n"
                                                                 R"({"method":"getbl)"
                                                                 "a\n"
                                                                 R"(ockcount"})"
                                                                 "0\n"
                                                                 "\n")};
        LineReader reader(invalid_chunked, MAX_HEADERS_SIZE);
        BOOST_CHECK(req.LoadControlData(reader));
        BOOST_CHECK(req.LoadHeaders(reader));
        BOOST_CHECK_EXCEPTION(req.LoadBody(reader), std::runtime_error, HasReason{"Improperly terminated chunk"});
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
        CService onion_address{Lookup("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaam2dqd.onion", 0, false).value()};
        auto result{server.BindAndStartListening(onion_address)};
        BOOST_REQUIRE(!result);
        BOOST_CHECK_EQUAL(result.error(), "Bind address family for aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaam2dqd.onion:0 not supported");
    }

    // This VALID address won't actually get used because we stubbed CreateSock()
    CService addr_bind{Lookup("0.0.0.0", 0, false).value()};

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
    std::shared_ptr<DynSock::Pipes> mock_client_socket_pipes{ConnectClient(full_request)};

    // Wait up to a minute to find and connect the client in the I/O loop
    int attempts{6000};
    while (server.GetConnectionsCount() < 1) {
        std::this_thread::sleep_for(10ms);
        BOOST_REQUIRE(--attempts > 0);
    }

    // Prepare a pointer to the client, we'll assign it from the request itself.
    std::shared_ptr<HTTPClient> client;

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

BOOST_AUTO_TEST_SUITE_END()
