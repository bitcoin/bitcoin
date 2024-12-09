// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <clientversion.h>
#include <util/fs.h>
#include <util/strencodings.h>

#include <iostream>
#include <optional>
#include <string>
#include <tinyformat.h>
#include <vector>

#ifdef WIN32
#include <process.h>
#include <windows.h>
#define execvp _execvp
#else
#include <unistd.h>
#endif

extern const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

static constexpr auto HELP_USAGE = R"(Usage: %1$s [OPTIONS] COMMAND...

Commands (run help command for more information):
  {gui,daemon,rpc,wallet,test,help}

Options:
  -m, --multiprocess     Run multiprocess binaries bitcoin-node, bitcoin-gui.\n"
  -M, --monolothic       Run monolithic binaries bitcoind, bitcoin-qt. (Default behavior)\n"
  -v, --version          Show version information
  -h, --help             Show this help message
)";

static constexpr auto HELP_COMMANDS = R"(Command overview:

%1$s gui [ARGS]     Start GUI, equivalent to running 'bitcoin-qt [ARGS]' or 'bitcoin-gui [ARGS]'.
%1$s daemon [ARGS]  Start daemon, equivalent to running 'bitcoind [ARGS]' or 'bitcoin-node [ARGS]'.
%1$s rpc [ARGS]     Call RPC method, equivalent to running 'bitcoin-cli -named [ARGS]'.
%1$s wallet [ARGS]  Call wallet command, equivalent to running 'bitcoin-wallet [ARGS]'.
%1$s tx [ARGS]      Manipulate hex-encoded transactions, equivalent to running 'bitcoin-tx [ARGS]'.
%1$s test [ARGS]    Run unit tests, equivalent to running 'test_bitcoin [ARGS]'.
%1$s help           Show this help message.
)";

struct CommandLine {
    bool use_multiprocess{false};
    bool show_version{false};
    bool show_help{false};
    std::string_view command;
    std::vector<std::string_view> args;
};

CommandLine ParseCommandLine(int argc, char* argv[]);
void ExecCommand(const std::vector<std::string>& args, std::string_view argv0);

int main(int argc, char* argv[])
{
    try {
        CommandLine cmd{ParseCommandLine(argc, argv)};
        if (cmd.show_version) {
            std::cout << FormatParagraph(LicenseInfo());
        }
        if (cmd.show_help || cmd.command.empty()) {
            tfm::format(std::cout, HELP_USAGE, argv[0]);
        }

        std::vector<std::string> args;
        if (cmd.command == "gui") {
            args.emplace_back(cmd.use_multiprocess ? "qt/bitcoin-gui" : "qt/bitcoin-qt");
        } else if (cmd.command == "daemon") {
            args.emplace_back(cmd.use_multiprocess ? "bitcoin-node" : "bitcoind");
        } else if (cmd.command == "rpc") {
            args.emplace_back("bitcoin-cli");
            args.emplace_back("-named");
        } else if (cmd.command == "wallet") {
            args.emplace_back("bitcoin-wallet");
        } else if (cmd.command == "tx") {
            args.emplace_back("bitcoin-tx");
        } else if (cmd.command == "test") {
            args.emplace_back("test/test_bitcoin");
        } else if (cmd.command == "help") {
            tfm::format(std::cout, HELP_COMMANDS, argv[0]);
        } else if (cmd.command == "mine") { // undocumented, used by tests
            args.emplace_back("bitcoin-mine");
        } else if (cmd.command == "util") { // undocumented, used by tests
            args.emplace_back("bitcoin-util");
        } else if (!cmd.command.empty()){
            throw std::runtime_error(strprintf("Unrecognized command: '%s'", cmd.command));
        }
        if (!args.empty()) {
            args.insert(args.end(), cmd.args.begin(), cmd.args.end());
            ExecCommand(args, argv[0]);
        }
    } catch (const std::exception& e) {
        tfm::format(std::cerr, "Error: %s\nTry '%s --help' for more information.", e.what(), argv[0]);
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
            cmd.args.emplace_back(arg);
        } else if (arg == "-m" || arg == "--multiprocess") {
            cmd.use_multiprocess = true;
        } else if (arg == "-M" || arg == "--monolithic") {
            cmd.use_multiprocess = false;
        } else if (arg == "-v" || arg == "--version") {
            cmd.show_version = true;
        } else if (arg == "-h" || arg == "--help") {
            cmd.show_help = true;
        } else if (arg.starts_with("-")) {
            throw std::runtime_error(strprintf("Unknown option: %s", arg));
        } else if (!arg.empty()) {
            cmd.command = arg;
        }
    }
    return cmd;
}

