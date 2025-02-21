// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/fs.h>

#include <string>
#include <vector>

#ifdef WIN32
#include <process.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace util {
int ExecVp(const char *file, char *const argv[])
{
#ifndef WIN32
    return execvp(file, argv);
#else
    std::vector<char*> new_argv;
    std::vector<std::string> escaped_args;
    for (char* const* arg_ptr{argv}; *arg_ptr; ++arg_ptr) {
        std::string_view arg{*arg_ptr};
        if (arg.find_first_of(" \t\"") == std::string_view::npos) {
            // Argument has no quotes or spaces so escaping not necessary.
            new_argv.push_back(*arg_ptr);
        } else {
            // Add escaping to the command line that the executable being
            // invoked will split up using the CommandLineToArgvW function,
            // which expects arguments with spaces to be quoted, quote
            // characters to be backslash-escaped, and backslashes to also be
            // backslash-escaped, but only if they precede a quote character.
            std::string escaped{'"'}; // Start with a quote
            for (size_t i = 0; i < arg.size(); ++i) {
                if (arg[i] == '\\') {
                    // Count consecutive backslashes
                    size_t backslash_count = 0;
                    while (i < arg.size() && arg[i] == '\\') {
                        ++backslash_count;
                        ++i;
                    }
                    if (i < arg.size() && arg[i] == '"') {
                        // Backslashes before a quote need to be doubled
                        escaped.append(backslash_count * 2 + 1, '\\');
                        escaped.push_back('"');
                    } else {
                        // Otherwise, backslashes remain as-is
                        escaped.append(backslash_count, '\\');
                        --i; // Compensate for the outer loop's increment
                    }
                } else if (arg[i] == '"') {
                    // Escape double quotes with a backslash
                    escaped.push_back('\\');
                    escaped.push_back('"');
                } else {
                    escaped.push_back(arg[i]);
                }
            }
            escaped.push_back('"'); // End with a quote
            escaped_args.emplace_back(std::move(escaped));
            new_argv.push_back((char *)escaped_args.back().c_str());
        }
    }
    new_argv.push_back(nullptr);
    return _execvp(file, new_argv.data());
#endif
}

fs::path GetExePath(std::string_view argv0)
{
    // Try to figure out where executable is located. This does a simplified
    // search that won't work perfectly on every platform and doesn't need to,
    // as it is only currently being used in a convenience wrapper binary to try
    // to prioritize locally built or installed executables over system
    // executables.
    const fs::path argv0_path{fs::PathFromString(std::string{argv0})};
    fs::path path{argv0_path};
    std::error_code ec;
#ifndef WIN32
    // If argv0 doesn't contain a path separator, it was invoked from the system
    // PATH and can be searched for there.
    if (!argv0_path.has_parent_path()) {
        if (const char* path_env = std::getenv("PATH")) {
            size_t start{0}, end{0};
            for (std::string_view paths{path_env}; end != std::string_view::npos; start = end + 1) {
                end = paths.find(':', start);
                fs::path candidate = fs::path(paths.substr(start, end - start)) / argv0_path;
                if (fs::is_regular_file(candidate, ec)) {
                    path = candidate;
                    break;
                }
            }
        }
    }
#else
    wchar_t module_path[MAX_PATH];
    if (GetModuleFileNameW(nullptr, module_path, MAX_PATH) > 0) {
        path = fs::path{module_path};
    }
#endif
    return path;
}

} // namespace util
