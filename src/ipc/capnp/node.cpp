// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <capnp/list.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <ipc/capnp/context.h>
#include <ipc/capnp/node-types.h>
#include <ipc/capnp/node.capnp.h>
#include <ipc/capnp/node.capnp.proxy-types.h>
#include <ipc/capnp/node.capnp.proxy.h>
#include <ipc/capnp/node.h>
#include <kj/async-io.h>
#include <kj/async-prelude.h>
#include <kj/async.h>
#include <kj/memory.h>
#include <kj/time.h>
#include <kj/timer.h>
#include <kj/units.h>
#include <mp/proxy-io.h>
#include <mp/proxy-types.h>
#include <mp/util.h>
#include <rpc/server.h>
#include <sys/types.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>

class CNodeStats;
struct CNodeStateStats;

namespace ipc {
namespace capnp {
void SetupNodeClient(ipc::Context& context)
{
    static_cast<Context&>(context).make_node_client = mp::MakeProxyClient<messages::Node, interfaces::Node>;
}

void SetupNodeServer(ipc::Context& context)
{
    static_cast<Context&>(context).make_node_server = mp::MakeProxyServer<messages::Node, interfaces::Node>;
}

class RpcTimer : public ::RPCTimerBase
{
public:
    RpcTimer(mp::EventLoop& loop, std::function<void(void)>& fn, int64_t millis)
        : m_fn(fn), m_promise(loop.m_io_context.provider->getTimer()
                                  .afterDelay(millis * kj::MILLISECONDS)
                                  .then([this]() { m_fn(); })
                                  .eagerlyEvaluate(nullptr))
    {
    }
    ~RpcTimer() noexcept override {}

    std::function<void(void)> m_fn;
    kj::Promise<void> m_promise;
};

class RpcTimerInterface : public ::RPCTimerInterface
{
public:
    RpcTimerInterface(mp::EventLoop& loop) : m_loop(loop) {}
    const char* Name() override { return "Cap'n Proto"; }
    RPCTimerBase* NewTimer(std::function<void(void)>& fn, int64_t millis) override
    {
        RPCTimerBase* result;
        m_loop.sync([&] { result = new RpcTimer(m_loop, fn, millis); });
        return result;
    }
    mp::EventLoop& m_loop;
};
} // namespace capnp
} // namespace ipc

namespace mp {
void ProxyServerMethodTraits<ipc::capnp::messages::Node::RpcSetTimerInterfaceIfUnsetParams>::invoke(Context& context)
{
    if (!context.proxy_server.m_timer_interface) {
        auto timer = std::make_unique<ipc::capnp::RpcTimerInterface>(context.proxy_server.m_context.connection->m_loop);
        context.proxy_server.m_timer_interface = std::move(timer);
    }
    context.proxy_server.m_impl->rpcSetTimerInterfaceIfUnset(context.proxy_server.m_timer_interface.get());
}

void ProxyServerMethodTraits<ipc::capnp::messages::Node::RpcUnsetTimerInterfaceParams>::invoke(Context& context)
{
    context.proxy_server.m_impl->rpcUnsetTimerInterface(context.proxy_server.m_timer_interface.get());
    context.proxy_server.m_timer_interface.reset();
}

void CustomReadMessage(InvokeContext& invoke_context,
                       ipc::capnp::messages::NodeStats::Reader const& reader,
                       std::tuple<CNodeStats, bool, CNodeStateStats>& node_stats)
{
    CNodeStats& node = std::get<0>(node_stats);
    ReadField(TypeList<CNodeStats>(), invoke_context, Make<ValueField>(reader), ReadDestValue(node));
    if ((std::get<1>(node_stats) = reader.hasStateStats())) {
        CNodeStateStats& state = std::get<2>(node_stats);
        ReadField(TypeList<CNodeStateStats>(), invoke_context, Make<ValueField>(reader.getStateStats()),
                  ReadDestValue(state));
    }
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const capnp::Data::Reader& reader,
                       CSubNet& subnet)
{
    std::string subnet_str = ipc::capnp::ToString(reader);
    subnet = LookupSubNet(subnet_str);
}

void CustomBuildMessage(InvokeContext& invoke_context,
                        const banmap_t& banmap,
                        ipc::capnp::messages::Banmap::Builder&& builder)
{
    builder.setJson(BanMapToJson(banmap).write());
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::Banmap::Reader& reader,
                       banmap_t& banmap)
{
    UniValue banmap_json;
    if (!banmap_json.read(ipc::capnp::ToString(reader.getJson()))) {
        throw std::runtime_error("Could not parse banmap json");
    }
    BanMapFromJson(banmap_json, banmap);
}

interfaces::WalletLoader& ProxyClientCustom<ipc::capnp::messages::Node, interfaces::Node>::walletLoader()
{
    if (!m_wallet_loader) {
        m_wallet_loader = self().customWalletLoader();
    }
    return *m_wallet_loader;
}
} // namespace mp
