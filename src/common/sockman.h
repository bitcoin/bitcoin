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
     * Possible status changes that can be passed to `EventI2PStatus()`.
     */
    enum class I2PStatus : uint8_t {
        /// The listen succeeded and we are now listening for incoming I2P connections.
        START_LISTENING,

        /// The listen failed and now we are not listening (even if START_LISTENING was signaled before).
        STOP_LISTENING,
    };

    virtual ~SockMan() = default;

    //
    // Non-virtual functions, to be reused by children classes.
    //

    /**
     * Bind to a new address:port, start listening and add the listen socket to `m_listen`.
     * @param[in] to Where to bind.
     * @param[out] err_msg Error string if an error occurs.
     * @retval true Success.
     * @retval false Failure, `err_msg` will be set.
     */
    bool BindAndStartListening(const CService& to, bilingual_str& err_msg);

    /**
     * Accept a connection.
     * @param[in] listen_sock Socket on which to accept the connection.
     * @param[out] addr Address of the peer that was accepted.
     * @return Newly created socket for the accepted connection.
     */
    std::unique_ptr<Sock> AcceptConnection(const Sock& listen_sock, CService& addr);

    /**
     * Generate an id for a newly created connection.
     */
    Id GetNewId();

    /**
     * Stop listening by closing all listening sockets.
     */
    void StopListening();

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
     * Be notified of a change in the state of the I2P connectivity.
     * The default behavior, implemented by `SockMan`, is to ignore this event.
     * @param[in] addr The address we started or stopped listening on.
     * @param[in] new_status New status.
     */
    virtual void EventI2PStatus(const CService& addr, I2PStatus new_status);

    /**
     * The id to assign to the next created connection. Used to generate ids of connections.
     */
    std::atomic<Id> m_next_id{0};
};

#endif // BITCOIN_COMMON_SOCKMAN_H
