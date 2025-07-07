# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xfcbbd3f49874128e;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Common = import "common.capnp";
using Handler = import "handler.capnp";
using Proxy = import "/mp/proxy.capnp";

$Proxy.include("interfaces/tracing.h");
$Proxy.includeTypes("ipc/capnp/tracing-types.h");

interface Tracing $Proxy.wrap("interfaces::Tracing") {
    traceUtxoCache @0 (context :Proxy.Context, callback :UtxoCacheTrace) -> (result :Handler.Handler);
}

interface UtxoCacheTrace $Proxy.wrap("interfaces::UtxoCacheTrace") {
    destroy @0 (context :Proxy.Context) -> ();
    add @1 (utxoInfo :UtxoInfo) -> ();
    spend @2 (utxoInfo :UtxoInfo) -> ();
    uncache @3 (utxoInfo :UtxoInfo) -> ();
}

struct UtxoInfo $Proxy.wrap("interfaces::UtxoInfo") {
    outpointHash @0 :Data $Proxy.name("outpoint_hash");
    outpointN @1 :UInt32 $Proxy.name("outpoint_n");
    height @2 :UInt32 $Proxy.name("height");
    value @3 :Int64 $Proxy.name("value");
    isCoinbase @4 :Bool $Proxy.name("is_coinbase");
}
