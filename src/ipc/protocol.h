// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_IPC_PROTOCOL_H
#define SYSCOIN_IPC_PROTOCOL_H

#include <interfaces/init.h>

#include <functional>
#include <memory>
#include <typeindex>

namespace ipc {
//! IPC protocol interface for calling IPC methods over sockets.
//!
//! There may be different implementations of this interface for different IPC
//! protocols (e.g. Cap'n Proto, gRPC, JSON-RPC, or custom protocols).
class Protocol
{
public:
    virtual ~Protocol() = default;

    //! Return Init interface that forwards requests over given socket descriptor.
    //! Socket communication is handled on a background thread.
    virtual std::unique_ptr<interfaces::Init> connect(int fd, const char* exe_name) = 0;

    //! Handle requests on provided socket descriptor, forwarding them to the
    //! provided Init interface. Socket communication is handled on the
    //! current thread, and this call blocks until the socket is closed.
    virtual void serve(int fd, const char* exe_name, interfaces::Init& init) = 0;

    //! Add cleanup callback to interface that will run when the interface is
    //! deleted.
    virtual void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) = 0;
};
} // namespace ipc

#endif // SYSCOIN_IPC_PROTOCOL_H
