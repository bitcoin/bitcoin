// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_PROCESS_H
#define BITCOIN_IPC_PROCESS_H

#include <util/fs.h>

#include <memory>
#include <mp/util.h>
#include <string>

namespace ipc {
class Protocol;

//! IPC process interface for spawning bitcoin processes and serving requests
//! in processes that have been spawned.
//!
//! There may be different implementations of this interface depending on the
//! platform (e.g. unix, windows).
class Process
{
public:
    virtual ~Process() = default;

    //! Spawn process and return socket file descriptor for communicating with
    //! it.
    virtual mp::ProcessId spawn(mp::SocketId socket, const std::string& new_exe_name, const fs::path& argv0_path) = 0;

    //! Wait for spawned process to exit and return its exit code.
    virtual int waitSpawned(mp::ProcessId pid) = 0;

    //! Parse command line and determine if current process is a spawned child
    //! process. If so, return true and a file descriptor for communicating
    //! with the parent process.
    virtual bool checkSpawned(int argc, char* argv[], mp::SocketId& socket) = 0;

    //! Canonicalize and connect to address, returning socket descriptor.
    virtual mp::SocketId connect(const fs::path& data_dir,
                        const std::string& dest_exe_name,
                        std::string& address) = 0;

    //! Create listening socket, bind and canonicalize address, and return socket descriptor.
    virtual mp::SocketId bind(const fs::path& data_dir,
                     const std::string& exe_name,
                     std::string& address) = 0;
};

//! Constructor for Process interface. Implementation will vary depending on
//! the platform (unix or windows).
std::unique_ptr<Process> MakeProcess();
} // namespace ipc

#endif // BITCOIN_IPC_PROCESS_H
