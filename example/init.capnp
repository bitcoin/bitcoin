# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xba5a7448664901b1;

using Cxx = import "/capnp/c++.capnp";
using Proxy = import "/mp/proxy.capnp";
using Calculator = import "calculator.capnp";
using Printer = import "printer.capnp";

$Proxy.include("calculator.h");
$Proxy.include("init.h");
$Proxy.include("printer.h");
$Proxy.includeTypes("types.h");

interface InitInterface $Proxy.wrap("Init") {
    construct @0 (threadMap: Proxy.ThreadMap) -> (threadMap :Proxy.ThreadMap);
    makeCalculator @1 (context :Proxy.Context, print :Printer.PrinterInterface) -> (result :Calculator.CalculatorInterface);
    makePrinter @2 (context :Proxy.Context) -> (result :Printer.PrinterInterface);
}
