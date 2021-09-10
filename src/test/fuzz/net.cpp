// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <chainparamsbase.h>
#include <net.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <protocol.h>
#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/net.h>
#include <test/util/setup_common.h>
#include <util/asmap.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

void initialize_net()
{
    static const auto testing_setup = MakeNoLogFileContext<>(CBaseChainParams::MAIN);
}

FUZZ_TARGET_INIT(net, initialize_net)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    CNode node{ConsumeNode(fuzzed_data_provider)};
    node.SetCommonVersion(fuzzed_data_provider.ConsumeIntegral<int>());
    while (fuzzed_data_provider.ConsumeBool()) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                node.CloseSocketDisconnect();
            },
            [&] {
                CNodeStats stats;
                node.CopyStats(stats);
            },
            [&] {
                const CNode* add_ref_node = node.AddRef();
                assert(add_ref_node == &node);
            },
            [&] {
                if (node.GetRefCount() > 0) {
                    node.Release();
                }
            },
            [&] {
                const std::optional<CInv> inv_opt = ConsumeDeserializable<CInv>(fuzzed_data_provider);
                if (!inv_opt) {
                    return;
                }
                node.AddKnownTx(inv_opt->hash);
            },
            [&] {
                node.PushTxInventory(ConsumeUInt256(fuzzed_data_provider));
            },
            [&] {
                const std::optional<CService> service_opt = ConsumeDeserializable<CService>(fuzzed_data_provider);
                if (!service_opt) {
                    return;
                }
                node.SetAddrLocal(*service_opt);
            },
            [&] {
                const std::vector<uint8_t> b = ConsumeRandomLengthByteVector(fuzzed_data_provider);
                bool complete;
                node.ReceiveMsgBytes(b, complete);
            });
    }

    (void)node.GetAddrLocal();
    (void)node.GetId();
    (void)node.GetLocalNonce();
    (void)node.GetLocalServices();
    const int ref_count = node.GetRefCount();
    assert(ref_count >= 0);
    (void)node.GetCommonVersion();

    const NetPermissionFlags net_permission_flags = ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS);
    (void)node.HasPermission(net_permission_flags);
    (void)node.ConnectedThroughNetwork();
}
