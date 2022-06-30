// Copyright (c) 2021 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WIDECOIN_IPC_CONTEXT_H
#define WIDECOIN_IPC_CONTEXT_H

namespace ipc {
//! Context struct used to give IPC protocol implementations or implementation
//! hooks access to application state, in case they need to run extra code that
//! isn't needed within a single process, like code copying global state from an
//! existing process to a new process when it's initialized, or code dealing
//! with shared objects that are created or destroyed remotely.
struct Context
{
};
} // namespace ipc

#endif // WIDECOIN_IPC_CONTEXT_H
