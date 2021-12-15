// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/test/wallet_test_fixture.h>

#include <scheduler.h>

WalletTestingSetup::WalletTestingSetup(const std::string& chainName)
    : TestingSetup(chainName),
      m_wallet(m_node.chain.get(), "", m_args, CreateMockWalletDatabase())
{
    m_wallet.LoadWallet();
    m_chain_notifications_handler = m_node.chain->handleNotifications({ &m_wallet, [](CWallet*) {} });
    m_wallet_client->registerRpcs();
}

WalletTestingSetup::~WalletTestingSetup()
{
    if (m_node.scheduler) m_node.scheduler->stop();
}
