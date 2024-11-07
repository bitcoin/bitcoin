// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_NET_H
#define BITCOIN_TEST_FUZZ_UTIL_NET_H

#include <addrman.h>
#include <addrman_impl.h>
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
#include <util/asmap.h>
#include <util/sock.h>

#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>

/**
 * Create a CNetAddr. It may have `addr.IsValid() == false`.
 * @param[in,out] fuzzed_data_provider Take data for the address from this, if `rand` is `nullptr`.
 * @param[in,out] rand If not nullptr, take data from it instead of from `fuzzed_data_provider`.
 * Prefer generating addresses using `fuzzed_data_provider` because it is not uniform. Only use
 * `rand` if `fuzzed_data_provider` is exhausted or its data is needed for other things.
 * @return a "random" network address.
 */
CNetAddr ConsumeNetAddr(FuzzedDataProvider& fuzzed_data_provider, FastRandomContext* rand = nullptr) noexcept;

class AddrManDeterministic : public AddrMan
{
public:
    explicit AddrManDeterministic(const NetGroupManager& netgroupman, FuzzedDataProvider& fuzzed_data_provider, int32_t check_ratio)
        : AddrMan(netgroupman, /*deterministic=*/true, check_ratio)
    {
        WITH_LOCK(m_impl->cs, m_impl->insecure_rand.Reseed(ConsumeUInt256(fuzzed_data_provider)));
    }

    /**
     * Compare with another AddrMan.
     * This compares:
     * - the values in `mapInfo` (the keys aka ids are ignored)
     * - vvNew entries refer to the same addresses
     * - vvTried entries refer to the same addresses
     */
    bool operator==(const AddrManDeterministic& other) const
    {
        LOCK2(m_impl->cs, other.m_impl->cs);

        if (m_impl->mapInfo.size() != other.m_impl->mapInfo.size() || m_impl->nNew != other.m_impl->nNew ||
            m_impl->nTried != other.m_impl->nTried) {
            return false;
        }

        // Check that all values in `mapInfo` are equal to all values in `other.mapInfo`.
        // Keys may be different.

        auto addrinfo_hasher = [](const AddrInfo& a) {
            CSipHasher hasher(0, 0);
            auto addr_key = a.GetKey();
            auto source_key = a.source.GetAddrBytes();
            hasher.Write(TicksSinceEpoch<std::chrono::seconds>(a.m_last_success));
            hasher.Write(a.nAttempts);
            hasher.Write(a.nRefCount);
            hasher.Write(a.fInTried);
            hasher.Write(a.GetNetwork());
            hasher.Write(a.source.GetNetwork());
            hasher.Write(addr_key.size());
            hasher.Write(source_key.size());
            hasher.Write(addr_key);
            hasher.Write(source_key);
            return (size_t)hasher.Finalize();
        };

        auto addrinfo_eq = [](const AddrInfo& lhs, const AddrInfo& rhs) {
            return std::tie(static_cast<const CService&>(lhs), lhs.source, lhs.m_last_success, lhs.nAttempts, lhs.nRefCount, lhs.fInTried) ==
                   std::tie(static_cast<const CService&>(rhs), rhs.source, rhs.m_last_success, rhs.nAttempts, rhs.nRefCount, rhs.fInTried);
        };

        using Addresses = std::unordered_set<AddrInfo, decltype(addrinfo_hasher), decltype(addrinfo_eq)>;

        const size_t num_addresses{m_impl->mapInfo.size()};

        Addresses addresses{num_addresses, addrinfo_hasher, addrinfo_eq};
        for (const auto& [id, addr] : m_impl->mapInfo) {
            addresses.insert(addr);
        }

        Addresses other_addresses{num_addresses, addrinfo_hasher, addrinfo_eq};
        for (const auto& [id, addr] : other.m_impl->mapInfo) {
            other_addresses.insert(addr);
        }

        if (addresses != other_addresses) {
            return false;
        }

        auto IdsReferToSameAddress = [&](nid_type id, nid_type other_id) EXCLUSIVE_LOCKS_REQUIRED(m_impl->cs, other.m_impl->cs) {
            if (id == -1 && other_id == -1) {
                return true;
            }
            if ((id == -1 && other_id != -1) || (id != -1 && other_id == -1)) {
                return false;
            }
            return m_impl->mapInfo.at(id) == other.m_impl->mapInfo.at(other_id);
        };

        // Check that `vvNew` contains the same addresses as `other.vvNew`. Notice - `vvNew[i][j]`
        // contains just an id and the address is to be found in `mapInfo.at(id)`. The ids
        // themselves may differ between `vvNew` and `other.vvNew`.
        for (size_t i = 0; i < ADDRMAN_NEW_BUCKET_COUNT; ++i) {
            for (size_t j = 0; j < ADDRMAN_BUCKET_SIZE; ++j) {
                if (!IdsReferToSameAddress(m_impl->vvNew[i][j], other.m_impl->vvNew[i][j])) {
                    return false;
                }
            }
        }

        // Same for `vvTried`.
        for (size_t i = 0; i < ADDRMAN_TRIED_BUCKET_COUNT; ++i) {
            for (size_t j = 0; j < ADDRMAN_BUCKET_SIZE; ++j) {
                if (!IdsReferToSameAddress(m_impl->vvTried[i][j], other.m_impl->vvTried[i][j])) {
                    return false;
                }
            }
        }

        return true;
    }
};

class FuzzedSock : public Sock
{
    FuzzedDataProvider& m_fuzzed_data_provider;

    /**
     * Data to return when `MSG_PEEK` is used as a `Recv()` flag.
     * If `MSG_PEEK` is used, then our `Recv()` returns some random data as usual, but on the next
     * `Recv()` call we must return the same data, thus we remember it here.
     */
    mutable std::vector<uint8_t> m_peek_data;

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

[[nodiscard]] inline NetGroupManager ConsumeNetGroupManager(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    std::vector<bool> asmap = ConsumeRandomLengthBitVector(fuzzed_data_provider);
    if (!SanityCheckASMap(asmap, 128)) asmap.clear();
    return NetGroupManager(asmap);
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

#endif // BITCOIN_TEST_FUZZ_UTIL_NET_H
