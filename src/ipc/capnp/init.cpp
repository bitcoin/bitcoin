// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <capnp/capability.h>
#include <interfaces/chain.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <interfaces/wallet.h>
#include <ipc/capnp/chain.capnp.h>
#include <ipc/capnp/chain.capnp.proxy.h>
#include <ipc/capnp/context.h>
#include <ipc/capnp/init-types.h>
#include <ipc/capnp/init.capnp.h>
#include <ipc/capnp/init.capnp.proxy.h>
#include <mp/proxy-io.h>

#include <memory>
#include <utility>

namespace mp {
template <typename Interface> struct ProxyClient;

::capnp::Void ProxyServerMethodTraits<ipc::capnp::messages::Init::MakeWalletLoaderParams>::invoke(Context& context)
{
    auto params = context.call_context.getParams();
    auto chain = std::make_unique<ProxyClient<ipc::capnp::messages::Chain>>(
        params.getChain(), context.proxy_server.m_context.connection, /* destroy_connection= */ false);
    auto wallet_loader = context.proxy_server.m_impl->makeWalletLoader(*chain);
    auto results = context.call_context.getResults();
    auto result = kj::heap<ProxyServer<ipc::capnp::messages::WalletLoader>>(std::shared_ptr<interfaces::WalletLoader>(wallet_loader.release()), *context.proxy_server.m_context.connection);
    result->m_context.cleanup.emplace_back([chain = chain.release()] { delete chain; });
    results.setResult(kj::mv(result));
    return {};
}
} // namespace mp
