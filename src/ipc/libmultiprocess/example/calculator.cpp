// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <calculator.h>
#include <charconv>
#include <fstream>
#include <init.capnp.h>
#include <init.capnp.proxy.h> // NOLINT(misc-include-cleaner)
#include <init.h>
#include <iostream>
#include <memory>
#include <mp/proxy-io.h>
#include <printer.h>
#include <stdexcept>
#include <string>
#include <utility>

class CalculatorImpl : public Calculator
{
public:
    CalculatorImpl(std::unique_ptr<Printer> printer) : m_printer(std::move(printer)) {}
    void solveEquation(const std::string& eqn) override { m_printer->print("Wow " + eqn + ", that's a tough one.\n"); }
    std::unique_ptr<Printer> m_printer;
};

class InitImpl : public Init
{
public:
    std::unique_ptr<Calculator> makeCalculator(std::unique_ptr<Printer> printer) override
    {
        return std::make_unique<CalculatorImpl>(std::move(printer));
    }
};

static void LogPrint(bool raise, const std::string& message)
{
    if (raise) throw std::runtime_error(message);
    std::ofstream("debug.log", std::ios_base::app) << message << std::endl;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cout << "Usage: mpcalculator <fd>\n";
        return 1;
    }
    int fd;
    if (std::from_chars(argv[1], argv[1] + strlen(argv[1]), fd).ec != std::errc{}) {
        std::cerr << argv[1] << " is not a number or is larger than an int\n";
        return 1;
    }
    mp::EventLoop loop("mpcalculator", LogPrint);
    std::unique_ptr<Init> init = std::make_unique<InitImpl>();
    mp::ServeStream<InitInterface>(loop, fd, *init);
    loop.loop();
    return 0;
}
