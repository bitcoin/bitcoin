// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <node/context.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/coinselection.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <random>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(coinselector_tests, WalletTestingSetup)

typedef std::set<std::shared_ptr<COutput>> CoinSet;

static const CoinEligibilityFilter filter_standard(1, 6, 0);
static int nextLockTime = 0;

static void add_coin(const CAmount& nValue, int nInput, SelectionResult& result, CAmount fee, CAmount long_term_fee)
{
    CMutableTransaction tx;
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    tx.nLockTime = nextLockTime++;        // so all transactions get different hashes
    std::shared_ptr<COutput> coin = std::make_shared<COutput>(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, /*input_bytes=*/ 148, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, fee);
    OutputGroup group;
    group.Insert(coin, /*ancestors=*/ 0, /*descendants=*/ 0);
    coin->long_term_fee = long_term_fee; // group.Insert() will modify long_term_fee, so we need to set it afterwards
    result.AddInput(group);
}

static void add_coin(CoinsResult& available_coins, CWallet& wallet, const CAmount& nValue, CFeeRate feerate = CFeeRate(0), int nAge = 6*24, bool fIsFromMe = false, int nInput =0, bool spendable = false, int custom_size = 0)
{
    CMutableTransaction tx;
    tx.nLockTime = nextLockTime++;        // so all transactions get different hashes
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    if (spendable) {
        tx.vout[nInput].scriptPubKey = GetScriptForDestination(*Assert(wallet.GetNewDestination(OutputType::BECH32, "")));
    }
    uint256 txid = tx.GetHash();

    LOCK(wallet.cs_wallet);
    auto ret = wallet.mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(txid), std::forward_as_tuple(MakeTransactionRef(std::move(tx)), TxStateInactive{}));
    assert(ret.second);
    CWalletTx& wtx = (*ret.first).second;
    const auto& txout = wtx.tx->vout.at(nInput);
    available_coins.Add(OutputType::BECH32, {COutPoint(wtx.GetHash(), nInput), txout, nAge, custom_size == 0 ? CalculateMaximumSignedInputSize(txout, &wallet, /*coin_control=*/nullptr) : custom_size, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, wtx.GetTxTime(), fIsFromMe, feerate});
}

// Helpers

static std::unique_ptr<CWallet> NewWallet(const node::NodeContext& m_node, const std::string& wallet_name = "")
{
    std::unique_ptr<CWallet> wallet = std::make_unique<CWallet>(m_node.chain.get(), wallet_name, CreateMockableWalletDatabase());
    BOOST_CHECK(wallet->LoadWallet() == DBErrors::LOAD_OK);
    LOCK(wallet->cs_wallet);
    wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    wallet->SetupDescriptorScriptPubKeyMans();
    return wallet;
}

// Tests that with the ideal conditions, the coin selector will always be able to find a solution that can pay the target value
BOOST_AUTO_TEST_CASE(SelectCoins_test)
{
    std::unique_ptr<CWallet> wallet = NewWallet(m_node);
    LOCK(wallet->cs_wallet); // Every 'SelectCoins' call requires it

    // Random generator stuff
    std::default_random_engine generator;
    std::exponential_distribution<double> distribution (100);
    FastRandomContext rand;

    // Run this test 100 times
    for (int i = 0; i < 100; ++i)
    {
        CoinsResult available_coins;
        CAmount balance{0};

        // Make a wallet with 1000 exponentially distributed random inputs
        for (int j = 0; j < 1000; ++j)
        {
            CAmount val = distribution(generator)*10000000;
            add_coin(available_coins, *wallet, val);
            balance += val;
        }

        // Generate a random fee rate in the range of 100 - 400
        CFeeRate rate(rand.randrange(300) + 100);

        // Generate a random target value between 1000 and wallet balance
        CAmount target = rand.randrange(balance - 1000) + 1000;

        // Perform selection
        CoinSelectionParams cs_params{
            rand,
            /*change_output_size=*/ 34,
            /*change_spend_size=*/ 148,
            /*min_change_target=*/ CENT,
            /*effective_feerate=*/ CFeeRate(0),
            /*long_term_feerate=*/ CFeeRate(0),
            /*discard_feerate=*/ CFeeRate(0),
            /*tx_noinputs_size=*/ 0,
            /*avoid_partial=*/ false,
        };
        cs_params.m_cost_of_change = 1;
        cs_params.min_viable_change = 1;
        CCoinControl cc;
        const auto result = SelectCoins(*wallet, available_coins, /*pre_set_inputs=*/{}, target, cc, cs_params);
        BOOST_CHECK(result);
        BOOST_CHECK_GE(result->GetSelectedValue(), target);
    }
}

