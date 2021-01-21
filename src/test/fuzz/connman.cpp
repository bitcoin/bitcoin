// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <chainparamsbase.h>
#include <net.h>
#include <netaddress.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/translation.h>

#include <cstdint>
#include <vector>

void initialize_connman()
{
    static const auto testing_setup = MakeFuzzingContext<>();
}

FUZZ_TARGET_INIT(connman, initialize_connman)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    CConnman connman{fuzzed_data_provider.ConsumeIntegral<uint64_t>(), fuzzed_data_provider.ConsumeIntegral<uint64_t>(), fuzzed_data_provider.ConsumeBool()};
    CAddress random_address;
    CNetAddr random_netaddr;
    CNode random_node = ConsumeNode(fuzzed_data_provider);
    CService random_service;
    CSubNet random_subnet;
    std::string random_string;
    while (fuzzed_data_provider.ConsumeBool()) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                random_address = ConsumeAddress(fuzzed_data_provider);
            },
            [&] {
                random_netaddr = ConsumeNetAddr(fuzzed_data_provider);
            },
            [&] {
                random_service = ConsumeService(fuzzed_data_provider);
            },
            [&] {
                random_subnet = ConsumeSubNet(fuzzed_data_provider);
            },
            [&] {
                random_string = fuzzed_data_provider.ConsumeRandomLengthString(64);
            },
            [&] {
                std::vector<CAddress> addresses;
                while (fuzzed_data_provider.ConsumeBool()) {
                    addresses.push_back(ConsumeAddress(fuzzed_data_provider));
                }
                // Limit nTimePenalty to int32_t to avoid signed integer overflow
                (void)connman.AddNewAddresses(addresses, ConsumeAddress(fuzzed_data_provider), fuzzed_data_provider.ConsumeIntegral<int32_t>());
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
                connman.ForEachNodeThen([](auto) {}, []() {});
            },
            [&] {
                (void)connman.ForNode(fuzzed_data_provider.ConsumeIntegral<NodeId>(), [&](auto) { return fuzzed_data_provider.ConsumeBool(); });
            },
            [&] {
                (void)connman.GetAddresses(fuzzed_data_provider.ConsumeIntegral<size_t>(), fuzzed_data_provider.ConsumeIntegral<size_t>());
            },
            [&] {
                (void)connman.GetAddresses(random_node, fuzzed_data_provider.ConsumeIntegral<size_t>(), fuzzed_data_provider.ConsumeIntegral<size_t>());
            },
            [&] {
                (void)connman.GetDeterministicRandomizer(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            },
            [&] {
                (void)connman.GetNodeCount(fuzzed_data_provider.PickValueInArray({CConnman::CONNECTIONS_NONE, CConnman::CONNECTIONS_IN, CConnman::CONNECTIONS_OUT, CConnman::CONNECTIONS_ALL}));
            },
            [&] {
                connman.MarkAddressGood(random_address);
            },
            [&] {
                (void)connman.OutboundTargetReached(fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                // Limit now to int32_t to avoid signed integer overflow
                (void)connman.PoissonNextSendInbound(fuzzed_data_provider.ConsumeIntegral<int32_t>(), fuzzed_data_provider.ConsumeIntegral<int>());
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
                connman.SetServices(random_service, ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS));
            },
            [&] {
                connman.SetTryNewOutboundPeer(fuzzed_data_provider.ConsumeBool());
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
}
