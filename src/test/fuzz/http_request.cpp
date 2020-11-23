// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httpserver.h>
#include <netaddress.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

extern "C" int evhttp_parse_firstline_(struct evhttp_request*, struct evbuffer*);
extern "C" int evhttp_parse_headers_(struct evhttp_request*, struct evbuffer*);
std::string RequestMethodString(HTTPRequest::RequestMethod m);

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    evhttp_request* evreq = evhttp_request_new(nullptr, nullptr);
    assert(evreq != nullptr);
    evreq->kind = EVHTTP_REQUEST;
    evbuffer* evbuf = evbuffer_new();
    assert(evbuf != nullptr);
    const std::vector<uint8_t> http_buffer = ConsumeRandomLengthByteVector(fuzzed_data_provider, 4096);
    evbuffer_add(evbuf, http_buffer.data(), http_buffer.size());
    if (evhttp_parse_firstline_(evreq, evbuf) != 1 || evhttp_parse_headers_(evreq, evbuf) != 1) {
        evbuffer_free(evbuf);
        evhttp_request_free(evreq);
        return;
    }

    HTTPRequest http_request{evreq, true};
    const HTTPRequest::RequestMethod request_method = http_request.GetRequestMethod();
    (void)RequestMethodString(request_method);
    (void)http_request.GetURI();
    (void)http_request.GetHeader("Host");
    const std::string header = fuzzed_data_provider.ConsumeRandomLengthString(16);
    (void)http_request.GetHeader(header);
    (void)http_request.WriteHeader(header, fuzzed_data_provider.ConsumeRandomLengthString(16));
    (void)http_request.GetHeader(header);
    const std::string body = http_request.ReadBody();
    assert(body.empty());
    const CService service = http_request.GetPeer();
    assert(service.ToString() == "[::]:0");

    evbuffer_free(evbuf);
    evhttp_request_free(evreq);
}
