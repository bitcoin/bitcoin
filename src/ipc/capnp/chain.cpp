// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <capnp/blob.h>
#include <capnp/capability.h>
#include <capnp/list.h>
#include <coins.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <interfaces/ipc.h>
#include <ipc/capnp/chain-types.h>
#include <ipc/capnp/chain.capnp.h>
#include <ipc/capnp/chain.capnp.proxy.h>
#include <ipc/capnp/common-types.h>
#include <ipc/capnp/context.h>
#include <ipc/capnp/handler.capnp.proxy.h>
#include <mp/proxy-io.h>
#include <mp/proxy-types.h>
#include <mp/util.h>
#include <primitives/block.h>
#include <rpc/server.h>
#include <streams.h>
#include <uint256.h>

#include <assert.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mp {
void CustomBuildMessage(InvokeContext& invoke_context,
                        const interfaces::FoundBlock& dest,
                        ipc::capnp::messages::FoundBlockParam::Builder&& builder)
{
    if (dest.m_hash) builder.setWantHash(true);
    if (dest.m_height) builder.setWantHeight(true);
    if (dest.m_time) builder.setWantTime(true);
    if (dest.m_max_time) builder.setWantMaxTime(true);
    if (dest.m_mtp_time) builder.setWantMtpTime(true);
    if (dest.m_in_active_chain) builder.setWantInActiveChain(true);
    if (dest.m_next_block) CustomBuildMessage(invoke_context, *dest.m_next_block, builder.initNextBlock());
    if (dest.m_data) builder.setWantData(true);
}

void FindBlock(const std::function<void()>& find,
               const ipc::capnp::messages::FoundBlockParam::Reader& reader,
               ipc::capnp::messages::FoundBlockResult::Builder&& builder,
               interfaces::FoundBlock& found_block)
{
    uint256 hash;
    int height = -1;
    int64_t time = -1;
    int64_t max_time = -1;
    int64_t mtp_time = -1;
    bool in_active_chain = -1;
    CBlock data;
    if (reader.getWantHash()) found_block.hash(hash);
    if (reader.getWantHeight()) found_block.height(height);
    if (reader.getWantTime()) found_block.time(time);
    if (reader.getWantMaxTime()) found_block.maxTime(max_time);
    if (reader.getWantMtpTime()) found_block.mtpTime(mtp_time);
    if (reader.getWantInActiveChain()) found_block.inActiveChain(in_active_chain);
    if (reader.getWantData()) found_block.data(data);
    if (reader.hasNextBlock()) {
        interfaces::FoundBlock next_block;
        found_block.nextBlock(next_block);
        FindBlock(find, reader.getNextBlock(), builder.initNextBlock(), next_block);
    } else {
        find();
    }
    if (!found_block.found) return;
    if (reader.getWantHash()) builder.setHash(ipc::capnp::ToArray(ipc::capnp::Serialize(hash)));
    if (reader.getWantHeight()) builder.setHeight(height);
    if (reader.getWantTime()) builder.setTime(time);
    if (reader.getWantMaxTime()) builder.setMaxTime(max_time);
    if (reader.getWantMtpTime()) builder.setMtpTime(mtp_time);
    if (reader.getWantInActiveChain()) builder.setInActiveChain(in_active_chain);
    if (reader.getWantData()) builder.setData(ipc::capnp::ToArray(ipc::capnp::Serialize(data)));
    builder.setFound(true);
}

void CustomPassMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::FoundBlockParam::Reader& reader,
                       ipc::capnp::messages::FoundBlockResult::Builder&& builder,
                       std::function<void(const interfaces::FoundBlock&)>&& fn)
{
    interfaces::FoundBlock found_block;
    FindBlock([&] { fn(found_block); }, reader, std::move(builder), found_block);
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::FoundBlockResult::Reader& reader,
                       const interfaces::FoundBlock& dest)
{
    if (!reader.getFound()) return;
    if (dest.m_hash) *dest.m_hash = ipc::capnp::Unserialize<uint256>(reader.getHash());
    if (dest.m_height) *dest.m_height = reader.getHeight();
    if (dest.m_time) *dest.m_time = reader.getTime();
    if (dest.m_max_time) *dest.m_max_time = reader.getMaxTime();
    if (dest.m_mtp_time) *dest.m_mtp_time = reader.getMtpTime();
    if (dest.m_in_active_chain) *dest.m_in_active_chain = reader.getInActiveChain();
    if (dest.m_next_block) CustomReadMessage(invoke_context, reader.getNextBlock(), *dest.m_next_block);
    if (dest.m_data) *dest.m_data = ipc::capnp::Unserialize<CBlock>(reader.getData());
}

::capnp::Void ProxyServerMethodTraits<ipc::capnp::messages::Chain::HandleRpcParams>::invoke(
    Context& context)
{
    auto params = context.call_context.getParams();
    auto command = params.getCommand();

    CRPCCommand::Actor actor;
    ReadField(TypeList<decltype(actor)>(), context, Make<ValueField>(command.getActor()), ReadDestValue(actor));
    std::vector<std::string> args;
    ReadField(TypeList<decltype(args)>(), context, Make<ValueField>(command.getArgNames()), ReadDestValue(args));

    auto rpc_command = std::make_unique<CRPCCommand>(command.getCategory(), command.getName(), std::move(actor),
                                               std::move(args), command.getUniqueId());
    auto handler = context.proxy_server.m_impl->handleRpc(*rpc_command);
    auto results = context.call_context.getResults();
    auto result = kj::heap<ProxyServer<ipc::capnp::messages::Handler>>(std::shared_ptr<interfaces::Handler>(handler.release()), *context.proxy_server.m_context.connection);
    result->m_context.cleanup.emplace_back([rpc_command = rpc_command.release()] { delete rpc_command; });
    results.setResult(kj::mv(result));
    return {};
}

void ProxyServerMethodTraits<ipc::capnp::messages::ChainClient::StartParams>::invoke(ChainContext& context)
{
    // This method is never called because ChainClient::Start is overridden by
    // WalletClient::Start. The custom implementation is needed just because
    // the CScheduler& argument this is supposed to pass is not serializable.
    assert(0);
}

bool CustomHasValue(InvokeContext& invoke_context, const Coin& coin)
{
    // Spent coins cannot be serialized due to an assert in Coin::Serialize.
    return !coin.IsSpent();
}
} // namespace mp
