# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xfb94225379685324;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Common = import "common.capnp";
using Proxy = import "/mp/proxy.capnp";
$Proxy.include("interfaces/events.h");
$Proxy.includeTypes("ipc/capnp/events-types.h");

interface Events $Proxy.wrap("interfaces::Events") {
    destroy @0 (context :Proxy.Context) -> ();
    # timeout is in milliseconds, default forever.
    waitNext @1 (context :Proxy.Context, after :UInt64, maxEvents :UInt32 = 1000, timeout :Float64 = Common.maxDouble) -> (result :List(Event));
    interrupt @2 () -> ();
}

struct EventOptions $Proxy.wrap("interfaces::EventOptions") {
    includeData @0 :Bool $Proxy.name("include_data");
}

# An event type is enabled when its field is set.
struct EventsOptions $Proxy.wrap("interfaces::EventsOptions") {
    blockConnected @0 :EventOptions $Proxy.name("block_connected");
    blockDisconnected @1 :EventOptions $Proxy.name("block_disconnected");
    txAddedToMempool @2 :EventOptions $Proxy.name("tx_added_to_mempool");
    txRemovedFromMempool @3 :EventOptions $Proxy.name("tx_removed_from_mempool");
}

enum RemovalReason {
    expiry @0;
    sizelimit @1;
    reorg @2;
    conflict @3;
    replaced @4;
    block @5;
}

# Not wrapped, because libmultiprocess doesn't map unions. Converted to and
# from interfaces::Event by hand in events-types.h.
struct Event {
    sequence @0 :UInt64;
    union {
        # Sent for every block connected to the active chain, including
        # during initial block download. Blocks connected by the assumeutxo
        # background chainstate are not reported.
        blockConnected @1 :BlockEvent;
        # Sent for every block disconnected from the active chain.
        blockDisconnected @2 :BlockEvent;
        # Sent only when a transaction is accepted to the mempool. In a reorg,
        # transactions from disconnected blocks that are accepted back to the
        # mempool are sent again.
        txAddedToMempool @3 :TxAddedToMempool;
        # Sent for every removal from the mempool, including transactions
        # included in a connected block (reason block). Removals with reason
        # block are sent before the blockConnected events of all blocks
        # connected at once, and not during initial block download.
        txRemovedFromMempool @4 :TxRemovedFromMempool;
    }
}

# In all event structs, data holds the serialized block or transaction and is
# unset unless include_data was requested for that event type.
struct BlockEvent $Proxy.wrap("interfaces::BlockEvent") {
    data @0 :Data;
    hash @1 :Data;
    height @2 :Int32;
}

struct TxAddedToMempool $Proxy.wrap("interfaces::TxAddedToMempool") {
    data @0 :Data;
    txid @1 :Data;
    wtxid @2 :Data;
    mempoolSequence @3 :UInt64 $Proxy.name("mempool_sequence");
    fee @4 :Int64;
    vsize @5 :Int64;
}

# mempoolSequence may arrive out of order, so use Event.sequence to detect
# dropped events.
struct TxRemovedFromMempool $Proxy.wrap("interfaces::TxRemovedFromMempool") {
    data @0 :Data;
    txid @1 :Data;
    wtxid @2 :Data;
    mempoolSequence @3 :UInt64 $Proxy.name("mempool_sequence");
    reason @4 :RemovalReason;
}
