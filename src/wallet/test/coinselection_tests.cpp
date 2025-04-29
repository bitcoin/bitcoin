// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <policy/policy.h>
#include <wallet/coinselection.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(coinselection_tests, TestingSetup)

static int next_lock_time = 0;
static FastRandomContext default_rand;

/** Default coin selection parameters (dcsp) allow us to only explicitly set
 * parameters when a diverging value is relevant in the context of a test.
 * We use P2WPKH input and output weights for the change weights. */
static CoinSelectionParams init_default_params()
{
    CoinSelectionParams dcsp{
        /*rng_fast*/default_rand,
        /*change_output_size=*/31,
        /*change_spend_size=*/68,
        /*min_change_target=*/50'000,
        /*effective_feerate=*/CFeeRate(5000),
        /*long_term_feerate=*/CFeeRate(10'000),
        /*discard_feerate=*/CFeeRate(3000),
        /*tx_noinputs_size=*/11 + 31, //static header size + output size
        /*avoid_partial=*/false,
    };
    dcsp.m_change_fee = /*155 sats=*/dcsp.m_effective_feerate.GetFee(dcsp.change_output_size);
    dcsp.min_viable_change = /*204 sats=*/dcsp.m_discard_feerate.GetFee(dcsp.change_spend_size);
    dcsp.m_cost_of_change = /*204 + 155 sats=*/dcsp.min_viable_change + dcsp.m_change_fee;
    dcsp.m_subtract_fee_outputs = false;
    return dcsp;
}

static const CoinSelectionParams default_cs_params = init_default_params();

/** Make one OutputGroup with a single UTXO that either has a given effective value (default) or a given amount (`is_eff_value = false`). */
static OutputGroup MakeCoin(const CAmount& amount, bool is_eff_value = true, CoinSelectionParams cs_params = default_cs_params, int custom_spending_vsize = 68)
{
    // Always assume that we only have one input
    CMutableTransaction tx;
    tx.vout.resize(1);
    CAmount fees = cs_params.m_effective_feerate.GetFee(custom_spending_vsize);
    tx.vout[0].nValue = amount + int(is_eff_value) * fees;
    tx.nLockTime = next_lock_time++;        // so all transactions get different hashes
    OutputGroup group(cs_params);
    group.Insert(std::make_shared<COutput>(COutPoint(tx.GetHash(), 0), tx.vout.at(0), /*depth=*/1, /*input_bytes=*/custom_spending_vsize, /*spendable=*/true, /*solvable=*/true, /*safe=*/true, /*time=*/0, /*from_me=*/false, /*fees=*/fees), /*ancestors=*/0, /*descendants=*/0);
    return group;
}

/** Make multiple OutputGroups with the given values as their effective value */
static void AddCoins(std::vector<OutputGroup>& utxo_pool, std::vector<CAmount> coins, CoinSelectionParams cs_params = default_cs_params)
{
    for (CAmount c : coins) {
        utxo_pool.push_back(MakeCoin(c, true, cs_params));
    }
}

/** Make multiple coins that share the same effective value */
static void AddDuplicateCoins(std::vector<OutputGroup>& utxo_pool, int count, int amount, CoinSelectionParams cs_params = default_cs_params) {
    for (int i = 0 ; i < count; ++i) {
        utxo_pool.push_back(MakeCoin(amount, true, cs_params));
    }
}

/** Check if SelectionResult a is equivalent to SelectionResult b.
 * Two results are equivalent if they are composed of the same input values, even if they have different inputs (i.e., same value, different prevout) */
static bool HaveEquivalentValues(const SelectionResult& a, const SelectionResult& b)
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

    auto ret = std::mismatch(a_amts.begin(), a_amts.end(), b_amts.begin());
    return ret.first == a_amts.end() && ret.second == b_amts.end();
}

static std::string InputAmountsToString(const SelectionResult& selection)
{
    return "[" + util::Join(selection.GetInputSet(), " ", [](const auto& input){ return util::ToString(input->txout.nValue);}) + "]";
}

