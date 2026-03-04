// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/syscoin-config.h>
#endif

#include <common/run_command.h>

#ifdef HAVE_BOOST_PROCESS
#include <boost/process.hpp>
#endif
#include <tinyformat.h>
#include <univalue.h>

#include <stdexcept>
#include <thread>

UniValue RunCommandParseJSON(const std::string& str_command, const std::string& str_std_in)
{
#ifdef HAVE_BOOST_PROCESS
    namespace bp = boost::process;

    UniValue result_json;
    bp::opstream stdin_stream;
    bp::ipstream stdout_stream;
    bp::ipstream stderr_stream;

    if (str_command.empty()) return UniValue::VNULL;

    bp::child c(
        str_command,
        bp::std_out > stdout_stream,
        bp::std_err > stderr_stream,
        bp::std_in < stdin_stream
    );
    if (!str_std_in.empty()) {
        stdin_stream << str_std_in << std::endl;
    }
    stdin_stream.pipe().close();

    auto collect_stream = [](bp::ipstream& stream, std::string& output) {
        std::string line;
        while (std::getline(stream, line)) {
            if (!output.empty()) output.push_back('\n');
            output += line;
        }
    };

    std::string result;
    std::string error;
    std::thread stdout_reader([&]() { collect_stream(stdout_stream, result); });
    std::thread stderr_reader([&]() { collect_stream(stderr_stream, error); });

    c.wait();
    stdout_reader.join();
    stderr_reader.join();

    const int n_error = c.exit_code();
    if (n_error) throw std::runtime_error(strprintf("RunCommandParseJSON error: process(%s) returned %d: %s\n", str_command, n_error, error));
    if (!result_json.read(result)) throw std::runtime_error("Unable to parse JSON: " + result);

    return result_json;
#else
    throw std::runtime_error("RunCommandParseJSON requires Boost::Process support (configure with --with-boost-process).");
#endif
}

UniValue RunCommandParseJSON(const std::vector<std::string>& command_and_args, const std::string& str_std_in)
{
#ifdef HAVE_BOOST_PROCESS
    namespace bp = boost::process;

    UniValue result_json;
    bp::opstream stdin_stream;
    bp::ipstream stdout_stream;
    bp::ipstream stderr_stream;

    if (command_and_args.empty()) return UniValue::VNULL;

    const std::string executable = command_and_args.front();
    std::vector<std::string> args;
    if (command_and_args.size() > 1) {
        args.assign(command_and_args.begin() + 1, command_and_args.end());
    }

    bp::child c(
        bp::exe = executable,
        bp::args = args,
        bp::std_out > stdout_stream,
        bp::std_err > stderr_stream,
        bp::std_in < stdin_stream
    );
    if (!str_std_in.empty()) {
        stdin_stream << str_std_in << std::endl;
    }
    stdin_stream.pipe().close();

    auto collect_stream = [](bp::ipstream& stream, std::string& output) {
        std::string line;
        while (std::getline(stream, line)) {
            if (!output.empty()) output.push_back('\n');
            output += line;
        }
    };

    std::string result;
    std::string error;
    std::thread stdout_reader([&]() { collect_stream(stdout_stream, result); });
    std::thread stderr_reader([&]() { collect_stream(stderr_stream, error); });

    c.wait();
    stdout_reader.join();
    stderr_reader.join();

    const int n_error = c.exit_code();
    if (n_error) {
        throw std::runtime_error(strprintf("RunCommandParseJSON error: process(%s) returned %d: %s\n", executable, n_error, error));
    }
    if (!result_json.read(result)) throw std::runtime_error("Unable to parse JSON: " + result);

    return result_json;
#else
    throw std::runtime_error("RunCommandParseJSON requires Boost::Process support (configure with --with-boost-process).");
#endif
}
