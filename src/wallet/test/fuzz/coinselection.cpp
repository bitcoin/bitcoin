// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/feerate.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <wallet/coinselection.h>

#include <numeric>
#include <span>
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

static CAmount CreateCoins(FuzzedDataProvider& fuzzed_data_provider, std::vector<COutput>& utxo_pool, CoinSelectionParams& coin_params, int& next_locktime)
{
    CAmount total_balance{0};
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        const int n_input{fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 10)};
        const int n_input_bytes{fuzzed_data_provider.ConsumeIntegralInRange<int>(41, 10000)};
        const CAmount amount{fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(1, MAX_MONEY)};
        if (total_balance + amount >= MAX_MONEY) {
            break;
        }
        AddCoin(amount, n_input, n_input_bytes, ++next_locktime, utxo_pool, coin_params.m_effective_feerate);
        total_balance += amount;
    }

    return total_balance;
}

static SelectionResult ManualSelection(std::vector<COutput>& utxos, const CAmount& total_amount, const bool& subtract_fee_outputs)
{
    SelectionResult result(total_amount, SelectionAlgorithm::MANUAL);
    std::set<std::shared_ptr<COutput>> utxo_pool;
    for (const auto& utxo : utxos) {
        utxo_pool.insert(std::make_shared<COutput>(utxo));
    }
    result.AddInputs(utxo_pool, subtract_fee_outputs);
    return result;
}

// Returns true if the result contains an error and the message is not empty
static bool HasErrorMsg(const util::Result<SelectionResult>& res) { return !util::ErrorString(res).empty(); }

FUZZ_TARGET(coin_grinder)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<COutput> utxo_pool;

    const CAmount target{fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(1, MAX_MONEY)};

    FastRandomContext fast_random_context{ConsumeUInt256(fuzzed_data_provider)};
    CoinSelectionParams coin_params{fast_random_context};
    coin_params.m_subtract_fee_outputs = fuzzed_data_provider.ConsumeBool();
    coin_params.m_long_term_feerate = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    coin_params.m_effective_feerate = CFeeRate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    coin_params.change_output_size = fuzzed_data_provider.ConsumeIntegralInRange<int>(10, 1000);
    coin_params.change_spend_size = fuzzed_data_provider.ConsumeIntegralInRange<int>(10, 1000);
    coin_params.m_cost_of_change= coin_params.m_effective_feerate.GetFee(coin_params.change_output_size) + coin_params.m_long_term_feerate.GetFee(coin_params.change_spend_size);
    coin_params.m_change_fee = coin_params.m_effective_feerate.GetFee(coin_params.change_output_size);
    // For other results to be comparable to SRD, we must align the change_target with SRD’s hardcoded behavior
    coin_params.m_min_change_target = CHANGE_LOWER + coin_params.m_change_fee;

    // Create some coins
    CAmount total_balance{0};
    CAmount max_spendable{0};
    int next_locktime{0};
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        const int n_input{fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 10)};
        const int n_input_bytes{fuzzed_data_provider.ConsumeIntegralInRange<int>(41, 10000)};
        const CAmount amount{fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(1, MAX_MONEY)};
        if (total_balance + amount >= MAX_MONEY) {
            break;
        }
        AddCoin(amount, n_input, n_input_bytes, ++next_locktime, utxo_pool, coin_params.m_effective_feerate);
        total_balance += amount;
        CAmount eff_value = amount - coin_params.m_effective_feerate.GetFee(n_input_bytes);
        max_spendable += eff_value;
    }

    std::vector<OutputGroup> group_pos;
    GroupCoins(fuzzed_data_provider, utxo_pool, coin_params, /*positive_only=*/true, group_pos);

    // Run coinselection algorithms
    auto result_cg = CoinGrinder(group_pos, target, coin_params.m_min_change_target, MAX_STANDARD_TX_WEIGHT);
    if (target + coin_params.m_min_change_target > max_spendable || HasErrorMsg(result_cg)) return; // We only need to compare algorithms if CoinGrinder has a solution
    assert(result_cg);
    if (!result_cg->GetAlgoCompleted()) return; // Bail out if CoinGrinder solution is not optimal

    auto result_srd = SelectCoinsSRD(group_pos, target, coin_params.m_change_fee, fast_random_context, MAX_STANDARD_TX_WEIGHT);
    if (result_srd && result_srd->GetChange(CHANGE_LOWER, coin_params.m_change_fee) > 0) { // exclude any srd solutions that don’t have change, err on excluding
        assert(result_srd->GetWeight() >= result_cg->GetWeight());
    }

    auto result_knapsack = KnapsackSolver(group_pos, target, coin_params.m_min_change_target, fast_random_context, MAX_STANDARD_TX_WEIGHT);
    if (result_knapsack && result_knapsack->GetChange(CHANGE_LOWER, coin_params.m_change_fee) > 0) { // exclude any knapsack solutions that don’t have change, err on excluding
        assert(result_knapsack->GetWeight() >= result_cg->GetWeight());
    }
}

