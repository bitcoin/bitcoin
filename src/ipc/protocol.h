// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_PROTOCOL_H
#define BITCOIN_IPC_PROTOCOL_H

#include <interfaces/init.h>

#include <functional>
#include <memory>
#include <mp/proxy-io.h>
#include <typeindex>

namespace ipc {
struct Context;

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
    //!
    //! @note It could be potentially useful in the future to add
    //! std::function<void()> on_disconnect callback argument here. But there
    //! isn't an immediate need, because the protocol implementation can clean
    //! up its own state (calling ProxyServer destructors, etc) on disconnect,
    //! and any client calls will just throw ipc::Exception errors after a
    //! disconnect.
    virtual std::unique_ptr<interfaces::Init> connect(mp::Stream stream) = 0;

    //! Listen for connections on provided socket descriptor, accept them, and
    //! handle requests on accepted connections. This method doesn't block, and
    //! performs I/O on a background thread.
    virtual void listen(mp::SocketId listen_fd, interfaces::Init& init) = 0;

    //! Handle requests from a stream provided by the make_stream callback,
    //! forwarding them to the provided Init interface. Socket communication is
    //! handled on the current thread, and this call blocks until the socket is
    //! closed. A callback is used to specify the stream because this method
    //! initializes the event loop and it may not be possible to create the
    //! stream before the event loop is initialized.
    //!
    //! @note: If this method is called, it needs be to called before connect()
    //! or listen() methods, because for ease of implementation this method is
    //! inflexible and always runs the event loop in the foreground thread. It
    //! can share its event loop with the other methods but can't share an event
    //! loop that was created by them. This isn't a problem because serve() is
    //! only called by spawned child processes that call it immediately to
    //! communicate back with parent processes.
    virtual void serve(interfaces::Init& init, const std::function<mp::Stream()>& make_stream) = 0;

    //! Make stream object from socket id.
    virtual mp::Stream makeStream(mp::SocketId socket) = 0;

    //! Disconnect any incoming connections that are still connected.
    virtual void disconnectIncoming() = 0;

    //! Add cleanup callback to interface that will run when the interface is
    //! deleted.
    virtual void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) = 0;

    //! Context accessor.
    virtual Context& context() = 0;
};
} // namespace ipc

#endif // BITCOIN_IPC_PROTOCOL_H
