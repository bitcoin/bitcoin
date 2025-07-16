// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(spend_tests, WalletTestingSetup)

BOOST_FIXTURE_TEST_CASE(SubtractFee, TestChain100Setup)
{
    CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    auto wallet = CreateSyncedWallet(*m_node.chain, *m_node.coinjoin_loader, m_node.chainman->ActiveChain(), m_args, coinbaseKey);

    // Check that a subtract-from-recipient transaction slightly less than the
    // coinbase input amount does not create a change output (because it would
    // be uneconomical to add and spend the output), and make sure it pays the
    // leftover input amount which would have been change to the recipient
    // instead of the miner.
    auto check_tx = [&wallet](CAmount leftover_input_amount) {
        CRecipient recipient{GetScriptForRawPubKey({}), 500 * COIN - leftover_input_amount, true /* subtract fee */};
        bilingual_str error;
        CCoinControl coin_control;
        coin_control.m_feerate.emplace(10000);
        coin_control.fOverrideFeeRate = true;
        FeeCalculation fee_calc;
        std::optional<CreatedTransactionResult> txr = CreateTransaction(*wallet, {recipient}, RANDOM_CHANGE_POSITION, error, coin_control, fee_calc);
        BOOST_CHECK(txr.has_value());
        BOOST_CHECK_EQUAL(txr->tx->vout.size(), 1);
        BOOST_CHECK_EQUAL(txr->tx->vout[0].nValue, recipient.nAmount + leftover_input_amount - txr->fee);
        BOOST_CHECK_GT(txr->fee, 0);
        return txr->fee;
    };

    // Send full input amount to recipient, check that only nonzero fee is
    // subtracted (to_reduce == fee).
    const CAmount fee{check_tx(0)};

    // Send slightly less than full input amount to recipient, check leftover
    // input amount is paid to recipient not the miner (to_reduce == fee - 123)
    BOOST_CHECK_EQUAL(fee, check_tx(123));

    // Send full input minus fee amount to recipient, check leftover input
    // amount is paid to recipient not the miner (to_reduce == 0)
    BOOST_CHECK_EQUAL(fee, check_tx(fee));

    // Send full input minus more than the fee amount to recipient, check
    // leftover input amount is paid to recipient not the miner (to_reduce ==
    // -123). This overpays the recipient instead of overpaying the miner more
    // than double the neccesary fee.
    BOOST_CHECK_EQUAL(fee, check_tx(fee + 123));
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
