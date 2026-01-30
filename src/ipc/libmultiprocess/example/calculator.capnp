# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xb67dbf34061180a9;

using Cxx = import "/capnp/c++.capnp";
using Proxy = import "/mp/proxy.capnp";

$Proxy.include("calculator.h");
$Proxy.includeTypes("types.h");

interface CalculatorInterface $Proxy.wrap("Calculator") {
    destroy @0 (context :Proxy.Context) -> ();
    solveEquation @1 (context :Proxy.Context, eqn: Text) -> ();
}
