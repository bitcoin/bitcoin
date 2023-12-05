// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_NET_H
#define BITCOIN_TEST_FUZZ_UTIL_NET_H

#include <net.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <node/connection_types.h>
#include <node/eviction.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/util.h>
#include <test/util/net.h>
#include <threadsafety.h>
#include <util/sock.h>

#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>

CNetAddr ConsumeNetAddr(FuzzedDataProvider& fuzzed_data_provider) noexcept;

class FuzzedSock : public Sock
{
    FuzzedDataProvider& m_fuzzed_data_provider;

    /**
     * Data to return when `MSG_PEEK` is used as a `Recv()` flag.
     * If `MSG_PEEK` is used, then our `Recv()` returns some random data as usual, but on the next
     * `Recv()` call we must return the same data, thus we remember it here.
     */
    mutable std::optional<uint8_t> m_peek_data;

    /**
     * Whether to pretend that the socket is select(2)-able. This is randomly set in the
     * constructor. It should remain constant so that repeated calls to `IsSelectable()`
     * return the same value.
     */
    const bool m_selectable;

public:
    explicit FuzzedSock(FuzzedDataProvider& fuzzed_data_provider);

    ~FuzzedSock() override;

    FuzzedSock& operator=(Sock&& other) override;

    ssize_t Send(const void* data, size_t len, int flags) const override;

    ssize_t Recv(void* buf, size_t len, int flags) const override;

    int Connect(const sockaddr*, socklen_t) const override;

    int Bind(const sockaddr*, socklen_t) const override;

    int Listen(int backlog) const override;

    std::unique_ptr<Sock> Accept(sockaddr* addr, socklen_t* addr_len) const override;

    int GetSockOpt(int level, int opt_name, void* opt_val, socklen_t* opt_len) const override;

    int SetSockOpt(int level, int opt_name, const void* opt_val, socklen_t opt_len) const override;

    int GetSockName(sockaddr* name, socklen_t* name_len) const override;

    bool SetNonBlocking() const override;

    bool IsSelectable() const override;

    bool Wait(std::chrono::milliseconds timeout, Event requested, Event* occurred = nullptr) const override;

    bool WaitMany(std::chrono::milliseconds timeout, EventsPerSock& events_per_sock) const override;

    bool IsConnected(std::string& errmsg) const override;
};

[[nodiscard]] inline FuzzedSock ConsumeSock(FuzzedDataProvider& fuzzed_data_provider)
{
    return FuzzedSock{fuzzed_data_provider};
}

inline CSubNet ConsumeSubNet(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return {ConsumeNetAddr(fuzzed_data_provider), fuzzed_data_provider.ConsumeIntegral<uint8_t>()};
}

inline CService ConsumeService(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    return {ConsumeNetAddr(fuzzed_data_provider), fuzzed_data_provider.ConsumeIntegral<uint16_t>()};
}

CAddress ConsumeAddress(FuzzedDataProvider& fuzzed_data_provider) noexcept;

template <bool ReturnUniquePtr = false>
auto ConsumeNode(FuzzedDataProvider& fuzzed_data_provider, const std::optional<NodeId>& node_id_in = std::nullopt) noexcept
{
    const NodeId node_id = node_id_in.value_or(fuzzed_data_provider.ConsumeIntegralInRange<NodeId>(0, std::numeric_limits<NodeId>::max()));
    const auto sock = std::make_shared<FuzzedSock>(fuzzed_data_provider);
    const CAddress address = ConsumeAddress(fuzzed_data_provider);
    const uint64_t keyed_net_group = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
    const uint64_t local_host_nonce = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
    const CAddress addr_bind = ConsumeAddress(fuzzed_data_provider);
    const std::string addr_name = fuzzed_data_provider.ConsumeRandomLengthString(64);
    const ConnectionType conn_type = fuzzed_data_provider.PickValueInArray(ALL_CONNECTION_TYPES);
    const bool inbound_onion{conn_type == ConnectionType::INBOUND ? fuzzed_data_provider.ConsumeBool() : false};
    NetPermissionFlags permission_flags = ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS);
    if constexpr (ReturnUniquePtr) {
        return std::make_unique<CNode>(node_id,
                                       sock,
                                       address,
                                       keyed_net_group,
                                       local_host_nonce,
                                       addr_bind,
                                       addr_name,
                                       conn_type,
                                       inbound_onion,
                                       CNodeOptions{ .permission_flags = permission_flags });
    } else {
        return CNode{node_id,
                     sock,
                     address,
                     keyed_net_group,
                     local_host_nonce,
                     addr_bind,
                     addr_name,
                     conn_type,
                     inbound_onion,
                     CNodeOptions{ .permission_flags = permission_flags }};
    }
}
inline std::unique_ptr<CNode> ConsumeNodeAsUniquePtr(FuzzedDataProvider& fdp, const std::optional<NodeId>& node_id_in = std::nullopt) { return ConsumeNode<true>(fdp, node_id_in); }

