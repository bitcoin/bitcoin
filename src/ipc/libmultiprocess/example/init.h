// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EXAMPLE_INIT_H
#define EXAMPLE_INIT_H

#include <calculator.h>
#include <memory>
#include <printer.h>

class Init
{
public:
    virtual ~Init() = default;
    virtual std::unique_ptr<Printer> makePrinter() { return nullptr; }
    virtual std::unique_ptr<Calculator> makeCalculator(std::unique_ptr<Printer> printer) { return nullptr; }
};

#endif // EXAMPLE_INIT_H
