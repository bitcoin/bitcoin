// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init.h>
#include <interfaces/chain.h>
#include <interfaces/echo.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <node/context.h>
#include <util/check.h>

#include <memory>

namespace init {
namespace {
const char* EXE_NAME = "bitcoin-gui";

class BitcoinGuiInit : public interfaces::Init
{
public:
    BitcoinGuiInit(const char* arg0) : m_ipc(interfaces::MakeIpc(EXE_NAME, arg0, *this))
    {
        InitContext(m_node);
        m_node.init = this;
    }
    std::unique_ptr<interfaces::Node> makeNode() override { return interfaces::MakeNode(m_node); }
    std::unique_ptr<interfaces::Chain> makeChain() override { return interfaces::MakeChain(m_node); }
    std::unique_ptr<interfaces::WalletLoader> makeWalletLoader(interfaces::Chain& chain) override
    {
        return MakeWalletLoader(chain, *Assert(m_node.args));
    }
    std::unique_ptr<interfaces::Echo> makeEcho() override { return interfaces::MakeEcho(); }
    interfaces::Ipc* ipc() override { return m_ipc.get(); }
    // bitcoin-gui accepts -ipcbind option even though it does not use it
    // directly. It just returns true here to accept the option because
    // bitcoin-node accepts the option, and bitcoin-gui accepts all bitcoin-node
    // options and will start the node with those options.
    bool canListenIpc() override { return true; }
    node::NodeContext m_node;
    std::unique_ptr<interfaces::Ipc> m_ipc;
};
} // namespace
} // namespace init

namespace interfaces {
std::unique_ptr<Init> MakeGuiInit(int argc, char* argv[])
{
    return std::make_unique<init::BitcoinGuiInit>(argc > 0 ? argv[0] : "");
}
} // namespace interfaces
