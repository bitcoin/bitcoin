// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/url.h>
#include <httpserver.h>
#include <netaddress.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/signalinterrupt.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>


std::string_view RequestMethodString(HTTPRequestMethod m);

FUZZ_TARGET(http_request)
{
    using util::LineReader;
    using namespace bitcoin_http;

    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::string http_buffer{fuzzed_data_provider.ConsumeRandomLengthString(4096)};

    HTTPRequest http_request;
    LineReader reader(http_buffer, MAX_HEADERS_SIZE);
    try {
        if (!http_request.LoadControlData(reader)) return;
        if (!http_request.LoadHeaders(reader)) return;
        if (!http_request.LoadBody(reader)) return;
    } catch (const std::runtime_error&) {
        return;
    }

    const HTTPRequestMethod request_method = http_request.GetRequestMethod();
    (void)RequestMethodString(request_method);
    (void)http_request.GetURI();
    (void)http_request.GetHeader("Host");
    std::string header = fuzzed_data_provider.ConsumeRandomLengthString(16);
    (void)http_request.GetHeader(header);
    (void)http_request.WriteHeader(std::string(header), fuzzed_data_provider.ConsumeRandomLengthString(16));
    (void)http_request.GetHeader(header);
    // Reaching here means LoadControlData/LoadHeaders/LoadBody all succeeded, so the
    // parsed body must be consistent with the message framing. Before libevent was
    // replaced with HTTPRequest (#35182), ReadBody() always returned an
    // empty string here; LoadBody now populates the body per RFC 9112 framing, so mirror
    // its branch logic to assert the body matches the framing that produced it.
    const std::string body = http_request.ReadBody();
    const auto transfer_encoding = http_request.GetHeader("Transfer-Encoding");
    const auto content_length = http_request.GetHeader("Content-Length");
    if (transfer_encoding && ToLower(*transfer_encoding) == "chunked") {
        // A chunked body is the concatenation of the decoded chunks, bounded by MAX_BODY_SIZE.
        assert(body.size() <= MAX_BODY_SIZE);
    } else if (content_length) {
        // A Content-Length body is exactly that many bytes.
        const auto parsed_length{ToIntegral<uint64_t>(*content_length)};
        assert(parsed_length);
        assert(body.size() == *parsed_length);
    } else {
        // Absent both framing headers there is no body.
        assert(body.empty());
    }
}

FUZZ_TARGET(http_query_parameter)
{
    using http_bitcoin::GetQueryParameterFromUri;

    // GetQueryParameterFromUri() parses the query string of a request target
    // supplied by a remote client. It is reached from the REST interface
    // (see rest.cpp) for parameters such as "count", "offset" and "verbose".
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::string uri{fuzzed_data_provider.ConsumeRandomLengthString(1024)};
    const std::string key{fuzzed_data_provider.ConsumeRandomLengthString(64)};
    const std::string value{fuzzed_data_provider.ConsumeRandomLengthString(1024)};
    (void)GetQueryParameterFromUri(uri, key);

    const std::string encoded_key{UrlEncode(key)};
    const std::string encoded_value{UrlEncode(value)};
    const std::string query_uri{"/endpoint?" + encoded_key + "=" + encoded_value};
    assert(GetQueryParameterFromUri(query_uri, key) == value);
    assert(GetQueryParameterFromUri("/endpoint?" + encoded_key, key) == "");
    assert(GetQueryParameterFromUri(query_uri + "&" + encoded_key + "=ignored", key) == value);
    assert(GetQueryParameterFromUri(query_uri + "#?" + encoded_key + "=ignored", key) == value);

    // First '?' must precede '#', and this name must not be key.
    const std::string dummy{key == "n" ? "m" : "n"};
    assert(!GetQueryParameterFromUri("/endpoint?" + dummy + "=1#?" + encoded_key + "=" + encoded_value, key));
}
