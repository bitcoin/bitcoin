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

std::string WaitCollectOutputFromChild(
    bp::child& process,
    bp::ipstream& stdout_stream,
    bp::ipstream& stderr_stream,
    const std::string& caller_for_errors,
    const std::string& command_for_errors)
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
        throw std::runtime_error(strprintf("%s error: process(%s) returned %d: %s\n", caller_for_errors, command_for_errors, n_error, error));
    }

    return result;
}

UniValue ParseJSONResult(const std::string& result)
{
    UniValue result_json;
    if (!result_json.read(result)) throw std::runtime_error("Unable to parse JSON: " + result);
    return result_json;
}

template <typename EmptyResultFn, typename ChildFactoryFn, typename ResultProcessorFn>
auto RunCommandWithPipes(
    bool has_command,
    const std::string& str_std_in,
    const std::string& caller_for_errors,
    const std::string& command_for_errors,
    EmptyResultFn empty_result_fn,
    ChildFactoryFn create_child_fn,
    ResultProcessorFn result_processor_fn) -> decltype(result_processor_fn(std::string{}))
{
    if (!has_command) return empty_result_fn();

    bp::opstream stdin_stream;
    bp::ipstream stdout_stream;
    bp::ipstream stderr_stream;

    bp::child process = create_child_fn(stdin_stream, stdout_stream, stderr_stream);
    if (!str_std_in.empty()) {
        stdin_stream << str_std_in << std::endl;
    }
    stdin_stream.pipe().close();

    const std::string output =
        WaitCollectOutputFromChild(process, stdout_stream, stderr_stream, caller_for_errors, command_for_errors);
    return result_processor_fn(output);
}
} // namespace
#endif

UniValue RunCommandParseJSON(const std::string& str_command, const std::string& str_std_in)
{
#ifdef RUN_COMMAND_HAS_BOOST_PROCESS
    return RunCommandWithPipes(
        !str_command.empty(),
        str_std_in,
        "RunCommandParseJSON",
        str_command,
        []() { return UniValue::VNULL; },
        [&](bp::opstream& stdin_stream, bp::ipstream& stdout_stream, bp::ipstream& stderr_stream) {
            return bp::child(
                str_command,
                bp::std_out > stdout_stream,
                bp::std_err > stderr_stream,
                bp::std_in < stdin_stream
            );
        },
        [](const std::string& output) { return ParseJSONResult(output); });
#else
    throw std::runtime_error("RunCommandParseJSON requires Boost::Process support (configure with --with-boost-process).");
#endif
}

std::string RunCommand(const std::string& str_command, const std::string& str_std_in)
{
#ifdef RUN_COMMAND_HAS_BOOST_PROCESS
    return RunCommandWithPipes(
        !str_command.empty(),
        str_std_in,
        "RunCommand",
        str_command,
        []() { return std::string{}; },
        [&](bp::opstream& stdin_stream, bp::ipstream& stdout_stream, bp::ipstream& stderr_stream) {
            return bp::child(
                str_command,
                bp::std_out > stdout_stream,
                bp::std_err > stderr_stream,
                bp::std_in < stdin_stream
            );
        },
        [](const std::string& output) { return output; });
#else
    throw std::runtime_error("RunCommand requires Boost::Process support (configure with --with-boost-process).");
#endif
}

UniValue RunCommandParseJSON(const std::vector<std::string>& command_and_args, const std::string& str_std_in)
{
#ifdef RUN_COMMAND_HAS_BOOST_PROCESS
    const bool has_command = !command_and_args.empty();
    const std::string executable = has_command ? command_and_args.front() : std::string{};
    std::vector<std::string> args;
    if (command_and_args.size() > 1) {
        args.assign(command_and_args.begin() + 1, command_and_args.end());
    }

    return RunCommandWithPipes(
        has_command,
        str_std_in,
        "RunCommandParseJSON",
        executable,
        []() { return UniValue::VNULL; },
        [&](bp::opstream& stdin_stream, bp::ipstream& stdout_stream, bp::ipstream& stderr_stream) {
            return bp::child(
                bp::exe = executable,
                bp::args = args,
                bp::std_out > stdout_stream,
                bp::std_err > stderr_stream,
                bp::std_in < stdin_stream
            );
        },
        [](const std::string& output) { return ParseJSONResult(output); });
#else
    throw std::runtime_error("RunCommandParseJSON requires Boost::Process support (configure with --with-boost-process).");
#endif
}

std::string RunCommand(const std::vector<std::string>& command_and_args, const std::string& str_std_in)
{
#ifdef RUN_COMMAND_HAS_BOOST_PROCESS
    const bool has_command = !command_and_args.empty();
    const std::string executable = has_command ? command_and_args.front() : std::string{};
    std::vector<std::string> args;
    if (command_and_args.size() > 1) {
        args.assign(command_and_args.begin() + 1, command_and_args.end());
    }

    return RunCommandWithPipes(
        has_command,
        str_std_in,
        "RunCommand",
        executable,
        []() { return std::string{}; },
        [&](bp::opstream& stdin_stream, bp::ipstream& stdout_stream, bp::ipstream& stderr_stream) {
            return bp::child(
                bp::exe = executable,
                bp::args = args,
                bp::std_out > stdout_stream,
                bp::std_err > stderr_stream,
                bp::std_in < stdin_stream
            );
        },
        [](const std::string& output) { return output; });
#else
    throw std::runtime_error("RunCommand requires Boost::Process support (configure with --with-boost-process).");
#endif
}
