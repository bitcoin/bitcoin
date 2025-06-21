// Copyright (c) 2020-2022 The Bitcoin Core developers
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
    const std::vector<uint8_t> http_buffer = ConsumeRandomLengthByteVector(fuzzed_data_provider, 4096);
    const std::vector<std::byte> http_bytes_buffer(reinterpret_cast<const std::byte*>(http_buffer.data()),
                                                   reinterpret_cast<const std::byte*>(http_buffer.data()) + http_buffer.size());

    HTTPRequest http_request;
    LineReader reader(http_bytes_buffer, MAX_HEADERS_SIZE);
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
    const std::string header = fuzzed_data_provider.ConsumeRandomLengthString(16);
    (void)http_request.GetHeader(header);
    (void)http_request.WriteHeader(header, fuzzed_data_provider.ConsumeRandomLengthString(16));
    (void)http_request.GetHeader(header);
    const std::string body = http_request.ReadBody();
    assert(body.empty());
}
