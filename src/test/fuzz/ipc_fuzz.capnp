# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xf918ff05f5bf04d1;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("test::fuzz::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("test/fuzz/ipc_fuzz.h");
$Proxy.includeTypes("test/fuzz/ipc_fuzz_types.h");

interface IpcFuzzInterface $Proxy.wrap("IpcFuzzImplementation") {
    add @0 (a :Int32, b :Int32) -> (result :Int32);
    passOutPoint @1 (arg :Data) -> (result :Data);
    passVectorUint8 @2 (arg :Data) -> (result :Data);
    passScript @3 (arg :Data) -> (result :Data);
}
