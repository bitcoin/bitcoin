# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0x9c3505dc45e146ac;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Common = import "common.capnp";
using Proxy = import "/mp/proxy.capnp";
$Proxy.include("interfaces/rpc.h");
$Proxy.includeTypes("ipc/capnp/rpc-types.h");

interface Rpc $Proxy.wrap("interfaces::Rpc") {
    executeRpc @0 (context :Proxy.Context, request :Text, uri :Text, user :Text) -> (result :Text);
}
