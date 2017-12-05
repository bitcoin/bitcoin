// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_NODE_TYPES_H
#define BITCOIN_IPC_CAPNP_NODE_TYPES_H

#include <ipc/capnp/common.capnp.proxy-types.h>
#include <ipc/capnp/context.h>
#include <ipc/capnp/node.capnp.proxy.h>
#include <ipc/capnp/wallet.capnp.proxy-types.h>

class CNodeStats;
struct CNodeStateStats;

namespace mp {
void CustomBuildMessage(InvokeContext& invoke_context,
                        const banmap_t& banmap,
                        ipc::capnp::messages::Banmap::Builder&& builder);

void CustomReadMessage(InvokeContext& invoke_context,
                        const ipc::capnp::messages::Banmap::Reader& reader,
                        banmap_t& banmap);

template <typename Value, typename Output>
void CustomBuildField(TypeList<std::tuple<CNodeStats, bool, CNodeStateStats>>,
                      Priority<1>,
                      InvokeContext& invoke_context,
                      Value&& stats,
                      Output&& output)
{
    BuildField(TypeList<CNodeStats>(), invoke_context, output, std::get<0>(stats));
    if (std::get<1>(stats)) {
        auto message_builder = output.get();
        using Accessor = ProxyStruct<ipc::capnp::messages::NodeStats>::StateStatsAccessor;
        StructField<Accessor, ipc::capnp::messages::NodeStats::Builder> field_output{message_builder};
        BuildField(TypeList<CNodeStateStats>(), invoke_context, field_output, std::get<2>(stats));
    }
}

void CustomReadMessage(InvokeContext& invoke_context,
                       ipc::capnp::messages::NodeStats::Reader const& reader,
                       std::tuple<CNodeStats, bool, CNodeStateStats>& node_stats);

template <typename Value, typename Output>
void CustomBuildField(TypeList<CSubNet>, Priority<1>, InvokeContext& invoke_context, Value&& subnet, Output&& output)
{
    std::string subnet_str = subnet.ToString();
    auto result = output.init(subnet_str.size());
    memcpy(result.begin(), subnet_str.data(), subnet_str.size());
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const capnp::Data::Reader& reader,
                       CSubNet& subnet);
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_NODE_TYPES_H