void FillNode(FuzzedDataProvider& fuzzed_data_provider, ConnmanTestMsg& connman, CNode& node) noexcept EXCLUSIVE_LOCKS_REQUIRED(NetEventsInterface::g_msgproc_mutex);

template<typename R>
void SimulationTest(Transport& initiator, Transport& responder, R& rng, FuzzedDataProvider& provider, const Span<std::string> messages)
{
    // Simulation test with two Transport objects, which send messages to each other, with
    // sending and receiving fragmented into multiple pieces that may be interleaved. It primarily
    // verifies that the sending and receiving side are compatible with each other, plus a few
    // sanity checks. It does not attempt to introduce errors in the communicated data.

    // Put the transports in an array for by-index access.
    const std::array<Transport*, 2> transports = {&initiator, &responder};

    // Two vectors representing in-flight bytes. inflight[i] is from transport[i] to transport[!i].
    std::array<std::vector<uint8_t>, 2> in_flight;

    // Two queues with expected messages. expected[i] is expected to arrive in transport[!i].
    std::array<std::deque<CSerializedNetMsg>, 2> expected;

    // Vectors with bytes last returned by GetBytesToSend() on transport[i].
    std::array<std::vector<uint8_t>, 2> to_send;

    // Last returned 'more' values (if still relevant) by transport[i]->GetBytesToSend(), for
    // both have_next_message false and true.
    std::array<std::optional<bool>, 2> last_more, last_more_next;

    // Whether more bytes to be sent are expected on transport[i], before and after
    // SetMessageToSend().
    std::array<std::optional<bool>, 2> expect_more, expect_more_next;

    // Function to consume a message type.
    auto msg_type_fn = [&]() {
        uint8_t v = provider.ConsumeIntegral<uint8_t>();
        if (v == 0xFF) {
            // If v is 0xFF, construct a valid (but possibly unknown) message type from the fuzz
            // data.
            std::string ret;
            while (ret.size() < CMessageHeader::COMMAND_SIZE) {
                char c = provider.ConsumeIntegral<char>();
                // Match the allowed characters in CMessageHeader::IsCommandValid(). Any other
                // character is interpreted as end.
                if (c < ' ' || c > 0x7E) break;
                ret += c;
            }
            return ret;
        } else {
            // Otherwise, use it as index into the list of known messages.
            return messages[v % messages.size()];
        }
    };

    // Function to construct a CSerializedNetMsg to send.
    auto make_msg_fn = [&](bool first) {
        CSerializedNetMsg msg;
        if (first) {
            // Always send a "version" message as first one.
            msg.m_type = "version";
        } else {
            msg.m_type = msg_type_fn();
        }
        // Determine size of message to send (limited to 75 kB for performance reasons).
        size_t size = provider.ConsumeIntegralInRange<uint32_t>(0, 75000);
        // Get payload of message from RNG.
        msg.data.resize(size);
        for (auto& v : msg.data) v = uint8_t(rng());
        // Return.
        return msg;
    };

    // The next message to be sent (initially version messages, but will be replaced once sent).
    std::array<CSerializedNetMsg, 2> next_msg = {
        make_msg_fn(/*first=*/true),
        make_msg_fn(/*first=*/true)
    };

    // Wrapper around transport[i]->GetBytesToSend() that performs sanity checks.
    auto bytes_to_send_fn = [&](int side) -> Transport::BytesToSend {
        // Invoke GetBytesToSend twice (for have_next_message = {false, true}). This function does
        // not modify state (it's const), and only the "more" return value should differ between
        // the calls.
        const auto& [bytes, more_nonext, msg_type] = transports[side]->GetBytesToSend(false);
        const auto& [bytes_next, more_next, msg_type_next] = transports[side]->GetBytesToSend(true);
        // Compare with expected more.
        if (expect_more[side].has_value()) assert(!bytes.empty() == *expect_more[side]);
        // Verify consistency between the two results.
        assert(bytes == bytes_next);
        assert(msg_type == msg_type_next);
        if (more_nonext) assert(more_next);
        // Compare with previously reported output.
        assert(to_send[side].size() <= bytes.size());
        assert(to_send[side] == Span{bytes}.first(to_send[side].size()));
        to_send[side].resize(bytes.size());
        std::copy(bytes.begin(), bytes.end(), to_send[side].begin());
        // Remember 'more' results.
        last_more[side] = {more_nonext};
        last_more_next[side] = {more_next};
        // Return.
        return {bytes, more_nonext, msg_type};
    };

    // Function to make side send a new message.
    auto new_msg_fn = [&](int side) {
        // Don't do anything if there are too many unreceived messages already.
        if (expected[side].size() >= 16) return;
        // Try to send (a copy of) the message in next_msg[side].
        CSerializedNetMsg msg = next_msg[side].Copy();
        bool queued = transports[side]->SetMessageToSend(msg);
        // Update expected more data.
        expect_more[side] = expect_more_next[side];
        expect_more_next[side] = std::nullopt;
        // Verify consistency of GetBytesToSend after SetMessageToSend
        bytes_to_send_fn(/*side=*/side);
        if (queued) {
            // Remember that this message is now expected by the receiver.
            expected[side].emplace_back(std::move(next_msg[side]));
            // Construct a new next message to send.
            next_msg[side] = make_msg_fn(/*first=*/false);
        }
    };

    // Function to make side send out bytes (if any).
    auto send_fn = [&](int side, bool everything = false) {
        const auto& [bytes, more, msg_type] = bytes_to_send_fn(/*side=*/side);
        // Don't do anything if no bytes to send.
        if (bytes.empty()) return false;
        size_t send_now = everything ? bytes.size() : provider.ConsumeIntegralInRange<size_t>(0, bytes.size());
        if (send_now == 0) return false;
        // Add bytes to the in-flight queue, and mark those bytes as consumed.
        in_flight[side].insert(in_flight[side].end(), bytes.begin(), bytes.begin() + send_now);
        transports[side]->MarkBytesSent(send_now);
        // If all to-be-sent bytes were sent, move last_more data to expect_more data.
        if (send_now == bytes.size()) {
            expect_more[side] = last_more[side];
            expect_more_next[side] = last_more_next[side];
        }
        // Remove the bytes from the last reported to-be-sent vector.
        assert(to_send[side].size() >= send_now);
        to_send[side].erase(to_send[side].begin(), to_send[side].begin() + send_now);
        // Verify that GetBytesToSend gives a result consistent with earlier.
        bytes_to_send_fn(/*side=*/side);
        // Return whether anything was sent.
        return send_now > 0;
    };

    // Function to make !side receive bytes (if any).
    auto recv_fn = [&](int side, bool everything = false) {
        // Don't do anything if no bytes in flight.
        if (in_flight[side].empty()) return false;
        // Decide span to receive
        size_t to_recv_len = in_flight[side].size();
        if (!everything) to_recv_len = provider.ConsumeIntegralInRange<size_t>(0, to_recv_len);
        Span<const uint8_t> to_recv = Span{in_flight[side]}.first(to_recv_len);
        // Process those bytes
        while (!to_recv.empty()) {
            size_t old_len = to_recv.size();
            bool ret = transports[!side]->ReceivedBytes(to_recv);
            // Bytes must always be accepted, as this test does not introduce any errors in
            // communication.
            assert(ret);
            // Clear cached expected 'more' information: if certainly no more data was to be sent
            // before, receiving bytes makes this uncertain.
            if (expect_more[!side] == false) expect_more[!side] = std::nullopt;
            if (expect_more_next[!side] == false) expect_more_next[!side] = std::nullopt;
            // Verify consistency of GetBytesToSend after ReceivedBytes
            bytes_to_send_fn(/*side=*/!side);
            bool progress = to_recv.size() < old_len;
            if (transports[!side]->ReceivedMessageComplete()) {
                bool reject{false};
                auto received = transports[!side]->GetReceivedMessage({}, reject);
                // Receiving must succeed.
                assert(!reject);
                // There must be a corresponding expected message.
                assert(!expected[side].empty());
                // The m_message_size field must be correct.
                assert(received.m_message_size == received.m_recv.size());
                // The m_type must match what is expected.
                assert(received.m_type == expected[side].front().m_type);
                // The data must match what is expected.
                assert(MakeByteSpan(received.m_recv) == MakeByteSpan(expected[side].front().data));
                expected[side].pop_front();
                progress = true;
            }
            // Progress must be made (by processing incoming bytes and/or returning complete
            // messages) until all received bytes are processed.
            assert(progress);
        }
        // Remove the processed bytes from the in_flight buffer.
        in_flight[side].erase(in_flight[side].begin(), in_flight[side].begin() + to_recv_len);
        // Return whether anything was received.
        return to_recv_len > 0;
    };

    // Main loop, interleaving new messages, sends, and receives.
    LIMITED_WHILE(provider.remaining_bytes(), 1000) {
        CallOneOf(provider,
            // (Try to) give the next message to the transport.
            [&] { new_msg_fn(/*side=*/0); },
            [&] { new_msg_fn(/*side=*/1); },
            // (Try to) send some bytes from the transport to the network.
            [&] { send_fn(/*side=*/0); },
            [&] { send_fn(/*side=*/1); },
            // (Try to) receive bytes from the network, converting to messages.
            [&] { recv_fn(/*side=*/0); },
            [&] { recv_fn(/*side=*/1); }
        );
    }

    // When we're done, perform sends and receives of existing messages to flush anything already
    // in flight.
    while (true) {
        bool any = false;
        if (send_fn(/*side=*/0, /*everything=*/true)) any = true;
        if (send_fn(/*side=*/1, /*everything=*/true)) any = true;
        if (recv_fn(/*side=*/0, /*everything=*/true)) any = true;
        if (recv_fn(/*side=*/1, /*everything=*/true)) any = true;
        if (!any) break;
    }

    // Make sure nothing is left in flight.
    assert(in_flight[0].empty());
    assert(in_flight[1].empty());

    // Make sure all expected messages were received.
    assert(expected[0].empty());
    assert(expected[1].empty());

    // Compare session IDs.
    assert(transports[0]->GetInfo().session_id == transports[1]->GetInfo().session_id);
}

