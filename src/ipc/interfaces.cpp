// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <ipc/capnp/protocol.h>
#include <ipc/process.h>
#include <ipc/protocol.h>
#include <logging.h>
#include <tinyformat.h>
#include <util/system.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

namespace ipc {
namespace {
class IpcImpl : public interfaces::Ipc
{
public:
    IpcImpl(const char* exe_name, const char* process_argv0, interfaces::Init& init)
        : m_exe_name(exe_name), m_process_argv0(process_argv0), m_init(init),
          m_protocol(ipc::capnp::MakeCapnpProtocol()), m_process(ipc::MakeProcess())
    {
    }
    std::unique_ptr<interfaces::Init> spawnProcess(const char* new_exe_name) override
    {
        int pid;
        int fd = m_process->spawn(new_exe_name, m_process_argv0, pid);
        LogPrint(::BCLog::IPC, "Process %s pid %i launched\n", new_exe_name, pid);
        auto init = m_protocol->connect(fd, m_exe_name);
        Ipc::addCleanup(*init, [this, new_exe_name, pid] {
            int status = m_process->waitSpawned(pid);
            LogPrint(::BCLog::IPC, "Process %s pid %i exited with status %i\n", new_exe_name, pid, status);
        });
        return init;
    }
    bool startSpawnedProcess(int argc, char* argv[], int& exit_status) override
    {
        exit_status = EXIT_FAILURE;
        int32_t fd = -1;
        if (!m_process->checkSpawned(argc, argv, fd)) {
            return false;
        }
        m_protocol->serve(fd, m_exe_name, m_init);
        exit_status = EXIT_SUCCESS;
        return true;
    }
    std::unique_ptr<interfaces::Init> connectAddress(std::string& address) override
    {
        if (address.empty() || address == "0") return nullptr;
        int fd = -1;
        std::string error;
        if (address == "auto") {
            // failure to connect with "auto" isn't an error. Caller can spawn a child process or just work offline.
            address = "unix";
            fd = m_process->connect(gArgs.GetDataDirNet(), "bitcoin-node", address, error);
            if (fd < 0) return nullptr;
        } else {
            fd = m_process->connect(gArgs.GetDataDirNet(), "bitcoin-node", address, error);
        }
        if (fd < 0) {
            throw std::runtime_error(
                strprintf("Could not connect to bitcoin-node IPC address '%s'. %s", address, error));
        }
        return m_protocol->connect(fd, m_exe_name);
    }
    bool listenAddress(std::string& address, std::string& error) override
    {
        int fd = m_process->bind(gArgs.GetDataDirNet(), m_exe_name, address, error);
        if (fd < 0) return false;
        m_protocol->listen(fd, m_exe_name, m_init);
        return true;
    }
    void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) override
    {
        m_protocol->addCleanup(type, iface, std::move(cleanup));
    }
    Context& context() override { return m_protocol->context(); }
    const char* m_exe_name;
    const char* m_process_argv0;
    interfaces::Init& m_init;
    std::unique_ptr<Protocol> m_protocol;
    std::unique_ptr<Process> m_process;
};
} // namespace
} // namespace ipc

namespace interfaces {
std::unique_ptr<Ipc> MakeIpc(const char* exe_name, const char* process_argv0, Init& init)
{
    return std::make_unique<ipc::IpcImpl>(exe_name, process_argv0, init);
}
} // namespace interfaces