static void TestBnBSuccess(std::string test_title, std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target, const std::vector<CAmount>& expected_input_amounts, const CoinSelectionParams& cs_params = default_cs_params, int custom_spending_vsize = 68)
{
    SelectionResult expected_result(CAmount(0), SelectionAlgorithm::BNB);
    CAmount expected_amount = 0;
    for (CAmount input_amount : expected_input_amounts) {
        OutputGroup group = MakeCoin(input_amount, true, cs_params, custom_spending_vsize);
        expected_amount += group.m_value;
        expected_result.AddInput(group);
    }

    const auto result = SelectCoinsBnB(utxo_pool, selection_target, /*cost_of_change=*/default_cs_params.m_cost_of_change, /*max_selection_weight=*/MAX_STANDARD_TX_WEIGHT);
    BOOST_CHECK_MESSAGE(result, "Falsy result in BnB-Success: " + test_title);
    BOOST_CHECK_MESSAGE(HaveEquivalentValues(expected_result, *result), strprintf("Result mismatch in BnB-Success: %s. Expected %s, but got %s", test_title, InputAmountsToString(expected_result), InputAmountsToString(*result)));
    BOOST_CHECK_MESSAGE(result->GetSelectedValue() == expected_amount, strprintf("Selected amount mismatch in BnB-Success: %s. Expected %d, but got %d", test_title, expected_amount, result->GetSelectedValue()));
}

static void TestBnBFail(std::string test_title, std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target)
{
    BOOST_CHECK_MESSAGE(!SelectCoinsBnB(utxo_pool, selection_target, /*cost_of_change=*/default_cs_params.m_cost_of_change, /*max_selection_weight=*/MAX_STANDARD_TX_WEIGHT), "BnB-Fail: " + test_title);
}

