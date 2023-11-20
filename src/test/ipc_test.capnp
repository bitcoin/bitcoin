# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xd71b0fc8727fdf83;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("gen");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("test/ipc_test.h");

interface FooInterface $Proxy.wrap("FooImplementation") {
    add @0 (a :Int32, b :Int32) -> (result :Int32);
}
