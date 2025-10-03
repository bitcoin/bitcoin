// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CONTEXT_H
#define BITCOIN_IPC_CONTEXT_H

#include <functional>

namespace ipc {
//! Context struct used to give IPC protocol implementations or implementation
//! hooks access to application state, in case they need to run extra code that
//! isn't needed within a single process, like code copying global state from an
//! existing process to a new process when it's initialized, or code dealing
//! with shared objects that are created or destroyed remotely.
struct Context
{
    //! Callback to initialize spawned process after receiving ArgsManager
    //! configuration from parent.
    std::function<void()> init_process;
};
} // namespace ipc

#endif // BITCOIN_IPC_CONTEXT_H
