// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_COMMON_SOCKMAN_H
#define BITCOIN_COMMON_SOCKMAN_H

#include <i2p.h>
#include <netaddress.h>
#include <netbase.h>
#include <util/fs.h>
#include <util/sock.h>
#include <util/translation.h>

#include <atomic>
#include <memory>
#include <queue>
#include <thread>
#include <variant>
#include <vector>

typedef int64_t NodeId;

/**
 * A socket manager class which handles socket operations.
 * To use this class, inherit from it and implement the pure virtual methods.
 * Handled operations:
 * - binding and listening on sockets
 * - starting of necessary threads to process socket operations
 * - accepting incoming connections
 * - making outbound connections
 * - closing connections
 * - waiting for IO readiness on sockets and doing send/recv accordingly
 */
class SockMan
{
public:

    virtual ~SockMan() = default;

    //
    // Non-virtual functions, to be reused by children classes.
    //

    /**
     * Bind to a new address:port, start listening and add the listen socket to `m_listen`.
     * Should be called before `StartSocketsThreads()`.
     * @param[in] to Where to bind.
     * @param[out] errmsg Error string if an error occurs.
     * @retval true Success.
     * @retval false Failure, `strError` will be set.
     */
    bool BindAndStartListening(const CService& to, bilingual_str& errmsg);

    /**
     * Options to influence `StartSocketsThreads()`.
     */
    struct Options {
        struct I2P {
            explicit I2P(const fs::path& file, const Proxy& proxy) : private_key_file{file}, sam_proxy{proxy} {}

            const fs::path private_key_file;
            const Proxy sam_proxy;
        };

        /**
         * I2P options. If set then a thread will be started that will accept incoming I2P connections.
         */
        std::optional<I2P> i2p;
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
     * A more readable std::tuple<std::string, uint16_t> for host and port.
     */
    struct StringHostIntPort {
        const std::string& host;
        uint16_t port;
    };

    /**
     * Make an outbound connection, save the socket internally and return a newly generated node id.
     * @param[in] to The address to connect to, either as CService or a host as string and port as
     * an integer, if the later is used, then `proxy` must be valid.
     * @param[in] is_important If true, then log failures with higher severity.
     * @param[in] proxy Proxy to connect through if `proxy.IsValid()` is true.
     * @param[out] proxy_failed If `proxy` is valid and the connection failed because of the
     * proxy, then it will be set to true.
     * @param[out] me If the connection was successful then this is set to the address on the
     * local side of the socket.
     * @return Newly generated node id, or std::nullopt if the operation fails.
     */
    std::optional<NodeId> ConnectAndMakeNodeId(const std::variant<CService, StringHostIntPort>& to,
                                               bool is_important,
                                               const Proxy& proxy,
                                               bool& proxy_failed,
                                               CService& me)
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex, !m_unused_i2p_sessions_mutex);

    /**
     * Disconnect a given peer by closing its socket and release resources occupied by it.
     * @return Whether the peer existed and its socket was closed by this call.
     */
    bool CloseConnection(NodeId node_id)
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Try to send some data to the given node.
     * @param[in] node_id Identifier of the node to send to.
     * @param[in] data The data to send, it might happen that only a prefix of this is sent.
     * @param[in] will_send_more Used as an optimization if the caller knows that they will
     * be sending more data soon after this call.
     * @param[out] errmsg If <0 is returned then this will contain a human readable message
     * explaining the error.
     * @retval >=0 The number of bytes actually sent.
     * @retval <0 A permanent error has occurred.
     */
    ssize_t SendBytes(NodeId node_id,
                      Span<const unsigned char> data,
                      bool will_send_more,
                      std::string& errmsg) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Close all sockets.
     */
    void CloseSockets();

    //
    // Pure virtual functions must be implemented by children classes.
    //

    /**
     * Be notified when a new connection has been accepted.
     * @param[in] node_id Id of the newly accepted connection.
     * @param[in] me The address and port at our side of the connection.
     * @param[in] them The address and port at the peer's side of the connection.
     * @retval true The new connection was accepted at the higher level.
     * @retval false The connection was refused at the higher level, so the
     * associated socket and node_id should be discarded by `SockMan`.
     */
    virtual bool EventNewConnectionAccepted(NodeId node_id,
                                            const CService& me,
                                            const CService& them) = 0;

