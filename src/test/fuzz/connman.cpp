// Copyright (c) 2020 The Widecoin Core developers
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

void initialize()
{
    InitializeFuzzingContext();
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    CConnman connman{fuzzed_data_provider.ConsumeIntegral<uint64_t>(), fuzzed_data_provider.ConsumeIntegral<uint64_t>(), fuzzed_data_provider.ConsumeBool()};
    CAddress random_address;
    CNetAddr random_netaddr;
    CNode random_node = ConsumeNode(fuzzed_data_provider);
    CService random_service;
    CSubNet random_subnet;
    std::string random_string;
    while (fuzzed_data_provider.ConsumeBool()) {
        switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 30)) {
        case 0:
            random_address = ConsumeAddress(fuzzed_data_provider);
            break;
        case 1:
            random_netaddr = ConsumeNetAddr(fuzzed_data_provider);
            break;
        case 2:
            random_service = ConsumeService(fuzzed_data_provider);
            break;
        case 3:
            random_subnet = ConsumeSubNet(fuzzed_data_provider);
            break;
        case 4:
            random_string = fuzzed_data_provider.ConsumeRandomLengthString(64);
            break;
        case 5: {
            std::vector<CAddress> addresses;
            while (fuzzed_data_provider.ConsumeBool()) {
                addresses.push_back(ConsumeAddress(fuzzed_data_provider));
            }
            // Limit nTimePenalty to int32_t to avoid signed integer overflow
            (void)connman.AddNewAddresses(addresses, ConsumeAddress(fuzzed_data_provider), fuzzed_data_provider.ConsumeIntegral<int32_t>());
            break;
        }
        case 6:
            connman.AddNode(random_string);
            break;
        case 7:
            connman.CheckIncomingNonce(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            break;
        case 8:
            connman.DisconnectNode(fuzzed_data_provider.ConsumeIntegral<NodeId>());
            break;
        case 9:
            connman.DisconnectNode(random_netaddr);
            break;
        case 10:
            connman.DisconnectNode(random_string);
            break;
        case 11:
            connman.DisconnectNode(random_subnet);
            break;
        case 12:
            connman.ForEachNode([](auto) {});
            break;
        case 13:
            connman.ForEachNodeThen([](auto) {}, []() {});
            break;
        case 14:
            (void)connman.ForNode(fuzzed_data_provider.ConsumeIntegral<NodeId>(), [&](auto) { return fuzzed_data_provider.ConsumeBool(); });
            break;
        case 15:
            (void)connman.GetAddresses(fuzzed_data_provider.ConsumeIntegral<size_t>(), fuzzed_data_provider.ConsumeIntegral<size_t>());
            break;
        case 16: {
            (void)connman.GetAddresses(random_node, fuzzed_data_provider.ConsumeIntegral<size_t>(), fuzzed_data_provider.ConsumeIntegral<size_t>());
            break;
        }
        case 17:
            (void)connman.GetDeterministicRandomizer(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            break;
        case 18:
            (void)connman.GetNodeCount(fuzzed_data_provider.PickValueInArray({CConnman::CONNECTIONS_NONE, CConnman::CONNECTIONS_IN, CConnman::CONNECTIONS_OUT, CConnman::CONNECTIONS_ALL}));
            break;
        case 19:
            connman.MarkAddressGood(random_address);
            break;
        case 20:
            (void)connman.OutboundTargetReached(fuzzed_data_provider.ConsumeBool());
            break;
        case 21:
            // Limit now to int32_t to avoid signed integer overflow
            (void)connman.PoissonNextSendInbound(fuzzed_data_provider.ConsumeIntegral<int32_t>(), fuzzed_data_provider.ConsumeIntegral<int>());
            break;
        case 22: {
            CSerializedNetMsg serialized_net_msg;
            serialized_net_msg.m_type = fuzzed_data_provider.ConsumeRandomLengthString(CMessageHeader::COMMAND_SIZE);
            serialized_net_msg.data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
            connman.PushMessage(&random_node, std::move(serialized_net_msg));
            break;
        }
        case 23:
            connman.RemoveAddedNode(random_string);
            break;
        case 24: {
            const std::vector<bool> asmap = ConsumeRandomLengthBitVector(fuzzed_data_provider);
            if (SanityCheckASMap(asmap)) {
                connman.SetAsmap(asmap);
            }
            break;
        }
        case 25:
            connman.SetBestHeight(fuzzed_data_provider.ConsumeIntegral<int>());
            break;
        case 26:
            connman.SetMaxOutboundTarget(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            break;
        case 27:
            connman.SetMaxOutboundTimeframe(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            break;
        case 28:
            connman.SetNetworkActive(fuzzed_data_provider.ConsumeBool());
            break;
        case 29:
            connman.SetServices(random_service, static_cast<ServiceFlags>(fuzzed_data_provider.ConsumeIntegral<uint64_t>()));
            break;
        case 30:
            connman.SetTryNewOutboundPeer(fuzzed_data_provider.ConsumeBool());
            break;
        }
    }
    (void)connman.GetAddedNodeInfo();
    (void)connman.GetBestHeight();
    (void)connman.GetExtraOutboundCount();
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
