// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#ifndef BITCOIN_IPC_CAPNP_CHAIN_TYPES_H
#define BITCOIN_IPC_CAPNP_CHAIN_TYPES_H

#include <coins.h>
#include <ipc/capnp/chain.capnp.proxy.h>
#include <ipc/capnp/common.capnp.proxy-types.h>
#include <ipc/capnp/handler.capnp.proxy-types.h>
#include <interfaces/chain.h>
#include <kernel/types.h>
#include <mp/type-interface.h>
#include <primitives/block.h>
#include <rpc/server.h>

//! Specialization of handleRpc needed because it takes a CRPCCommand& reference
//! argument, so a manual cleanup callback is needed to free the passed
//! CRPCCommand struct and proxy ActorCallback object.
template <>
struct mp::ProxyServerMethodTraits<ipc::capnp::messages::UnstableChain, ipc::capnp::messages::UnstableChain::HandleRpcParams>
{
    using Context = ServerContext<ipc::capnp::messages::UnstableChain,
                                  ipc::capnp::messages::UnstableChain::HandleRpcParams,
                                  ipc::capnp::messages::UnstableChain::HandleRpcResults>;
    static ::capnp::Void invoke(Context& context);
};

//! Specialization of start method needed to provide CScheduler& reference
//! argument.
template <>
struct mp::ProxyServerMethodTraits<ipc::capnp::messages::ChainClient, ipc::capnp::messages::ChainClient::StartParams>
{
    using ChainContext = ServerContext<ipc::capnp::messages::ChainClient,
                                       ipc::capnp::messages::ChainClient::StartParams,
                                       ipc::capnp::messages::ChainClient::StartResults>;
    static void invoke(ChainContext& context);
};

namespace mp {
//! Serve interfaces::Chain objects sent as stable Chain capabilities with
//! ProxyServer<UnstableChain> servers, and read received Chain capabilities
//! into ProxyClient<UnstableChain> clients. The stable Chain interface has no
//! C++ wrapper of its own because ProxyClient<Chain> could not implement the
//! interfaces::Chain methods that are only in UnstableChain. UnstableChain
//! extends Chain, so an UnstableChain server can be returned where a Chain
//! capability is expected, and C++ code that receives a Chain capability (like
//! the wallet calling Init::makeChain()) can call any interfaces::Chain method.
//!
//! This also means any client holding a stable Chain capability can cast it to
//! UnstableChain and call unstable methods. That is allowed: the Chain schema
//! only describes what is stable, and clients that cast are opting into the
//! same lack of compatibility guarantees as clients calling
//! Init.makeUnstableChain.
template <>
kj::Own<ipc::capnp::messages::Chain::Server>
CustomMakeProxyServer<ipc::capnp::messages::Chain, interfaces::Chain>(InvokeContext& context, std::shared_ptr<interfaces::Chain>&& impl);
template <>
std::unique_ptr<interfaces::Chain>
CustomMakeProxyClient<ipc::capnp::messages::Chain, interfaces::Chain>(InvokeContext& context, ipc::capnp::messages::Chain::Client&& client);

//! Overload CustomBuildMessage, CustomPassMessage, and CustomReadMessage to
//! serialize interfaces::FoundBlock parameters. Custom conversion functions are
//! needed because there is not a 1:1 correspondence between members of the C++
//! FoundBlock class and the Cap'n Proto FoundBlockParam and FoundBlockResult
//! structs. Separate param and result structs are needed because Cap'n Proto
//! only has input and output parameters, not in/out parameters like C++.
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

//! Overload CustomBuildMessage and CustomPassMessage to serialize
//! interfaces::BlockInfo parameters. Custom conversion functions are needed
//! because BlockInfo structs contain pointer and reference members, and custom
//! code is needed to deal with their lifetimes. Specifics are described in the
//! implementation of these functions.
void CustomBuildMessage(InvokeContext& invoke_context,
                        const interfaces::BlockInfo& block,
                        ipc::capnp::messages::BlockInfo::Builder&& builder);
void CustomPassMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::BlockInfo::Reader& reader,
                       ::capnp::Void builder,
                       std::function<void(const interfaces::BlockInfo&)>&& fn);

//! CScheduler& server-side argument handling. Skips argument so it can
//! be handled by ProxyServerMethodTraits ChainClient::Start code.
template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
void CustomPassField(TypeList<CScheduler&>, ServerContext& server_context, const Fn& fn, Args&&... args)
{
    fn.invoke(server_context, std::forward<Args>(args)...);
}

//! CRPCCommand& server-side argument handling. Skips argument so it can
//! be handled by ProxyServerMethodTraits UnstableChain::HandleRpc code.
template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
void CustomPassField(TypeList<const CRPCCommand&>, ServerContext& server_context, const Fn& fn, Args&&... args)
{
    fn.invoke(server_context, std::forward<Args>(args)...);
}

//! Override to avoid assert failures that would happen trying to serialize
//! spent coins. Probably it would be best for Coin serialization code not
//! to assert, but avoiding serialization in this case is harmless.
bool CustomHasValue(InvokeContext& invoke_context, const Coin& coin);

//! Overload CustomBuildMessage and ThrowField to serialize HelpResult.
inline void CustomBuildMessage(InvokeContext& invoke_context, const HelpResult& dest, ipc::capnp::messages::HelpResult::Builder&& builder)
{
    builder.setMessage(dest.what());
}

template <typename Input>
void ThrowField(TypeList<HelpResult>, InvokeContext& invoke_context, Input&& input)
{
    auto data = input.get().getMessage();
    throw HelpResult(std::string(CharCast(data.begin()), data.size()));
}
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_CHAIN_TYPES_H
