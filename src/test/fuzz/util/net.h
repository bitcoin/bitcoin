// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_TEST_FUZZ_UTIL_NET_H
#define SYSCOIN_TEST_FUZZ_UTIL_NET_H

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

#endif // SYSCOIN_TEST_FUZZ_UTIL_NET_H
