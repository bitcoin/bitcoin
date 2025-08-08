// Copyright (c) 2012-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/amount.h>
#include <interfaces/chain.h>
#include <node/context.h>
#include <outputtype.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <random.h>
#include <sync.h>
#include <util/result.h>
#include <wallet/coinselection.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>

#include <cassert>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

using node::NodeContext;
using wallet::AttemptSelection;
using wallet::CHANGE_LOWER;
using wallet::COutput;
using wallet::CWallet;
using wallet::CWalletTx;
using wallet::CoinEligibilityFilter;
using wallet::CoinSelectionParams;
using wallet::CreateMockableWalletDatabase;
using wallet::OutputGroup;
using wallet::SelectCoinsBnB;
using wallet::TxStateInactive;

static void addCoin(const CAmount& nValue, const CWallet& wallet, std::vector<std::unique_ptr<CWalletTx>>& wtxs)
{
    static int nextLockTime = 0;
    CMutableTransaction tx;
    tx.nLockTime = nextLockTime++; // so all transactions get different hashes
    tx.vout.resize(1);
    tx.vout[0].nValue = nValue;
    wtxs.push_back(std::make_unique<CWalletTx>(MakeTransactionRef(std::move(tx)), TxStateInactive{}));
}

// Simple benchmark for wallet coin selection that exercises a worst-case
// scenario for Knapsack: All UTXOs are necessary, but it is not an exact
// match, so the only eligible input set is only discovered on the second pass
// after all random walks fail to produce a solution.
static void KnapsackWorstCase(benchmark::Bench& bench)
{
    NodeContext node;
    auto chain = interfaces::MakeChain(node);
    CWallet wallet(chain.get(), "", CreateMockableWalletDatabase());
    std::vector<std::unique_ptr<CWalletTx>> wtxs;
    LOCK(wallet.cs_wallet);

    // Add coins.
    for (int i = 0; i < 1000; ++i) {
        addCoin(1000 * COIN, wallet, wtxs);
    }
    addCoin(3 * COIN, wallet, wtxs);

    // Create coins
    wallet::CoinsResult available_coins;
    for (const auto& wtx : wtxs) {
        const auto txout = wtx->tx->vout.at(0);
        available_coins.coins[OutputType::BECH32].emplace_back(COutPoint(wtx->GetHash(), 0), txout, /*depth=*/6 * 24, CalculateMaximumSignedInputSize(txout, &wallet, /*coin_control=*/nullptr), /*spendable=*/true, /*solvable=*/true, /*safe=*/true, wtx->GetTxTime(), /*from_me=*/true, /*fees=*/ 0);
    }

    const CoinEligibilityFilter filter_standard(1, 6, 0);
    FastRandomContext rand{};
    const CoinSelectionParams coin_selection_params{
        rand,
        /*change_output_size=*/ 34,
        /*change_spend_size=*/ 148,
        /*min_change_target=*/ CHANGE_LOWER,
        /*effective_feerate=*/ CFeeRate(20'000),
        /*long_term_feerate=*/ CFeeRate(10'000),
        /*discard_feerate=*/ CFeeRate(3000),
        /*tx_noinputs_size=*/ 0,
        /*avoid_partial=*/ false,
    };
    auto group = wallet::GroupOutputs(wallet, available_coins, coin_selection_params, {{filter_standard}})[filter_standard];
    bench.run([&] {
        auto result = AttemptSelection(wallet.chain(), 1002.99 * COIN, group, coin_selection_params, /*allow_mixed_output_types=*/true);
        assert(result);
        assert(result->GetSelectedValue() == 1003 * COIN);
        assert(result->GetInputSet().size() == 2);
    });
}

// This benchmark is based on a UTXO pool composed of 400 UTXOs. The UTXOs are
// pseudorandomly generated to be of the four relevant output types P2PKH,
// P2SH-P2WPKH, P2WPKH, and P2TR UTXOs, and fall in the range of 10'000 sats to
// 1 ₿ with larger amounts being more likely.
// This UTXO pool is used to run coin selection for 100 pseudorandom selection
// targets from 0.1–2 ₿. Altogether, this gives us a deterministic benchmark
// with a hopefully somewhat representative coin selection scenario.
static void CoinSelectionOnDiverseWallet(benchmark::Bench& bench)
{
    NodeContext node;
    auto chain = interfaces::MakeChain(node);
    CWallet wallet(chain.get(), "", CreateMockableWalletDatabase());
    std::vector<std::unique_ptr<CWalletTx>> wtxs;
    LOCK(wallet.cs_wallet);

    // Use arbitrary static seed for generating a pseudorandom scenario
    uint256 arb_seed = uint256("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    FastRandomContext det_rand{arb_seed};

    // Add coins.
    for (int i = 0; i < 400; ++i) {
        int x{det_rand.randrange(100)};
        if (x < 50) {
            // 0.0001–0.001 COIN
            addCoin(det_rand.randrange(90'000) + 10'000, wallet, wtxs);
        } else if (x < 75)  {
            // 0.001–0.01 COIN
            addCoin(det_rand.randrange(900'000) + 100'000, wallet, wtxs);
        } else if (x < 95) {
            // 0.01–0.1 COIN
            addCoin(det_rand.randrange(9'000'000) + 1'000'000, wallet, wtxs);
        } else {
            // 0.1–1 COIN
            addCoin(det_rand.randrange(90'000'000) + 10'000'000, wallet, wtxs);
        }
    }

    // Create coins
    wallet::CoinsResult available_coins;
    for (const auto& wtx : wtxs) {
        const auto txout = wtx->tx->vout.at(0);
        int y{det_rand.randrange(100)};
        if (y < 35) {
            available_coins.coins[OutputType::LEGACY].emplace_back(COutPoint(wtx->GetHash(), 0), txout, /*depth=*/6 * 24, CalculateMaximumSignedInputSize(txout, &wallet, /*coin_control=*/nullptr), /*spendable=*/true, /*solvable=*/true, /*safe=*/true, wtx->GetTxTime(), /*from_me=*/true, /*fees=*/ 0);
        } else if (y < 55) {
            available_coins.coins[OutputType::P2SH_SEGWIT].emplace_back(COutPoint(wtx->GetHash(), 0), txout, /*depth=*/6 * 24, CalculateMaximumSignedInputSize(txout, &wallet, /*coin_control=*/nullptr), /*spendable=*/true, /*solvable=*/true, /*safe=*/true, wtx->GetTxTime(), /*from_me=*/true, /*fees=*/ 0);
        } else if (y < 90) {
            available_coins.coins[OutputType::BECH32].emplace_back(COutPoint(wtx->GetHash(), 0), txout, /*depth=*/6 * 24, CalculateMaximumSignedInputSize(txout, &wallet, /*coin_control=*/nullptr), /*spendable=*/true, /*solvable=*/true, /*safe=*/true, wtx->GetTxTime(), /*from_me=*/true, /*fees=*/ 0);
        } else {
            available_coins.coins[OutputType::BECH32M].emplace_back(COutPoint(wtx->GetHash(), 0), txout, /*depth=*/6 * 24, CalculateMaximumSignedInputSize(txout, &wallet, /*coin_control=*/nullptr), /*spendable=*/true, /*solvable=*/true, /*safe=*/true, wtx->GetTxTime(), /*from_me=*/true, /*fees=*/ 0);
        }
    }

    std::vector<CAmount> targets;
    for (int i = 0; i < 100; ++i) {
        // 0.1–1 COIN
        targets.push_back(det_rand.randrange(90'000'000) + 10'000'000);
    }

    // Allow actual randomness for selection
    FastRandomContext rand{};
    const CoinEligibilityFilter filter_standard(1, 6, 0);
    const CoinSelectionParams coin_selection_params{
        rand,
        /*change_output_size=*/ 31,
        /*change_spend_size=*/ 68,
        /*min_change_target=*/ CHANGE_LOWER,
        /*effective_feerate=*/ CFeeRate(20'000),
        /*long_term_feerate=*/ CFeeRate(10'000),
        /*discard_feerate=*/ CFeeRate(3000),
        /*tx_noinputs_size=*/ 72,
        /*avoid_partial=*/ false,
    };
    auto group = wallet::GroupOutputs(wallet, available_coins, coin_selection_params, {{filter_standard}})[filter_standard];

    bench.run([&] {
        for (const auto& target : targets) {
            auto result = AttemptSelection(wallet.chain(), target, group, coin_selection_params, /*allow_mixed_output_types=*/true);
            assert(result);
            assert(result->GetSelectedValue() >= target);
        }
    });
}

// Copied from src/wallet/test/coinselector_tests.cpp
static void add_coin(const CAmount& nValue, int nInput, std::vector<OutputGroup>& set)
{
    CMutableTransaction tx;
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    COutput output(COutPoint(tx.GetHash(), nInput), tx.vout.at(nInput), /*depth=*/ 0, /*input_bytes=*/ -1, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, /*time=*/ 0, /*from_me=*/ true, /*fees=*/ 0);
    set.emplace_back();
    set.back().Insert(std::make_shared<COutput>(output), /*ancestors=*/ 0, /*descendants=*/ 0);
}
// Copied from src/wallet/test/coinselector_tests.cpp
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
    // Setup
    std::vector<OutputGroup> utxo_pool;

    bench.run([&] {
        // Benchmark
        CAmount target = make_hard_case(17, utxo_pool);
        SelectCoinsBnB(utxo_pool, target, 0, MAX_STANDARD_TX_WEIGHT); // Should exhaust

        // Cleanup
        utxo_pool.clear();
    });
}

BENCHMARK(KnapsackWorstCase, benchmark::PriorityLevel::HIGH);
BENCHMARK(CoinSelectionOnDiverseWallet, benchmark::PriorityLevel::HIGH);
BENCHMARK(BnBExhaustion, benchmark::PriorityLevel::HIGH);
