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
 * - accepting incoming connections
 */
class SockMan
{
public:
    /**
     * Each connection is assigned an unique id of this type.
     */
    using Id = int64_t;

    /**
     * Bind to a new address:port, start listening and add the listen socket to `m_listen`.
     * @param[in] to Where to bind.
     * @returns {} or the reason for failure.
     */
    util::Result<void> BindAndStartListening(const CService& to);

    /**
     * Stop listening by closing all listening sockets.
     */
    void StopListening();

protected:
    /**
     * Accept a connection.
     * @param[in] listen_sock Socket on which to accept the connection.
     * @param[out] addr Address of the peer that was accepted.
     * @return Newly created socket for the accepted connection.
     */
    std::unique_ptr<Sock> AcceptConnection(const Sock& listen_sock, CService& addr);

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
     * The id to assign to the next created connection.
     */
    std::atomic<Id> m_next_id{0};
};

#endif // BITCOIN_COMMON_SOCKMAN_H
