// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_SOCK_H
#define BITCOIN_UTIL_SOCK_H

#include <compat.h>

#include <chrono>
#include <string>

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
     * wrapper can be unit-tested if this method is overridden by a mock Sock implementation.
     */
    virtual ssize_t Send(const void* data, size_t len, int flags) const;

    /**
     * recv(2) wrapper. Equivalent to `recv(this->Get(), buf, len, flags);`. Code that uses this
     * wrapper can be unit-tested if this method is overridden by a mock Sock implementation.
     */
    virtual ssize_t Recv(void* buf, size_t len, int flags) const;

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
     * @return true on success and false otherwise
     */
    virtual bool Wait(std::chrono::milliseconds timeout, Event requested) const;

private:
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
