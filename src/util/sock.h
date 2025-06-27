// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_SOCK_H
#define BITCOIN_UTIL_SOCK_H

#include <compat/compat.h>
#include <threadinterrupt.h>
#include <util/time.h>

#include <chrono>
#include <memory>
#include <string>

/**
 * Maximum time to wait for I/O readiness.
 * It will take up until this time to break off in case of an interruption.
 */
static constexpr auto MAX_WAIT_FOR_IO = 1s;

enum class SocketEventsMode : int8_t {
    Select = 0,
    Poll = 1,
    EPoll = 2,
    KQueue = 3,

    Unknown = -1
};

/* Converts SocketEventsMode value to string with additional check to report modes not compiled for as unknown */
constexpr std::string_view SEMToString(const SocketEventsMode val) {
    switch (val) {
        case (SocketEventsMode::Select):
            return "select";
#ifdef USE_POLL
        case (SocketEventsMode::Poll):
            return "poll";
#endif /* USE_POLL */
#ifdef USE_EPOLL
        case (SocketEventsMode::EPoll):
            return "epoll";
#endif /* USE_EPOLL */
#ifdef USE_KQUEUE
        case (SocketEventsMode::KQueue):
            return "kqueue";
#endif /* USE_KQUEUE */
        default:
            return "unknown";
    };
}

/* Converts string to SocketEventsMode value with additional check to report modes not compiled for as unknown */
constexpr SocketEventsMode SEMFromString(std::string_view str) {
    if (str == "select") { return SocketEventsMode::Select; }
#ifdef USE_POLL
    if (str == "poll")   { return SocketEventsMode::Poll;   }
#endif /* USE_POLL */
#ifdef USE_EPOLL
    if (str == "epoll")  { return SocketEventsMode::EPoll;  }
#endif /* USE_EPOLL */
#ifdef USE_KQUEUE
    if (str == "kqueue") { return SocketEventsMode::KQueue; }
#endif /* USE_KQUEUE */
    return SocketEventsMode::Unknown;
}

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
    [[nodiscard]] virtual SOCKET Get() const;

    /**
     * send(2) wrapper. Equivalent to `send(this->Get(), data, len, flags);`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual ssize_t Send(const void* data, size_t len, int flags) const;

    /**
     * recv(2) wrapper. Equivalent to `recv(this->Get(), buf, len, flags);`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual ssize_t Recv(void* buf, size_t len, int flags) const;

    /**
     * connect(2) wrapper. Equivalent to `connect(this->Get(), addr, addrlen)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual int Connect(const sockaddr* addr, socklen_t addr_len) const;

    /**
     * bind(2) wrapper. Equivalent to `bind(this->Get(), addr, addr_len)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual int Bind(const sockaddr* addr, socklen_t addr_len) const;

    /**
     * listen(2) wrapper. Equivalent to `listen(this->Get(), backlog)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual int Listen(int backlog) const;

    /**
     * accept(2) wrapper. Equivalent to `std::make_unique<Sock>(accept(this->Get(), addr, addr_len))`.
     * Code that uses this wrapper can be unit tested if this method is overridden by a mock Sock
     * implementation.
     * The returned unique_ptr is empty if `accept()` failed in which case errno will be set.
     */
    [[nodiscard]] virtual std::unique_ptr<Sock> Accept(sockaddr* addr, socklen_t* addr_len) const;

    /**
     * getsockopt(2) wrapper. Equivalent to
     * `getsockopt(this->Get(), level, opt_name, opt_val, opt_len)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual int GetSockOpt(int level,
                                         int opt_name,
                                         void* opt_val,
                                         socklen_t* opt_len) const;

    /**
     * setsockopt(2) wrapper. Equivalent to
     * `setsockopt(this->Get(), level, opt_name, opt_val, opt_len)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual int SetSockOpt(int level,
                                         int opt_name,
                                         const void* opt_val,
                                         socklen_t opt_len) const;

    /**
     * getsockname(2) wrapper. Equivalent to
     * `getsockname(this->Get(), name, name_len)`. Code that uses this
     * wrapper can be unit tested if this method is overridden by a mock Sock implementation.
     */
    [[nodiscard]] virtual int GetSockName(sockaddr* name, socklen_t* name_len) const;

    /**
     * Set the non-blocking option on the socket.
     * @return true if set successfully
     */
    [[nodiscard]] virtual bool SetNonBlocking() const;

    /**
     * Check if the underlying socket can be used for `select(2)` (or the `Wait()` method).
     * @return true if selectable
     */
    [[nodiscard]] virtual bool IsSelectable() const;

    using Event = uint8_t;

    /**
     * If passed to `Wait()`, then it will wait for readiness to read from the socket.
     */
    static constexpr Event RECV = 0b001;

    /**
     * If passed to `Wait()`, then it will wait for readiness to send to the socket.
     */
    static constexpr Event SEND = 0b010;

    /**
     * Ignored if passed to `Wait()`, but could be set in the occurred events if an
     * exceptional condition has occurred on the socket or if it has been disconnected.
     */
    static constexpr Event ERR = 0b100;

    /**
     * Wait for readiness for input (recv) or output (send).
     * @param[in] timeout Wait this much for at least one of the requested events to occur.
     * @param[in] requested Wait for those events, bitwise-or of `RECV` and `SEND`.
     * @param[out] occurred If not nullptr and the function returns `true`, then this
     * indicates which of the requested events occurred (`ERR` will be added, even if
     * not requested, if an exceptional event occurs on the socket).
     * A timeout is indicated by return value of `true` and `occurred` being set to 0.
     * @return true on success (or timeout, if `occurred` of 0 is returned), false otherwise
     */
    [[nodiscard]] virtual bool Wait(std::chrono::milliseconds timeout,
                                    Event requested,
                                    Event* occurred = nullptr) const;
#ifdef USE_POLL
    bool WaitPoll(std::chrono::milliseconds timeout, Event requested, Event* occurred = nullptr) const;
#endif /* USE_POLL */
    bool WaitSelect(std::chrono::milliseconds timeout, Event requested, Event* occurred = nullptr) const;

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
    [[nodiscard]] virtual std::string RecvUntilTerminator(uint8_t terminator,
                                                          std::chrono::milliseconds timeout,
                                                          CThreadInterrupt& interrupt,
                                                          size_t max_data) const;

    /**
     * Check if still connected.
     * @param[out] errmsg The error string, if the socket has been disconnected.
     * @return true if connected
     */
    [[nodiscard]] virtual bool IsConnected(std::string& errmsg) const;

protected:
    /**
     * Contained socket. `INVALID_SOCKET` designates the object is empty.
     */
    SOCKET m_socket;

private:
    /**
     * Close `m_socket` if it is not `INVALID_SOCKET`.
     */
    void Close();
};

/** Return readable error string for a network error code */
std::string NetworkErrorString(int err);

#endif // BITCOIN_UTIL_SOCK_H
