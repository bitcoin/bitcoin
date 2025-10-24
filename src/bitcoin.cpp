// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <clientversion.h>
#include <common/args.h>
#include <common/system.h>
#include <util/fs.h>
#include <util/exec.h>
#include <util/strencodings.h>
#include <util/translation.h>

#include <iostream>
#include <string>
#include <tinyformat.h>
#include <vector>

const TranslateFn G_TRANSLATION_FUN{nullptr};

static constexpr auto HELP_USAGE = R"(Usage: %s [OPTIONS] COMMAND...

Options:
  -m, --multiprocess     Run multiprocess binaries bitcoin-node, bitcoin-gui.
  -M, --monolithic       Run monolithic binaries bitcoind, bitcoin-qt. (Default behavior)
  -v, --version          Show version information
  -h, --help             Show full help message

Commands:
  gui [ARGS]     Start GUI, equivalent to running 'bitcoin-qt [ARGS]' or 'bitcoin-gui [ARGS]'.
  node [ARGS]    Start node, equivalent to running 'bitcoind [ARGS]' or 'bitcoin-node [ARGS]'.
  rpc [ARGS]     Call RPC method, equivalent to running 'bitcoin-cli -named [ARGS]'.
  wallet [ARGS]  Call wallet command, equivalent to running 'bitcoin-wallet [ARGS]'.
  tx [ARGS]      Manipulate hex-encoded transactions, equivalent to running 'bitcoin-tx [ARGS]'.
  help           Show full help message.
)";

static constexpr auto HELP_FULL = R"(
Additional less commonly used commands:
  bench [ARGS]      Run bench command, equivalent to running 'bench_bitcoin [ARGS]'.
  chainstate [ARGS] Run bitcoin kernel chainstate util, equivalent to running 'bitcoin-chainstate [ARGS]'.
  test [ARGS]       Run unit tests, equivalent to running 'test_bitcoin [ARGS]'.
  test-gui [ARGS]   Run GUI unit tests, equivalent to running 'test_bitcoin-qt [ARGS]'.
)";

static constexpr auto HELP_SHORT = R"(
Run '%s help' to see additional commands (e.g. for testing and debugging).
)";

struct CommandLine {
    std::optional<bool> use_multiprocess;
    bool show_version{false};
    bool show_help{false};
    std::string_view command;
    std::vector<const char*> args;
};

CommandLine ParseCommandLine(int argc, char* argv[]);
bool UseMultiprocess(const CommandLine& cmd);
static void ExecCommand(const std::vector<const char*>& args, std::string_view argv0);

