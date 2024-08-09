// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_INIT_TYPES_H
#define BITCOIN_IPC_CAPNP_INIT_TYPES_H

#include <ipc/capnp/chain.capnp.proxy-types.h>
#include <ipc/capnp/echo.capnp.proxy-types.h>
#include <ipc/capnp/init.capnp.proxy.h>
#include <ipc/capnp/node.capnp.proxy-types.h>

namespace mp {
//! Specialization of makeWalletLoader needed because it takes a Chain& reference
//! argument, not a unique_ptr<Chain> argument, so a manual cleanup
//! callback is needed to clean up the ProxyServer<messages::Chain> proxy object.
template <>
struct ProxyServerMethodTraits<ipc::capnp::messages::Init::MakeWalletLoaderParams>
{
    using Context = ServerContext<ipc::capnp::messages::Init,
                                  ipc::capnp::messages::Init::MakeWalletLoaderParams,
                                  ipc::capnp::messages::Init::MakeWalletLoaderResults>;
    static capnp::Void invoke(Context& context);
};

//! Chain& server-side argument handling. Skips argument so it can
//! be handled by ProxyServerCustom code.
template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
void CustomPassField(TypeList<interfaces::Chain&>, ServerContext& server_context, const Fn& fn, Args&&... args)
{
    fn.invoke(server_context, std::forward<Args>(args)...);
}
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_INIT_TYPES_H