FUZZ_TARGET(coin_grinder_is_optimal)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    FastRandomContext fast_random_context{ConsumeUInt256(fuzzed_data_provider)};
    CoinSelectionParams coin_params{fast_random_context};
    coin_params.m_subtract_fee_outputs = false;
    // Set effective feerate up to MAX_MONEY sats per 1'000'000 vB (2'100'000'000 sat/vB = 21'000 BTC/kvB).
    coin_params.m_effective_feerate = CFeeRate{ConsumeMoney(fuzzed_data_provider, MAX_MONEY), 1'000'000};
    coin_params.m_min_change_target = ConsumeMoney(fuzzed_data_provider);

    // Create some coins
    CAmount max_spendable{0};
    int next_locktime{0};
    static constexpr unsigned max_output_groups{16};
    std::vector<OutputGroup> group_pos;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), max_output_groups)
    {
        // With maximum m_effective_feerate and n_input_bytes = 1'000'000, input_fee <= MAX_MONEY.
        const int n_input_bytes{fuzzed_data_provider.ConsumeIntegralInRange<int>(1, 1'000'000)};
        // Only make UTXOs with positive effective value
        const CAmount input_fee = coin_params.m_effective_feerate.GetFee(n_input_bytes);
        // Ensure that each UTXO has at least an effective value of 1 sat
        const CAmount eff_value{fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(1, MAX_MONEY + group_pos.size() - max_spendable - max_output_groups)};
        const CAmount amount{eff_value + input_fee};
        std::vector<COutput> temp_utxo_pool;

        AddCoin(amount, /*n_input=*/0, n_input_bytes, ++next_locktime, temp_utxo_pool, coin_params.m_effective_feerate);
        max_spendable += eff_value;

        auto output_group = OutputGroup(coin_params);
        output_group.Insert(std::make_shared<COutput>(temp_utxo_pool.at(0)), /*ancestors=*/0, /*descendants=*/0);
        group_pos.push_back(output_group);
    }
    size_t num_groups = group_pos.size();
    assert(num_groups <= max_output_groups);

    // Only choose targets below max_spendable
    const CAmount target{fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(1, std::max(CAmount{1}, max_spendable - coin_params.m_min_change_target))};

    // Brute force optimal solution
    CAmount best_amount{MAX_MONEY};
    int best_weight{std::numeric_limits<int>::max()};
    for (uint32_t pattern = 1; (pattern >> num_groups) == 0; ++pattern) {
        CAmount subset_amount{0};
        int subset_weight{0};
        for (unsigned i = 0; i < num_groups; ++i) {
            if ((pattern >> i) & 1) {
                subset_amount += group_pos[i].GetSelectionAmount();
                subset_weight += group_pos[i].m_weight;
            }
        }
        if ((subset_amount >= target + coin_params.m_min_change_target) && (subset_weight < best_weight || (subset_weight == best_weight && subset_amount < best_amount))) {
            best_weight = subset_weight;
            best_amount = subset_amount;
        }
    }

    if (best_weight < std::numeric_limits<int>::max()) {
        // Sufficient funds and acceptable weight: CoinGrinder should find at least one solution
        int high_max_selection_weight = fuzzed_data_provider.ConsumeIntegralInRange<int>(best_weight, std::numeric_limits<int>::max());

        auto result_cg = CoinGrinder(group_pos, target, coin_params.m_min_change_target, high_max_selection_weight);
        assert(result_cg);
        assert(result_cg->GetWeight() <= high_max_selection_weight);
        assert(result_cg->GetSelectedEffectiveValue() >= target + coin_params.m_min_change_target);
        assert(best_weight < result_cg->GetWeight() || (best_weight == result_cg->GetWeight() && best_amount <= result_cg->GetSelectedEffectiveValue()));
        if (result_cg->GetAlgoCompleted()) {
            // If CoinGrinder exhausted the search space, it must return the optimal solution
            assert(best_weight == result_cg->GetWeight());
            assert(best_amount == result_cg->GetSelectedEffectiveValue());
        }
    }

    // CoinGrinder cannot ever find a better solution than the brute-forced best, or there is none in the first place
    int low_max_selection_weight = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, best_weight - 1);
    auto result_cg = CoinGrinder(group_pos, target, coin_params.m_min_change_target, low_max_selection_weight);
    // Max_weight should have been exceeded, or there were insufficient funds
    assert(!result_cg);
}

