// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <wallet/coinselection.h>

#include <vector>

namespace wallet {

static void AddCoin(const CAmount& value, int n_input, int n_input_bytes, int locktime, std::vector<COutput>& coins, CFeeRate fee_rate)
{
    CMutableTransaction tx;
    tx.vout.resize(n_input + 1);
    tx.vout[n_input].nValue = value;
    tx.nLockTime = locktime; // all transactions get different hashes
    coins.emplace_back(COutPoint(tx.GetHash(), n_input), tx.vout.at(n_input), /*depth=*/0, n_input_bytes, /*spendable=*/true, /*solvable=*/true, /*safe=*/true, /*time=*/0, /*from_me=*/true, fee_rate);
}

// Randomly distribute coins to instances of OutputGroup
static void GroupCoins(FuzzedDataProvider& fuzzed_data_provider, const std::vector<COutput>& coins, const CoinSelectionParams& coin_params, bool positive_only, std::vector<OutputGroup>& output_groups)
{
    auto output_group = OutputGroup(coin_params);
    bool valid_outputgroup{false};
    for (auto& coin : coins) {
        if (!positive_only || (positive_only && coin.GetEffectiveValue() > 0)) {
            output_group.Insert(std::make_shared<COutput>(coin), /*ancestors=*/0, /*descendants=*/0);
        }
        // If positive_only was specified, nothing was inserted, leading to an empty output group
        // that would be invalid for the BnB algorithm
        valid_outputgroup = !positive_only || output_group.GetSelectionAmount() > 0;
        if (valid_outputgroup && fuzzed_data_provider.ConsumeBool()) {
            output_groups.push_back(output_group);
            output_group = OutputGroup(coin_params);
            valid_outputgroup = false;
        }
    }
    if (valid_outputgroup) output_groups.push_back(output_group);
}

FUZZ_TARGET(coinselection)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<COutput> utxo_pool;

    const CFeeRate long_term_fee_rate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    const CFeeRate effective_fee_rate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    const CAmount cost_of_change{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    const CAmount target{fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(1, MAX_MONEY)};
    const bool subtract_fee_outputs{fuzzed_data_provider.ConsumeBool()};

    FastRandomContext fast_random_context{ConsumeUInt256(fuzzed_data_provider)};
    CoinSelectionParams coin_params{fast_random_context};
    coin_params.m_subtract_fee_outputs = subtract_fee_outputs;
    coin_params.m_long_term_feerate = long_term_fee_rate;
    coin_params.m_effective_feerate = effective_fee_rate;
    coin_params.change_output_size = fuzzed_data_provider.ConsumeIntegralInRange<int>(10, 1000);
    coin_params.m_change_fee = effective_fee_rate.GetFee(coin_params.change_output_size);

    // Create some coins
    CAmount total_balance{0};
    int next_locktime{0};
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        const int n_input{fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 10)};
        const int n_input_bytes{fuzzed_data_provider.ConsumeIntegralInRange<int>(100, 10000)};
        const CAmount amount{fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(1, MAX_MONEY)};
        if (total_balance + amount >= MAX_MONEY) {
            break;
        }
        AddCoin(amount, n_input, n_input_bytes, ++next_locktime, utxo_pool, coin_params.m_effective_feerate);
        total_balance += amount;
    }

    std::vector<OutputGroup> group_pos;
    GroupCoins(fuzzed_data_provider, utxo_pool, coin_params, /*positive_only=*/true, group_pos);
    std::vector<OutputGroup> group_all;
    GroupCoins(fuzzed_data_provider, utxo_pool, coin_params, /*positive_only=*/false, group_all);

    // Run coinselection algorithms
    const auto result_bnb = SelectCoinsBnB(group_pos, target, cost_of_change);

    auto result_srd = SelectCoinsSRD(group_pos, target, fast_random_context);
    if (result_srd) result_srd->ComputeAndSetWaste(cost_of_change, cost_of_change, 0);

    CAmount change_target{GenerateChangeTarget(target, coin_params.m_change_fee, fast_random_context)};
    auto result_knapsack = KnapsackSolver(group_all, target, change_target, fast_random_context);
    if (result_knapsack) result_knapsack->ComputeAndSetWaste(cost_of_change, cost_of_change, 0);

    // If the total balance is sufficient for the target and we are not using
    // effective values, Knapsack should always find a solution.
    if (total_balance >= target && subtract_fee_outputs) {
        assert(result_knapsack);
    }
}

} // namespace wallet
