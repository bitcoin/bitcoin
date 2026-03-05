// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/syscoin-config.h>
#endif

#include <common/run_command.h>

#if defined(HAVE_BOOST_PROCESS) || defined(ENABLE_EXTERNAL_SIGNER)
#define RUN_COMMAND_HAS_BOOST_PROCESS 1
#include <boost/process.hpp>
#endif
#include <tinyformat.h>
#include <univalue.h>

#include <stdexcept>
#include <thread>

#ifdef RUN_COMMAND_HAS_BOOST_PROCESS
namespace {
namespace bp = boost::process;

void CollectStream(bp::ipstream& stream, std::string& output)
{
    std::string line;
    while (std::getline(stream, line)) {
        if (!output.empty()) output.push_back('\n');
        output += line;
    }
}

std::string WaitCollectOutputFromChild(bp::child& process, bp::ipstream& stdout_stream, bp::ipstream& stderr_stream, const std::string& command_for_errors)
{
    std::string result;
    std::string error;
    std::thread stdout_reader([&]() { CollectStream(stdout_stream, result); });
    std::thread stderr_reader([&]() { CollectStream(stderr_stream, error); });

    const auto join_reader_threads = [&]() {
        if (stdout_reader.joinable()) stdout_reader.join();
        if (stderr_reader.joinable()) stderr_reader.join();
    };

    try {
        process.wait();
    } catch (...) {
        join_reader_threads();
        throw;
    }
    join_reader_threads();

    const int n_error = process.exit_code();
    if (n_error) {
        throw std::runtime_error(strprintf("RunCommandParseJSON error: process(%s) returned %d: %s\n", command_for_errors, n_error, error));
    }

    return result;
}

UniValue ParseJSONResult(const std::string& result)
{
    UniValue result_json;
    if (!result_json.read(result)) throw std::runtime_error("Unable to parse JSON: " + result);
    return result_json;
}
} // namespace
#endif

UniValue RunCommandParseJSON(const std::string& str_command, const std::string& str_std_in)
{
#ifdef RUN_COMMAND_HAS_BOOST_PROCESS
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

    return ParseJSONResult(WaitCollectOutputFromChild(c, stdout_stream, stderr_stream, str_command));
#else
    throw std::runtime_error("RunCommandParseJSON requires Boost::Process support (configure with --with-boost-process).");
#endif
}

std::string RunCommand(const std::string& str_command, const std::string& str_std_in)
{
#ifdef RUN_COMMAND_HAS_BOOST_PROCESS
    bp::opstream stdin_stream;
    bp::ipstream stdout_stream;
    bp::ipstream stderr_stream;

    if (str_command.empty()) return {};

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

    return WaitCollectOutputFromChild(c, stdout_stream, stderr_stream, str_command);
#else
    throw std::runtime_error("RunCommand requires Boost::Process support (configure with --with-boost-process).");
#endif
}

UniValue RunCommandParseJSON(const std::vector<std::string>& command_and_args, const std::string& str_std_in)
{
#ifdef RUN_COMMAND_HAS_BOOST_PROCESS
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

    return ParseJSONResult(WaitCollectOutputFromChild(c, stdout_stream, stderr_stream, executable));
#else
    throw std::runtime_error("RunCommandParseJSON requires Boost::Process support (configure with --with-boost-process).");
#endif
}

std::string RunCommand(const std::vector<std::string>& command_and_args, const std::string& str_std_in)
{
#ifdef RUN_COMMAND_HAS_BOOST_PROCESS
    bp::opstream stdin_stream;
    bp::ipstream stdout_stream;
    bp::ipstream stderr_stream;

    if (command_and_args.empty()) return {};

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

    return WaitCollectOutputFromChild(c, stdout_stream, stderr_stream, executable);
#else
    throw std::runtime_error("RunCommand requires Boost::Process support (configure with --with-boost-process).");
#endif
}
