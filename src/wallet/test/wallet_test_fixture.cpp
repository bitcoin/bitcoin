// Copyright (c) 2016-2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>

#include <scheduler.h>
#include <util/chaintype.h>

namespace wallet {
WalletTestingSetup::WalletTestingSetup(const ChainType chainType)
    : TestingSetup(chainType),
      m_wallet_loader{interfaces::MakeWalletLoader(*m_node.chain, *Assert(m_node.args))},
      m_wallet(m_node.chain.get(), "", CreateMockableWalletDatabase())
{
    m_wallet.LoadWallet();
    m_chain_notifications_handler = m_node.chain->handleNotifications({ &m_wallet, [](CWallet*) {} });
    m_wallet_loader->registerRpcs();
}

WalletTestingSetup::~WalletTestingSetup()
{
    if (m_node.scheduler) m_node.scheduler->stop();
}
} // namespace wallet
