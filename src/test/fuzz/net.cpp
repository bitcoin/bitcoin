// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <chainparamsbase.h>
#include <net.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <optional.h>
#include <protocol.h>
#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cstdint>
#include <string>
#include <vector>

void initialize_net()
{
    static const BasicTestingSetup basic_testing_setup;
}

FUZZ_TARGET_INIT(net, initialize_net)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    CNode node{ConsumeNode(fuzzed_data_provider)};
    node.SetCommonVersion(fuzzed_data_provider.ConsumeIntegral<int>());
    while (fuzzed_data_provider.ConsumeBool()) {
        switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 10)) {
        case 0: {
            node.CloseSocketDisconnect();
            break;
        }
        case 1: {
            node.MaybeSetAddrName(fuzzed_data_provider.ConsumeRandomLengthString(32));
            break;
        }
        case 2: {
            const std::vector<bool> asmap = ConsumeRandomLengthBitVector(fuzzed_data_provider);
            if (!SanityCheckASMap(asmap)) {
                break;
            }
            CNodeStats stats;
            node.copyStats(stats, asmap);
            break;
        }
        case 3: {
            const CNode* add_ref_node = node.AddRef();
            assert(add_ref_node == &node);
            break;
        }
        case 4: {
            if (node.GetRefCount() > 0) {
                node.Release();
            }
            break;
        }
        case 5: {
            if (node.m_addr_known == nullptr) {
                break;
            }
            const std::optional<CAddress> addr_opt = ConsumeDeserializable<CAddress>(fuzzed_data_provider);
            if (!addr_opt) {
                break;
            }
            node.AddAddressKnown(*addr_opt);
            break;
        }
        case 6: {
            if (node.m_addr_known == nullptr) {
                break;
            }
            const std::optional<CAddress> addr_opt = ConsumeDeserializable<CAddress>(fuzzed_data_provider);
            if (!addr_opt) {
                break;
            }
            FastRandomContext fast_random_context{ConsumeUInt256(fuzzed_data_provider)};
            node.PushAddress(*addr_opt, fast_random_context);
            break;
        }
        case 7: {
            const std::optional<CInv> inv_opt = ConsumeDeserializable<CInv>(fuzzed_data_provider);
            if (!inv_opt) {
                break;
            }
            node.AddKnownTx(inv_opt->hash);
            break;
        }
        case 8: {
            node.PushTxInventory(ConsumeUInt256(fuzzed_data_provider));
            break;
        }
        case 9: {
            const std::optional<CService> service_opt = ConsumeDeserializable<CService>(fuzzed_data_provider);
            if (!service_opt) {
                break;
            }
            node.SetAddrLocal(*service_opt);
            break;
        }
        case 10: {
            const std::vector<uint8_t> b = ConsumeRandomLengthByteVector(fuzzed_data_provider);
            bool complete;
            node.ReceiveMsgBytes(b, complete);
            break;
        }
        }
    }

    (void)node.GetAddrLocal();
    (void)node.GetAddrName();
    (void)node.GetId();
    (void)node.GetLocalNonce();
    (void)node.GetLocalServices();
    const int ref_count = node.GetRefCount();
    assert(ref_count >= 0);
    (void)node.GetCommonVersion();
    (void)node.RelayAddrsWithConn();

    const NetPermissionFlags net_permission_flags = fuzzed_data_provider.ConsumeBool() ?
                                                        fuzzed_data_provider.PickValueInArray<NetPermissionFlags>({NetPermissionFlags::PF_NONE, NetPermissionFlags::PF_BLOOMFILTER, NetPermissionFlags::PF_RELAY, NetPermissionFlags::PF_FORCERELAY, NetPermissionFlags::PF_NOBAN, NetPermissionFlags::PF_MEMPOOL, NetPermissionFlags::PF_ISIMPLICIT, NetPermissionFlags::PF_ALL}) :
                                                        static_cast<NetPermissionFlags>(fuzzed_data_provider.ConsumeIntegral<uint32_t>());
    (void)node.HasPermission(net_permission_flags);
    (void)node.ConnectedThroughNetwork();
}
