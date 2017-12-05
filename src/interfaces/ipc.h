// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_IPC_H
#define BITCOIN_INTERFACES_IPC_H

#include <functional>
#include <memory>
#include <typeindex>

namespace interfaces {
class Init;

//! Interface providing access to interprocess-communication (IPC)
//! functionality.
class Ipc
{
public:
    virtual ~Ipc() = default;

    //! Spawn a child process returning pointer to its Init interface.
    virtual std::unique_ptr<Init> spawnProcess(const char* exe_name) = 0;

    //! If this is a spawned process, block and handle requests from the parent
    //! process by forwarding them to this process's Init interface, then return
    //! true. If this is not a spawned child process, return false.
    virtual bool startSpawnedProcess(int argc, char* argv[], int& exit_status) = 0;

    //! Add cleanup callback to remote interface that will run when the
    //! interface is deleted.
    template<typename Interface>
    void addCleanup(Interface& iface, std::function<void()> cleanup)
    {
        addCleanup(typeid(Interface), &iface, std::move(cleanup));
    }

protected:
    //! Internal implementation of public addCleanup method (above) as a
    //! type-erased virtual function, since template functions can't be virtual.
    virtual void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) = 0;
};

//! Return implementation of Ipc interface.
std::unique_ptr<Ipc> MakeIpc(const char* exe_name, const char* process_argv0, Init& init);
} // namespace interfaces

#endif // BITCOIN_INTERFACES_IPC_H
