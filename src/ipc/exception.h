// Copyright (c) 2021 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_IPC_EXCEPTION_H
#define TORTOISECOIN_IPC_EXCEPTION_H

#include <stdexcept>

namespace ipc {
//! Exception class thrown when a call to remote method fails due to an IPC
//! error, like a socket getting disconnected.
class Exception : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};
} // namespace ipc

#endif // TORTOISECOIN_IPC_EXCEPTION_H
