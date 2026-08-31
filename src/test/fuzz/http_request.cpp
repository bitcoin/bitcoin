// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httpserver.h>
#include <netaddress.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/net.h>
#include <test/util/time.h>
#include <util/check.h>
#include <util/signalinterrupt.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
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

//! Expose HTTPRemoteClient with the receive buffer.
class FuzzClient : public HTTPRemoteClient
{
public:
    FuzzClient() : HTTPRemoteClient{/*id=*/0, /*addr=*/CService(), /*socket=*/std::make_unique<ZeroSock>()} {}
    void Receive(std::string_view s) { MutateRecvBuffer().append(s); }
};

int StateRank(HTTPRequest::State s)
{
    switch (s) {
    case HTTPRequest::State::Init: return 0;
    case HTTPRequest::State::NeedsHeaders: return 1;
    case HTTPRequest::State::NeedsBody: return 2;
    case HTTPRequest::State::Complete: return 3;
    case HTTPRequest::State::Error: return 4;
    }
    assert(false);
}

struct ParsedRequest {
    HTTPRequestMethod method;
    std::string uri;
    std::string host;
    std::string body;
    bool operator==(const ParsedRequest&) const = default;
};

struct RunResult {
    std::vector<ParsedRequest> requests;
    bool errored{false};
    std::string remainder;
    HTTPRequest::State last_state{HTTPRequest::State::Init};
};

//! Take every request the client can parse right now, checking what must hold
//! at each I/O cycle boundary.
void Drain(const std::shared_ptr<FuzzClient>& client, RunResult& out)
{
    while (true) {
        // A failed request is inert: further reads consume nothing.
        if (const HTTPRequest* cur{client->GetRequest()};
            cur && cur->GetState() == HTTPRequest::State::Error) {
            const size_t buffered{client->GetRecvBuffer().size()};
            Assert(HTTPRemoteClient::TryReadRequest(client) == nullptr);
            assert(client->GetRecvBuffer().size() == buffered);
            out.errored = true;
            break;
        }

        std::unique_ptr<HTTPRequest> req{HTTPRemoteClient::TryReadRequest(client)};

        if (!req) {
            if (const HTTPRequest* cur{client->GetRequest()}) {
                // Complete is always handed back, never left behind.
                assert(cur->GetState() != HTTPRequest::State::Complete);
                assert(StateRank(cur->GetState()) >= StateRank(out.last_state));
                out.last_state = cur->GetState();
                if (cur->GetState() == HTTPRequest::State::Error) out.errored = true;
                if (const auto chunk_size{cur->GetChunkSize()}) {
                    assert(cur->GetChunkProgress() <= *chunk_size);
                }
                assert(cur->ReadBody().size() <= MAX_BODY_SIZE);
            }
            break;
        }

        assert(req->GetState() == HTTPRequest::State::Complete);
        CheckBodyMatchesFraming(*req);
        out.requests.emplace_back(req->GetRequestMethod(), req->GetURI(),
                                  req->GetHeader("Host").value_or(""), req->ReadBody());

        // While a request is with a worker, nothing new is parsed or consumed.
        const size_t buffered{client->GetRecvBuffer().size()};
        Assert(HTTPRemoteClient::TryReadRequest(client) == nullptr);
        assert(client->GetRecvBuffer().size() == buffered);

        req->WriteReply(HTTP_OK, ""); // clears m_req_busy
        out.last_state = HTTPRequest::State::Init;
    }
}

//! HTTP framing is defined by the byte stream, not by how TCP happened to split
//! it, so parsing the same bytes whole and in pieces must give the same result.
//! Only the piecewise run reaches the resumable parse paths.
void CheckSegmentationIndependence(const std::string& input, FuzzedDataProvider& provider)
{
    // WriteReply() stamps a wall-clock Date header and the client stamps a
    // steady-clock idle time, both of which the fuzz determinism check rejects.
    FakeNodeClock clock{1610000000s};
    FakeSteadyClock steady_clock;

    // The whole stream arrives in one I/O cycle. This is similar to
    // SingleShotParse(), although here we pipe it through the client and also
    // parse multiple requests.
    RunResult one_shot;
    {
        auto client{std::make_shared<FuzzClient>()};
        client->Receive(input);
        Drain(client, one_shot);
        one_shot.remainder = client->GetRecvBuffer();
    }

    // The same stream arrives in arbitrary pieces. Once the provider runs dry
    // this becomes one byte per cycle, the maximally fragmented case.
    RunResult sliced;
    {
        auto client{std::make_shared<FuzzClient>()};
        std::string_view remaining{input};
        while (!remaining.empty()) {
            const size_t n{provider.ConsumeIntegralInRange<size_t>(1, remaining.size())};
            client->Receive(remaining.substr(0, n));
            remaining = remaining.substr(n);
            Drain(client, sliced);
        }
        sliced.remainder = client->GetRecvBuffer();
    }

    assert(one_shot.requests == sliced.requests);
    assert(one_shot.errored == sliced.errored);
    // The whole receive buffer is discarded on a parse error, so in the sliced case
    // what is left over depends on how much had arrived when the error fired.
    if (!one_shot.errored) assert(one_shot.remainder == sliced.remainder);
}

} // namespace

FUZZ_TARGET(http_request)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    // MAX_HEADERS_SIZE is 8192: leave room for a headers section that can
    // reach the limit, plus a body.
    const std::string http_buffer{fuzzed_data_provider.ConsumeRandomLengthString(2 * MAX_HEADERS_SIZE)};

    SingleShotParse(http_buffer, fuzzed_data_provider);
    CheckSegmentationIndependence(http_buffer, fuzzed_data_provider);
}
