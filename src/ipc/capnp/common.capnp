# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xcd2c6232cb484a28;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.includeTypes("ipc/capnp/common-types.h");

struct BlockRef $Proxy.wrap("interfaces::BlockRef") {
    hash @0 :Data;
    height @1 :Int32;
}
