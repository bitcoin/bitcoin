# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xf918ff05f5bf04d1;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("test::fuzz::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("ipc/test/fuzz/ipc_fuzz.h");
$Proxy.includeTypes("ipc/test/fuzz/ipc_fuzz_types.h");

interface IpcFuzzInterface $Proxy.wrap("IpcFuzzImplementation") {
    add @0 (a :Int32, b :Int32) -> (result :Int32);
    passOutPoint @1 (arg :Data) -> (result :Data);
    passVectorUint8 @2 (arg :Data) -> (result :Data);
    passScript @3 (arg :Data) -> (result :Data);
    passUniValue @4 (arg :Text) -> (result :Text);
    passTransaction @5 (arg :Data) -> (result :Data);
    initThreadMap @6 (threadMap :Proxy.ThreadMap) -> (threadMap :Proxy.ThreadMap);
    callCallback @7 (context :Proxy.Context, callback :IpcFuzzCallback, arg :Int32) -> (result :Int32);
    consumeTransaction @8 (arg :Data) -> ();
    consumeUniValue @9 (arg :Text) -> ();
}

interface IpcFuzzCallback $Proxy.wrap("IpcFuzzCallback") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, arg :Int32) -> (result :Int32);
}
