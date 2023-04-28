// Copyright (c) 2021-2022 The Bitcoin Core developers
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
BOOST_FIXTURE_TEST_SUITE(spend_tests, WalletTestingSetup)

BOOST_FIXTURE_TEST_CASE(SubtractFee, TestChain100Setup)
{
    CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    auto wallet = CreateSyncedWallet(*m_node.chain, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()), coinbaseKey);

    // Check that a subtract-from-recipient transaction slightly less than the
    // coinbase input amount does not create a change output (because it would
    // be uneconomical to add and spend the output), and make sure it pays the
    // leftover input amount which would have been change to the recipient
    // instead of the miner.
    auto check_tx = [&wallet](CAmount leftover_input_amount) {
        CRecipient recipient{GetScriptForRawPubKey({}), 50 * COIN - leftover_input_amount, /*subtract_fee=*/true};
        constexpr int RANDOM_CHANGE_POSITION = -1;
        CCoinControl coin_control;
        coin_control.m_feerate.emplace(10000);
        coin_control.fOverrideFeeRate = true;
        // We need to use a change type with high cost of change so that the leftover amount will be dropped to fee instead of added as a change output
        coin_control.m_change_type = OutputType::LEGACY;
        auto res = CreateTransaction(*wallet, {recipient}, RANDOM_CHANGE_POSITION, coin_control);
        BOOST_CHECK(res);
        const auto& txr = *res;
        BOOST_CHECK_EQUAL(txr.tx->vout.size(), 1);
        BOOST_CHECK_EQUAL(txr.tx->vout[0].nValue, recipient.nAmount + leftover_input_amount - txr.fee);
        BOOST_CHECK_GT(txr.fee, 0);
        return txr.fee;
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
    // than double the necessary fee.
    BOOST_CHECK_EQUAL(fee, check_tx(fee + 123));
}

static void TestFillInputToWeight(int64_t additional_weight, std::vector<int64_t> expected_stack_sizes)
{
    static const int64_t EMPTY_INPUT_WEIGHT = GetTransactionInputWeight(CTxIn());

    CTxIn input;
    int64_t target_weight = EMPTY_INPUT_WEIGHT + additional_weight;
    BOOST_CHECK(FillInputToWeight(input, target_weight));
    BOOST_CHECK_EQUAL(GetTransactionInputWeight(input), target_weight);
    BOOST_CHECK_EQUAL(input.scriptWitness.stack.size(), expected_stack_sizes.size());
    for (unsigned int i = 0; i < expected_stack_sizes.size(); ++i) {
        BOOST_CHECK_EQUAL(input.scriptWitness.stack[i].size(), expected_stack_sizes[i]);
    }
}

BOOST_FIXTURE_TEST_CASE(FillInputToWeightTest, BasicTestingSetup)
{
    {
        // Less than or equal minimum of 165 should not add any witness data
        CTxIn input;
        BOOST_CHECK(!FillInputToWeight(input, -1));
        BOOST_CHECK_EQUAL(GetTransactionInputWeight(input), 165);
        BOOST_CHECK_EQUAL(input.scriptWitness.stack.size(), 0);
        BOOST_CHECK(!FillInputToWeight(input, 0));
        BOOST_CHECK_EQUAL(GetTransactionInputWeight(input), 165);
        BOOST_CHECK_EQUAL(input.scriptWitness.stack.size(), 0);
        BOOST_CHECK(!FillInputToWeight(input, 164));
        BOOST_CHECK_EQUAL(GetTransactionInputWeight(input), 165);
        BOOST_CHECK_EQUAL(input.scriptWitness.stack.size(), 0);
        BOOST_CHECK(FillInputToWeight(input, 165));
        BOOST_CHECK_EQUAL(GetTransactionInputWeight(input), 165);
        BOOST_CHECK_EQUAL(input.scriptWitness.stack.size(), 0);
    }

    // Make sure we can add at least one weight
    TestFillInputToWeight(1, {0});

    // 1 byte compact size uint boundary
    TestFillInputToWeight(252, {251});
    TestFillInputToWeight(253, {83, 168});
    TestFillInputToWeight(262, {86, 174});
    TestFillInputToWeight(263, {260});

    // 3 byte compact size uint boundary
    TestFillInputToWeight(65535, {65532});
    TestFillInputToWeight(65536, {21842, 43688});
    TestFillInputToWeight(65545, {21845, 43694});
    TestFillInputToWeight(65546, {65541});

    // Note: We don't test the next boundary because of memory allocation constraints.
}

BOOST_FIXTURE_TEST_CASE(wallet_duplicated_preset_inputs_test, TestChain100Setup)
{
    // Verify that the wallet's Coin Selection process does not include pre-selected inputs twice in a transaction.

    // Add 4 spendable UTXO, 50 BTC each, to the wallet (total balance 200 BTC)
    for (int i = 0; i < 4; i++) CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    auto wallet = CreateSyncedWallet(*m_node.chain, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()), coinbaseKey);

    LOCK(wallet->cs_wallet);
    auto available_coins = AvailableCoins(*wallet);
    std::vector<COutput> coins = available_coins.All();
    // Preselect the first 3 UTXO (150 BTC total)
    std::set<COutPoint> preset_inputs = {coins[0].outpoint, coins[1].outpoint, coins[2].outpoint};

    // Try to create a tx that spends more than what preset inputs + wallet selected inputs are covering for.
    // The wallet can cover up to 200 BTC, and the tx target is 299 BTC.
    std::vector<CRecipient> recipients = {{GetScriptForDestination(*Assert(wallet->GetNewDestination(OutputType::BECH32, "dummy"))),
                                           /*nAmount=*/299 * COIN, /*fSubtractFeeFromAmount=*/true}};
    CCoinControl coin_control;
    coin_control.m_allow_other_inputs = true;
    for (const auto& outpoint : preset_inputs) {
        coin_control.Select(outpoint);
    }

    // Attempt to send 299 BTC from a wallet that only has 200 BTC. The wallet should exclude
    // the preset inputs from the pool of available coins, realize that there is not enough
    // money to fund the 299 BTC payment, and fail with "Insufficient funds".
    //
    // Even with SFFO, the wallet can only afford to send 200 BTC.
    // If the wallet does not properly exclude preset inputs from the pool of available coins
    // prior to coin selection, it may create a transaction that does not fund the full payment
    // amount or, through SFFO, incorrectly reduce the recipient's amount by the difference
    // between the original target and the wrongly counted inputs (in this case 99 BTC)
    // so that the recipient's amount is no longer equal to the user's selected target of 299 BTC.

    // First case, use 'subtract_fee_from_outputs=true'
    util::Result<CreatedTransactionResult> res_tx = CreateTransaction(*wallet, recipients, /*change_pos*/-1, coin_control);
    BOOST_CHECK(!res_tx.has_value());

    // Second case, don't use 'subtract_fee_from_outputs'.
    recipients[0].fSubtractFeeFromAmount = false;
    res_tx = CreateTransaction(*wallet, recipients, /*change_pos*/-1, coin_control);
    BOOST_CHECK(!res_tx.has_value());
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