std::unique_ptr<Transport> MakeV1Transport(NodeId nodeid) noexcept
{
    return std::make_unique<V1Transport>(nodeid, SER_NETWORK, INIT_PROTO_VERSION);
}

template<typename RNG>
std::unique_ptr<Transport> MakeV2Transport(NodeId nodeid, bool initiator, RNG& rng, FuzzedDataProvider& provider)
{
    // Retrieve key
    auto key = ConsumePrivateKey(provider);
    if (!key.IsValid()) return {};
    // Construct garbage
    size_t garb_len = provider.ConsumeIntegralInRange<size_t>(0, V2Transport::MAX_GARBAGE_LEN);
    std::vector<uint8_t> garb;
    if (garb_len <= 64) {
        // When the garbage length is up to 64 bytes, read it directly from the fuzzer input.
        garb = provider.ConsumeBytes<uint8_t>(garb_len);
        garb.resize(garb_len);
    } else {
        // If it's longer, generate it from the RNG. This avoids having large amounts of
        // (hopefully) irrelevant data needing to be stored in the fuzzer data.
        for (auto& v : garb) v = uint8_t(rng());
    }
    // Retrieve entropy
    auto ent = provider.ConsumeBytes<std::byte>(32);
    ent.resize(32);
    // Use as entropy SHA256(ent || garbage). This prevents a situation where the fuzzer manages to
    // include the garbage terminator (which is a function of both ellswift keys) in the garbage.
    // This is extremely unlikely (~2^-116) with random keys/garbage, but the fuzzer can choose
    // both non-randomly and dependently. Since the entropy is hashed anyway inside the ellswift
    // computation, no coverage should be lost by using a hash as entropy, and it removes the
    // possibility of garbage that happens to contain what is effectively a hash of the keys.
    CSHA256().Write(UCharCast(ent.data()), ent.size())
             .Write(garb.data(), garb.size())
             .Finalize(UCharCast(ent.data()));

    return std::make_unique<V2Transport>(nodeid, initiator, SER_NETWORK, INIT_PROTO_VERSION, key, ent, std::move(garb));
}

#endif // BITCOIN_TEST_FUZZ_UTIL_NET_H
