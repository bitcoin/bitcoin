# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xe102a54b33a43a20;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("mp::test::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("mp/test/foo.h");
$Proxy.includeTypes("mp/test/foo-types.h");

interface FooInterface $Proxy.wrap("mp::test::FooImplementation") {
    add @0 (a :Int32, b :Int32) -> (result :Int32);
    addOut @19 (a :Int32, b :Int32) -> (ret :Int32);
    addInOut @20 (x :Int32, sum :Int32) -> (sum :Int32);
    mapSize @1 (map :List(Pair(Text, Text))) -> (result :Int32);
    pass @2 (arg :FooStruct) -> (result :FooStruct);
    raise @3 (arg :FooStruct) -> (error :FooStruct $Proxy.exception("mp::test::FooStruct"));
    initThreadMap @4 (threadMap: Proxy.ThreadMap) -> (threadMap :Proxy.ThreadMap);
    callback @5 (context :Proxy.Context, callback :FooCallback, arg: Int32) -> (result :Int32);
    callbackUnique @6 (context :Proxy.Context, callback :FooCallback, arg: Int32) -> (result :Int32);
    callbackShared @7 (context :Proxy.Context, callback :FooCallback, arg: Int32) -> (result :Int32);
    saveCallback @8 (context :Proxy.Context, callback :FooCallback) -> ();
    callbackSaved @9 (context :Proxy.Context, arg: Int32) -> (result :Int32);
    callbackExtended @10 (context :Proxy.Context, callback :ExtendedCallback, arg: Int32) -> (result :Int32);
    passCustom @11 (arg :FooCustom) -> (result :FooCustom);
    passEmpty @12 (arg :FooEmpty) -> (result :FooEmpty);
    passData @24 (arg :Data) -> (result :Data);
    passMessage @13 (arg :FooMessage) -> (result :FooMessage);
    passMutable @14 (arg :FooMutable) -> (arg :FooMutable);
    passEnum @15 (arg :Int32) -> (result :Int32);
    passDouble @23 (arg :Float64) -> (result :Float64);
    passFn @16 (context :Proxy.Context, fn :FooFn) -> (result :Int32);
    callFn @17 () -> ();
    callFnAsync @18 (context :Proxy.Context) -> ();
    callIntFnAsync @21 (context :Proxy.Context, arg :Int32) -> (result :Int32);
    passDataPointers @22 (arg :List(Data)) -> (result :List(Data));
}

interface FooCallback $Proxy.wrap("mp::test::FooCallback") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, arg :Int32) -> (result :Int32);
}

interface ExtendedCallback extends(FooCallback) $Proxy.wrap("mp::test::ExtendedCallback") {
    callExtended @0 (context :Proxy.Context, arg :Int32) -> (result :Int32);
}

interface FooFn $Proxy.wrap("ProxyCallback<std::function<int()>>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context) -> (result :Int32);
}

struct FooStruct $Proxy.wrap("mp::test::FooStruct") {
    name @0 :Text;
    setInt @1 :List(Int32) $Proxy.name("set_int");
    vectorBool @2 :List(Bool) $Proxy.name("vector_bool");
    optionalInt @3 :Int32 $Proxy.name("optional_int");
    hasOptionalInt @4 :Bool;
    unorderedSetInt @5 :List(Int32) $Proxy.name("unordered_set_int");
    mapStringInt @6 :List(StringIntPair) $Proxy.name("map_string_int");
}

struct FooCustom $Proxy.wrap("mp::test::FooCustom") {
    v1 @0 :Text;
    v2 @1 :Int32;
}

struct FooEmpty $Proxy.wrap("mp::test::FooEmpty") {
}

struct FooMessage {
    message @0 :Text;
}

struct FooMutable {
    message @0 :Text;
}

struct Pair(T1, T2) {
    first @0 :T1;
    second @1 :T2;
}

struct StringIntPair {
    first @0 :Text;
    second @1 :Int32;
}
