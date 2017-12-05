// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

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
} // namespace capnp
} // namespace ipc

namespace mp {
void CustomReadMessage(InvokeContext& invoke_context,
                       ipc::capnp::messages::NodeStats::Reader const& reader,
                       std::tuple<CNodeStats, bool, CNodeStateStats>& node_stats)
{
    CNodeStats& node = std::get<0>(node_stats);
    ReadField(TypeList<CNodeStats>(), invoke_context, Make<ValueField>(reader), ReadDestUpdate(node));
    if ((std::get<1>(node_stats) = reader.hasStateStats())) {
        CNodeStateStats& state = std::get<2>(node_stats);
        ReadField(TypeList<CNodeStateStats>(), invoke_context, Make<ValueField>(reader.getStateStats()),
                  ReadDestUpdate(state));
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
        m_wallet_loader = static_cast<Sub&>(*this).customWalletLoader();
    }
    return *m_wallet_loader;
}
} // namespace mp
