// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <init.h>
#include <interfaces/chain.h>
#include <interfaces/echo.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <ipc/context.h>
#include <node/context.h>
#include <util/check.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace ipc {
namespace capnp {
void SetupNodeServer(ipc::Context& context);
std::string GlobalArgsNetwork();
} // namespace capnp
} // namespace ipc

namespace init {
namespace {
const char* EXE_NAME = "bitcoin-node";

class BitcoinNodeInit : public interfaces::Init
{
public:
    BitcoinNodeInit(node::NodeContext& node, const char* arg0)
        : m_node(node), m_ipc(interfaces::MakeIpc(EXE_NAME, "", arg0, *this))
    {
        InitContext(m_node);
        m_node.init = this;
        // Extra initialization code that runs when a bitcoin-node process is
        // spawned by a bitcoin-gui process, after the ArgsManager configuration
        // is transferred from the parent process to the child process.
        m_ipc->context().init_process = [this] {
            InitLogging(*Assert(m_node.args), m_ipc->logSuffix());
            InitParameterInteraction(*Assert(m_node.args));
        };
        ipc::capnp::SetupNodeServer(m_ipc->context());
    }
    std::unique_ptr<interfaces::Node> makeNode() override { return interfaces::MakeNode(m_node); }
    std::unique_ptr<interfaces::Chain> makeChain() override { return interfaces::MakeChain(m_node); }
    std::unique_ptr<interfaces::Mining> makeMining() override { return interfaces::MakeMining(m_node); }
    std::unique_ptr<interfaces::Echo> makeEcho() override { return interfaces::MakeEcho(); }
    interfaces::Ipc* ipc() override { return m_ipc.get(); }
    bool canListenIpc() override { return true; }
    node::NodeContext& m_node;
    std::unique_ptr<interfaces::Ipc> m_ipc;
};
} // namespace
} // namespace init

namespace interfaces {
std::unique_ptr<Init> MakeNodeInit(node::NodeContext& node, int argc, char* argv[], int& exit_status)
{
    auto init = std::make_unique<init::BitcoinNodeInit>(node, argc > 0 ? argv[0] : "");
    // Check if bitcoin-node is being invoked as an IPC server by the gui. If
    // so, then bypass normal execution and just respond to requests over the
    // IPC channel and return null.
    if (init->m_ipc->startSpawnedProcess(argc, argv, exit_status)) {
        return nullptr;
    }
    return init;
}
} // namespace interfaces
