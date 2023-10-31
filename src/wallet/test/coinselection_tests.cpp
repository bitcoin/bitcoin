// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <node/context.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <random.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/coinselection.h>
#include <wallet/fees.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <random>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(coinselection_tests, WalletTestingSetup)

static int nextLockTime = 0;
static FastRandomContext default_rand;

static CoinSelectionParams init_default_params()
{
    CoinSelectionParams dcsp{
        /*rng_fast*/ default_rand,
        /*change_output_size=*/ 31,
        /*change_spend_size=*/ 68,
        /*min_change_target=*/ 50'000,
        /*effective_feerate=*/ CFeeRate(5000),
        /*long_term_feerate=*/ CFeeRate(10'000),
        /*discard_feerate=*/ CFeeRate(3000),
        /*tx_noinputs_size=*/ 11 + 31, //static header size + output size
        /*avoid_partial=*/ false,
    };
    dcsp.m_change_fee = /*155 sats=*/ dcsp.m_effective_feerate.GetFee(dcsp.change_output_size);
    dcsp.m_cost_of_change = /*204 + 155 sats=*/ dcsp.m_discard_feerate.GetFee(dcsp.change_spend_size) + dcsp.m_change_fee;
    dcsp.min_viable_change = /*204 sats=*/ dcsp.m_discard_feerate.GetFee(dcsp.change_spend_size);
    dcsp.m_subtract_fee_outputs = false;
    return dcsp;
}

static const CoinSelectionParams default_cs_params = init_default_params();

/** Check if SelectionResult a is equivalent to SelectionResult b.
 * Equivalent means same input values, but maybe different inputs (i.e. same value, different prevout) */
static bool EquivalentResult(const SelectionResult& a, const SelectionResult& b)
{
    std::vector<CAmount> a_amts;
    std::vector<CAmount> b_amts;
    for (const auto& coin : a.GetInputSet()) {
        a_amts.push_back(coin->txout.nValue);
    }
    for (const auto& coin : b.GetInputSet()) {
        b_amts.push_back(coin->txout.nValue);
    }
    std::sort(a_amts.begin(), a_amts.end());
    std::sort(b_amts.begin(), b_amts.end());

    std::pair<std::vector<CAmount>::iterator, std::vector<CAmount>::iterator> ret = std::mismatch(a_amts.begin(), a_amts.end(), b_amts.begin());
    return ret.first == a_amts.end() && ret.second == b_amts.end();
}

inline std::vector<OutputGroup>& GroupCoins(const std::vector<COutput>& available_coins, const CoinSelectionParams& cs_params = default_cs_params, bool subtract_fee_outputs = false)
{
    static std::vector<OutputGroup> static_groups;
    static_groups.clear();
    for (auto& coin : available_coins) {
        static_groups.emplace_back(cs_params);
        OutputGroup& group = static_groups.back();
        group.Insert(std::make_shared<COutput>(coin), /*ancestors=*/ 0, /*descendants=*/ 0);
        group.m_subtract_fee_outputs = subtract_fee_outputs;
    }
    return static_groups;
}

static COutput MakeCoin(const CAmount& amount, bool is_eff_value = true, int nInput = 0, CFeeRate feerate = default_cs_params.m_effective_feerate, int custom_spending_vsize = 68) {
    CMutableTransaction tx;
    tx.vout.resize(nInput + 1);
    CAmount fees = feerate.GetFee(custom_spending_vsize);
    tx.vout[nInput].nValue = amount + int(is_eff_value) * fees;
    tx.nLockTime = nextLockTime++;        // so all transactions get different hashes
    return COutput{COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, /*input_bytes=*/ custom_spending_vsize, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, /*fees=*/ fees};
}

static void AddCoins(std::vector<COutput>& utxo_pool, std::vector<CAmount> coins, CFeeRate feerate = default_cs_params.m_effective_feerate) {
    for (int c : coins) {
        utxo_pool.push_back(MakeCoin(c, true, 0, feerate));
    }
}

static void AddDuplicateCoins(std::vector<COutput>& utxo_pool, int count, int amount) {
    for (int i = 0 ; i < count; ++i) {
        utxo_pool.push_back(MakeCoin(amount));
    }
}

std::optional<SelectionResult> SelectCoinsKnapsack(std::vector<COutput>& utxo_pool, const CAmount& selection_target, FastRandomContext& rng = default_cs_params.rng_fast, CAmount change_target = 25'000, const int& max_weight = MAX_STANDARD_TX_WEIGHT)
{
    auto res{KnapsackSolver(GroupCoins(utxo_pool), selection_target, change_target, rng, max_weight)};
    return res ? std::optional<SelectionResult>(*res) : std::nullopt;
}

/* Test Knapsack gets a specific input set composition */
static void TestKnapsackMatch(std::string test_title, std::vector<COutput>& utxo_pool, const CAmount& selection_target, const std::vector<CAmount>& expected_input_amounts)
{
    SelectionResult expected_result(CAmount(0), SelectionAlgorithm::KNAPSACK);
    CAmount expected_amount = 0;
    for (int input_amount : expected_input_amounts) {
        OutputGroup group;
        COutput coin = MakeCoin(input_amount);
        expected_amount += coin.txout.nValue;
        group.Insert(std::make_shared<COutput>(coin), /*ancestors=*/ 0, /*descendants=*/ 0);
        expected_result.AddInput(group);
    }

    const auto result = SelectCoinsKnapsack(utxo_pool, selection_target);
    BOOST_CHECK_MESSAGE(result, "Knapsack-Result: " + test_title);
    BOOST_CHECK(EquivalentResult(expected_result, *result));
    BOOST_CHECK_EQUAL(result->GetSelectedValue(), expected_amount);
    expected_result.Clear();
}

static void TestKnapsackFail(std::string test_title, std::vector<COutput>& utxo_pool, const CAmount& selection_target)
{
    BOOST_CHECK_MESSAGE(!SelectCoinsKnapsack(utxo_pool, selection_target), "Knapsack-Fail: " + test_title);
}

BOOST_AUTO_TEST_CASE(knapsack_predictable_test)
{
    std::vector<COutput> utxo_pool;

    // Fail for empty UTXO pool
    TestKnapsackFail("Empty UTXO pool", utxo_pool, /*selection_target=*/ 1 * CENT);

    AddCoins(utxo_pool, {1 * CENT, 3 * CENT, 5 * CENT, 7 * CENT, 11 * CENT});

    TestKnapsackMatch("Select matching single UTXO", utxo_pool, /*selection_target=*/ 5 * CENT, /*expected_input_amounts=*/ {5 * CENT});
    TestKnapsackMatch("Select matching two UTXOs", utxo_pool, /*selection_target=*/ 6 * CENT, /*expected_input_amounts=*/ {1 * CENT, 5 * CENT});
    TestKnapsackMatch("Select lowest larger", utxo_pool, /*selection_target=*/ 2 * CENT, /*expected_input_amounts=*/ {3 * CENT});
    TestKnapsackMatch("Select sum of lower UTXOs", utxo_pool, /*selection_target=*/ 4 * CENT, /*expected_input_amounts=*/ {1 * CENT, 3 * CENT});
    TestKnapsackMatch("Select everything", utxo_pool, /*selection_target=*/ 27 * CENT, /*expected_input_amounts=*/ {1 * CENT, 3 * CENT, 5 * CENT, 7 * CENT, 11 * CENT});
    TestKnapsackFail("Target exceeds available coins", utxo_pool, /*selection_target=*/ 27.01 * CENT);
    TestKnapsackMatch("Select closest combination", utxo_pool, /*selection_target=*/ 17.5 * CENT, /*expected_input_amounts=*/ {7 * CENT, 11 * CENT});
    // 7 is closer than 3+5
    TestKnapsackMatch("Closer lowest larger preferred over closest combination", utxo_pool, /*selection_target=*/ 6.5 * CENT, /*expected_input_amounts=*/ {7 * CENT});
    // 1+3 is closer than 5
    TestKnapsackMatch("Closer combination is preferred over lowest larger", utxo_pool, /*selection_target=*/ 3.5 * CENT, /*expected_input_amounts=*/ {1 * CENT, 3 * CENT});
    // 1+3+7 vs 11
    TestKnapsackMatch("Lowest larger is preferred in case of tie", utxo_pool, /*selection_target=*/ 10.5 * CENT, /*expected_input_amounts=*/ {11 * CENT});
    // 7+3 is enough for target and min_change
    TestKnapsackMatch("Exactly min_change", utxo_pool, /*selection_target=*/ 9.975 * CENT, /*expected_input_amounts=*/ {3 * CENT, 7 * CENT});
    // 7+3 is enough, but not enough for min_change (9.976+0.025=10.00111000340)
    TestKnapsackMatch("Select more to get min_change", utxo_pool, /*selection_target=*/ 9.976 * CENT, /*expected_input_amounts=*/ {11 * CENT});
}

static void TestBnBSuccess(std::string test_title, std::vector<COutput>& utxo_pool, const CAmount& selection_target, const std::vector<CAmount>& expected_input_amounts, const CFeeRate& feerate = default_cs_params.m_effective_feerate )
{
    SelectionResult expected_result(CAmount(0), SelectionAlgorithm::BNB);
    CAmount expected_amount = 0;
    for (int input_amount : expected_input_amounts) {
        OutputGroup group;
        COutput coin = MakeCoin(input_amount, true, 0, feerate);
        expected_amount += coin.txout.nValue;
        group.Insert(std::make_shared<COutput>(coin), /*ancestors=*/ 0, /*descendants=*/ 0);
        expected_result.AddInput(group);
    }

    const auto result = SelectCoinsBnB(GroupCoins(utxo_pool), selection_target, /*cost_of_change=*/ default_cs_params.m_cost_of_change, /*max_weight=*/MAX_STANDARD_TX_WEIGHT);
    BOOST_CHECK_MESSAGE(result, "BnB-Success: " + test_title);
    BOOST_CHECK(EquivalentResult(expected_result, *result));
    BOOST_CHECK_EQUAL(result->GetSelectedValue(), expected_amount);
    expected_result.Clear();
}

static void TestBnBFail(std::string test_title, std::vector<COutput>& utxo_pool, const CAmount& selection_target, const std::string& expected_error = "")
{
    const auto& no_res = SelectCoinsBnB(GroupCoins(utxo_pool), selection_target, /*cost_of_change=*/ default_cs_params.m_cost_of_change, /*max_weight=*/MAX_STANDARD_TX_WEIGHT);
    BOOST_CHECK_MESSAGE(!no_res, "BnB-Fail: " + test_title);
    if (expected_error != "") {
        BOOST_CHECK_MESSAGE(util::ErrorString(no_res).original.find(expected_error) != std::string::npos, "Found expected error message: " + expected_error);
    }
}

BOOST_AUTO_TEST_CASE(bnb_test)
{
    std::vector<COutput> utxo_pool;

    // Fail for empty UTXO pool
    TestBnBFail("Empty UTXO pool", utxo_pool, /*selection_target=*/ 1 * CENT);

    AddCoins(utxo_pool, {1 * CENT, 3 * CENT, 5 * CENT});

    // Simple success cases
    TestBnBSuccess("Select smallest UTXO", utxo_pool, /*selection_target=*/ 1 * CENT, /*expected_input_amounts=*/ {1 * CENT});
    TestBnBSuccess("Select middle UTXO", utxo_pool, /*selection_target=*/ 3 * CENT, /*expected_input_amounts=*/ {3 * CENT});
    TestBnBSuccess("Select biggest UTXO", utxo_pool, /*selection_target=*/ 5 * CENT, /*expected_input_amounts=*/ {5 * CENT});
    TestBnBSuccess("Select two UTXOs", utxo_pool, /*selection_target=*/ 4 * CENT, /*expected_input_amounts=*/ {1 * CENT, 3 * CENT});
    TestBnBSuccess("Select all UTXOs", utxo_pool, /*selection_target=*/ 9 * CENT, /*expected_input_amounts=*/ {1 * CENT, 3 * CENT, 5 * CENT});

    // BnB finds changeless solution while overshooting by up to cost_of_change
    TestBnBSuccess("Select upper bound", utxo_pool, /*selection_target=*/ 9 * CENT - default_cs_params.m_cost_of_change, /*expected_input_amounts=*/ {1 * CENT, 3 * CENT, 5 * CENT});
    // BnB fails to find changeless solution when overshooting by cost_of_change + 1 sat
    TestBnBFail("Overshoot upper bound", utxo_pool, /*selection_target=*/ 9 * CENT - default_cs_params.m_cost_of_change - 1);

    // Simple cases without BnB solution
    TestBnBFail("Smallest combination too big", utxo_pool, /*selection_target=*/ 0.5 * CENT);
    TestBnBFail("No UTXO combination in target window", utxo_pool, /*selection_target=*/ 7 * CENT);
    TestBnBFail("Select more than available", utxo_pool, /*selection_target=*/ 10 * CENT);

    // Test skipping of equivalent input sets
    std::vector<COutput> clone_pool;
    AddCoins(clone_pool, {2 * CENT, 7 * CENT, 7 * CENT});
    AddDuplicateCoins(clone_pool, 50'000, 5 * CENT);
    TestBnBSuccess("Skip equivalent input sets", clone_pool, /*selection_target=*/ 16 * CENT, /*expected_input_amounts=*/ {2 * CENT, 7 * CENT, 7 * CENT});

    // Test BnB attempt limit
    std::vector<COutput> doppelganger_pool;
    std::vector<CAmount> doppelgangers;
    std::vector<CAmount> expected_inputs;
    for (int i = 0; i < 17; ++i) {
        if (i < 8) {
            // 8 smallest UTXOs can be combined to create expected_result
            doppelgangers.push_back(1 * CENT + i);
            expected_inputs.push_back(doppelgangers[i]);
        } else {
            // Any 8 UTXOs including one larger UTXO exceed target window
            doppelgangers.push_back(1 * CENT + default_cs_params.m_cost_of_change + i);
        }
    }
    AddCoins(doppelganger_pool, doppelgangers);
    TestBnBSuccess("Combine smallest 8 of 17 unique UTXOs", doppelganger_pool, /*selection_target=*/ 8 * CENT, /*expected_input_amounts=*/ expected_inputs);

    AddCoins(doppelganger_pool, {1 * CENT + default_cs_params.m_cost_of_change + 17});
    TestBnBFail("Exhaust looking for smallest 8 of 18 unique UTXOs", doppelganger_pool, /*selection_target=*/ 8 * CENT);
}

BOOST_AUTO_TEST_CASE(bnb_max_weight_test)
{
    std::vector<COutput> max_weight_pool;
    AddCoins(max_weight_pool, {1 * CENT, 8 * CENT, 9 * CENT, 10 * CENT});
    // Add a coin that is necessary for all solutions and too heavy
    max_weight_pool.push_back(MakeCoin(/*effective_value=*/ 5 * CENT, true, 0, /*effective_feerate=*/ default_cs_params.m_effective_feerate, MAX_STANDARD_TX_WEIGHT));
    TestBnBFail("Fail on excessive selection weight", max_weight_pool, /*selection_target=*/ 16 * CENT, /*expected_error=*/ "The inputs size exceeds the maximum weight");
    AddCoins(max_weight_pool, {5 * CENT});
    TestBnBSuccess("Avoid heavy input when unnecessary", max_weight_pool, /*selection_target=*/ 16 * CENT, /*expected_input_amounts=*/ {1 * CENT, 5 * CENT, 10 * CENT});
}

BOOST_AUTO_TEST_CASE(bnb_feerate_sensitivity_test)
{
    // Create sets of UTXOs with the same effective amounts at different feerates (but different absolute amounts)
    std::vector<COutput> low_feerate_pool; // 5 sat/vB (lower than long_term_feerate of 10 sat/vB)
    AddCoins(low_feerate_pool, {2 * CENT, 3 * CENT, 5 * CENT, 10 * CENT});
    TestBnBSuccess("Select many inputs at low feerates", low_feerate_pool, /*selection_target=*/ 10 * CENT, /*expected_input_amounts=*/ {2 * CENT, 3 * CENT, 5 * CENT});

    std::vector<COutput> high_feerate_pool; // 25 sat/vB (greater than long_term_feerate of 10 sat/vB)
    AddCoins(high_feerate_pool, {2 * CENT, 3 * CENT, 5 * CENT, 10 * CENT}, CFeeRate{25'000});
    TestBnBSuccess("Select one input at high feerates", high_feerate_pool, /*selection_target=*/ 10 * CENT, /*expected_input_amounts=*/ {10 * CENT}, CFeeRate{25'000});
}

//TODO: SelectCoins Test
    // TODO: Test at `SelectCoins`/spend.cpp level that changeless solutions can be achieved by combining preset inputs with BnB solutions
    // TODO: Test that immature coins are not considered
    // TODO: Test eligibility filters
    // TODO: Test that self-sent coins are filtered differently than foreign-sent coins

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
