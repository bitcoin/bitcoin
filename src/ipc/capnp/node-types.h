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

//! Specialization of rpcSetTimerInterfaceIfUnset needed because it takes a
//! RPCTimerInterface* argument, which requires custom code to provide a
//! compatible timer.
template <>
struct mp::ProxyServerMethodTraits<ipc::capnp::messages::Node::RpcSetTimerInterfaceIfUnsetParams>
{
    using Context = ServerContext<ipc::capnp::messages::Node,
                                  ipc::capnp::messages::Node::RpcSetTimerInterfaceIfUnsetParams,
                                  ipc::capnp::messages::Node::RpcSetTimerInterfaceIfUnsetResults>;
    static void invoke(Context& context);
};

//! Specialization of rpcUnsetTimerInterface needed because it takes a
//! RPCTimerInterface* argument, which requires custom code to provide a
//! compatible timer.
template <>
struct mp::ProxyServerMethodTraits<ipc::capnp::messages::Node::RpcUnsetTimerInterfaceParams>
{
    using Context = ServerContext<ipc::capnp::messages::Node,
                                  ipc::capnp::messages::Node::RpcUnsetTimerInterfaceParams,
                                  ipc::capnp::messages::Node::RpcUnsetTimerInterfaceResults>;
    static void invoke(Context& context);
};

namespace mp {
//! Specialization of MakeProxyClient for Node to that constructs a client
//! object through a function pointer so client object code relying on
//! net_processing types doesn't need to get linked into the bitcoin-wallet
//! executable.
template <>
inline std::unique_ptr<interfaces::Node> CustomMakeProxyClient<ipc::capnp::messages::Node, interfaces::Node>(
    InvokeContext& context, ipc::capnp::messages::Node::Client&& client)
{
    ipc::capnp::Context& ipc_context = *static_cast<ipc::capnp::Context*>(context.connection.m_loop.m_context);
    return ipc_context.make_node_client(context, kj::mv(client));
}

//! Specialization of MakeProxyServer for Node to that constructs a server
//! object through a function pointer so server object code relying on
//! net_processing types doesn't need to get linked into the bitcoin-wallet
//! executable.
template <>
inline kj::Own<ipc::capnp::messages::Node::Server> CustomMakeProxyServer<ipc::capnp::messages::Node, interfaces::Node>(
    InvokeContext& context, std::shared_ptr<interfaces::Node>&& impl)
{
    ipc::capnp::Context& ipc_context = *static_cast<ipc::capnp::Context*>(context.connection.m_loop.m_context);
    return ipc_context.make_node_server(context, std::move(impl));
}

//! RPCTimerInterface* server-side argument handling. Skips argument so it can
//! be handled by ProxyServerCustom code.
template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
void CustomPassField(TypeList<RPCTimerInterface*>, ServerContext& server_context, const Fn& fn, Args&&... args)
{
    fn.invoke(server_context, std::forward<Args>(args)...);
}

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
