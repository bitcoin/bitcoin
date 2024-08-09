// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_COMMON_H
#define BITCOIN_IPC_CAPNP_COMMON_H

#include <ipc/capnp/common.capnp.h>
#include <common/args.h>

#include <string>

class RPCTimerInterface;

namespace mp {
struct InvokeContext;
} // namespace mp

namespace ipc {
namespace capnp {
//! GlobalArgs client-side argument handling. Builds message from ::gArgs variable.
void BuildGlobalArgs(mp::InvokeContext& invoke_context, messages::GlobalArgs::Builder&& builder);

//! GlobalArgs server-side argument handling. Reads message into ::gArgs variable.
void ReadGlobalArgs(mp::InvokeContext& invoke_context, const messages::GlobalArgs::Reader& reader);
} // namespace capnp
} // namespace ipc

#endif // BITCOIN_IPC_CAPNP_COMMON_H
