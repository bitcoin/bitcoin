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

struct BLSCT100Setup : public TestBLSCTChain100Setup {
    BLSCT100Setup()
        : TestBLSCTChain100Setup{blsct::SubAddress()} {}
};

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(blsct_chain_tests, WalletTestingSetup)

BOOST_FIXTURE_TEST_CASE(SyncTest, BLSCT100Setup)
{
    CreateAndProcessBlock({});
    auto wallet = CreateBLSCTWallet(*m_node.chain, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()));
    BOOST_CHECK(SyncBLSCTWallet(wallet, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain())));

    // Check that a subtract-from-recipient transaction slightly less than the
    // coinbase input amount does not create a change output (because it would
    // be uneconomical to add and spend the output), and make sure it pays the
    // leftover input amount which would have been change to the recipient
    // instead of the miner.
    // auto check_tx = [&wallet](CAmount leftover_input_amount) {
    //     CRecipient recipient{GetScriptForRawPubKey({}), 50 * COIN - leftover_input_amount, /*subtract_fee=*/true};
    //     constexpr int RANDOM_CHANGE_POSITION = -1;
    //     CCoinControl coin_control;
    //     coin_control.m_feerate.emplace(10000);
    //     coin_control.fOverrideFeeRate = true;
    //     // We need to use a change type with high cost of change so that the leftover amount will be dropped to fee instead of added as a change output
    //     coin_control.m_change_type = OutputType::LEGACY;
    //     auto res = CreateTransaction(*wallet, {recipient}, RANDOM_CHANGE_POSITION, coin_control);
    //     BOOST_CHECK(res);
    //     const auto& txr = *res;
    //     BOOST_CHECK_EQUAL(txr.tx->vout.size(), 1);
    //     BOOST_CHECK_EQUAL(txr.tx->vout[0].nValue, recipient.nAmount + leftover_input_amount - txr.fee);
    //     BOOST_CHECK_GT(txr.fee, 0);
    //     return txr.fee;
    // };

    // // Send full input amount to recipient, check that only nonzero fee is
    // // subtracted (to_reduce == fee).
    // const CAmount fee{check_tx(0)};

    // // Send slightly less than full input amount to recipient, check leftover
    // // input amount is paid to recipient not the miner (to_reduce == fee - 123)
    // BOOST_CHECK_EQUAL(fee, check_tx(123));

    // // Send full input minus fee amount to recipient, check leftover input
    // // amount is paid to recipient not the miner (to_reduce == 0)
    // BOOST_CHECK_EQUAL(fee, check_tx(fee));

    // // Send full input minus more than the fee amount to recipient, check
    // // leftover input amount is paid to recipient not the miner (to_reduce ==
    // // -123). This overpays the recipient instead of overpaying the miner more
    // // than double the necessary fee.
    // BOOST_CHECK_EQUAL(fee, check_tx(fee + 123));
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
