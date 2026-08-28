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

using namespace bitcoin_http;

namespace {

//! A fully parsed request must have a body consistent with its framing headers.
//! Before libevent was replaced with HTTPRequest (#35182), ReadBody() always
//! returned an empty string; LoadBody() now populates the body per RFC 9112
//! framing, so mirror its branch logic here.
void CheckBodyMatchesFraming(const HTTPRequest& req)
{
    const std::string body{req.ReadBody()};
    const auto transfer_encoding{req.GetHeader("Transfer-Encoding")};
    const auto content_length{req.GetHeader("Content-Length")};
    if (transfer_encoding && ToLower(*transfer_encoding) == "chunked") {
        assert(body.size() <= MAX_BODY_SIZE);
    } else if (content_length) {
        const auto parsed_length{ToIntegral<uint64_t>(*content_length)};
        assert(parsed_length);
        assert(body.size() == *parsed_length);
    } else {
        assert(body.empty());
    }
}

//! Parse the whole buffer in a single pass, as one I/O cycle would.
void SingleShotParse(const std::string& http_buffer, FuzzedDataProvider& provider)
{
    using util::LineReader;

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
    std::string header = provider.ConsumeRandomLengthString(16);
    (void)http_request.GetHeader(header);
    (void)http_request.WriteHeader(std::string(header), provider.ConsumeRandomLengthString(16));
    (void)http_request.GetHeader(header);
    CheckBodyMatchesFraming(http_request);
}

} // namespace

FUZZ_TARGET(http_request)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::string http_buffer{fuzzed_data_provider.ConsumeRandomLengthString(4096)};

    SingleShotParse(http_buffer, fuzzed_data_provider);
}
