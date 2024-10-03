// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
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

namespace {
const BasicTestingSetup* g_setup;

int32_t GetCheckRatio()
{
    return std::clamp<int32_t>(g_setup->m_node.args->GetArg("-checkaddrman", 0), 0, 1000000);
}
} // namespace

void initialize_net()
{
    static const auto testing_setup = MakeNoLogFileContext<>(CBaseChainParams::MAIN);
    g_setup = testing_setup.get();
}

// From src/test/fuzz/addrman.cpp
extern NetGroupManager ConsumeNetGroupManager(FuzzedDataProvider& fuzzed_data_provider) noexcept;

FUZZ_TARGET_INIT(net, initialize_net)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    CNode node{ConsumeNode(fuzzed_data_provider)};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    node.SetCommonVersion(fuzzed_data_provider.ConsumeIntegral<int>());
    while (fuzzed_data_provider.ConsumeBool()) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                NetGroupManager netgroupman{ConsumeNetGroupManager(fuzzed_data_provider)};
                AddrMan addrman(netgroupman, /*deterministic=*/false, GetCheckRatio());
                CConnman connman{fuzzed_data_provider.ConsumeIntegral<uint64_t>(), fuzzed_data_provider.ConsumeIntegral<uint64_t>(), addrman, netgroupman};
                node.CloseSocketDisconnect(&connman);
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
    const int ref_count = node.GetRefCount();
    assert(ref_count >= 0);
    (void)node.GetCommonVersion();

    const NetPermissionFlags net_permission_flags = ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS);
    (void)node.HasPermission(net_permission_flags);
    (void)node.ConnectedThroughNetwork();
}
