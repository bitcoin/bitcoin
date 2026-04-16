// Copyright (c) 2024-present The Bitcoin Core developers
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

static const int P2WPKH_INPUT_VSIZE = 68;
static const int P2WPKH_OUTPUT_VSIZE = 31;

/**
 * This set of feerates is used in the tests to test edge cases around the
 * default minimum feerate and other potential special cases:
 * - zero: 0 s/kvB
 * - minimum non-zero s/kvB: 1 s/kvB
 * - just below the new default minimum feerate: 99 s/kvB
 * - new default minimum feerate: 100 s/kvB
 * - old default minimum feerate: 1000 s/kvB
 * - a few non-round realistic feerates around default minimum feerate,
 * dust feerate, and default LTFRE: 315 s/kvB, 2345 s/kvB, and
 * 10'292 s/kvB
 * - a high feerate that has been exceeded occasionally: 59'764 s/kvB
 * - a huge feerate that is extremely uncommon: 1'500'000 s/kvB */
static const std::vector<int> FEERATES = {0, 1, 99, 100, 315, 1'000, 2'345, 10'292, 59'764, 1'500'000};

/** Default coin selection parameters allow us to only explicitly set
 * parameters when a diverging value is relevant in the context of a test,
 * without reiterating the defaults in every test. We use P2WPKH input and
 * output weights for the change weights. */
