// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_HANDLER_H
#define BITCOIN_INTERFACES_HANDLER_H

#include <functional>
#include <memory>

namespace boost {
namespace signals2 {
class connection;
} // namespace signals2
} // namespace boost

namespace interfaces {

//! Generic interface for managing an event handler or callback function
//! registered with another interface.
class Handler
{
public:
    virtual ~Handler() = default;

    //! Connect the handler, start receiving notifications.
    virtual void connect() {};

    //! Disconnect the handler, stop receiving notifications.
    virtual void disconnect() = 0;

    //! Return whether handler is currently connected.
    virtual bool connected() = 0;

    //! Interrupt the handler, allowing it to disconnect more quickly. (Useful
    //! to disconnect multiple handlers in parallel when disconnect is
    //! blocking.)
    virtual void interrupt() {};

    //! Block until all current notifications are received.
    virtual void sync() {};
};

//! Return handler wrapping a boost signal connection.
std::unique_ptr<Handler> MakeSignalHandler(boost::signals2::connection connection);

//! Return handler wrapping a cleanup function.
std::unique_ptr<Handler> MakeCleanupHandler(std::function<void()> cleanup);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_HANDLER_H
