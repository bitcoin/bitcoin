# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0x893db95f456ed0e3;

using Cxx = import "/capnp/c++.capnp";
using Proxy = import "/mp/proxy.capnp";

$Proxy.include("printer.h");
$Proxy.includeTypes("types.h");

interface PrinterInterface $Proxy.wrap("Printer") {
    destroy @0 (context :Proxy.Context) -> ();
    print @1 (context :Proxy.Context, text: Text) -> ();
}
