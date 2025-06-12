// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_COMMON_SOCKMAN_H
#define BITCOIN_COMMON_SOCKMAN_H

#include <netaddress.h>
#include <util/result.h>
#include <util/sock.h>
#include <util/translation.h>

#include <atomic>
#include <memory>
#include <vector>

/**
 * A socket manager class which handles socket operations.
 * To use this class, inherit from it and implement the pure virtual methods.
 * Handled operations:
 * - binding and listening on sockets
 * - starting of necessary threads to process socket operations
 * - accepting incoming connections
 * - closing connections
 * - waiting for IO readiness on sockets and doing send/recv accordingly
 */
class SockMan
{
public:
    /**
     * Each connection is assigned an unique id of this type.
     */
    using Id = int64_t;

    virtual ~SockMan()
    {
        Assume(!m_thread_socket_handler.joinable()); // Missing call to JoinSocketsThreads()
        Assume(m_connected.empty()); // Missing call to CloseConnection()
        Assume(m_listen.empty()); // Missing call to StopListening()
    }

    //
    // Non-virtual functions, to be reused by children classes.
    //

    /**
     * Bind to a new address:port, start listening and add the listen socket to `m_listen`.
     * Should be called before `StartSocketsThreads()`.
     * @param[in] to Where to bind.
     * @returns {} or the reason for failure.
     */
    util::Result<void> BindAndStartListening(const CService& to);

    /**
     * Options to influence `StartSocketsThreads()`.
     */
    struct Options {
        std::string_view socket_handler_thread_name;
    };

    /**
     * Start the necessary threads for sockets IO.
     */
    void StartSocketsThreads(const Options& options);

    /**
     * Join (wait for) the threads started by `StartSocketsThreads()` to exit.
     */
    void JoinSocketsThreads();

    /**
     * Stop network activity
     */
    void InterruptNet() { m_interrupt_net(); }

    /**
     * Destroy a given connection by closing its socket and release resources occupied by it.
     * @param[in] id Connection to destroy.
     * @return Whether the connection existed and its socket was closed by this call.
     */
    bool CloseConnection(Id id)
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Stop listening by closing all listening sockets.
     */
    void StopListening();

protected:
    //
    // Pure virtual functions must be implemented by children classes.
    //

    /**
     * Be notified when a new connection has been accepted.
     * @param[in] id Id of the newly accepted connection.
     * @param[in] me The address and port at our side of the connection.
     * @param[in] them The address and port at the peer's side of the connection.
     * @retval true The new connection was accepted at the higher level.
     * @retval false The connection was refused at the higher level, so the
     * associated socket and id should be discarded by `SockMan`.
     */
    virtual bool EventNewConnectionAccepted(Id id,
                                            const CService& me,
                                            const CService& them) = 0;

    //
    // Non-pure virtual functions can be overridden by children classes or left
    // alone to use the default implementation from SockMan.
    //

    /**
     * Can be used to temporarily pause sends on a connection.
     * SockMan would only call Send() if this returns true.
     * The implementation in SockMan always returns true.
     * @param[in] id Connection for which to confirm or omit the next call to EventReadyToSend().
     */
    virtual bool ShouldTryToSend(Id id) const;

    /**
     * SockMan would only call Recv() on a connection's socket if this returns true.
     * Can be used to temporarily pause receives on a connection.
     * The implementation in SockMan always returns true.
     * @param[in] id Connection for which to confirm or omit the next receive.
     */
    virtual bool ShouldTryToRecv(Id id) const;

    /**
     * List of listening sockets.
     */
    std::vector<std::shared_ptr<Sock>> m_listen;

private:
    /**
     * Generate an id for a newly created connection.
     */
    Id GetNewId();

    /**
     * The id to assign to the next created connection. Used to generate ids of connections.
     */
    std::atomic<Id> m_next_id{0};

    /**
     * After a new socket with a peer has been created, configure its flags,
     * make a new connection id and call `EventNewConnectionAccepted()`.
     * @param[in] sock The newly created socket.
     * @param[in] me Address at our end of the connection.
     * @param[in] them Address of the new peer.
     */
    void NewSockAccepted(std::unique_ptr<Sock>&& sock, const CService& me, const CService& them)
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Accept a connection.
     * @param[in] listen_sock Socket on which to accept the connection.
     * @param[out] addr Address of the peer that was accepted.
     * @return Newly created socket for the accepted connection.
     */
    std::unique_ptr<Sock> AcceptConnection(const Sock& listen_sock, CService& addr);

    /**
     * The sockets used by a connection.
     */
    struct ConnectionSocket {
        explicit ConnectionSocket(std::unique_ptr<Sock>&& s)
            : sock{std::move(s)}
        {
        }

        /**
         * Mutex that serializes the Send() and Recv() calls on `sock`.
         */
        Mutex mutex;

        /**
         * Underlying socket.
         * `shared_ptr` (instead of `unique_ptr`) is used to avoid premature close of the
         * underlying file descriptor by one thread while another thread is poll(2)-ing
         * it for activity.
         * @see https://github.com/bitcoin/bitcoin/issues/21744 for details.
         */
        std::shared_ptr<Sock> sock;
    };

    /**
     * Info about which socket has which event ready and its connection id.
     */
    struct IOReadiness {
        /**
         * Map of socket -> socket events. For example:
         * socket1 -> { requested = SEND|RECV, occurred = RECV }
         * socket2 -> { requested = SEND, occurred = SEND }
         */
        Sock::EventsPerSock events_per_sock;

        /**
         * Map of socket -> connection id (in `m_connected`). For example
         * socket1 -> id=23
         * socket2 -> id=56
         */
        std::unordered_map<Sock::EventsPerSock::key_type,
                           SockMan::Id,
                           Sock::HashSharedPtrSock,
                           Sock::EqualSharedPtrSock>
            ids_per_sock;
    };

    /**
     * Check connected and listening sockets for IO readiness and process them accordingly.
     */
    void ThreadSocketHandler()
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Generate a collection of sockets to check for IO readiness.
     * @return Sockets to check for readiness plus an aux map to find the
     * corresponding connection id given a socket.
     */
    IOReadiness GenerateWaitSockets()
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Accept incoming connections, one from each read-ready listening socket.
     * @param[in] events_per_sock Sockets that are ready for IO.
     */
    void SocketHandlerListening(const Sock::EventsPerSock& events_per_sock)
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * This is signaled when network activity should cease.
     */
    CThreadInterrupt m_interrupt_net;

    /**
     * Thread that sends to and receives from sockets and accepts connections.
     */
    std::thread m_thread_socket_handler;

    mutable Mutex m_connected_mutex;

    /**
     * Sockets for existent connections.
     * The `shared_ptr` makes it possible to create a snapshot of this by simply copying
     * it (under `m_connected_mutex`).
     */
    std::unordered_map<Id, std::shared_ptr<ConnectionSocket>> m_connected GUARDED_BY(m_connected_mutex);
};

#endif // BITCOIN_COMMON_SOCKMAN_H
