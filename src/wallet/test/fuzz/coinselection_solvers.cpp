// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <util/check.h>
#include <wallet/coinselection.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

namespace wallet {
namespace {

// Max UTXOs per solver iteration: keeps quadratic solvers fast yet non-trivial.
constexpr size_t MAX_COINS{64};
constexpr int MIN_VSIZE{40};
constexpr int MAX_VSIZE{400};

CoinSelectionParams MakeParams(FuzzedDataProvider& fdp, FastRandomContext& rng)
{
    CoinSelectionParams params{
        rng,
        /*change_output_size=*/fdp.ConsumeIntegralInRange<int>(MIN_VSIZE, MAX_VSIZE),
        /*change_spend_size=*/fdp.ConsumeIntegralInRange<int>(MIN_VSIZE, MAX_VSIZE),
        /*min_change_target=*/fdp.ConsumeIntegralInRange<CAmount>(0, 2'000'000),
        /*effective_feerate=*/CFeeRate{fdp.ConsumeIntegralInRange<CAmount>(0, 100'000)},
        /*long_term_feerate=*/CFeeRate{fdp.ConsumeIntegralInRange<CAmount>(0, 100'000)},
        /*discard_feerate=*/CFeeRate{fdp.ConsumeIntegralInRange<CAmount>(0, 100'000)},
        /*tx_noinputs_size=*/fdp.ConsumeIntegralInRange<int>(10, 1'000),
        /*avoid_partial=*/fdp.ConsumeBool(),
    };
    params.m_change_fee = params.m_effective_feerate.GetFee(params.change_output_size);
    params.min_viable_change = params.m_discard_feerate.GetFee(params.change_spend_size);
    params.m_cost_of_change = params.m_change_fee + params.min_viable_change;
    params.m_subtract_fee_outputs = fdp.ConsumeBool();
    return params;
}

std::vector<OutputGroup> MakePool(FuzzedDataProvider& fdp, const CoinSelectionParams& params, FastRandomContext& rng)
{
    std::vector<OutputGroup> pool;
    const size_t num_coins{fdp.ConsumeIntegralInRange<size_t>(0, MAX_COINS)};
    pool.reserve(num_coins);
    for (size_t i = 0; i < num_coins; ++i) {
        CMutableTransaction tx;
        tx.vout.resize(1);
        tx.vout[0].nValue = fdp.ConsumeIntegralInRange<CAmount>(0, 21'000'000);
        tx.nLockTime = rng.rand32(); // ensure distinct txids
        const int input_bytes{fdp.ConsumeIntegralInRange<int>(MIN_VSIZE, MAX_VSIZE)};
        const CAmount fee{params.m_effective_feerate.GetFee(input_bytes)};
        OutputGroup group{params};
        group.Insert(std::make_shared<COutput>(
                         COutPoint(tx.GetHash(), 0), tx.vout.at(0),
                         /*depth=*/fdp.ConsumeIntegralInRange<int>(0, 100),
                         input_bytes,
                         /*solvable=*/true, /*safe=*/true,
                         /*time=*/0, /*from_me=*/fdp.ConsumeBool(),
                         /*fees=*/fee),
                     /*ancestors=*/0, /*cluster_count=*/0);
        pool.push_back(std::move(group));
    }
    return pool;
}

// BnB/Grinder/SRD require strictly positive effective-value groups.
std::vector<OutputGroup> PositiveGroups(const std::vector<OutputGroup>& pool)
{
    std::vector<OutputGroup> positive;
    for (const auto& group : pool) {
        if (group.GetSelectionAmount() > 0) positive.push_back(group);
    }
    return positive;
}

} // namespace

FUZZ_TARGET(coinselection)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};
    FastRandomContext rng{ConsumeUInt256(fdp)};

    CoinSelectionParams params{MakeParams(fdp, rng)};
    std::vector<OutputGroup> pool{MakePool(fdp, params, rng)};
    const CAmount target{fdp.ConsumeIntegralInRange<CAmount>(0, 21'000'000)};
    const CAmount change_target{fdp.ConsumeIntegralInRange<CAmount>(0, 2'000'000)};
    const int max_selection_weight{fdp.ConsumeIntegralInRange<int>(0, MAX_STANDARD_TX_WEIGHT)};

    // The solvers require a positive target and a non-empty pool of positive
    // effective-value groups; production callers (SelectCoins) guarantee both.
    // Mirror that contract here so the fuzzer exercises valid call sites.
    const auto positive_pool{PositiveGroups(pool)};
    if (target <= 0 || positive_pool.empty()) return;

    {
        auto p{positive_pool};
        if (auto bnb{SelectCoinsBnB(p, target, params.m_cost_of_change, max_selection_weight)}) {
            Assert(bnb->GetSelectedValue() >= target);
            Assert(bnb->GetWeight() <= max_selection_weight);
        }
    }
    {
        auto p{positive_pool};
        if (auto grinder{CoinGrinder(p, target, change_target, max_selection_weight)}) {
            Assert(grinder->GetSelectedValue() >= target);
            Assert(grinder->GetWeight() <= max_selection_weight);
        }
    }
    {
        if (auto srd{SelectCoinsSRD(positive_pool, target, params.m_change_fee, rng, max_selection_weight)}) {
            Assert(srd->GetSelectedValue() >= target);
            Assert(srd->GetWeight() <= max_selection_weight);
        }
    }
    {
        // Knapsack tolerates a mixed pool (positive and negative effective values).
        auto mixed_pool{pool};
        if (auto knapsack{KnapsackSolver(mixed_pool, target, change_target, rng, max_selection_weight)}) {
            Assert(knapsack->GetSelectedValue() >= target);
            Assert(knapsack->GetWeight() <= max_selection_weight);
        }
    }
}

} // namespace wallet
