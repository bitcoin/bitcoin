// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/amount.h>
#include <outputtype.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <random.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <util/result.h>
#include <wallet/coinselection.h>
#include <wallet/db.h>
#include <wallet/spend.h>
#include <wallet/sqlite.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace wallet {
static void addCoin(const CAmount& nValue, std::vector<std::unique_ptr<CWalletTx>>& wtxs)
{
    static int nextLockTime = 0;
    CMutableTransaction tx;
    tx.nLockTime = nextLockTime++; // so all transactions get different hashes
    tx.vout.resize(1);
    tx.vout[0].nValue = nValue;
    wtxs.push_back(std::make_unique<CWalletTx>(MakeTransactionRef(std::move(tx)), TxStateInactive{}));
}

// This benchmark is based on a large diverse UTXO pool. The UTXOs are
// pseudorandomly generated and assigned one of the four relevant output types
// P2PKH, P2SH-P2WPKH, P2WPKH, and P2TR UTXOs.
// Smaller amounts are more likely to be generated than larger amounts. This
// UTXO pool is used to run coin selection for pseudorandom selection targets.
// Altogether, this gives us a deterministic benchmark with a somewhat
// representative coin selection scenario.
static void CoinSelection(benchmark::Bench& bench)
{
    const auto test_setup = MakeNoLogFileContext<TestingSetup>();
    CWallet wallet(test_setup->m_node.chain.get(), "", MakeInMemoryWalletDatabase());
    std::vector<std::unique_ptr<CWalletTx>> wtxs;
    LOCK(wallet.cs_wallet);

    // Keep selection deterministic for benchmark stability
    FastRandomContext det_rand{/*fDeterministic=*/true};

    // Generate coin amounts biased towards smaller amounts
    for (int i = 0; i < 400; ++i) {
        CAmount amount;
        int p{det_rand.randrange(100)};
        if (p < 50) {
            amount = 10'000 + det_rand.randrange(90'000);
        } else if (p < 75) {
            amount = 100'000 + det_rand.randrange(900'000);
        } else if (p < 95) {
            amount = 1'000'000 + det_rand.randrange(9'000'000);
        } else {
            amount = 10'000'000 + det_rand.randrange(90'000'000);
        }
        addCoin(amount, wtxs);
    }

    // Create coins from the amounts assigning them various output types
    wallet::CoinsResult available_coins;
    for (const auto& wtx : wtxs) {
        const auto txout = wtx->tx->vout.at(0);
        OutputType outtype;
        int input_bytes;
        int y{det_rand.randrange(100)};
        if (y < 35) {
            outtype = OutputType::LEGACY;
            input_bytes = 148;
        } else if (y < 55) {
            outtype = OutputType::P2SH_SEGWIT;
            input_bytes = 91;
        } else if (y < 90) {
            outtype = OutputType::BECH32;
            input_bytes = 68;
        } else {
            outtype = OutputType::BECH32M;
            input_bytes = 58;
        }
        CAmount fees = 20 * input_bytes;
        available_coins.coins[outtype].emplace_back(COutPoint(wtx->GetHash(), 0), txout, /*depth=*/6 * 24, /*input_bytes=*/input_bytes, /*solvable=*/true, /*safe=*/true, wtx->GetTxTime(), /*from_me=*/true, /*fees=*/fees);
    }

    const CoinEligibilityFilter filter_standard(/*conf_mine=*/1, /*conf_theirs=*/6, /*max_ancestors=*/0);

    constexpr size_t NUM_TARGETS{10};
    std::vector<CAmount> targets;
    targets.reserve(NUM_TARGETS);
    for (size_t i{0}; i < NUM_TARGETS; ++i) {
        targets.push_back(10'000'000 + det_rand.randrange(90'000'000));
    }

    std::optional<FastRandomContext> rng;
    std::optional<CoinSelectionParams> params;
    std::vector<wallet::OutputGroupTypeMap> groups;
    bench.batch(NUM_TARGETS).unit("selection").epochIterations(1)
        .setup([&] {
            rng.emplace(/*fDeterministic=*/true);
            params.emplace(*rng);

            params->change_output_size = 31;
            params->change_spend_size = 68;
            params->m_min_change_target = CHANGE_LOWER;
            params->m_effective_feerate = CFeeRate{20'000};
            params->m_long_term_feerate = CFeeRate{10'000};
            params->m_discard_feerate = CFeeRate{3000};
            params->tx_noinputs_size = 72;
            params->m_avoid_partial_spends = false;

            params->m_change_fee = params->m_effective_feerate.GetFee(params->change_output_size);
            params->min_viable_change = params->m_discard_feerate.GetFee(params->change_spend_size);
            params->m_cost_of_change = params->min_viable_change + params->m_change_fee;

            groups.assign(NUM_TARGETS, wallet::GroupOutputs(wallet, available_coins, *params, {{filter_standard}})[filter_standard]);
        })
        .run([&] {
            for (size_t i{0}; i < NUM_TARGETS; ++i) {
                auto result{AttemptSelection(wallet.chain(), targets[i], groups[i], *params, /*allow_mixed_output_types=*/true)};
                assert(result && result->GetSelectedValue() >= targets[i]);
            }
        });
}

static void add_coin(const CAmount& nValue, uint32_t nInput, std::vector<OutputGroup>& set)
{
    CMutableTransaction tx;
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    COutput output(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/0, /*input_bytes=*/-1, /*solvable=*/true, /*safe=*/true, /*time=*/0, /*from_me=*/true, /*fees=*/0);
    set.emplace_back();
    set.back().Insert(std::make_shared<COutput>(output), /*ancestors=*/0, /*cluster_count=*/0);
}

static CAmount make_hard_case(int utxos, std::vector<OutputGroup>& utxo_pool)
{
    utxo_pool.clear();
    CAmount target = 0;
    for (int i = 0; i < utxos; ++i) {
        target += CAmount{1} << (utxos+i);
        add_coin(CAmount{1} << (utxos+i), 2*i, utxo_pool);
        add_coin((CAmount{1} << (utxos+i)) + (CAmount{1} << (utxos-1-i)), 2*i + 1, utxo_pool);
    }
    return target;
}

static void BnBExhaustion(benchmark::Bench& bench)
{
    std::vector<OutputGroup> utxo_pool;
    CAmount target;
    bench.setup([&] { target = make_hard_case(17, utxo_pool); })
        .run([&] {
            auto res{SelectCoinsBnB(utxo_pool, target, /*cost_of_change=*/0, MAX_STANDARD_TX_WEIGHT)}; // Should exhaust
            ankerl::nanobench::doNotOptimizeAway(res);
        });
}

BENCHMARK(CoinSelection);
BENCHMARK(BnBExhaustion);
}; // namespace wallet
