// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//! @file ipc/types.h is a home for public enum and struct type definitions
//! that are used internally by ipc code, but also used externally by wallet,
//! mining or GUI code.
//!
//! This file is intended to define only simple types that do not have external
//! dependencies. More complicated types should be defined in dedicated header
//! files.

#ifndef BITCOIN_IPC_TYPES_H
#define BITCOIN_IPC_TYPES_H

#include <cstddef>
#include <string>

namespace ipc {

//! Default accepted client connection limit for each IPC listening socket.
inline constexpr size_t DEFAULT_MAX_CONNECTIONS{16};

//! Maximum accepted client connection limit for an IPC listening socket.
//! This conservative bound leaves ample room for other file descriptor
//! reservations in int-based initialization accounting.
inline constexpr size_t MAX_CONNECTIONS{size_t{1} << 20};

struct ListenAddress {
    //! Socket address passed to ipc::Process::bind(), with any -ipcbind socket
    //! options stripped.
    std::string address;

    //! Maximum simultaneous client connections accepted by this listener.
    size_t max_connections{DEFAULT_MAX_CONNECTIONS};
};

} // namespace ipc

#endif // BITCOIN_IPC_TYPES_H
