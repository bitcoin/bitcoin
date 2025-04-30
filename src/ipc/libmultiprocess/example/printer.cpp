// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
    mp::SocketId socket{mp::StartSpawned(argv[1])};
    mp::EventLoop loop("mpprinter", LogPrint);
    std::unique_ptr<Init> init = std::make_unique<InitImpl>();
    mp::Stream stream{loop.m_io_context.lowLevelProvider->wrapSocketFd(socket, kj::LowLevelAsyncIoProvider::TAKE_OWNERSHIP)};
    mp::ServeStream<InitInterface>(loop, std::move(stream), *init);
    loop.loop();
    return 0;
}
