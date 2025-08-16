// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <printer.h>

#include <init.capnp.h>
#include <init.capnp.proxy.h> // NOLINT(misc-include-cleaner) // IWYU pragma: keep

#include <charconv>
#include <cstring>
#include <fstream>
#include <iostream>
#include <kj/async.h>
#include <kj/common.h>
#include <kj/memory.h>
#include <memory>
#include <mp/proxy-io.h>
#include <stdexcept>
#include <string>
#include <system_error>

class PrinterImpl : public Printer
{
public:
    void print(const std::string& message) override { std::cout << "mpprinter: " << message << std::endl; }
};

class InitImpl : public Init
{
public:
    std::unique_ptr<Printer> makePrinter() override { return std::make_unique<PrinterImpl>(); }
};

static void LogPrint(bool raise, const std::string& message)
{
    if (raise) throw std::runtime_error(message);
    std::ofstream("debug.log", std::ios_base::app) << message << std::endl;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cout << "Usage: mpprinter <fd>\n";
        return 1;
    }
    int fd;
    if (std::from_chars(argv[1], argv[1] + strlen(argv[1]), fd).ec != std::errc{}) {
        std::cerr << argv[1] << " is not a number or is larger than an int\n";
        return 1;
    }
    mp::EventLoop loop("mpprinter", LogPrint);
    std::unique_ptr<Init> init = std::make_unique<InitImpl>();
    mp::ServeStream<InitInterface>(loop, fd, *init);
    loop.loop();
    return 0;
}
