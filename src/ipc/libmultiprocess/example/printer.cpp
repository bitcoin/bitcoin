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

static void LogPrint(mp::LogMessage log_data)
{
    if (log_data.level == mp::Log::Raise) throw std::runtime_error(log_data.message);
    std::ofstream("debug.log", std::ios_base::app) << log_data.message << std::endl;
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