BOOST_AUTO_TEST_CASE(waste_test)
{
    const CAmount fee{100};
    const CAmount change_cost{125};
    const CAmount fee_diff{40};
    const CAmount in_amt{3 * COIN};
    const CAmount target{2 * COIN};
    const CAmount excess{in_amt - fee * 2 - target};

    // The following tests that the waste is calculated correctly in various scenarios.
    // ComputeAndSetWaste will first determine the size of the change output. We don't really
    // care about the change and just want to use the variant that always includes the change_cost,
    // so min_viable_change and change_fee are set to 0 to ensure that.
    {
        // Waste with change is the change cost and difference between fee and long term fee
        SelectionResult selection1{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection1, fee, fee - fee_diff);
        add_coin(2 * COIN, 2, selection1, fee, fee - fee_diff);
        selection1.ComputeAndSetWaste(/*min_viable_change=*/0, change_cost, /*change_fee=*/0);
        BOOST_CHECK_EQUAL(fee_diff * 2 + change_cost, selection1.GetWaste());

        // Waste will be greater when fee is greater, but long term fee is the same
        SelectionResult selection2{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection2, fee * 2, fee - fee_diff);
        add_coin(2 * COIN, 2, selection2, fee * 2, fee - fee_diff);
        selection2.ComputeAndSetWaste(/*min_viable_change=*/0, change_cost, /*change_fee=*/0);
        BOOST_CHECK_GT(selection2.GetWaste(), selection1.GetWaste());

        // Waste with change is the change cost and difference between fee and long term fee
        // With long term fee greater than fee, waste should be less than when long term fee is less than fee
        SelectionResult selection3{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection3, fee, fee + fee_diff);
        add_coin(2 * COIN, 2, selection3, fee, fee + fee_diff);
        selection3.ComputeAndSetWaste(/*min_viable_change=*/0, change_cost, /*change_fee=*/0);
        BOOST_CHECK_EQUAL(fee_diff * -2 + change_cost, selection3.GetWaste());
        BOOST_CHECK_LT(selection3.GetWaste(), selection1.GetWaste());
    }

    {
        // Waste without change is the excess and difference between fee and long term fee
        SelectionResult selection_nochange1{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection_nochange1, fee, fee - fee_diff);
        add_coin(2 * COIN, 2, selection_nochange1, fee, fee - fee_diff);
        selection_nochange1.ComputeAndSetWaste(/*min_viable_change=*/0, /*change_cost=*/0, /*change_fee=*/0);
        BOOST_CHECK_EQUAL(fee_diff * 2 + excess, selection_nochange1.GetWaste());

        // Waste without change is the excess and difference between fee and long term fee
        // With long term fee greater than fee, waste should be less than when long term fee is less than fee
        SelectionResult selection_nochange2{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection_nochange2, fee, fee + fee_diff);
        add_coin(2 * COIN, 2, selection_nochange2, fee, fee + fee_diff);
        selection_nochange2.ComputeAndSetWaste(/*min_viable_change=*/0, /*change_cost=*/0, /*change_fee=*/0);
        BOOST_CHECK_EQUAL(fee_diff * -2 + excess, selection_nochange2.GetWaste());
        BOOST_CHECK_LT(selection_nochange2.GetWaste(), selection_nochange1.GetWaste());
    }

    {
        // Waste with change and fee == long term fee is just cost of change
        SelectionResult selection{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection, fee, fee);
        add_coin(2 * COIN, 2, selection, fee, fee);
        selection.ComputeAndSetWaste(/*min_viable_change=*/0, change_cost, /*change_fee=*/0);
        BOOST_CHECK_EQUAL(change_cost, selection.GetWaste());
    }

    {
        // Waste without change and fee == long term fee is just the excess
        SelectionResult selection{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection, fee, fee);
        add_coin(2 * COIN, 2, selection, fee, fee);
        selection.ComputeAndSetWaste(/*min_viable_change=*/0, /*change_cost=*/0, /*change_fee=*/0);
        BOOST_CHECK_EQUAL(excess, selection.GetWaste());
    }

    {
        // No Waste when fee == long_term_fee, no change, and no excess
        const CAmount exact_target{in_amt - fee * 2};
        SelectionResult selection{exact_target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection, fee, fee);
        add_coin(2 * COIN, 2, selection, fee, fee);
        selection.ComputeAndSetWaste(/*min_viable_change=*/0, /*change_cost=*/0, /*change_fee=*/0);
        BOOST_CHECK_EQUAL(0, selection.GetWaste());
    }

    {
        // No Waste when (fee - long_term_fee) == (-cost_of_change), and no excess
        SelectionResult selection{target, SelectionAlgorithm::MANUAL};
        const CAmount new_change_cost{fee_diff * 2};
        add_coin(1 * COIN, 1, selection, fee, fee + fee_diff);
        add_coin(2 * COIN, 2, selection, fee, fee + fee_diff);
        selection.ComputeAndSetWaste(/*min_viable_change=*/0, new_change_cost, /*change_fee=*/0);
        BOOST_CHECK_EQUAL(0, selection.GetWaste());
    }

    {
        // No Waste when (fee - long_term_fee) == (-excess), no change cost
        const CAmount new_target{in_amt - fee * 2 - fee_diff * 2};
        SelectionResult selection{new_target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection, fee, fee + fee_diff);
        add_coin(2 * COIN, 2, selection, fee, fee + fee_diff);
        selection.ComputeAndSetWaste(/*min_viable_change=*/0, /*change_cost=*/0, /*change_fee=*/0);
        BOOST_CHECK_EQUAL(0, selection.GetWaste());
    }

    {
        // Negative waste when the long term fee is greater than the current fee and the selected value == target
        const CAmount exact_target{3 * COIN - 2 * fee};
        SelectionResult selection{exact_target, SelectionAlgorithm::MANUAL};
        const CAmount target_waste1{-2 * fee_diff}; // = (2 * fee) - (2 * (fee + fee_diff))
        add_coin(1 * COIN, 1, selection, fee, fee + fee_diff);
        add_coin(2 * COIN, 2, selection, fee, fee + fee_diff);
        selection.ComputeAndSetWaste(/*min_viable_change=*/0, /*change_cost=*/0, /*change_fee=*/0);
        BOOST_CHECK_EQUAL(target_waste1, selection.GetWaste());
    }

    {
        // Negative waste when the long term fee is greater than the current fee and change_cost < - (inputs * (fee - long_term_fee))
        SelectionResult selection{target, SelectionAlgorithm::MANUAL};
        const CAmount large_fee_diff{90};
        const CAmount target_waste2{-2 * large_fee_diff + change_cost}; // = (2 * fee) - (2 * (fee + large_fee_diff)) + change_cost
        add_coin(1 * COIN, 1, selection, fee, fee + large_fee_diff);
        add_coin(2 * COIN, 2, selection, fee, fee + large_fee_diff);
        selection.ComputeAndSetWaste(/*min_viable_change=*/0, change_cost, /*change_fee=*/0);
        BOOST_CHECK_EQUAL(target_waste2, selection.GetWaste());
    }
}


