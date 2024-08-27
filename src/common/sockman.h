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

typedef int64_t NodeId;

/**
 * A socket manager class which handles socket operations.
 * To use this class, inherit from it and implement the pure virtual methods.
 * Handled operations:
 * - binding and listening on sockets
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
     * @param[in] to Where to bind.
     * @param[out] errmsg Error string if an error occurs.
     * @retval true Success.
     * @retval false Failure, `strError` will be set.
     */
    bool BindAndStartListening(const CService& to, bilingual_str& errmsg);

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

    /**
     * List of listening sockets.
     */
    std::vector<std::shared_ptr<Sock>> m_listen;

private:

    //
    // Pure virtual functions must be implemented by children classes.
    //

    //
    // Non-pure virtual functions can be overridden by children classes or left
    // alone to use the default implementation from SockMan.
    //

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
     * The id to assign to the next created node. Used to generate ids of nodes.
     */
    std::atomic<NodeId> m_next_node_id{0};
};

#endif // BITCOIN_COMMON_SOCKMAN_H