BOOST_AUTO_TEST_CASE(bnb_test)
{
    std::vector<int> feerates = {0, 1, 5'000, 10'000, 25'000, 59'764, 500'000, 999'000, 1'500'000};

    for (int feerate : feerates) {
        std::vector<OutputGroup> utxo_pool;

        CoinSelectionParams cs_params = init_default_params();
        cs_params.m_effective_feerate = CFeeRate{feerate};

        // Fail for empty UTXO pool
        TestBnBFail("Empty UTXO pool", utxo_pool, /*selection_target=*/1 * CENT);

        AddCoins(utxo_pool, {1 * CENT, 3 * CENT, 5 * CENT}, cs_params);

        // Simple success cases
        TestBnBSuccess("Select smallest UTXO", utxo_pool, /*selection_target=*/1 * CENT, /*expected_input_amounts=*/{1 * CENT}, cs_params);
        TestBnBSuccess("Select middle UTXO", utxo_pool, /*selection_target=*/3 * CENT, /*expected_input_amounts=*/{3 * CENT}, cs_params);
        TestBnBSuccess("Select biggest UTXO", utxo_pool, /*selection_target=*/5 * CENT, /*expected_input_amounts=*/{5 * CENT}, cs_params);
        TestBnBSuccess("Select two UTXOs", utxo_pool, /*selection_target=*/4 * CENT, /*expected_input_amounts=*/{1 * CENT, 3 * CENT}, cs_params);
        TestBnBSuccess("Select all UTXOs", utxo_pool, /*selection_target=*/9 * CENT, /*expected_input_amounts=*/{1 * CENT, 3 * CENT, 5 * CENT}, cs_params);

        // BnB finds changeless solution while overshooting by up to cost_of_change
        TestBnBSuccess("Select upper bound", utxo_pool, /*selection_target=*/4 * CENT - default_cs_params.m_cost_of_change, /*expected_input_amounts=*/{1 * CENT, 3 * CENT}, cs_params);

        // BnB fails to find changeless solution when overshooting by cost_of_change + 1 sat
        TestBnBFail("Overshoot upper bound", utxo_pool, /*selection_target=*/4 * CENT - default_cs_params.m_cost_of_change - 1);

        // Simple cases without BnB solution
        TestBnBFail("Smallest combination too big", utxo_pool, /*selection_target=*/0.5 * CENT);
        TestBnBFail("No UTXO combination in target window", utxo_pool, /*selection_target=*/7 * CENT);
        TestBnBFail("Select more than available", utxo_pool, /*selection_target=*/10 * CENT);

        // Test skipping of equivalent input sets
        std::vector<OutputGroup> clone_pool;
        AddCoins(clone_pool, {2 * CENT, 7 * CENT, 7 * CENT}, cs_params);
        AddDuplicateCoins(clone_pool, 50'000, 5 * CENT, cs_params);
        TestBnBSuccess("Skip equivalent input sets", clone_pool, /*selection_target=*/16 * CENT, /*expected_input_amounts=*/{2 * CENT, 7 * CENT, 7 * CENT}, cs_params);

        /* Test BnB attempt limit (`TOTAL_TRIES`)
         *
         * Generally, on a diverse UTXO pool BnB will quickly pass over UTXOs bigger than the target and then start
         * combining small counts of UTXOs that in sum remain under the selection_target+cost_of_change. When there are
         * multiple UTXOs that have matching amount and cost, combinations with equivalent input sets are skipped. The
         * UTXO pool for this test is specifically crafted to create as much branching as possible. The selection target
         * is 8 CENT while all UTXOs are slightly bigger than 1 CENT. The smallest eight are 100,000…100,007 sats, while
         * the larger nine are 100,368…100,375 (i.e., 100,008…100,016 sats plus cost_of_change (359 sats)).
         *
         * Because BnB will only select input sets that fall between selection_target and selection_target +
         * cost_of_change, and the search traverses the UTXO pool from large to small amounts, the search will visit
         * every single combination of eight inputs. All except the last combination will overshoot by more than
         * cost_of_change on the eighth input, because the larger nine inputs each exceed 1 CENT by more than
         * cost_of_change. Only the last combination consisting of the eight smallest UTXOs falls into the target
         * window.
         */
        std::vector<OutputGroup> doppelganger_pool;
        std::vector<CAmount> doppelgangers;
        std::vector<CAmount> expected_inputs;
        for (int i = 0; i < 17; ++i) {
            if (i < 8) {
                // The eight smallest UTXOs can be combined to create expected_result
                doppelgangers.push_back(1 * CENT + i);
                expected_inputs.push_back(doppelgangers[i]);
            } else {
                // Any eight UTXOs including at least one UTXO with the added cost_of_change will exceed target window
                doppelgangers.push_back(1 * CENT + default_cs_params.m_cost_of_change + i);
            }
        }
        AddCoins(doppelganger_pool, doppelgangers, cs_params);
        // Among up to 17 unique UTXOs of similar effective value we will find a solution composed of the eight smallest UTXOs
        TestBnBSuccess("Combine smallest 8 of 17 unique UTXOs", doppelganger_pool, /*selection_target=*/8 * CENT, /*expected_input_amounts=*/expected_inputs, cs_params);

        // Starting with 18 unique UTXOs of similar effective value we will not find the solution due to exceeding the attempt limit
        AddCoins(doppelganger_pool, {1 * CENT + default_cs_params.m_cost_of_change + 17}, cs_params);
        TestBnBFail("Exhaust looking for smallest 8 of 18 unique UTXOs", doppelganger_pool, /*selection_target=*/8 * CENT);
    }
}

BOOST_AUTO_TEST_CASE(bnb_feerate_sensitivity_test)
{
    // Create sets of UTXOs with the same effective amounts at different feerates (but different absolute amounts)
    std::vector<OutputGroup> low_feerate_pool; // 5 sat/vB (default, and lower than long_term_feerate of 10 sat/vB)
    AddCoins(low_feerate_pool, {2 * CENT, 3 * CENT, 5 * CENT, 10 * CENT});
    TestBnBSuccess("Select many inputs at low feerates", low_feerate_pool, /*selection_target=*/10 * CENT, /*expected_input_amounts=*/{2 * CENT, 3 * CENT, 5 * CENT});

    CoinSelectionParams high_feerate_params = init_default_params();
    high_feerate_params.m_effective_feerate = CFeeRate{25'000};
    std::vector<OutputGroup> high_feerate_pool; // 25 sat/vB (greater than long_term_feerate of 10 sat/vB)
    AddCoins(high_feerate_pool, {2 * CENT, 3 * CENT, 5 * CENT, 10 * CENT}, high_feerate_params);
    TestBnBSuccess("Select one input at high feerates", high_feerate_pool, /*selection_target=*/10 * CENT, /*expected_input_amounts=*/{10 * CENT}, high_feerate_params);

    // Add heavy inputs {6, 7} to existing {2, 3, 5, 10}
    low_feerate_pool.push_back(MakeCoin(6 * CENT, true, default_cs_params, /*custom_spending_vsize=*/500));
    low_feerate_pool.push_back(MakeCoin(7 * CENT, true, default_cs_params, /*custom_spending_vsize=*/500));
    TestBnBSuccess("Prefer two heavy inputs over two light inputs at low feerates", low_feerate_pool, /*selection_target=*/13 * CENT, /*expected_input_amounts=*/{6 * CENT, 7 * CENT}, default_cs_params, /*custom_spending_vsize=*/500);

    high_feerate_pool.push_back(MakeCoin(6 * CENT, true, high_feerate_params, /*custom_spending_vsize=*/500));
    high_feerate_pool.push_back(MakeCoin(7 * CENT, true, high_feerate_params, /*custom_spending_vsize=*/500));
    TestBnBSuccess("Prefer two light inputs over two heavy inputs at high feerates", high_feerate_pool, /*selection_target=*/13 * CENT, /*expected_input_amounts=*/{3 * CENT, 10 * CENT}, high_feerate_params);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
