// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/chain.h>
#include <interfaces/coinjoin.h>
#include <interfaces/echo.h>
#include <interfaces/init.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <node/context.h>
#include <util/system.h>

#include <memory>

namespace init {
namespace {
class BitcoinQtInit : public interfaces::Init
{
public:
    BitcoinQtInit()
    {
        m_node.args = &gArgs;
        m_node.init = this;
    }
    std::unique_ptr<interfaces::Node> makeNode() override { return interfaces::MakeNode(m_node); }
    std::unique_ptr<interfaces::Chain> makeChain() override { return interfaces::MakeChain(m_node); }
    std::unique_ptr<interfaces::CoinJoin::Loader> makeCoinJoinLoader() override
    {
        return interfaces::MakeCoinJoinLoader(m_node);
    }
    std::unique_ptr<interfaces::WalletLoader> makeWalletLoader(interfaces::Chain& chain, interfaces::CoinJoin::Loader& coinjoin_loader) override
    {
        return MakeWalletLoader(chain, *Assert(m_node.args), m_node, coinjoin_loader);
    }
    std::unique_ptr<interfaces::Echo> makeEcho() override { return interfaces::MakeEcho(); }
    node::NodeContext m_node;
};
} // namespace
} // namespace init

namespace interfaces {
std::unique_ptr<Init> MakeGuiInit(int argc, char* argv[])
{
    return std::make_unique<init::BitcoinQtInit>();
}
} // namespace interfaces