enum class CoinSelectionAlgorithm {
    BNB,
    SRD,
    KNAPSACK,
};

template<CoinSelectionAlgorithm Algorithm>
void FuzzCoinSelectionAlgorithm(std::span<const uint8_t> buffer) {
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<COutput> utxo_pool;

    const CFeeRate long_term_fee_rate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    const CFeeRate effective_fee_rate{ConsumeMoney(fuzzed_data_provider, /*max=*/COIN)};
    // Discard feerate must be at least dust relay feerate
    const CFeeRate discard_fee_rate{fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(DUST_RELAY_TX_FEE, COIN)};
    const CAmount target{fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(1, MAX_MONEY)};
    const bool subtract_fee_outputs{fuzzed_data_provider.ConsumeBool()};

    FastRandomContext fast_random_context{ConsumeUInt256(fuzzed_data_provider)};
    CoinSelectionParams coin_params{fast_random_context};
    coin_params.m_subtract_fee_outputs = subtract_fee_outputs;
    coin_params.m_long_term_feerate = long_term_fee_rate;
    coin_params.m_effective_feerate = effective_fee_rate;
    coin_params.change_output_size = fuzzed_data_provider.ConsumeIntegralInRange(1, MAX_SCRIPT_SIZE);
    coin_params.m_change_fee = effective_fee_rate.GetFee(coin_params.change_output_size);
    coin_params.m_discard_feerate = discard_fee_rate;
    coin_params.change_spend_size = fuzzed_data_provider.ConsumeIntegralInRange<int>(41, 1000);
    const auto change_spend_fee{coin_params.m_discard_feerate.GetFee(coin_params.change_spend_size)};
    coin_params.m_cost_of_change = coin_params.m_change_fee + change_spend_fee;
    CScript change_out_script = CScript() << std::vector<unsigned char>(coin_params.change_output_size, OP_TRUE);
    const auto dust{GetDustThreshold(CTxOut{/*nValueIn=*/0, change_out_script}, coin_params.m_discard_feerate)};
    coin_params.min_viable_change = std::max(change_spend_fee + 1, dust);

    int next_locktime{0};
    CAmount total_balance{CreateCoins(fuzzed_data_provider, utxo_pool, coin_params, next_locktime)};

    std::vector<OutputGroup> group_pos;
    GroupCoins(fuzzed_data_provider, utxo_pool, coin_params, /*positive_only=*/true, group_pos);

    int max_selection_weight = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, std::numeric_limits<int>::max());

    std::optional<SelectionResult> result;

    if constexpr (Algorithm == CoinSelectionAlgorithm::BNB) {
        if (!coin_params.m_subtract_fee_outputs) {
            auto result_bnb = SelectCoinsBnB(group_pos, target, coin_params.m_cost_of_change, max_selection_weight);
            if (result_bnb) {
                result = *result_bnb;
                assert(result_bnb->GetChange(coin_params.min_viable_change, coin_params.m_change_fee) == 0);
                assert(result_bnb->GetSelectedValue() >= target);
                assert(result_bnb->GetWeight() <= max_selection_weight);
                (void)result_bnb->GetShuffledInputVector();
                (void)result_bnb->GetInputSet();
            }
        }
    }

    if constexpr (Algorithm == CoinSelectionAlgorithm::SRD) {
        auto result_srd = SelectCoinsSRD(group_pos, target, coin_params.m_change_fee, fast_random_context, max_selection_weight);
        if (result_srd) {
            result = *result_srd;
            assert(result_srd->GetSelectedValue() >= target);
            assert(result_srd->GetChange(CHANGE_LOWER, coin_params.m_change_fee) > 0);
            assert(result_srd->GetWeight() <= max_selection_weight);
            result_srd->SetBumpFeeDiscount(ConsumeMoney(fuzzed_data_provider));
            result_srd->RecalculateWaste(coin_params.min_viable_change, coin_params.m_cost_of_change, coin_params.m_change_fee);
            (void)result_srd->GetShuffledInputVector();
            (void)result_srd->GetInputSet();
        }
    }

    if constexpr (Algorithm == CoinSelectionAlgorithm::KNAPSACK) {
        std::vector<OutputGroup> group_all;
        GroupCoins(fuzzed_data_provider, utxo_pool, coin_params, /*positive_only=*/false, group_all);

        for (const OutputGroup& group : group_all) {
            const CoinEligibilityFilter filter{fuzzed_data_provider.ConsumeIntegral<int>(), fuzzed_data_provider.ConsumeIntegral<int>(), fuzzed_data_provider.ConsumeIntegral<uint64_t>()};
            (void)group.EligibleForSpending(filter);
        }

        CAmount change_target{GenerateChangeTarget(target, coin_params.m_change_fee, fast_random_context)};
        auto result_knapsack = KnapsackSolver(group_all, target, change_target, fast_random_context, max_selection_weight);
        // If the total balance is sufficient for the target and we are not using
        // effective values, Knapsack should always find a solution (unless the selection exceeded the max tx weight).
        if (total_balance >= target && subtract_fee_outputs && !HasErrorMsg(result_knapsack)) {
            assert(result_knapsack);
        }
        if (result_knapsack) {
            result = *result_knapsack;
            assert(result_knapsack->GetSelectedValue() >= target);
            assert(result_knapsack->GetWeight() <= max_selection_weight);
            result_knapsack->SetBumpFeeDiscount(ConsumeMoney(fuzzed_data_provider));
            result_knapsack->RecalculateWaste(coin_params.min_viable_change, coin_params.m_cost_of_change, coin_params.m_change_fee);
            (void)result_knapsack->GetShuffledInputVector();
            (void)result_knapsack->GetInputSet();
        }
    }

    std::vector<COutput> utxos;
    CAmount new_total_balance{CreateCoins(fuzzed_data_provider, utxos, coin_params, next_locktime)};
    if (new_total_balance > 0) {
        std::set<std::shared_ptr<COutput>> new_utxo_pool;
        for (const auto& utxo : utxos) {
            new_utxo_pool.insert(std::make_shared<COutput>(utxo));
        }
        if (result) {
            const auto weight{result->GetWeight()};
            result->AddInputs(new_utxo_pool, subtract_fee_outputs);
            assert(result->GetWeight() > weight);
        }
    }

    std::vector<COutput> manual_inputs;
    CAmount manual_balance{CreateCoins(fuzzed_data_provider, manual_inputs, coin_params, next_locktime)};
    if (manual_balance == 0) return;
    auto manual_selection{ManualSelection(manual_inputs, manual_balance, coin_params.m_subtract_fee_outputs)};
    if (result) {
        const CAmount old_target{result->GetTarget()};
        const std::set<std::shared_ptr<COutput>> input_set{result->GetInputSet()};
        const int old_weight{result->GetWeight()};
        result->Merge(manual_selection);
        assert(result->GetInputSet().size() == input_set.size() + manual_inputs.size());
        assert(result->GetTarget() == old_target + manual_selection.GetTarget());
        assert(result->GetWeight() == old_weight + manual_selection.GetWeight());
    }
}

FUZZ_TARGET(coinselection_bnb) {
    FuzzCoinSelectionAlgorithm<CoinSelectionAlgorithm::BNB>(buffer);
}

FUZZ_TARGET(coinselection_srd) {
    FuzzCoinSelectionAlgorithm<CoinSelectionAlgorithm::SRD>(buffer);
}

FUZZ_TARGET(coinselection_knapsack) {
    FuzzCoinSelectionAlgorithm<CoinSelectionAlgorithm::KNAPSACK>(buffer);
}

} // namespace wallet
