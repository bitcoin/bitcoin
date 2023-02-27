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
    InitializeFuzzingContext();
}

FUZZ_TARGET_INIT(connman, initialize_connman)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    CAddrMan addrman;
    CConnman connman{fuzzed_data_provider.ConsumeIntegral<uint64_t>(), fuzzed_data_provider.ConsumeIntegral<uint64_t>(), addrman};
    CNetAddr random_netaddr;
    CNode random_node = ConsumeNode(fuzzed_data_provider);
    CSubNet random_subnet;
    std::string random_string;
    while (fuzzed_data_provider.ConsumeBool()) {
        switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 23)) {
        case 0:
            random_netaddr = ConsumeNetAddr(fuzzed_data_provider);
            break;
        case 1:
            random_subnet = ConsumeSubNet(fuzzed_data_provider);
            break;
        case 2:
            random_string = fuzzed_data_provider.ConsumeRandomLengthString(64);
            break;
        case 3:
            connman.AddNode(random_string);
            break;
        case 4:
            connman.CheckIncomingNonce(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            break;
        case 5:
            connman.DisconnectNode(fuzzed_data_provider.ConsumeIntegral<NodeId>());
            break;
        case 6:
            connman.DisconnectNode(random_netaddr);
            break;
        case 7:
            connman.DisconnectNode(random_string);
            break;
        case 8:
            connman.DisconnectNode(random_subnet);
            break;
        case 9:
            connman.ForEachNode([](auto) {});
            break;
        case 10:
            connman.ForEachNodeThen([](auto) {}, []() {});
            break;
        case 11:
            (void)connman.ForNode(fuzzed_data_provider.ConsumeIntegral<NodeId>(), [&](auto) { return fuzzed_data_provider.ConsumeBool(); });
            break;
        case 12:
            (void)connman.GetAddresses();
            break;
        case 13: {
            (void)connman.GetAddresses();
            break;
        }
        case 14:
            (void)connman.GetDeterministicRandomizer(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            break;
        case 15:
            (void)connman.GetNodeCount(fuzzed_data_provider.PickValueInArray({CConnman::CONNECTIONS_NONE, CConnman::CONNECTIONS_IN, CConnman::CONNECTIONS_OUT, CConnman::CONNECTIONS_ALL}));
            break;
        case 16:
            (void)connman.OutboundTargetReached(fuzzed_data_provider.ConsumeBool());
            break;
        case 17:
            // Limit now to int32_t to avoid signed integer overflow
            (void)connman.PoissonNextSendInbound(fuzzed_data_provider.ConsumeIntegral<int32_t>(), fuzzed_data_provider.ConsumeIntegral<int>());
            break;
        case 18: {
            CSerializedNetMsg serialized_net_msg;
            serialized_net_msg.command = fuzzed_data_provider.ConsumeRandomLengthString(CMessageHeader::COMMAND_SIZE);
            serialized_net_msg.data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
            connman.PushMessage(&random_node, std::move(serialized_net_msg));
            break;
        }
        case 19:
            connman.RemoveAddedNode(random_string);
            break;
        case 20: {
            const std::vector<bool> asmap = ConsumeRandomLengthBitVector(fuzzed_data_provider);
            if (SanityCheckASMap(asmap)) {
                connman.SetAsmap(asmap);
            }
            break;
        }
        case 21:
            connman.SetBestHeight(fuzzed_data_provider.ConsumeIntegral<int>());
            break;
        case 22:
            connman.SetNetworkActive(fuzzed_data_provider.ConsumeBool());
            break;
        case 23:
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