    /**
     * Called when the socket is ready to send data and `ShouldTryToSend()` has
     * returned true. This is where the higher level code serializes its messages
     * and calls `SockMan::SendBytes()`.
     * @param[in] node_id Id of the node whose socket is ready to send.
     * @param[out] cancel_recv Should always be set upon return and if it is true,
     * then the next attempt to receive data from that node will be omitted.
     */
    virtual void EventReadyToSend(NodeId node_id, bool& cancel_recv) = 0;

    /**
     * Called when new data has been received.
     * @param[in] node_id Node for which the data arrived.
     * @param[in] data Data buffer.
     * @param[in] n Number of bytes in `data`.
     */
    virtual void EventGotData(NodeId node_id, const uint8_t* data, size_t n) = 0;

    /**
     * Called when the remote peer has sent an EOF on the socket. This is a graceful
     * close of their writing side, we can still send and they will receive, if it
     * makes sense at the application level.
     * @param[in] node_id Node whose socket got EOF.
     */
    virtual void EventGotEOF(NodeId node_id) = 0;

    /**
     * Called when we get an irrecoverable error trying to read from a socket.
     * @param[in] node_id Node whose socket got an error.
     * @param[in] errmsg Message describing the error.
     */
    virtual void EventGotPermanentReadError(NodeId node_id, const std::string& errmsg) = 0;

    //
    // Non-pure virtual functions can be overridden by children classes or left
    // alone to use the default implementation from SockMan.
    //

    /**
     * SockMan would only call EventReadyToSend() if this returns true.
     * Can be used to temporary pause sends for a node.
     * The implementation in SockMan always returns true.
     * @param[in] node_id Node for which to confirm or cancel a call to EventReadyToSend().
     */
    virtual bool ShouldTryToSend(NodeId node_id) const;

    /**
     * SockMan would only call Recv() on a node's socket if this returns true.
     * Can be used to temporary pause receives for a node.
     * The implementation in SockMan always returns true.
     * @param[in] node_id Node for which to confirm or cancel a receive.
     */
    virtual bool ShouldTryToRecv(NodeId node_id) const;

    /**
     * SockMan has completed the current send+recv iteration for a node.
     * It will do another send+recv for this node after processing all other nodes.
     * Can be used to execute periodic tasks for a given node.
     * The implementation in SockMan does nothing.
     * @param[in] node_id Node for which send+recv has been done.
     */
    virtual void EventIOLoopCompletedForNode(NodeId node_id);

    /**
     * SockMan has completed send+recv for all nodes.
     * Can be used to execute periodic tasks for all nodes.
     * The implementation in SockMan does nothing.
     */
    virtual void EventIOLoopCompletedForAllPeers();

    /**
     * Be notified of a change in the state of listening for incoming I2P connections.
     * The default behavior, implemented by `SockMan`, is to ignore this event.
     * @param[in] addr Our listening address.
     * @param[in] success If true then the listen succeeded and we are now
     * listening for incoming I2P connections at `addr`. If false then the
     * call failed and now we are not listening (even if this was invoked
     * before with `true`).
     */
    virtual void EventI2PListen(const CService& addr, bool success);

    /**
     * This is signaled when network activity should cease.
     * A pointer to it is saved in `m_i2p_sam_session`, so make sure that
     * the lifetime of `interruptNet` is not shorter than
     * the lifetime of `m_i2p_sam_session`.
     */
    CThreadInterrupt interruptNet;

protected:

    /**
     * During some tests mocked sockets are created outside of `SockMan`, make it
     * possible to add those so that send/recv can be exercised.
     */
    void TestOnlyAddExistentNode(NodeId node_id, std::unique_ptr<Sock>&& sock)
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

private:

    /**
     * Cap on the size of `m_unused_i2p_sessions`, to ensure it does not
     * unexpectedly use too much memory.
     */
    static constexpr size_t MAX_UNUSED_I2P_SESSIONS_SIZE{10};

    /**
     * The sockets used by a connected node - a data socket and an optional I2P session.
     */
    struct NodeSockets {
        explicit NodeSockets(std::unique_ptr<Sock>&& s)
            : sock{std::move(s)}
        {
        }

        explicit NodeSockets(std::shared_ptr<Sock>&& s, std::unique_ptr<i2p::sam::Session>&& sess)
            : sock{std::move(s)},
              i2p_transient_session{std::move(sess)}
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

