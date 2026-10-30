# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xcd2c6232cb484a28;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("ipc/capnp/common.h");
$Proxy.includeTypes("ipc/capnp/common-types.h");

struct BlockRef $Proxy.wrap("interfaces::BlockRef") {
    hash @0 :Data;
    height @1 :Int32;
}

struct PSBTFillOptions $Proxy.wrap("common::PSBTFillOptions") {
    sign @0 :Bool = true;
    sighashType @1 :Int32 $Proxy.name("sighash_type");
    hasSighashType @2 :Bool;
    finalize @3 :Bool = true;
    bip32Derivs @4 :Bool $Proxy.name("bip32_derivs");
}

struct Settings $Proxy.wrap("common::Settings") {
   forcedSettings @0 :List(Pair(Text, Text)) $Proxy.name("forced_settings");
   commandLineOptions @1 :List(Pair(Text, List(Text))) $Proxy.name("command_line_options");
   rwSettings @2 :List(Pair(Text, Text)) $Proxy.name("rw_settings");
   roConfig @3 :List(Pair(Text, List(Pair(Text, List(Text))))) $Proxy.name("ro_config");
}

struct GlobalArgs $Proxy.count(0) {
   settings @0 :Settings;
   configPath @1 : Text;
}

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

struct Expected(Value, Error) {
    union {
        value @0 :Value;
        error @1 :Error;
    }
}

# Wrapper for util::Expected<void, Error>
struct ExpectedVoid(Error) {
    union {
        value @0 :Void;
        error @1 :Error;
    }
}

struct Pair(Key, Value) {
    key @0 :Key;
    value @1 :Value;
}

struct PairInt64(Key) {
    key @0 :Key;
    value @1 :Int64;
}

struct PairUInt64(Key) {
    key @0 :Key;
    value @1 :UInt64;
}
