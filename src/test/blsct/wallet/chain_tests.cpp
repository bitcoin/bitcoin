// Copyright (c) 2023 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <policy/fees.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(blsct_chain_tests, WalletTestingSetup)

BOOST_FIXTURE_TEST_CASE(SyncTest, TestBLSCTChain100Setup)
{
    CreateAndProcessBlock({});
    auto wallet = CreateBLSCTWallet(*m_node.chain, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()));
    BOOST_CHECK(SyncBLSCTWallet(wallet, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain())));
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
