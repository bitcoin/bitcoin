// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_CHAIN_TYPES_H
#define BITCOIN_IPC_CAPNP_CHAIN_TYPES_H

#include <ipc/capnp/chain.capnp.proxy.h>
#include <ipc/capnp/common.capnp.proxy-types.h>
#include <ipc/capnp/handler.capnp.proxy-types.h>
#include <interfaces/chain.h>
#include <policy/fees.h>
#include <rpc/server.h>

//! Specialization of handleRpc needed because it takes a CRPCCommand& reference
//! argument, so a manual cleanup callback is needed to free the passed
//! CRPCCommand struct and proxy ActorCallback object.
template <>
struct mp::ProxyServerMethodTraits<ipc::capnp::messages::Chain::HandleRpcParams>
{
    using Context = ServerContext<ipc::capnp::messages::Chain,
                                  ipc::capnp::messages::Chain::HandleRpcParams,
                                  ipc::capnp::messages::Chain::HandleRpcResults>;
    static ::capnp::Void invoke(Context& context);
};

//! Specialization of start method needed to provide CScheduler& reference
//! argument.
template <>
struct mp::ProxyServerMethodTraits<ipc::capnp::messages::ChainClient::StartParams>
{
    using ChainContext = ServerContext<ipc::capnp::messages::ChainClient,
                                       ipc::capnp::messages::ChainClient::StartParams,
                                       ipc::capnp::messages::ChainClient::StartResults>;
    static void invoke(ChainContext& context);
};

namespace mp {
void CustomBuildMessage(InvokeContext& invoke_context,
                        const interfaces::FoundBlock& dest,
                        ipc::capnp::messages::FoundBlockParam::Builder&& builder);
void CustomPassMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::FoundBlockParam::Reader& reader,
                       ipc::capnp::messages::FoundBlockResult::Builder&& builder,
                       std::function<void(const interfaces::FoundBlock&)>&& fn);
void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::FoundBlockResult::Reader& reader,
                       const interfaces::FoundBlock& dest);

void CustomBuildMessage(InvokeContext& invoke_context,
                        const interfaces::BlockInfo& block,
                        ipc::capnp::messages::BlockInfo::Builder&& builder);
void CustomPassMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::BlockInfo::Reader& reader,
                       ::capnp::Void builder,
                       std::function<void(const interfaces::BlockInfo&)>&& fn);

//! CScheduler& server-side argument handling. Skips argument so it can
//! be handled by ProxyServerCustom code.
template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
void CustomPassField(TypeList<CScheduler&>, ServerContext& server_context, const Fn& fn, Args&&... args)
{
    fn.invoke(server_context, std::forward<Args>(args)...);
}

//! CRPCCommand& server-side argument handling. Skips argument so it can
//! be handled by ProxyServerCustom code.
template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
void CustomPassField(TypeList<const CRPCCommand&>, ServerContext& server_context, const Fn& fn, Args&&... args)
{
    fn.invoke(server_context, std::forward<Args>(args)...);
}

//! Override to avoid assert failures that would happen trying to serialize
//! spent coins. Probably it would be best for Coin serialization code not
//! to assert, but avoiding serialization in this case is harmless.
bool CustomHasValue(InvokeContext& invoke_context, const Coin& coin);
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_CHAIN_TYPES_H
