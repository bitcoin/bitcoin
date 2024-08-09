# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xcd2c6232cb484a28;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.includeTypes("ipc/capnp/common-types.h");

struct BilingualStr $Proxy.wrap("bilingual_str") {
    original @0 :Text;
    translated @1 :Text;
}

struct Result(Value) {
    value @0 :Value;
    error @1: BilingualStr;
}

# Wrapper for util::Result<void>
struct ResultVoid(Value) {
    error @0: BilingualStr;
}

struct Pair(Key, Value) {
    key @0 :Key;
    value @1 :Value;
}

struct PairInt64(Key) {
    key @0 :Key;
    value @1 :Int64;
}
