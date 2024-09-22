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
#include <thread>
#include <vector>

typedef int64_t NodeId;

/**
 * A socket manager class which handles socket operations.
 * To use this class, inherit from it and implement the pure virtual methods.
 * Handled operations:
 * - binding and listening on sockets
 * - starting of necessary threads to process socket operations
 * - accepting incoming connections
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
     * Accept a connection.
     * @param[in] listen_sock Socket on which to accept the connection.
     * @param[out] addr Address of the peer that was accepted.
     * @return Newly created socket for the accepted connection.
     */
    std::unique_ptr<Sock> AcceptConnection(const Sock& listen_sock, CService& addr);

    /**
     * Generate an id for a newly created node.
     */
    NodeId GetNewNodeId();

    /**
     * Close all sockets.
     */
    void CloseSockets();

    //
    // Pure virtual functions must be implemented by children classes.
    //

    /**
     * Be notified when a new connection has been accepted.
     * @param[in] sock Connected socket to communicate with the peer.
     * @param[in] me The address and port at our side of the connection.
     * @param[in] them The address and port at the peer's side of the connection.
     */
    virtual void EventNewConnectionAccepted(std::unique_ptr<Sock>&& sock,
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

private:

    /**
     * Accept incoming I2P connections in a loop and call
     * `EventNewConnectionAccepted()` for each new connection.
     */
    void ThreadI2PAccept();

    /**
     * The id to assign to the next created node. Used to generate ids of nodes.
     */
    std::atomic<NodeId> m_next_node_id{0};

    /**
     * Thread that accepts incoming I2P connections in a loop, can be stopped via `interruptNet`.
     */
    std::thread m_thread_i2p_accept;
};

#endif // BITCOIN_COMMON_SOCKMAN_H
