// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init.capnp.h>
#include <init.capnp.proxy.h>

#include <cstring> // IWYU pragma: keep
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <kj/async.h>
#include <kj/common.h>
#include <memory>
#include <mp/proxy-io.h>
#include <mp/util.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace fs = std::filesystem;

static auto Spawn(mp::EventLoop& loop, const std::string& process_argv0, const std::string& new_exe_name)
{
    int pid;
    const int fd = mp::SpawnProcess(pid, [&](int fd) -> std::vector<std::string> {
        fs::path path = process_argv0;
        path.remove_filename();
        path.append(new_exe_name);
        return {path.string(), std::to_string(fd)};
    });
    return std::make_tuple(mp::ConnectStream<InitInterface>(loop, fd), pid);
}

static void LogPrint(mp::LogMessage log_data)
{
    if (log_data.level == mp::Log::Raise) throw std::runtime_error(log_data.message);
    std::ofstream("debug.log", std::ios_base::app) << log_data.message << std::endl;
}

int main(int argc, char** argv)
{
    if (argc != 1) {
        std::cout << "Usage: mpexample\n";
        return 1;
    }

    std::promise<mp::EventLoop*> promise;
    std::thread loop_thread([&] {
        mp::EventLoop loop("mpexample", LogPrint);
        promise.set_value(&loop);
        loop.loop();
    });
    mp::EventLoop* loop = promise.get_future().get();

    auto [printer_init, printer_pid] = Spawn(*loop, argv[0], "mpprinter");
    auto [calc_init, calc_pid] = Spawn(*loop, argv[0], "mpcalculator");
    auto calc = calc_init->makeCalculator(printer_init->makePrinter());
    while (true) {
        std::string eqn;
        std::cout << "Enter the equation, or \"exit\" to quit: ";
        std::getline(std::cin, eqn);
        if (eqn == "exit") break;
        calc->solveEquation(eqn);
    }
    calc.reset();
    calc_init.reset();
    mp::WaitProcess(calc_pid);
    printer_init.reset();
    mp::WaitProcess(printer_pid);
    loop_thread.join();
    std::cout << "Bye!" << std::endl;
    return 0;
}
