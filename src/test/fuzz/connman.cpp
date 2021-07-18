// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <chainparamsbase.h>
#include <net.h>
#include <net_processing.h>
#include <netaddress.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/translation.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace {
const TestingSetup* g_setup;
} // namespace

void initialize_connman()
{
    static const auto testing_setup = MakeNoLogFileContext<TestingSetup>();
    g_setup = testing_setup.get();
}

struct CConnmanTest : public CConnman {
    CConnmanTest(FuzzedDataProvider& fuzzed_data_provider, CAddrMan& addrman)
        : CConnman(fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
                   fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
                   addrman,
                   fuzzed_data_provider.ConsumeBool())
    {
        // I2P code paths will also be executed if this is set.
        m_i2p_sam_session =
            std::make_unique<i2p::sam::Session>(gArgs.GetDataDirNet() / "fuzzed_i2p_private_key",
                                                ConsumeService(fuzzed_data_provider),
                                                &interruptNet);
    }

    void CreateNodeFromAcceptedSocketPublic(std::unique_ptr<Sock> sock,
                                            NetPermissionFlags permissions,
                                            const CAddress& addr_bind,
                                            const CAddress& addr_peer)
    {
        CreateNodeFromAcceptedSocket(std::move(sock), permissions, addr_bind, addr_peer);
    }

    bool InitBindsPublic(const std::vector<CService>& binds,
                         const std::vector<NetWhitebindPermissions>& white_binds,
                         const std::vector<CService>& onion_binds)
    {
        return InitBinds(binds, white_binds, onion_binds);
    }

    void SocketHandlerPublic() { SocketHandler(); }
};

