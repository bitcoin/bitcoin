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
        struct I2P {
            explicit I2P(const fs::path& file, const Proxy& proxy, std::string_view accept_thread_name)
                : private_key_file{file},
                  sam_proxy{proxy},
                  accept_thread_name{accept_thread_name}
            {}

            const fs::path private_key_file;
            const Proxy sam_proxy;
            const std::string_view accept_thread_name;
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
     * Generate an id for a newly created connection.
     */
    Id GetNewId();

    /**
     * Stop listening by closing all listening sockets.
     */
    void StopListening();

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

    //
    // Non-pure virtual functions can be overridden by children classes or left
    // alone to use the default implementation from SockMan.
    //

    /**
     * Can be used to temporarily pause sends on a connection.
     * The implementation in SockMan always returns true.
     * @param[in] id Connection for which to confirm or omit the next send.
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
     * Be notified of a change in the state of the I2P connectivity.
     * The default behavior, implemented by `SockMan`, is to ignore this event.
     * @param[in] addr The address we started or stopped listening on.
     * @param[in] new_status New status.
     */
    virtual void EventI2PStatus(const CService& addr, I2PStatus new_status);

    /**
     * Accept incoming I2P connections in a loop and call
     * `EventNewConnectionAccepted()` for each new connection.
     */
    void ThreadI2PAccept();

    /**
     * The id to assign to the next created connection. Used to generate ids of connections.
     */
    std::atomic<Id> m_next_id{0};

    /**
     * Thread that accepts incoming I2P connections in a loop, can be stopped via `interruptNet`.
     */
    std::thread m_thread_i2p_accept;
};

#endif // BITCOIN_COMMON_SOCKMAN_H