BOOST_AUTO_TEST_CASE(bump_fee_test)
{
    const CAmount fee{100};
    const CAmount min_viable_change{200};
    const CAmount change_cost{125};
    const CAmount change_fee{35};
    const CAmount fee_diff{40};
    const CAmount target{2 * COIN};

    {
        SelectionResult selection{target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection, /*fee=*/fee, /*long_term_fee=*/fee + fee_diff);
        add_coin(2 * COIN, 2, selection, fee, fee + fee_diff);
        const std::vector<std::shared_ptr<COutput>> inputs = selection.GetShuffledInputVector();

        for (size_t i = 0; i < inputs.size(); ++i) {
            inputs[i]->ApplyBumpFee(20*(i+1));
        }

        selection.ComputeAndSetWaste(min_viable_change, change_cost, change_fee);
        CAmount expected_waste = fee_diff * -2 + change_cost + /*bump_fees=*/60;
        BOOST_CHECK_EQUAL(expected_waste, selection.GetWaste());

        selection.SetBumpFeeDiscount(30);
        selection.ComputeAndSetWaste(min_viable_change, change_cost, change_fee);
        expected_waste = fee_diff * -2 + change_cost + /*bump_fees=*/60 - /*group_discount=*/30;
        BOOST_CHECK_EQUAL(expected_waste, selection.GetWaste());
    }

    {
        // Test with changeless transaction
        //
        // Bump fees and excess both contribute fully to the waste score,
        // therefore, a bump fee group discount will not change the waste
        // score as long as we do not create change in both instances.
        CAmount changeless_target = 3 * COIN - 2 * fee - 100;
        SelectionResult selection{changeless_target, SelectionAlgorithm::MANUAL};
        add_coin(1 * COIN, 1, selection, /*fee=*/fee, /*long_term_fee=*/fee + fee_diff);
        add_coin(2 * COIN, 2, selection, fee, fee + fee_diff);
        const std::vector<std::shared_ptr<COutput>> inputs = selection.GetShuffledInputVector();

        for (size_t i = 0; i < inputs.size(); ++i) {
            inputs[i]->ApplyBumpFee(20*(i+1));
        }

        selection.ComputeAndSetWaste(min_viable_change, change_cost, change_fee);
        CAmount expected_waste = fee_diff * -2 + /*bump_fees=*/60 + /*excess = 100 - bump_fees*/40;
        BOOST_CHECK_EQUAL(expected_waste, selection.GetWaste());

        selection.SetBumpFeeDiscount(30);
        selection.ComputeAndSetWaste(min_viable_change, change_cost, change_fee);
        expected_waste = fee_diff * -2 + /*bump_fees=*/60 - /*group_discount=*/30 + /*excess = 100 - bump_fees + group_discount*/70;
        BOOST_CHECK_EQUAL(expected_waste, selection.GetWaste());
    }
}

