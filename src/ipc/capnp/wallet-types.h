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

class CKey;
namespace wallet {
class CCoinControl;
} // namespace wallet

namespace mp {
void CustomBuildMessage(InvokeContext& invoke_context,
                        const CTxDestination& dest,
                        ipc::capnp::messages::TxDestination::Builder&& builder);
void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::TxDestination::Reader& reader,
                       CTxDestination& dest);
void CustomBuildMessage(InvokeContext& invoke_context,
                        const WitnessUnknown& dest,
                        ipc::capnp::messages::WitnessUnknown::Builder&& builder);
void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::WitnessUnknown::Reader& reader,
                       WitnessUnknown& dest);
void CustomBuildMessage(InvokeContext& invoke_context, const CKey& key, ipc::capnp::messages::Key::Builder&& builder);
void CustomReadMessage(InvokeContext& invoke_context, const ipc::capnp::messages::Key::Reader& reader, CKey& key);
void CustomBuildMessage(InvokeContext& invoke_context,
                        const wallet::CCoinControl& coin_control,
                        ipc::capnp::messages::CoinControl::Builder&& builder);
void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::CoinControl::Reader& reader,
                       wallet::CCoinControl& coin_control);

//! CustomReadField implementation needed for converting ::capnp::Data messages
//! to PKHash objects because PKHash doesn't currently have a Span constructor.
//! This could be dropped and replaced with a more generic ::capnp::Data
//! CustomReadField function as described in common-types.h near the generic
//! CustomBuildField function which is already used to build ::capnp::Data
//! messages from PKHash objects.
template <typename Reader, typename ReadDest>
decltype(auto) CustomReadField(
    TypeList<PKHash>, Priority<1>, InvokeContext& invoke_context, Reader&& reader, ReadDest&& read_dest)
{
    return read_dest.construct(ipc::capnp::ToBlob<uint160>(reader.get()));
}
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_WALLET_TYPES_H