FUZZ_TARGET_INIT(connman, initialize_connman)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));

    // Mock CreateSock() to create FuzzedSock.
    auto CreateSockOrig = CreateSock;
    CreateSock = [&fuzzed_data_provider](const CService&) {
        return std::make_unique<FuzzedSock>(fuzzed_data_provider);
    };

    // Mock g_dns_lookup() to return a fuzzed address.
    g_dns_lookup = [&fuzzed_data_provider](const std::string&, bool) {
        return std::vector<CNetAddr>{ConsumeNetAddr(fuzzed_data_provider)};
    };

    CConnmanTest connman{fuzzed_data_provider, *g_setup->m_node.addrman};
    CConnman::Options options;
    options.m_msgproc = g_setup->m_node.peerman.get();
    connman.Init(options);

    CNetAddr random_netaddr;
    CAddress random_address;
    CNode random_node = ConsumeNode(fuzzed_data_provider);
    CSubNet random_subnet;
    std::string random_string;
    while (fuzzed_data_provider.ConsumeBool()) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                random_netaddr = ConsumeNetAddr(fuzzed_data_provider);
            },
            [&] {
                random_address = ConsumeAddress(fuzzed_data_provider);
            },
            [&] {
                random_subnet = ConsumeSubNet(fuzzed_data_provider);
            },
            [&] {
                random_string = fuzzed_data_provider.ConsumeRandomLengthString(64);
            },
            [&] {
                connman.AddNode(random_string);
            },
            [&] {
                connman.CheckIncomingNonce(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            },
            [&] {
                connman.DisconnectNode(fuzzed_data_provider.ConsumeIntegral<NodeId>());
            },
            [&] {
                connman.DisconnectNode(random_netaddr);
            },
            [&] {
                connman.DisconnectNode(random_string);
            },
            [&] {
                connman.DisconnectNode(random_subnet);
            },
            [&] {
                connman.ForEachNode([](auto) {});
            },
            [&] {
                (void)connman.ForNode(fuzzed_data_provider.ConsumeIntegral<NodeId>(), [&](auto) { return fuzzed_data_provider.ConsumeBool(); });
            },
            [&] {
                (void)connman.GetAddresses(
                    /* max_addresses */ fuzzed_data_provider.ConsumeIntegral<size_t>(),
                    /* max_pct */ fuzzed_data_provider.ConsumeIntegral<size_t>(),
                    /* network */ std::nullopt);
            },
            [&] {
                (void)connman.GetAddresses(
                    /* requestor */ random_node,
                    /* max_addresses */ fuzzed_data_provider.ConsumeIntegral<size_t>(),
                    /* max_pct */ fuzzed_data_provider.ConsumeIntegral<size_t>());
            },
            [&] {
                (void)connman.GetDeterministicRandomizer(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            },
            [&] {
                (void)connman.GetNodeCount(fuzzed_data_provider.PickValueInArray({ConnectionDirection::None, ConnectionDirection::In, ConnectionDirection::Out, ConnectionDirection::Both}));
            },
            [&] {
                (void)connman.OutboundTargetReached(fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                // Limit now to int32_t to avoid signed integer overflow
                (void)connman.PoissonNextSendInbound(
                        std::chrono::microseconds{fuzzed_data_provider.ConsumeIntegral<int32_t>()},
                        std::chrono::seconds{fuzzed_data_provider.ConsumeIntegral<int>()});
            },
            [&] {
                CSerializedNetMsg serialized_net_msg;
                serialized_net_msg.m_type = fuzzed_data_provider.ConsumeRandomLengthString(CMessageHeader::COMMAND_SIZE);
                serialized_net_msg.data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
                connman.PushMessage(&random_node, std::move(serialized_net_msg));
            },
            [&] {
                connman.RemoveAddedNode(random_string);
            },
            [&] {
                const std::vector<bool> asmap = ConsumeRandomLengthBitVector(fuzzed_data_provider);
                if (SanityCheckASMap(asmap)) {
                    connman.SetAsmap(asmap);
                }
            },
            [&] {
                connman.SetNetworkActive(fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                connman.SetTryNewOutboundPeer(fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                const auto& to_addr{random_address};

                const bool count_failure{fuzzed_data_provider.ConsumeBool()};

                CSemaphoreGrant grant;
                CSemaphoreGrant* grant_ptr{fuzzed_data_provider.ConsumeBool() ? nullptr : &grant};

                const char* to_str{fuzzed_data_provider.ConsumeBool() ? nullptr :
                                                                        random_string.c_str()};

                ConnectionType conn_type{
                    fuzzed_data_provider.PickValueInArray(ALL_CONNECTION_TYPES)};
                if (conn_type == ConnectionType::INBOUND) { // INBOUND is not allowed
                    conn_type = ConnectionType::OUTBOUND_FULL_RELAY;
                }

                connman.OpenNetworkConnection(to_addr, count_failure, grant_ptr, to_str, conn_type);
            },
            [&] {
                connman.SetNetworkActive(true);

                NetPermissionFlags permissions{
                    ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS)};
                auto me = ConsumeAddress(fuzzed_data_provider);
                auto peer = ConsumeAddress(fuzzed_data_provider);
                auto sock = CreateSock(peer);

                connman.CreateNodeFromAcceptedSocketPublic(std::move(sock), permissions, me, peer);
            },
            [&] {
                const std::vector<CService> binds{
                    ConsumeServiceVector(fuzzed_data_provider, 5)};

                const std::vector<CService> onion_binds{
                    ConsumeServiceVector(fuzzed_data_provider, 5)};

                std::vector<NetWhitebindPermissions> white_binds(
                    fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 5));
                for (size_t i = 0; i < white_binds.size(); ++i) {
                    white_binds[i].m_flags =
                        ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS);
                    white_binds[i].m_service = ConsumeService(fuzzed_data_provider);
                }

                connman.InitBindsPublic(binds, white_binds, onion_binds);
            },
            [&] {
                connman.SocketHandlerPublic();
            });
    }
    (void)connman.GetAddedNodeInfo();
    (void)connman.GetExtraFullOutboundCount();
    (void)connman.GetLocalServices();
    (void)connman.GetMaxOutboundTarget();
    (void)connman.GetMaxOutboundTimeframe();
    (void)connman.GetMaxOutboundTimeLeftInCycle();
    (void)connman.GetNetworkActive();
    std::vector<CNodeStats> stats;
    connman.GetNodeStats(stats);
    (void)connman.GetOutboundTargetBytesLeft();
    (void)connman.GetReceiveFloodSize();
    (void)connman.GetTotalBytesRecv();
    (void)connman.GetTotalBytesSent();
    (void)connman.GetTryNewOutboundPeer();
    (void)connman.GetUseAddrmanOutgoing();

    CreateSock = CreateSockOrig;
}
