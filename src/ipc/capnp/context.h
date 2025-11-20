// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_CONTEXT_H
#define BITCOIN_IPC_CAPNP_CONTEXT_H

#include <ipc/capnp/node.capnp.h>
#include <ipc/context.h>

namespace interfaces {
class Node;
} // namespace interfaces
namespace mp {
struct InvokeContext;
} // namespace mp

namespace ipc {
namespace capnp {
//! Cap'n Proto context struct. Generally the parent ipc::Context struct should
//! be used instead of this struct to give all IPC protocols access to
//! application state, so there aren't unnecessary differences between IPC
//! protocols. But this specialized struct can be used to pass capnp-specific
//! function and object types to capnp hooks.
struct Context : ipc::Context
{
    using MakeNodeClient = std::unique_ptr<interfaces::Node>(mp::InvokeContext& context,
                                                             messages::Node::Client&& client);
    using MakeNodeServer = kj::Own<messages::Node::Server>(mp::InvokeContext& context,
                                                           std::shared_ptr<interfaces::Node> impl);
    MakeNodeClient* make_node_client = nullptr;
    MakeNodeServer* make_node_server = nullptr;
};
} // namespace capnp
} // namespace ipc

#endif // BITCOIN_IPC_CAPNP_CONTEXT_H
