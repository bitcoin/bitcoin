// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_SOCK_H
#define BITCOIN_UTIL_SOCK_H

#include <compat.h>
#include <threadinterrupt.h>
#include <util/time.h>

#include <chrono>
#include <string>

/**
 * Maximum time to wait for I/O readiness.
 * It will take up until this time to break off in case of an interruption.
 */
static constexpr auto MAX_WAIT_FOR_IO = 1s;

/**
 * RAII helper class that manages a socket. Mimics `std::unique_ptr`, but instead of a pointer it
 * contains a socket and closes it automatically when it goes out of scope.
 */
class Sock
{
public:
    /**
     * Default constructor, creates an empty object that does nothing when destroyed.
     */
    Sock();

    /**
     * Take ownership of an existent socket.
     */
    explicit Sock(SOCKET s);

    /**
     * Copy constructor, disabled because closing the same socket twice is undesirable.
     */
    Sock(const Sock&) = delete;

    /**
     * Move constructor, grab the socket from another object and close ours (if set).
     */
    Sock(Sock&& other);

    /**
     * Destructor, close the socket or do nothing if empty.
     */
    virtual ~Sock();

    /**
     * Copy assignment operator, disabled because closing the same socket twice is undesirable.
     */
    Sock& operator=(const Sock&) = delete;

    /**
     * Move assignment operator, grab the socket from another object and close ours (if set).
     */
    virtual Sock& operator=(Sock&& other);

    /**
     * Check whether *this owns a socket.
     */
    virtual explicit operator bool() const;

    /**
     * Get the value of the contained socket.
     * @return socket or INVALID_SOCKET if empty
     */
    virtual SOCKET Get() const;

    /**
     * Get the value of the contained socket and drop ownership. It will not be closed by the
     * destructor after this call.
     * @return socket or INVALID_SOCKET if empty
     */
    virtual SOCKET Release();

    /**
     * Close if non-empty.
     */
    virtual void Reset();

    /**
     * send(2) wrapper. Equivalent to `send(this->Get(), data, len, flags);`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    virtual ssize_t Send(const void* data, size_t len, int flags) const;

    /**
     * recv(2) wrapper. Equivalent to `recv(this->Get(), buf, len, flags);`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    virtual ssize_t Recv(void* buf, size_t len, int flags) const;

    /**
     * connect(2) wrapper. Equivalent to `connect(this->Get(), addr, addrlen)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    virtual int Connect(const sockaddr* addr, socklen_t addr_len) const;

    /**
     * getsockopt(2) wrapper. Equivalent to
     * `getsockopt(this->Get(), level, opt_name, opt_val, opt_len)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    virtual int GetSockOpt(int level, int opt_name, void* opt_val, socklen_t* opt_len) const;

    /**
     * setsockopt(2) wrapper. Equivalent to
     * `setsockopt(this->Get(), level, opt_name, opt_val, opt_len)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    virtual int SetSockOpt(int level, int opt_name, const void* opt_val, socklen_t opt_len) const;

    /**
     * getsockname(2) wrapper. Equivalent to
     * `getsockname(this->Get(), name, name_len)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    virtual int GetSockName(sockaddr* name, socklen_t* name_len) const;

    /**
     * Shortcut to set the TCP_NODELAY option with SetSockOpt().
     * @return true if set successfully
     */
    [[nodiscard]] virtual bool SetNoDelay() const;

    /**
     * Check if the underlying socket can be used for `select(2)` (or the `Wait()` method).
     * @return true if selectable
     */
    [[nodiscard]] virtual bool IsSelectable() const;

    using Event = uint8_t;

    /**
     * If passed to `Wait()`, then it will wait for readiness to read from the socket.
     */
    static constexpr Event RECV = 0b01;

    /**
     * If passed to `Wait()`, then it will wait for readiness to send to the socket.
     */
    static constexpr Event SEND = 0b10;

    /**
     * Wait for readiness for input (recv) or output (send).
     * @param[in] timeout Wait this much for at least one of the requested events to occur.
     * @param[in] requested Wait for those events, bitwise-or of `RECV` and `SEND`.
     * @param[out] occurred If not nullptr and `true` is returned, then upon return this
     * indicates which of the requested events occurred. A timeout is indicated by return
     * value of `true` and `occurred` being set to 0.
     * @return true on success and false otherwise
     */
    virtual bool Wait(std::chrono::milliseconds timeout,
                      Event requested,
                      Event* occurred = nullptr) const;

    /* Higher level, convenience, methods. These may throw. */

    /**
     * Send the given data, retrying on transient errors.
     * @param[in] data Data to send.
     * @param[in] timeout Timeout for the entire operation.
     * @param[in] interrupt If this is signaled then the operation is canceled.
     * @throws std::runtime_error if the operation cannot be completed. In this case only some of
     * the data will be written to the socket.
     */
    virtual void SendComplete(const std::string& data,
                              std::chrono::milliseconds timeout,
                              CThreadInterrupt& interrupt) const;

    /**
     * Read from socket until a terminator character is encountered. Will never consume bytes past
     * the terminator from the socket.
     * @param[in] terminator Character up to which to read from the socket.
     * @param[in] timeout Timeout for the entire operation.
     * @param[in] interrupt If this is signaled then the operation is canceled.
     * @param[in] max_data The maximum amount of data (in bytes) to receive. If this many bytes
     * are received and there is still no terminator, then this method will throw an exception.
     * @return The data that has been read, without the terminating character.
     * @throws std::runtime_error if the operation cannot be completed. In this case some bytes may
     * have been consumed from the socket.
     */
    virtual std::string RecvUntilTerminator(uint8_t terminator,
                                            std::chrono::milliseconds timeout,
                                            CThreadInterrupt& interrupt,
                                            size_t max_data) const;

    /**
     * Check if still connected.
     * @param[out] errmsg The error string, if the socket has been disconnected.
     * @return true if connected
     */
    virtual bool IsConnected(std::string& errmsg) const;

protected:
    /**
     * Contained socket. `INVALID_SOCKET` designates the object is empty.
     */
    SOCKET m_socket;
};

/** Return readable error string for a network error code */
std::string NetworkErrorString(int err);

/** Close socket and set hSocket to INVALID_SOCKET */
bool CloseSocket(SOCKET& hSocket);

#endif // BITCOIN_UTIL_SOCK_H