int main(int argc, char* argv[])
{
    SetupEnvironment();

    try {
        CommandLine cmd{ParseCommandLine(argc, argv)};
        if (cmd.show_version) {
            tfm::format(std::cout, "%s version %s\n%s", CLIENT_NAME, FormatFullVersion(), FormatParagraph(LicenseInfo()));
            return EXIT_SUCCESS;
        }

        std::string exe_name{fs::PathToString(fs::PathFromString(argv[0]).filename())};
        std::vector<const char*> args;
        if (cmd.show_help || cmd.command.empty()) {
            tfm::format(std::cout, HELP_USAGE, exe_name);
            if (cmd.show_help) {
                tfm::format(std::cout, HELP_FULL);
                return EXIT_SUCCESS;
            } else {
                tfm::format(std::cout, HELP_SHORT, exe_name);
                return EXIT_FAILURE;
            }
        } else if (cmd.command == "gui") {
            args.emplace_back(UseMultiprocess(cmd) ? "bitcoin-gui" : "bitcoin-qt");
        } else if (cmd.command == "node") {
            args.emplace_back(UseMultiprocess(cmd) ? "bitcoin-node" : "bitcoind");
        } else if (cmd.command == "rpc") {
            args.emplace_back("bitcoin-cli");
            // Since "bitcoin rpc" is a new interface that doesn't need to be
            // backward compatible, enable -named by default so it is convenient
            // for callers to use a mix of named and unnamed parameters. Callers
            // can override this by specifying -nonamed, but should not need to
            // unless they are passing string values containing '=' characters
            // as unnamed parameters.
            args.emplace_back("-named");
        } else if (cmd.command == "wallet") {
            args.emplace_back("bitcoin-wallet");
        } else if (cmd.command == "tx") {
            args.emplace_back("bitcoin-tx");
        } else if (cmd.command == "bench") {
            args.emplace_back("bench_bitcoin");
        } else if (cmd.command == "chainstate") {
            args.emplace_back("bitcoin-chainstate");
        } else if (cmd.command == "test") {
            args.emplace_back("test_bitcoin");
        } else if (cmd.command == "test-gui") {
            args.emplace_back("test_bitcoin-qt");
        } else if (cmd.command == "util") {
            args.emplace_back("bitcoin-util");
        } else {
            throw std::runtime_error(strprintf("Unrecognized command: '%s'", cmd.command));
        }
        if (!args.empty()) {
            args.insert(args.end(), cmd.args.begin(), cmd.args.end());
            ExecCommand(args, argv[0]);
        }
    } catch (const std::exception& e) {
        tfm::format(std::cerr, "Error: %s\nTry '%s --help' for more information.\n", e.what(), argv[0]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

CommandLine ParseCommandLine(int argc, char* argv[])
{
    CommandLine cmd;
    cmd.args.reserve(argc);
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (!cmd.command.empty()) {
            cmd.args.emplace_back(argv[i]);
        } else if (arg == "-m" || arg == "--multiprocess") {
            cmd.use_multiprocess = true;
        } else if (arg == "-M" || arg == "--monolithic") {
            cmd.use_multiprocess = false;
        } else if (arg == "-v" || arg == "--version") {
            cmd.show_version = true;
        } else if (arg == "-h" || arg == "--help" || arg == "help") {
            cmd.show_help = true;
        } else if (arg.starts_with("-")) {
            throw std::runtime_error(strprintf("Unknown option: %s", arg));
        } else if (!arg.empty()) {
            cmd.command = arg;
        }
    }
    return cmd;
}

bool UseMultiprocess(const CommandLine& cmd)
{
    // If -m or -M options were explicitly specified, there is no need to
    // further parse arguments to determine which to use.
    if (cmd.use_multiprocess) return *cmd.use_multiprocess;

    ArgsManager args;
    args.SetDefaultFlags(ArgsManager::ALLOW_ANY);
    std::string error_message;
    auto argv{cmd.args};
    argv.insert(argv.begin(), nullptr);
    if (!args.ParseParameters(argv.size(), argv.data(), error_message)) {
        tfm::format(std::cerr, "Warning: failed to parse subcommand command line options: %s\n", error_message);
    }
    if (!args.ReadConfigFiles(error_message, true)) {
        tfm::format(std::cerr, "Warning: failed to parse subcommand config: %s\n", error_message);
    }
    args.SelectConfigNetwork(args.GetChainTypeString());

    // If any -ipc* options are set these need to be processed by a
    // multiprocess-capable binary.
    return args.IsArgSet("-ipcbind") || args.IsArgSet("-ipcconnect") || args.IsArgSet("-ipcfd");
}

//! Execute the specified bitcoind, bitcoin-qt or other command line in `args`
//! using src, bin and libexec directory paths relative to this executable, where
//! the path to this executable is specified in `wrapper_argv0`.
//!
//! @param args Command line arguments to execute, where first argument should
//!             be a relative path to a bitcoind, bitcoin-qt or other executable
//!             that will be located on the PATH or relative to wrapper_argv0.
//!
//! @param wrapper_argv0 String containing first command line argument passed to
//!                      main() to run the current executable. This is used to
//!                      help determine the path to the current executable and
//!                      how to look for new executables.
//
//! @note This function doesn't currently print anything but can be debugged
//! from the command line using strace or dtrace like:
//!
//!     strace -e trace=execve -s 10000 build/bin/bitcoin ...
//!     dtrace -n 'proc:::exec-success  /pid == $target/ { trace(curpsinfo->pr_psargs); }' -c ...
static void ExecCommand(const std::vector<const char*>& args, std::string_view wrapper_argv0)
{
    // Construct argument string for execvp
    std::vector<const char*> exec_args{args};
    exec_args.emplace_back(nullptr);

    // Try to call ExecVp with given exe path.
    auto try_exec = [&](fs::path exe_path, bool allow_notfound = true) {
        std::string exe_path_str{fs::PathToString(exe_path)};
        exec_args[0] = exe_path_str.c_str();
        if (util::ExecVp(exec_args[0], (char*const*)exec_args.data()) == -1) {
            if (allow_notfound && errno == ENOENT) return false;
            throw std::system_error(errno, std::system_category(), strprintf("execvp failed to execute '%s'", exec_args[0]));
        }
        throw std::runtime_error("execvp returned unexpectedly");
    };

    // Get the wrapper executable path.
    const fs::path wrapper_path{util::GetExePath(wrapper_argv0)};

    // Try to resolve any symlinks and figure out the directory containing the wrapper executable.
    std::error_code ec;
    auto wrapper_dir{fs::weakly_canonical(wrapper_path, ec)};
    if (wrapper_dir.empty()) wrapper_dir = wrapper_path; // Restore previous path if weakly_canonical failed.
    wrapper_dir = wrapper_dir.parent_path();

    // Get path of the executable to be invoked.
    const fs::path arg0{fs::PathFromString(args[0])};

    // Decide whether to fall back to the operating system to search for the
    // specified executable. Avoid doing this if it looks like the wrapper
    // executable was invoked by path, rather than by search, to avoid
    // unintentionally launching system executables in a local build.
    // (https://github.com/bitcoin/bitcoin/pull/31375#discussion_r1861814807)
    const bool fallback_os_search{!fs::PathFromString(std::string{wrapper_argv0}).has_parent_path()};

    // If wrapper is installed in a bin/ directory, look for target executable
    // in libexec/
    (wrapper_dir.filename() == "bin" && try_exec(wrapper_dir.parent_path() / "libexec" / arg0.filename())) ||
#ifdef WIN32
    // Otherwise check the "daemon" subdirectory in a windows install.
    (!wrapper_dir.empty() && try_exec(wrapper_dir / "daemon" / arg0.filename())) ||
#endif
    // Otherwise look for target executable next to current wrapper
    (!wrapper_dir.empty() && try_exec(wrapper_dir / arg0.filename(), fallback_os_search)) ||
    // Otherwise just look on the system path.
    (fallback_os_search && try_exec(arg0.filename(), false));
}
