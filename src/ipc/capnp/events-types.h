// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_EVENTS_TYPES_H
#define BITCOIN_IPC_CAPNP_EVENTS_TYPES_H

#include <interfaces/events.h>
#include <ipc/capnp/common-types.h>
#include <ipc/capnp/events.capnp.proxy.h>
#include <primitives/block.h>
#include <util/overloaded.h>

#include <stdexcept>
#include <variant>

namespace mp {
//! Build the Event union by hand, since libmultiprocess doesn't map
//! std::variant to capnp unions. Union members use the generated code.
inline void CustomBuildMessage(InvokeContext& invoke_context,
                               const interfaces::Event& src,
                               ipc::capnp::messages::Event::Builder&& builder)
{
    using Struct = ProxyStruct<ipc::capnp::messages::Event>;
    builder.setSequence(src.sequence);
    std::visit(util::Overloaded{
        [&](const interfaces::BlockConnected& e) {
            BuildField(TypeList<interfaces::BlockEvent>(), invoke_context, Make<StructField, Struct::BlockConnectedAccessor>(builder), e);
        },
        [&](const interfaces::BlockDisconnected& e) {
            BuildField(TypeList<interfaces::BlockEvent>(), invoke_context, Make<StructField, Struct::BlockDisconnectedAccessor>(builder), e);
        },
        [&](const interfaces::TxAddedToMempool& e) {
            BuildField(TypeList<interfaces::TxAddedToMempool>(), invoke_context, Make<StructField, Struct::TxAddedToMempoolAccessor>(builder), e);
        },
        [&](const interfaces::TxRemovedFromMempool& e) {
            BuildField(TypeList<interfaces::TxRemovedFromMempool>(), invoke_context, Make<StructField, Struct::TxRemovedFromMempoolAccessor>(builder), e);
        },
    }, src.data);
}

//! Read one union member. Wire is the wrapped type, which differs from
//! LocalType for BlockConnected and BlockDisconnected (both BlockEvent).
template <typename Accessor, typename LocalType, typename Wire = LocalType>
LocalType ReadEventData(InvokeContext& invoke_context, const ipc::capnp::messages::Event::Reader& reader)
{
    LocalType value;
    ReadField(TypeList<Wire>(), invoke_context, Make<StructField, Accessor>(reader), ReadDestUpdate(static_cast<Wire&>(value)));
    return value;
}

inline void CustomReadMessage(InvokeContext& invoke_context,
                              const ipc::capnp::messages::Event::Reader& reader,
                              interfaces::Event& dest)
{
    using Struct = ProxyStruct<ipc::capnp::messages::Event>;
    using Which = ipc::capnp::messages::Event::Which;
    dest.sequence = reader.getSequence();
    switch (reader.which()) {
    case Which::BLOCK_CONNECTED:
        dest.data = ReadEventData<Struct::BlockConnectedAccessor, interfaces::BlockConnected, interfaces::BlockEvent>(invoke_context, reader);
        return;
    case Which::BLOCK_DISCONNECTED:
        dest.data = ReadEventData<Struct::BlockDisconnectedAccessor, interfaces::BlockDisconnected, interfaces::BlockEvent>(invoke_context, reader);
        return;
    case Which::TX_ADDED_TO_MEMPOOL:
        dest.data = ReadEventData<Struct::TxAddedToMempoolAccessor, interfaces::TxAddedToMempool>(invoke_context, reader);
        return;
    case Which::TX_REMOVED_FROM_MEMPOOL:
        dest.data = ReadEventData<Struct::TxRemovedFromMempoolAccessor, interfaces::TxRemovedFromMempool>(invoke_context, reader);
        return;
    }
    throw std::runtime_error("Unknown event type received over IPC");
}
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_EVENTS_TYPES_H
