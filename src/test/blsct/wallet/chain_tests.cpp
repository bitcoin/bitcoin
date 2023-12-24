// Copyright (c) 2023 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/txfactory.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <policy/fees.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/receive.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(chain_tests, WalletTestingSetup)

BOOST_FIXTURE_TEST_CASE(SyncTest, TestBLSCTChain100Setup)
{
    CreateAndProcessBlock({});
    auto wallet = CreateBLSCTWallet(*m_node.chain, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()));
    BOOST_CHECK(SyncBLSCTWallet(wallet, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain())));

    auto blsct_km = wallet->GetBLSCTKeyMan();
    auto walletDestination = blsct::SubAddress(std::get<blsct::DoublePublicKey>(blsct_km->GetNewDestination(0).value()));

    LOCK(wallet->cs_wallet);

    for (size_t i = 0; i <= COINBASE_MATURITY; i++) {
        CreateAndProcessBlock({}, walletDestination);
    }

    BOOST_CHECK(SyncBLSCTWallet(wallet, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain())));

    BOOST_CHECK(GetBalance(*wallet).m_mine_trusted == 50 * COIN);
    BOOST_CHECK(GetBalance(*wallet).m_mine_immature == 50 * COINBASE_MATURITY * COIN);

    auto available_coins = AvailableCoins(*wallet);
    std::vector<COutput> coins = available_coins.All();

    BOOST_ASSERT(coins.size() == 1);

    // Create Transaction sending to another address
    auto tx = blsct::TxFactory::CreateTransaction(wallet.get(), wallet->GetOrCreateBLSCTKeyMan(), blsct::SubAddress(), 1 * COIN, "test");

    BOOST_ASSERT(tx != std::nullopt);

    auto block = CreateAndProcessBlock({tx.value()}, walletDestination);
    BOOST_CHECK(SyncBLSCTWallet(wallet, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain())));

    auto wtx = wallet->GetWalletTx(block.vtx[1]->GetHash());

    BOOST_CHECK(wtx != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
