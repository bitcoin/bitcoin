// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
#include <chainparams.h>
#include <common/args.h>
#include <net.h>
#include <net_processing.h>
#include <netaddress.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/fuzz/util/threadinterrupt.h>
#include <test/util/setup_common.h>
#include <util/translation.h>

#include <cstdint>
#include <vector>

namespace {
const TestingSetup* g_setup;

int32_t GetCheckRatio()
{
    return std::clamp<int32_t>(g_setup->m_node.args->GetIntArg("-checkaddrman", 0), 0, 1000000);
}

} // namespace

void initialize_connman()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(connman, .init = initialize_connman)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    auto netgroupman{ConsumeNetGroupManager(fuzzed_data_provider)};
    auto addr_man_ptr{std::make_unique<AddrManDeterministic>(netgroupman, fuzzed_data_provider, GetCheckRatio())};
    if (fuzzed_data_provider.ConsumeBool()) {
        const std::vector<uint8_t> serialized_data{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
        DataStream ds{serialized_data};
        try {
            ds >> *addr_man_ptr;
        } catch (const std::ios_base::failure&) {
            addr_man_ptr = std::make_unique<AddrManDeterministic>(netgroupman, fuzzed_data_provider, GetCheckRatio());
        }
    }
    AddrManDeterministic& addr_man{*addr_man_ptr};
    auto net_events{ConsumeNetEvents(fuzzed_data_provider)};

    // Mock CreateSock() to create FuzzedSock.
    auto CreateSockOrig = CreateSock;
    CreateSock = [&fuzzed_data_provider](int, int, int) {
        return std::make_unique<FuzzedSock>(fuzzed_data_provider);
    };

    // Mock g_dns_lookup() to return a fuzzed address.
    auto g_dns_lookup_orig = g_dns_lookup;
    g_dns_lookup = [&fuzzed_data_provider](const std::string&, bool) {
        return std::vector<CNetAddr>{ConsumeNetAddr(fuzzed_data_provider)};
    };

    ConnmanTestMsg connman{fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
                     fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
                     addr_man,
                     netgroupman,
                     Params(),
                     fuzzed_data_provider.ConsumeBool(),
                     ConsumeThreadInterrupt(fuzzed_data_provider)};

    const uint64_t max_outbound_limit{fuzzed_data_provider.ConsumeIntegral<uint64_t>()};
    CConnman::Options options;
    options.m_msgproc = &net_events;
    options.nMaxOutboundLimit = max_outbound_limit;
    connman.Init(options);

    CNetAddr random_netaddr;
    CAddress random_address;
    CNode random_node = ConsumeNode(fuzzed_data_provider);
    CSubNet random_subnet;
    std::string random_string;

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100) {
        CNode& p2p_node{*ConsumeNodeAsUniquePtr(fuzzed_data_provider).release()};
        connman.AddTestNode(p2p_node);
    }

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
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
                connman.AddNode({random_string, fuzzed_data_provider.ConsumeBool()});
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
                auto max_addresses = fuzzed_data_provider.ConsumeIntegral<size_t>();
                auto max_pct = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 100);
                auto filtered = fuzzed_data_provider.ConsumeBool();
                (void)connman.GetAddressesUnsafe(max_addresses, max_pct, /*network=*/std::nullopt, filtered);
            },
            [&] {
                auto max_addresses = fuzzed_data_provider.ConsumeIntegral<size_t>();
                auto max_pct = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 100);
                (void)connman.GetAddresses(/*requestor=*/random_node, max_addresses, max_pct);
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
                CSerializedNetMsg serialized_net_msg;
                serialized_net_msg.m_type = fuzzed_data_provider.ConsumeRandomLengthString(CMessageHeader::MESSAGE_TYPE_SIZE);
                serialized_net_msg.data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
                connman.PushMessage(&random_node, std::move(serialized_net_msg));
            },
            [&] {
                connman.RemoveAddedNode(random_string);
            },
            [&] {
                connman.SetNetworkActive(fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                connman.SetTryNewOutboundPeer(fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                ConnectionType conn_type{
                    fuzzed_data_provider.PickValueInArray(ALL_CONNECTION_TYPES)};
                if (conn_type == ConnectionType::INBOUND) { // INBOUND is not allowed
                    conn_type = ConnectionType::OUTBOUND_FULL_RELAY;
                }

                connman.OpenNetworkConnection(
                    /*addrConnect=*/random_address,
                    /*fCountFailure=*/fuzzed_data_provider.ConsumeBool(),
                    /*grant_outbound=*/{},
                    /*pszDest=*/fuzzed_data_provider.ConsumeBool() ? nullptr : random_string.c_str(),
                    /*conn_type=*/conn_type,
                    /*use_v2transport=*/fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                connman.SetNetworkActive(fuzzed_data_provider.ConsumeBool());
                const auto peer = ConsumeAddress(fuzzed_data_provider);
                connman.CreateNodeFromAcceptedSocketPublic(
                    /*sock=*/CreateSock(AF_INET, SOCK_STREAM, IPPROTO_TCP),
                    /*permissions=*/ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS),
                    /*addr_bind=*/ConsumeAddress(fuzzed_data_provider),
                    /*addr_peer=*/peer);
            },
            [&] {
                CConnman::Options options;

                options.vBinds = ConsumeServiceVector(fuzzed_data_provider);

                options.vWhiteBinds = std::vector<NetWhitebindPermissions>{
                    fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 5)};
                for (auto& wb : options.vWhiteBinds) {
                    wb.m_flags = ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS);
                    wb.m_service = ConsumeService(fuzzed_data_provider);
                }

                options.onion_binds = ConsumeServiceVector(fuzzed_data_provider);

                options.bind_on_any = options.vBinds.empty() && options.vWhiteBinds.empty() &&
                                      options.onion_binds.empty();

                connman.InitBindsPublic(options);
            },
            [&] {
                connman.SocketHandlerPublic();
            });
    }
    (void)connman.GetAddedNodeInfo(fuzzed_data_provider.ConsumeBool());
    (void)connman.GetExtraFullOutboundCount();
    (void)connman.GetLocalServices();
    assert(connman.GetMaxOutboundTarget() == max_outbound_limit);
    (void)connman.GetMaxOutboundTimeframe();
    (void)connman.GetMaxOutboundTimeLeftInCycle();
    (void)connman.GetNetworkActive();
    std::vector<CNodeStats> stats;
    connman.GetNodeStats(stats);
    (void)connman.GetOutboundTargetBytesLeft();
    (void)connman.GetTotalBytesRecv();
    (void)connman.GetTotalBytesSent();
    (void)connman.GetTryNewOutboundPeer();
    (void)connman.GetUseAddrmanOutgoing();
    (void)connman.ASMapHealthCheck();

    connman.ClearTestNodes();
    g_dns_lookup = g_dns_lookup_orig;
    CreateSock = CreateSockOrig;
}
