// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_IPC_H
#define BITCOIN_INTERFACES_IPC_H

#include <util/result.h>
#include <util/translation.h>

#include <charconv>
#include <functional>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <typeindex>

namespace ipc {
struct Context;
} // namespace ipc

namespace interfaces {
class Init;

//! Interface providing access to interprocess-communication (IPC)
//! functionality. The IPC implementation is responsible for establishing
//! connections between a controlling process and a process being controlled.
//! When a connection is established, the process being controlled returns an
//! interfaces::Init pointer to the controlling process, which the controlling
//! process can use to get access to other interfaces and functionality.
//!
//! When spawning a new process, the steps are:
//!
//! 1. The controlling process calls interfaces::Ipc::spawnProcess(), which
//!    calls ipc::Process::spawn(), which spawns a new process and returns a
//!    socketpair file descriptor for communicating with it.
//!    interfaces::Ipc::spawnProcess() then calls ipc::Protocol::connect()
//!    passing the socketpair descriptor, which returns a local proxy
//!    interfaces::Init implementation calling remote interfaces::Init methods.
//! 2. The spawned process calls interfaces::Ipc::startSpawnProcess(), which
//!    calls ipc::Process::checkSpawned() to read command line arguments and
//!    determine whether it is a spawned process and what socketpair file
//!    descriptor it should use. It then calls ipc::Protocol::serve() to handle
//!    incoming requests from the socketpair and invoke interfaces::Init
//!    interface methods, and exit when the socket is closed.
//! 3. The controlling process calls local proxy interfaces::Init object methods
//!    to make other proxy objects calling other remote interfaces. It can also
//!    destroy the initial interfaces::Init object to close the connection and
//!    shut down the spawned process.
//!
//! When connecting to an existing process, the steps are similar to spawning a
//! new process, except a socket is created instead of a socketpair, and
//! destroying an Init interface doesn't end the process, since there can be
//! multiple connections.
class Ipc
{
public:
    static constexpr size_t DEFAULT_MAX_CONNECTIONS{16};

    struct BindAddress
    {
        std::string address;
        size_t max_connections{DEFAULT_MAX_CONNECTIONS};
    };

    static util::Result<BindAddress> ParseBindAddress(std::string address)
    {
        constexpr std::string_view UNIX_PREFIX{"unix:"};
        if (!address.starts_with(UNIX_PREFIX)) return BindAddress{std::move(address)};

        const std::string_view address_view{address};
        const std::string_view unix_suffix{address_view.substr(UNIX_PREFIX.size())};
        if (unix_suffix.find_first_of("=,") != std::string_view::npos &&
            unix_suffix.find(':') == std::string_view::npos) {
            return util::Error{Untranslated("Missing unix socket path before socket options; use unix::<options> for the default path")};
        }

        const auto option_pos = address.rfind(':');
        if (option_pos == std::string::npos || option_pos < UNIX_PREFIX.size()) {
            return BindAddress{std::move(address)};
        }

        const std::string_view options_view{address_view.substr(option_pos + 1)};
        if (options_view.empty() || options_view.find_first_of("=,") == std::string_view::npos) {
            return BindAddress{std::move(address)};
        }

        BindAddress bind{
            .address = address.substr(0, option_pos),
        };
        size_t next{0};
        while (next <= options_view.size()) {
            const size_t end{options_view.find(',', next)};
            const std::string_view option{options_view.substr(next, end - next)};
            if (option.empty()) {
                return util::Error{Untranslated("Empty socket option")};
            }
            const size_t equals{option.find('=')};
            const std::string_view name{option.substr(0, equals)};
            const std::string_view value{equals == std::string_view::npos ? std::string_view{} : option.substr(equals + 1)};
            if (name == "max-connections") {
                if (value.empty()) {
                    return util::Error{Untranslated("Missing value for max-connections option")};
                }
                int64_t parsed_limit{-1};
                const auto [last, ec] = std::from_chars(value.data(), value.data() + value.size(), parsed_limit);
                if (ec != std::errc{} || last != value.data() + value.size() || parsed_limit < 0) {
                    return util::Error{Untranslated("Invalid max-connections value '" + std::string{value} + "'")};
                }
                bind.max_connections = static_cast<size_t>(parsed_limit);
            } else {
                return util::Error{Untranslated("Unknown socket option '" + std::string{name} + "'")};
            }
            if (end == std::string_view::npos) break;
            next = end + 1;
        }
        return bind;
    }

    virtual ~Ipc() = default;

    //! Spawn a child process returning pointer to its Init interface.
    virtual std::unique_ptr<Init> spawnProcess(const char* exe_name) = 0;

    //! If this is a spawned process, block and handle requests from the parent
    //! process by forwarding them to this process's Init interface, then return
    //! true. If this is not a spawned child process, return false.
    virtual bool startSpawnedProcess(int argc, char* argv[], int& exit_status) = 0;

    //! Connect to a socket address and return a pointer to its Init interface.
    //! Returns a non-null pointer if the connection was established, returns
    //! null if address is empty ("") or disabled ("0") or if a connection was
    //! refused but not required ("auto"), and throws an exception if there was
    //! an unexpected error.
    virtual std::unique_ptr<Init> connectAddress(std::string& address) = 0;

    //! Listen on a socket address exposing this process's init interface to
    //! clients. Throws an exception if there was an error.
    virtual void listenAddress(BindAddress& bind) = 0;

    //! Disconnect any incoming connections that are still connected.
    virtual void disconnectIncoming() = 0;

    //! Add cleanup callback to remote interface that will run when the
    //! interface is deleted.
    template<typename Interface>
    void addCleanup(Interface& iface, std::function<void()> cleanup)
    {
        addCleanup(typeid(Interface), &iface, std::move(cleanup));
    }

    //! IPC context struct accessor (see struct definition for more description).
    virtual ipc::Context& context() = 0;

protected:
    //! Internal implementation of public addCleanup method (above) as a
    //! type-erased virtual function, since template functions can't be virtual.
    virtual void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) = 0;
};

//! Return implementation of Ipc interface.
std::unique_ptr<Ipc> MakeIpc(const char* exe_name, const char* process_argv0, Init& init);
} // namespace interfaces

#endif // BITCOIN_INTERFACES_IPC_H
