# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xf2c5cfa319406aa6;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("interfaces/chain.h");
$Proxy.include("interfaces/echo.h");
$Proxy.include("interfaces/init.h");
$Proxy.include("interfaces/node.h");
$Proxy.includeTypes("ipc/capnp/init-types.h");

using Chain = import "chain.capnp";
using Common = import "common.capnp";
using Echo = import "echo.capnp";
using Node = import "node.capnp";
using Wallet = import "wallet.capnp";

interface Init $Proxy.wrap("interfaces::Init") {
    construct @0 (threadMap: Proxy.ThreadMap) -> (threadMap :Proxy.ThreadMap);
    makeEcho @1 (context :Proxy.Context) -> (result :Echo.Echo);
    makeNode @2 (context :Proxy.Context) -> (result :Node.Node);
    makeChain @3 (context :Proxy.Context) -> (result :Chain.Chain);
    makeWalletLoader @4 (context :Proxy.Context, globalArgs :Common.GlobalArgs, chain :Chain.Chain) -> (result :Wallet.WalletLoader);
}
