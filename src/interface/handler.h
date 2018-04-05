// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACE_HANDLER_H
#define BITCOIN_INTERFACE_HANDLER_H

#include <memory>

namespace boost {
namespace signals2 {
class connection;
} // namespace signals2
} // namespace boost

namespace interface {

//! Generic interface for managing an event handler or callback function
//! registered with another interface. Has a single disconnect method to cancel
//! the registration and prevent any future notifications.
class Handler
{
public:
    virtual ~Handler() {}

    //! Disconnect the handler.
    virtual void disconnect() = 0;
};

//! Return handler wrapping a boost signal connection.
std::unique_ptr<Handler> MakeHandler(boost::signals2::connection connection);

} // namespace interface

#endif // BITCOIN_INTERFACE_HANDLER_H
