// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_COMMON_SOCKMAN_H
#define BITCOIN_COMMON_SOCKMAN_H

#include <netaddress.h>
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

    virtual ~SockMan() = default;

    //
    // Non-virtual functions, to be reused by children classes.
    //

    /**
     * Bind to a new address:port, start listening and add the listen socket to `m_listen`.
     * Should be called before `StartSocketsThreads()`.
     * @param[in] to Where to bind.
     * @param[out] err_msg Error string if an error occurs.
     * @retval true Success.
     * @retval false Failure, `err_msg` will be set.
     */
    bool BindAndStartListening(const CService& to, bilingual_str& err_msg);

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
     * Accept a connection.
     * @param[in] listen_sock Socket on which to accept the connection.
     * @param[out] addr Address of the peer that was accepted.
     * @return Newly created socket for the accepted connection.
     */
    std::unique_ptr<Sock> AcceptConnection(const Sock& listen_sock, CService& addr);

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
     * Generate an id for a newly created connection.
     */
    Id GetNewId();

    /**
     * Destroy a given connection by closing its socket and release resources occupied by it.
     * @param[in] id Connection to destroy.
     * @return Whether the connection existed and its socket was closed by this call.
     */
    bool CloseConnection(Id id)
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Try to send some data over the given connection.
     * @param[in] id Identifier of the connection.
     * @param[in] data The data to send, it might happen that only a prefix of this is sent.
     * @param[in] will_send_more Used as an optimization if the caller knows that they will
     * be sending more data soon after this call.
     * @param[out] errmsg If <0 is returned then this will contain a human readable message
     * explaining the error.
     * @retval >=0 The number of bytes actually sent.
     * @retval <0 A permanent error has occurred.
     */
    ssize_t SendBytes(Id id,
                      std::span<const unsigned char> data,
                      bool will_send_more,
                      std::string& errmsg) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Stop listening by closing all listening sockets.
     */
    void StopListening();

    /**
     * This is signaled when network activity should cease.
     */
    CThreadInterrupt interruptNet;

    /**
     * List of listening sockets.
     */
    std::vector<std::shared_ptr<Sock>> m_listen;

private:

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

    /**
     * Called when the socket is ready to send data and `ShouldTryToSend()` has
     * returned true. This is where the higher level code serializes its messages
     * and calls `SockMan::SendBytes()`.
     * @param[in] id Id of the connection whose socket is ready to send.
     * @param[out] cancel_recv Should always be set upon return and if it is true,
     * then the next attempt to receive data from that connection will be omitted.
     */
    virtual void EventReadyToSend(Id id, bool& cancel_recv) = 0;

    /**
     * Called when new data has been received.
     * @param[in] id Connection for which the data arrived.
     * @param[in] data Received data.
     */
    virtual void EventGotData(Id id, std::span<const uint8_t> data) = 0;

    /**
     * Called when the remote peer has sent an EOF on the socket. This is a graceful
     * close of their writing side, we can still send and they will receive, if it
     * makes sense at the application level.
     * @param[in] id Connection whose socket got EOF.
     */
    virtual void EventGotEOF(Id id) = 0;

    /**
     * Called when we get an irrecoverable error trying to read from a socket.
     * @param[in] id Connection whose socket got an error.
     * @param[in] errmsg Message describing the error.
     */
    virtual void EventGotPermanentReadError(Id id, const std::string& errmsg) = 0;

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
     * SockMan has completed the current send+recv iteration for a given connection.
     * It will do another send+recv for this connection after processing all other connections.
     * Can be used to execute periodic tasks for a given connection.
     * The implementation in SockMan does nothing.
     * @param[in] id Connection for which send+recv has been done.
     */
    virtual void EventIOLoopCompletedForOne(Id id);

    /**
     * SockMan has completed send+recv for all connections.
     * Can be used to execute periodic tasks for all connections, like closing
     * connections due to higher level logic.
     * The implementation in SockMan does nothing.
     */
    virtual void EventIOLoopCompletedForAll();

    /**
     * The sockets used by a connection.
     */
    struct ConnectionSockets {
        explicit ConnectionSockets(std::unique_ptr<Sock>&& s)
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
     * Do the read/write for connected sockets that are ready for IO.
     * @param[in] io_readiness Which sockets are ready and their connection ids.
     */
    void SocketHandlerConnected(const IOReadiness& io_readiness)
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Accept incoming connections, one from each read-ready listening socket.
     * @param[in] events_per_sock Sockets that are ready for IO.
     */
    void SocketHandlerListening(const Sock::EventsPerSock& events_per_sock)
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Retrieve an entry from m_connected.
     * @param[in] id Connection id to search for.
     * @return ConnectionSockets for the given connection id or empty shared_ptr if not found.
     */
    std::shared_ptr<ConnectionSockets> GetConnectionSockets(Id id) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * The id to assign to the next created connection. Used to generate ids of connections.
     */
    std::atomic<Id> m_next_id{0};

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
    std::unordered_map<Id, std::shared_ptr<ConnectionSockets>> m_connected GUARDED_BY(m_connected_mutex);
};

#endif // BITCOIN_COMMON_SOCKMAN_H
