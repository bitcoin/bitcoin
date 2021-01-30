// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/util.h>
#include <version.h>

void FillNode(FuzzedDataProvider& fuzzed_data_provider, CNode& node, bool init_version) noexcept
{
    const ServiceFlags remote_services = ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS);
    const NetPermissionFlags permission_flags = ConsumeWeakEnum(fuzzed_data_provider, ALL_NET_PERMISSION_FLAGS);
    const int32_t version = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(MIN_PEER_PROTO_VERSION, std::numeric_limits<int32_t>::max());
    const bool filter_txs = fuzzed_data_provider.ConsumeBool();

    node.nServices = remote_services;
    node.m_permissionFlags = permission_flags;
    if (init_version) {
        node.nVersion = version;
        node.SetCommonVersion(std::min(version, PROTOCOL_VERSION));
    }
    if (node.m_tx_relay != nullptr) {
        LOCK(node.m_tx_relay->cs_filter);
        node.m_tx_relay->fRelayTxes = filter_txs;
    }
}
