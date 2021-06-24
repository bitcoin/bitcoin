// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_WALLET_TYPES_H
#define BITCOIN_IPC_CAPNP_WALLET_TYPES_H

#include <ipc/capnp/chain.capnp.proxy-types.h>
#include <ipc/capnp/common.capnp.proxy-types.h>
#include <ipc/capnp/wallet.capnp.proxy.h>
#include <scheduler.h>
#include <wallet/wallet.h>

class CCoinControl;
class CKey;

namespace mp {
void CustomBuildMessage(InvokeContext& invoke_context,
                        const CTxDestination& dest,
                        ipc::capnp::messages::TxDestination::Builder&& builder);
void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::TxDestination::Reader& reader,
                       CTxDestination& dest);
void CustomBuildMessage(InvokeContext& invoke_context, const CKey& key, ipc::capnp::messages::Key::Builder&& builder);
void CustomReadMessage(InvokeContext& invoke_context, const ipc::capnp::messages::Key::Reader& reader, CKey& key);
void CustomBuildMessage(InvokeContext& invoke_context,
                        const CCoinControl& coin_control,
                        ipc::capnp::messages::CoinControl::Builder&& builder);
void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::CoinControl::Reader& reader,
                       CCoinControl& coin_control);

template <typename Reader, typename ReadDest>
decltype(auto) CustomReadField(
    TypeList<PKHash>, Priority<1>, InvokeContext& invoke_context, Reader&& reader, ReadDest&& read_dest)
{
    return read_dest.construct(ipc::capnp::ToBlob<uint160>(reader.get()));
}
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_WALLET_TYPES_H