BOOST_AUTO_TEST_CASE(effective_value_test)
{
    const int input_bytes = 148;
    const CFeeRate feerate(1000);
    const CAmount nValue = 10000;
    const int nInput = 0;

    CMutableTransaction tx;
    tx.vout.resize(1);
    tx.vout[nInput].nValue = nValue;

    // standard case, pass feerate in constructor
    COutput output1(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, input_bytes, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, feerate);
    const CAmount expected_ev1 = 9852; // 10000 - 148
    BOOST_CHECK_EQUAL(output1.GetEffectiveValue(), expected_ev1);

    // input bytes unknown (input_bytes = -1), pass feerate in constructor
    COutput output2(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, /*input_bytes=*/ -1, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, feerate);
    BOOST_CHECK_EQUAL(output2.GetEffectiveValue(), nValue); // The effective value should be equal to the absolute value if input_bytes is -1

    // negative effective value, pass feerate in constructor
    COutput output3(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, input_bytes, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, CFeeRate(100000));
    const CAmount expected_ev3 = -4800; // 10000 - 14800
    BOOST_CHECK_EQUAL(output3.GetEffectiveValue(), expected_ev3);

    // standard case, pass fees in constructor
    const CAmount fees = 148;
    COutput output4(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, input_bytes, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, fees);
    BOOST_CHECK_EQUAL(output4.GetEffectiveValue(), expected_ev1);

    // input bytes unknown (input_bytes = -1), pass fees in constructor
    COutput output5(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, /*input_bytes=*/ -1, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, /*fees=*/ 0);
    BOOST_CHECK_EQUAL(output5.GetEffectiveValue(), nValue); // The effective value should be equal to the absolute value if input_bytes is -1
}

static util::Result<SelectionResult> select_coins(const CAmount& target, const CoinSelectionParams& cs_params, const CCoinControl& cc, std::function<CoinsResult(CWallet&)> coin_setup, const node::NodeContext& m_node)
{
    std::unique_ptr<CWallet> wallet = NewWallet(m_node);
    auto available_coins = coin_setup(*wallet);

    LOCK(wallet->cs_wallet);
    auto result = SelectCoins(*wallet, available_coins, /*pre_set_inputs=*/ {}, target, cc, cs_params);
    if (result) {
        const auto signedTxSize = 10 + 34 + 68 * result->GetInputSet().size(); // static header size + output size + inputs size (P2WPKH)
        BOOST_CHECK_LE(signedTxSize * WITNESS_SCALE_FACTOR, MAX_STANDARD_TX_WEIGHT);

        BOOST_CHECK_GE(result->GetSelectedValue(), target);
    }
    return result;
}

