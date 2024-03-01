// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
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

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