        /**
         * When transient I2P sessions are used, then each node has its own session, otherwise
         * all nodes use the session from `m_i2p_sam_session` and share the same I2P address.
         * I2P sessions involve a data/transport socket (in `sock`) and a control socket
         * (in `i2p_transient_session`). For transient sessions, once the data socket `sock` is
         * closed, the control socket is not going to be used anymore and would be just taking
         * resources. Storing it here makes its deletion together with `sock` automatic.
         */
        std::unique_ptr<i2p::sam::Session> i2p_transient_session;
    };

    /**
     * Info about which socket has which event ready and its node id.
     */
    struct IOReadiness {
        Sock::EventsPerSock events_per_sock;
        std::unordered_map<Sock::EventsPerSock::key_type, NodeId> node_ids_per_sock;
    };

    /**
     * Accept incoming I2P connections in a loop and call
     * `EventNewConnectionAccepted()` for each new connection.
     */
    void ThreadI2PAccept()
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Check connected and listening sockets for IO readiness and process them accordingly.
     */
    void ThreadSocketHandler()
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Accept a connection.
     * @param[in] listen_sock Socket on which to accept the connection.
     * @param[out] addr Address of the peer that was accepted.
     * @return Newly created socket for the accepted connection.
     */
    std::unique_ptr<Sock> AcceptConnection(const Sock& listen_sock, CService& addr);

    /**
     * After a new socket with a peer has been created, configure its flags,
     * make a new node id and call `EventNewConnectionAccepted()`.
     * @param[in] sock The newly created socket.
     * @param[in] me Address at our end of the connection.
     * @param[in] them Address of the new peer.
     */
    void NewSockAccepted(std::unique_ptr<Sock>&& sock, const CService& me, const CService& them)
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Generate an id for a newly created node.
     */
    NodeId GetNewNodeId();

    /**
     * Generate a collection of sockets to check for IO readiness.
     * @return Sockets to check for readiness plus an aux map to find the
     * corresponding node id given a socket.
     */
    IOReadiness GenerateWaitSockets()
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * Do the read/write for connected sockets that are ready for IO.
     * @param[in] io_readiness Which sockets are ready and their node ids.
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
     * @param[in] node_id Node id to search for.
     * @return NodeSockets for the given node id or empty shared_ptr if not found.
     */
    std::shared_ptr<NodeSockets> GetNodeSockets(NodeId node_id) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_connected_mutex);

    /**
     * The id to assign to the next created node. Used to generate ids of nodes.
     */
    std::atomic<NodeId> m_next_node_id{0};

    /**
     * Thread that sends to and receives from sockets and accepts connections.
     */
    std::thread m_thread_socket_handler;

    /**
     * Thread that accepts incoming I2P connections in a loop, can be stopped via `interruptNet`.
     */
    std::thread m_thread_i2p_accept;

    /**
     * Mutex protecting m_i2p_sam_sessions.
     */
    Mutex m_unused_i2p_sessions_mutex;

    /**
     * A pool of created I2P SAM transient sessions that should be used instead
     * of creating new ones in order to reduce the load on the I2P network.
     * Creating a session in I2P is not cheap, thus if this is not empty, then
     * pick an entry from it instead of creating a new session. If connecting to
     * a host fails, then the created session is put to this pool for reuse.
     */
    std::queue<std::unique_ptr<i2p::sam::Session>> m_unused_i2p_sessions GUARDED_BY(m_unused_i2p_sessions_mutex);

    /**
     * I2P SAM session.
     * Used to accept incoming and make outgoing I2P connections from a persistent
     * address.
     */
    std::unique_ptr<i2p::sam::Session> m_i2p_sam_session;

    /**
     * List of listening sockets.
     */
    std::vector<std::shared_ptr<Sock>> m_listen;

    mutable Mutex m_connected_mutex;

    /**
     * Sockets for connected peers.
     * The `shared_ptr` makes it possible to create a snapshot of this by simply copying
     * it (under `m_connected_mutex`).
     */
    std::unordered_map<NodeId, std::shared_ptr<NodeSockets>> m_connected GUARDED_BY(m_connected_mutex);
};

#endif // BITCOIN_COMMON_SOCKMAN_H
