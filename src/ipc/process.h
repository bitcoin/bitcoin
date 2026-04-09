// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_PROCESS_H
#define BITCOIN_IPC_PROCESS_H

#include <util/fs.h>

#include <charconv>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace ipc {
class Protocol;

struct BindAddress
{
    std::string address;
    std::optional<size_t> max_connections;
};

inline std::optional<BindAddress> ParseBindAddress(std::string address, std::string& error)
{
    constexpr std::string_view MAX_CONNECTIONS_ARG{":max-connections="};
    constexpr std::string_view MAX_CONNECTIONS_VALUE{"max-connections="};

    if (!address.starts_with("unix:")) {
        return BindAddress{std::move(address), std::nullopt};
    }

    if (std::string_view{address}.substr(5).starts_with(MAX_CONNECTIONS_VALUE)) {
        error = "Missing unix socket path before max-connections option; use unix::max-connections=<n> for the default path";
        return std::nullopt;
    }

    const auto option_pos = address.rfind(MAX_CONNECTIONS_ARG);
    if (option_pos == std::string::npos || option_pos < 5) {
        return BindAddress{std::move(address), std::nullopt};
    }

    const std::string value{address.substr(option_pos + MAX_CONNECTIONS_ARG.size())};
    if (value.empty()) {
        error = "Missing value for max-connections option";
        return std::nullopt;
    }

    int64_t parsed_limit{-1};
    const auto [last, ec] = std::from_chars(value.data(), value.data() + value.size(), parsed_limit);
    if (ec != std::errc{} || last != value.data() + value.size() || parsed_limit < 0) {
        error = "Invalid max-connections value '" + value + "'";
        return std::nullopt;
    }

    address.erase(option_pos);
    return BindAddress{std::move(address), static_cast<size_t>(parsed_limit)};
}

//! IPC process interface for spawning bitcoin processes and serving requests
//! in processes that have been spawned.
//!
//! There will be different implementations of this interface depending on the
//! platform (e.g. unix, windows).
class Process
{
public:
    virtual ~Process() = default;

    //! Spawn process and return socket file descriptor for communicating with
    //! it.
    virtual int spawn(const std::string& new_exe_name, const fs::path& argv0_path, int& pid) = 0;

    //! Wait for spawned process to exit and return its exit code.
    virtual int waitSpawned(int pid) = 0;

    //! Parse command line and determine if current process is a spawned child
    //! process. If so, return true and a file descriptor for communicating
    //! with the parent process.
    virtual bool checkSpawned(int argc, char* argv[], int& fd) = 0;

    //! Canonicalize and connect to address, returning socket descriptor.
    virtual int connect(const fs::path& data_dir,
                        const std::string& dest_exe_name,
                        std::string& address) = 0;

    //! Create listening socket, bind and canonicalize address, and return socket descriptor.
    virtual int bind(const fs::path& data_dir,
                     const std::string& exe_name,
                     std::string& address) = 0;
};

//! Constructor for Process interface. Implementation will vary depending on
//! the platform (unix or windows).
std::unique_ptr<Process> MakeProcess();
} // namespace ipc

#endif // BITCOIN_IPC_PROCESS_H
