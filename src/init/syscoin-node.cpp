// Copyright (c) 2021 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/echo.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <node/context.h>

#include <memory>

namespace init {
namespace {
const char* EXE_NAME = "syscoin-node";

class SyscoinNodeInit : public interfaces::Init
{
public:
    SyscoinNodeInit(NodeContext& node, const char* arg0)
        : m_node(node),
          m_ipc(interfaces::MakeIpc(EXE_NAME, arg0, *this))
    {
        m_node.init = this;
    }
    std::unique_ptr<interfaces::Echo> makeEcho() override { return interfaces::MakeEcho(); }
    interfaces::Ipc* ipc() override { return m_ipc.get(); }
    NodeContext& m_node;
    std::unique_ptr<interfaces::Ipc> m_ipc;
};
} // namespace
} // namespace init

namespace interfaces {
std::unique_ptr<Init> MakeNodeInit(NodeContext& node, int argc, char* argv[], int& exit_status)
{
    auto init = std::make_unique<init::SyscoinNodeInit>(node, argc > 0 ? argv[0] : "");
    // Check if syscoin-node is being invoked as an IPC server. If so, then
    // bypass normal execution and just respond to requests over the IPC
    // channel and return null.
    if (init->m_ipc->startSpawnedProcess(argc, argv, exit_status)) {
        return nullptr;
    }
    return init;
}
} // namespace interfaces