static CoinSelectionParams init_cs_params(int eff_feerate = 5000)
{
    CoinSelectionParams csp{
        /*rng_fast=*/default_rand,
        /*change_output_size=*/P2WPKH_OUTPUT_VSIZE,
        /*change_spend_size=*/P2WPKH_INPUT_VSIZE,
        /*min_change_target=*/50'000,
        /*effective_feerate=*/CFeeRate(eff_feerate),
        /*long_term_feerate=*/CFeeRate(10'000),
        /*discard_feerate=*/CFeeRate(3000),
        /*tx_noinputs_size=*/11 + P2WPKH_OUTPUT_VSIZE, //static header size + output size
        /*avoid_partial=*/false,
    };
    csp.m_change_fee = csp.m_effective_feerate.GetFee(csp.change_output_size); // 155 sats for default feerate of 5000 s/kvB
    csp.min_viable_change = /*204 sats=*/csp.m_discard_feerate.GetFee(csp.change_spend_size);
    csp.m_cost_of_change = csp.min_viable_change + csp.m_change_fee; // 204 + 155 sats for default feerate of 5000 s/kvB
    csp.m_subtract_fee_outputs = false;
    return csp;
}

static const CoinSelectionParams default_cs_params = init_cs_params();

/** Make one OutputGroup with a single UTXO that either has a given effective value (default) or a given amount (`is_eff_value = false`). */
static OutputGroup MakeCoin(const CAmount& amount, bool is_eff_value = true, CoinSelectionParams cs_params = default_cs_params, int custom_spending_vsize = P2WPKH_INPUT_VSIZE)
{
    // Always assume that we only have one input
    CMutableTransaction tx;
    tx.vout.resize(1);
    CAmount fees = cs_params.m_effective_feerate.GetFee(custom_spending_vsize);
    tx.vout[0].nValue = amount + int(is_eff_value) * fees;
    tx.nLockTime = next_lock_time++;        // so all transactions get different hashes
    OutputGroup group(cs_params);
    group.Insert(std::make_shared<COutput>(COutPoint(tx.GetHash(), 0), tx.vout.at(0), /*depth=*/1, /*input_bytes=*/custom_spending_vsize, /*solvable=*/true, /*safe=*/true, /*time=*/0, /*from_me=*/false, /*fees=*/fees), /*ancestors=*/0, /*cluster_count=*/0);
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

static void TestBnBSuccess(std::string test_title, std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target, const std::vector<CAmount>& expected_input_amounts, const CoinSelectionParams& cs_params = default_cs_params, const int custom_spending_vsize = P2WPKH_INPUT_VSIZE, const int max_selection_weight = MAX_STANDARD_TX_WEIGHT)
{
    SelectionResult expected_result(CAmount(0), SelectionAlgorithm::BNB);
    CAmount expected_amount = 0;
    for (CAmount input_amount : expected_input_amounts) {
        OutputGroup group = MakeCoin(input_amount, true, cs_params, custom_spending_vsize);
        expected_amount += group.m_value;
        expected_result.AddInput(group);
    }

    const auto result = SelectCoinsBnB(utxo_pool, selection_target, /*cost_of_change=*/cs_params.m_cost_of_change, max_selection_weight);
    BOOST_CHECK_MESSAGE(result, "Falsy result in BnB-Success: " + test_title);
    BOOST_CHECK_MESSAGE(HaveEquivalentValues(expected_result, *result), strprintf("Result mismatch in BnB-Success: %s. Expected %s, but got %s", test_title, InputAmountsToString(expected_result), InputAmountsToString(*result)));
    BOOST_CHECK_MESSAGE(result->GetSelectedValue() == expected_amount, strprintf("Selected amount mismatch in BnB-Success: %s. Expected %d, but got %d", test_title, expected_amount, result->GetSelectedValue()));
    BOOST_CHECK_MESSAGE(result->GetWeight() <= max_selection_weight, strprintf("Selected weight is higher than permitted in BnB-Success: %s. Expected %d, but got %d", test_title, max_selection_weight, result->GetWeight()));
}

static void TestBnBFail(std::string test_title, std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target, const CoinSelectionParams& cs_params = default_cs_params, int max_selection_weight = MAX_STANDARD_TX_WEIGHT, const bool expect_max_weight_exceeded = false)
{
    const auto result = SelectCoinsBnB(utxo_pool, selection_target, /*cost_of_change=*/cs_params.m_cost_of_change, max_selection_weight);
    BOOST_CHECK_MESSAGE(!result, "BnB-Fail: " + test_title);
    bool max_weight_exceeded = util::ErrorString(result).original.find("The inputs size exceeds the maximum weight") != std::string::npos;
    BOOST_CHECK(expect_max_weight_exceeded == max_weight_exceeded);
}

BOOST_AUTO_TEST_CASE(bnb_test)
{
    for (int feerate : FEERATES) {
        std::vector<OutputGroup> utxo_pool;

        const CoinSelectionParams cs_params = init_cs_params(feerate);

        TestBnBFail("Empty UTXO pool", utxo_pool, /*selection_target=*/1 * CENT, cs_params);

        AddCoins(utxo_pool, {1 * CENT, 3 * CENT, 5 * CENT}, cs_params);

        // Simple success cases
        TestBnBSuccess("Select smallest UTXO", utxo_pool, /*selection_target=*/1 * CENT, /*expected_input_amounts=*/{1 * CENT}, cs_params);
        TestBnBSuccess("Select middle UTXO", utxo_pool, /*selection_target=*/3 * CENT, /*expected_input_amounts=*/{3 * CENT}, cs_params);
        TestBnBSuccess("Select biggest UTXO", utxo_pool, /*selection_target=*/5 * CENT, /*expected_input_amounts=*/{5 * CENT}, cs_params);
        TestBnBSuccess("Select two UTXOs", utxo_pool, /*selection_target=*/4 * CENT, /*expected_input_amounts=*/{1 * CENT, 3 * CENT}, cs_params);
        TestBnBSuccess("Select all UTXOs", utxo_pool, /*selection_target=*/9 * CENT, /*expected_input_amounts=*/{1 * CENT, 3 * CENT, 5 * CENT}, cs_params);

        // BnB finds changeless solution while overshooting by up to cost_of_change
        TestBnBSuccess("Select upper bound", utxo_pool, /*selection_target=*/4 * CENT - cs_params.m_cost_of_change, /*expected_input_amounts=*/{1 * CENT, 3 * CENT}, cs_params);

        // BnB fails to find changeless solution when overshooting by cost_of_change + 1 sat
        TestBnBFail("Overshoot upper bound", utxo_pool, /*selection_target=*/4 * CENT - cs_params.m_cost_of_change - 1, cs_params);

        TestBnBSuccess("Select max weight", utxo_pool, /*selection_target=*/4 * CENT, /*expected_input_amounts=*/{1 * CENT, 3 * CENT}, cs_params, /*custom_spending_vsize=*/P2WPKH_INPUT_VSIZE, /*max_selection_weight=*/4 * 2 * P2WPKH_INPUT_VSIZE);

        TestBnBFail("Exceed max weight", utxo_pool, /*selection_target=*/4 * CENT, cs_params, /*max_selection_weight=*/4 * 2 * P2WPKH_INPUT_VSIZE - 1, /*expect_max_weight_exceeded=*/true);

        // Simple cases without BnB solution
        TestBnBFail("Smallest combination too big", utxo_pool, /*selection_target=*/0.5 * CENT, cs_params);
        TestBnBFail("No UTXO combination in target window", utxo_pool, /*selection_target=*/7 * CENT, cs_params);
        TestBnBFail("Select more than available", utxo_pool, /*selection_target=*/10 * CENT, cs_params);

        // Test skipping of equivalent input sets
        std::vector<OutputGroup> clone_pool;
        AddCoins(clone_pool, {2 * CENT, 7 * CENT, 7 * CENT}, cs_params);
        AddDuplicateCoins(clone_pool, /*count=*/50'000, /*amount=*/5 * CENT, cs_params);
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
                doppelgangers.push_back(1 * CENT + cs_params.m_cost_of_change + i);
            }
        }
        AddCoins(doppelganger_pool, doppelgangers, cs_params);
        // Among up to 17 unique UTXOs of similar effective value we will find a solution composed of the eight smallest UTXOs
        TestBnBSuccess("Combine smallest 8 of 17 unique UTXOs", doppelganger_pool, /*selection_target=*/8 * CENT, /*expected_input_amounts=*/expected_inputs, cs_params);

        // Starting with 18 unique UTXOs of similar effective value we will not find the solution due to exceeding the attempt limit
        AddCoins(doppelganger_pool, {1 * CENT + cs_params.m_cost_of_change + 17}, cs_params);
        TestBnBFail("Exhaust looking for smallest 8 of 18 unique UTXOs", doppelganger_pool, /*selection_target=*/8 * CENT, cs_params);
    }
}

BOOST_AUTO_TEST_CASE(bnb_feerate_sensitivity_test)
{
    // Create sets of UTXOs with the same effective amounts at different feerates (but different absolute amounts)
    std::vector<OutputGroup> low_feerate_pool; // 5 sat/vB (default, and lower than long_term_feerate of 10 sat/vB)
    AddCoins(low_feerate_pool, {2 * CENT, 3 * CENT, 5 * CENT, 10 * CENT});
    TestBnBSuccess("Select many inputs at low feerates", low_feerate_pool, /*selection_target=*/10 * CENT, /*expected_input_amounts=*/{2 * CENT, 3 * CENT, 5 * CENT});

    const CoinSelectionParams high_feerate_params = init_cs_params(/*eff_feerate=*/25'000);
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

static void TestSRDSuccess(std::string test_title, std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target, const CoinSelectionParams& cs_params = default_cs_params, const int max_selection_weight = MAX_STANDARD_TX_WEIGHT)
{
    CAmount expected_min_amount = selection_target + cs_params.m_change_fee + CHANGE_LOWER;

    const auto result = SelectCoinsSRD(utxo_pool, selection_target, cs_params.m_change_fee, cs_params.rng_fast, max_selection_weight);
    BOOST_CHECK_MESSAGE(result, "Falsy result in SRD-Success: " + test_title);
    const CAmount selected_effective_value = result->GetSelectedEffectiveValue();
    BOOST_CHECK_MESSAGE(selected_effective_value >= expected_min_amount, strprintf("Selected effective value is lower than expected in SRD-Success: %s. Expected %d, but got %d", test_title, expected_min_amount, selected_effective_value));
    BOOST_CHECK_MESSAGE(result->GetWeight() <= max_selection_weight, strprintf("Selected weight is higher than permitted in SRD-Success: %s. Expected %d, but got %d", test_title, max_selection_weight, result->GetWeight()));
}

static void TestSRDFail(std::string test_title, std::vector<OutputGroup>& utxo_pool, const CAmount& selection_target, const CoinSelectionParams& cs_params = default_cs_params, int max_selection_weight = MAX_STANDARD_TX_WEIGHT, const bool expect_max_weight_exceeded = false)
{
    const auto result = SelectCoinsSRD(utxo_pool, selection_target, cs_params.m_change_fee, cs_params.rng_fast, max_selection_weight);
    BOOST_CHECK_MESSAGE(!result, "SRD-Fail: " + test_title);
    bool max_weight_exceeded = util::ErrorString(result).original.find("The inputs size exceeds the maximum weight") != std::string::npos;
    BOOST_CHECK(expect_max_weight_exceeded == max_weight_exceeded);
}

BOOST_AUTO_TEST_CASE(srd_test)
{
    for (int feerate : FEERATES) {
        std::vector<OutputGroup> utxo_pool;

        const CoinSelectionParams cs_params = init_cs_params(feerate);

        TestSRDFail("Empty UTXO pool", utxo_pool, /*selection_target=*/1 * CENT, cs_params);

        AddCoins(utxo_pool, {1 * CENT, 3 * CENT, 5 * CENT}, cs_params);

        TestSRDSuccess("Select 21k sats", utxo_pool, /*selection_target=*/21'000, cs_params);
        TestSRDSuccess("Select 1 CENT", utxo_pool, /*selection_target=*/1 * CENT, cs_params);
        TestSRDSuccess("Select 3.125 CENT", utxo_pool, /*selection_target=*/3'125'000, cs_params);
        TestSRDSuccess("Select 4 CENT", utxo_pool, /*selection_target=*/4 * CENT, cs_params);
        TestSRDSuccess("Select 7 CENT", utxo_pool, /*selection_target=*/7 * CENT, cs_params);

        // The minimum change amount for SRD is the feerate dependent `change_fee` plus CHANGE_LOWER
        TestSRDSuccess("Create minimum change", utxo_pool, /*selection_target=*/9 * CENT - cs_params.m_change_fee - CHANGE_LOWER, cs_params);
        TestSRDFail("Undershoot minimum change by one sat", utxo_pool, /*selection_target=*/9 * CENT - cs_params.m_change_fee - CHANGE_LOWER + 1, cs_params);
        TestSRDFail("Spend more than available", utxo_pool, /*selection_target=*/9 * CENT + 1, cs_params);
        TestSRDFail("Spend everything", utxo_pool, /*selection_target=*/9 * CENT, cs_params);

        AddDuplicateCoins(utxo_pool, /*count=*/100, /*amount=*/5 * CENT, cs_params);
        AddDuplicateCoins(utxo_pool, /*count=*/3, /*amount=*/7 * CENT, cs_params);
        TestSRDSuccess("Select most valuable UTXOs for acceptable weight", utxo_pool, /*selection_target=*/20 * CENT, cs_params, /*max_selection_weight=*/4 * 4 * (P2WPKH_INPUT_VSIZE - 1 ));
        TestSRDFail("No acceptable weight possible", utxo_pool, /*selection_target=*/25 * CENT, cs_params, /*max_selection_weight=*/4 * 3 * P2WPKH_INPUT_VSIZE, /*expect_max_weight_exceeded=*/true);
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
