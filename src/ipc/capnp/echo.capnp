# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0x888b4f7f51e691f7;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("interfaces/echo.h");
$Proxy.includeTypes("ipc/capnp/echo-types.h");

interface Echo $Proxy.wrap("interfaces::Echo") {
    destroy @0 (context :Proxy.Context) -> ();
    echo @1 (context :Proxy.Context, echo: Text) -> (result :Text);
}