// Execute the specified bitcoind, bitcoin-qt or other command line in `args`
// using src, bin and libexec directory paths relative to this executable, where
// the path to this executable is specified `argv0`.
//
// This function doesn't currently print anything but can be debugged from
// the command line using strace like:
//
//     strace -e trace=execve -s 10000 build/src/bitcoin ...
void ExecCommand(const std::vector<std::string>& args, std::string_view argv0)
{
    // Construct argument string for execvp
    std::vector<const char*> cstr_args{};
    cstr_args.reserve(args.size() + 1);
    for (const auto& arg : args) {
        cstr_args.emplace_back(arg.c_str());
    }
    cstr_args.emplace_back(nullptr);

    // Try to call execvp with given exe path.
    auto try_exec = [&](fs::path exe_path, bool allow_notfound = true) {
        std::string exe_path_str{fs::PathToString(exe_path)};
        cstr_args[0] = exe_path_str.c_str();
        if (execvp(cstr_args[0], (char*const*)cstr_args.data()) == -1) {
            if (allow_notfound && errno == ENOENT) return false;
            throw std::system_error(errno, std::system_category(), strprintf("execvp failed to execute '%s'", cstr_args[0]));
        }
        return true; // Will not actually be reached if execvp succeeds
    };

    // Try to figure out where current executable is located. This is a
    // simplified search that won't work perfectly on every platform and doesn't
    // need to, as it is only trying to prioritize locally built or installed
    // executables over system executables. We may want to add options to
    // override this behavior in the future, though.
    const fs::path exe_path{fs::PathFromString(std::string{argv0})};
    fs::path exe_dir(argv0);
    std::error_code ec;
#ifndef WIN32
    if (argv0.find('/') == std::string_view::npos) {
        if (const char* path_env = std::getenv("PATH")) {
            size_t start{0}, end{0};
            for (std::string_view paths{path_env}; end != std::string_view::npos; start = end + 1) {
                end = paths.find(':', start);
                fs::path candidate = fs::path(paths.substr(start, end - start)) / exe_dir;
                if (fs::exists(candidate, ec) && fs::is_regular_file(candidate, ec)) {
                    exe_dir = candidate;
                    break;
                }
            }
        }
    }
#else
    wchar_t module_path[MAX_PATH];
    if (GetModuleFileNameW(nullptr, module_path, MAX_PATH) > 0) {
        exe_dir = fs::path{module_path};
    } else {
        tfm::format(std::cerr, "Warning: Failed to get executable path. Error: %s\n", GetLastError());
    }
#endif
    exe_dir = fs::weakly_canonical(exe_dir, ec).parent_path();

    // Search for executables on system PATH only if this executable was invoked
    // from the PATH, to avoid unintentionally launching system executables in a
    // local build
    // (https://github.com/bitcoin/bitcoin/pull/31375#discussion_r1861814807)
    const bool use_system_path{!exe_path.has_parent_path()};
    const fs::path arg0{fs::PathFromString(args[0])};

    // If exe is in a CMake build tree, first look for target executable
    // relative to it.
    (exe_dir.filename() == "src" && try_exec(exe_dir / arg0)) ||
    // Otherwise if exe is installed in a bin/ directory, first look for target
    // executable in libexec/
    (exe_dir.filename() == "bin" && try_exec(fs::path{exe_dir.parent_path()} / "libexec" / arg0.filename())) ||
    // Otherwise look for target executable next to current exe
    try_exec(exe_dir / arg0.filename(), use_system_path) ||
    // Otherwise just look on the system path.
    (use_system_path && try_exec(arg0.filename(), false));
};
