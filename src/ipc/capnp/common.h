// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_COMMON_H
#define BITCOIN_IPC_CAPNP_COMMON_H

#include <ipc/capnp/common.capnp.h>
#include <util/system.h>

#include <string>

class RPCTimerInterface;

namespace mp {
struct InvokeContext;
} // namespace mp

namespace ipc {
namespace capnp {
//! Wrapper around GlobalArgs struct to expose public members.
struct GlobalArgs : public ArgsManager
{
    using ArgsManager::cs_args;
    using ArgsManager::m_network;
    using ArgsManager::m_network_only_args;
    using ArgsManager::m_settings;
};

//! GlobalArgs client-side argument handling. Builds message from ::gArgs variable.
void BuildGlobalArgs(mp::InvokeContext& invoke_context, messages::GlobalArgs::Builder&& builder);

//! GlobalArgs server-side argument handling. Reads message into ::gArgs variable.
void ReadGlobalArgs(mp::InvokeContext& invoke_context, const messages::GlobalArgs::Reader& reader);

//! GlobalArgs network string accessor.
std::string GlobalArgsNetwork();
} // namespace capnp
} // namespace ipc

#endif // BITCOIN_IPC_CAPNP_COMMON_H
