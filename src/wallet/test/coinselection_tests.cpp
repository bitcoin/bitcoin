// Copyright (c) 2024 The Bitcoin Core developers
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

/** Default coin selection parameters (dcsp) allow us to only explicitly set
 * parameters when a diverging value is relevant in the context of a test. */
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

/** Make one coin that either has a given effective value (default) or a given amount (`is_eff_value = false`). */
static COutput MakeCoin(const CAmount& amount, bool is_eff_value = true, int nInput = 0, CFeeRate feerate = default_cs_params.m_effective_feerate, int custom_spending_vsize = 68)
{
    CMutableTransaction tx;
    tx.vout.resize(nInput + 1);
    CAmount fees = feerate.GetFee(custom_spending_vsize);
    tx.vout[nInput].nValue = amount + int(is_eff_value) * fees;
    tx.nLockTime = nextLockTime++;        // so all transactions get different hashes
    return COutput{COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 1, /*input_bytes=*/ custom_spending_vsize, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ false, /*fees=*/ fees};
}

/** Make multiple coins with given effective values */
static void AddCoins(std::vector<COutput>& utxo_pool, std::vector<CAmount> coins, CFeeRate feerate = default_cs_params.m_effective_feerate)
{
    for (int c : coins) {
        utxo_pool.push_back(MakeCoin(c, true, 0, feerate));
    }
}

/** Make multiple coins that share the same effective value */
static void AddDuplicateCoins(std::vector<COutput>& utxo_pool, int count, int amount) {
    for (int i = 0 ; i < count; ++i) {
        utxo_pool.push_back(MakeCoin(amount));
    }
}

/** Group available coins into OutputGroups */
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

static void TestBnBSuccess(std::string test_title, std::vector<COutput>& utxo_pool, const CAmount& selection_target, const std::vector<CAmount>& expected_input_amounts, const CFeeRate& feerate = default_cs_params.m_effective_feerate)
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

static void TestBnBFail(std::string test_title, std::vector<COutput>& utxo_pool, const CAmount& selection_target)
{
    BOOST_CHECK_MESSAGE(!SelectCoinsBnB(GroupCoins(utxo_pool), selection_target, /*cost_of_change=*/ default_cs_params.m_cost_of_change, /*max_weight=*/MAX_STANDARD_TX_WEIGHT), "BnB-Fail: " + test_title);
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
            // Any 8 UTXOs including at least one UTXO with the added cost_of_change will exceed target window
            doppelgangers.push_back(1 * CENT + default_cs_params.m_cost_of_change + i);
        }
    }
    AddCoins(doppelganger_pool, doppelgangers);
    // Among up to 17 unique UTXOs of similar effective value we will find a solution composed of the eight smallest
    TestBnBSuccess("Combine smallest 8 of 17 unique UTXOs", doppelganger_pool, /*selection_target=*/ 8 * CENT, /*expected_input_amounts=*/ expected_inputs);

    // Starting with 18 unique UTXOs of similar effective value we will not find the solution
    AddCoins(doppelganger_pool, {1 * CENT + default_cs_params.m_cost_of_change + 17});
    TestBnBFail("Exhaust looking for smallest 8 of 18 unique UTXOs", doppelganger_pool, /*selection_target=*/ 8 * CENT);
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

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
