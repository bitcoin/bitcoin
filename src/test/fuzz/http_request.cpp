// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
    using http_bitcoin::HTTPRequest;
    using http_bitcoin::MAX_HEADERS_SIZE;
    using util::LineReader;

    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<std::byte> http_buffer{ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider, 4096)};

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
    // replaced with http_bitcoin::HTTPRequest (#35182), ReadBody() always returned an
    // empty string here; LoadBody now populates the body per RFC 9112 framing, so mirror
    // its branch logic to assert the body matches the framing that produced it.
    const std::string body = http_request.ReadBody();
    const auto [has_transfer_encoding, transfer_encoding] = http_request.GetHeader("Transfer-Encoding");
    const auto [has_content_length, content_length] = http_request.GetHeader("Content-Length");
    if (has_transfer_encoding && ToLower(transfer_encoding) == "chunked") {
        // A chunked body is the concatenation of the decoded chunks, bounded by MAX_BODY_SIZE.
        assert(body.size() <= http_bitcoin::MAX_BODY_SIZE);
    } else if (has_content_length) {
        // A Content-Length body is exactly that many bytes.
        const auto parsed_length{ToIntegral<uint64_t>(content_length)};
        assert(parsed_length);
        assert(body.size() == *parsed_length);
    } else {
        // Absent both framing headers there is no body.
        assert(body.empty());
    }
}
