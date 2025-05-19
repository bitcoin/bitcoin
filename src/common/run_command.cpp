// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <common/run_command.h>

#include <logging.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/string.h>

#ifdef ENABLE_SUBPROCESS
#include <util/subprocess.h>
#endif // ENABLE_SUBPROCESS

#include <thread>

#ifndef WIN32
std::string ShellEscape(const std::string& arg)
{
    std::string escaped = arg;
    util::ReplaceAll(escaped, "'", "'\"'\"'");
    return "'" + escaped + "'";
}
#endif

void RunShell(const std::string& str_command)
{
#ifdef ENABLE_SUBPROCESS
    namespace sp = subprocess;
    try {
#ifdef WIN32
        // Emulate _system behavior.
        auto c = sp::Popen("cmd.exe /c " + str_command);
#else
        // Emulate system(3) behavior, but don't leak file descriptors.
        auto c = sp::Popen({"/bin/sh", "-c", str_command}, sp::close_fds{true});
#endif
        int err = c.wait();
        if (err) {
            LogError("RunShell: %s returned %d", str_command, err);
        }
    } catch (const std::runtime_error &e) {
        LogError("RunShell: %s exception %s", str_command, e.what());
    }
#else
    LogError("RunShell: compiled without subprocess support");
#endif
}

void RunShellInThread(const std::string& str_command)
{
    std::thread t(RunShell, str_command);
    t.detach(); // thread runs free
}

UniValue RunCommandParseJSON(const std::string& str_command, const std::string& str_std_in)
{
#ifdef ENABLE_SUBPROCESS
    namespace sp = subprocess;

    UniValue result_json;
    std::istringstream stdout_stream;
    std::istringstream stderr_stream;

    if (str_command.empty()) return UniValue::VNULL;

    auto c = sp::Popen(str_command, sp::input{sp::PIPE}, sp::output{sp::PIPE}, sp::error{sp::PIPE}, sp::close_fds{true});
    if (!str_std_in.empty()) {
        c.send(str_std_in);
    }
    auto [out_res, err_res] = c.communicate();
    stdout_stream.str(std::string{out_res.buf.begin(), out_res.buf.end()});
    stderr_stream.str(std::string{err_res.buf.begin(), err_res.buf.end()});

    std::string result;
    std::string error;
    std::getline(stdout_stream, result);
    std::getline(stderr_stream, error);

    const int n_error = c.retcode();
    if (n_error) throw std::runtime_error(strprintf("RunCommandParseJSON error: process(%s) returned %d: %s\n", str_command, n_error, error));
    if (!result_json.read(result)) throw std::runtime_error("Unable to parse JSON: " + result);

    return result_json;
#else
    throw std::runtime_error("Compiled without subprocess support (required for external signing).");
#endif // ENABLE_SUBPROCESS
}