static bool has_coin(const CoinSet& set, CAmount amount)
{
    return std::any_of(set.begin(), set.end(), [&](const auto& coin) { return coin->GetEffectiveValue() == amount; });
}

BOOST_AUTO_TEST_CASE(check_max_weight)
{
    const CAmount target = 49.5L * COIN;
    CCoinControl cc;

    FastRandomContext rand;
    CoinSelectionParams cs_params{
        rand,
        /*change_output_size=*/34,
        /*change_spend_size=*/68,
        /*min_change_target=*/CENT,
        /*effective_feerate=*/CFeeRate(0),
        /*long_term_feerate=*/CFeeRate(0),
        /*discard_feerate=*/CFeeRate(0),
        /*tx_noinputs_size=*/10 + 34, // static header size + output size
        /*avoid_partial=*/false,
    };

    {
        // Scenario 1:
        // The actor starts with 1x 50.0 BTC and 1515x 0.033 BTC (~100.0 BTC total) unspent outputs
        // Then tries to spend 49.5 BTC
        // The 50.0 BTC output should be selected, because the transaction would otherwise be too large

        // Perform selection

        const auto result = select_coins(
            target, cs_params, cc, [&](CWallet& wallet) {
                CoinsResult available_coins;
                for (int j = 0; j < 1515; ++j) {
                    add_coin(available_coins, wallet, CAmount(0.033 * COIN), CFeeRate(0), 144, false, 0, true);
                }

                add_coin(available_coins, wallet, CAmount(50 * COIN), CFeeRate(0), 144, false, 0, true);
                return available_coins;
            },
            m_node);

        BOOST_CHECK(result);
        // Verify that only the 50 BTC UTXO was selected
        const auto& selection_res = result->GetInputSet();
        BOOST_CHECK(selection_res.size() == 1);
        BOOST_CHECK((*selection_res.begin())->GetEffectiveValue() == 50 * COIN);
    }

    {
        // Scenario 2:

        // The actor starts with 400x 0.0625 BTC and 2000x 0.025 BTC (75.0 BTC total) unspent outputs
        // Then tries to spend 49.5 BTC
        // A combination of coins should be selected, such that the created transaction is not too large

        // Perform selection
        const auto result = select_coins(
            target, cs_params, cc, [&](CWallet& wallet) {
                CoinsResult available_coins;
                for (int j = 0; j < 400; ++j) {
                    add_coin(available_coins, wallet, CAmount(0.0625 * COIN), CFeeRate(0), 144, false, 0, true);
                }
                for (int j = 0; j < 2000; ++j) {
                    add_coin(available_coins, wallet, CAmount(0.025 * COIN), CFeeRate(0), 144, false, 0, true);
                }
                return available_coins;
            },
            m_node);

        BOOST_CHECK(has_coin(result->GetInputSet(), CAmount(0.0625 * COIN)));
        BOOST_CHECK(has_coin(result->GetInputSet(), CAmount(0.025 * COIN)));
    }

    {
        // Scenario 3:

        // The actor starts with 1515x 0.033 BTC (49.995 BTC total) unspent outputs
        // No results should be returned, because the transaction would be too large

        // Perform selection
        const auto result = select_coins(
            target, cs_params, cc, [&](CWallet& wallet) {
                CoinsResult available_coins;
                for (int j = 0; j < 1515; ++j) {
                    add_coin(available_coins, wallet, CAmount(0.033 * COIN), CFeeRate(0), 144, false, 0, true);
                }
                return available_coins;
            },
            m_node);

        // No results
        // 1515 inputs * 68 bytes = 103,020 bytes
        // 103,020 bytes * 4 = 412,080 weight, which is above the MAX_STANDARD_TX_WEIGHT of 400,000
        BOOST_CHECK(!result);
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
