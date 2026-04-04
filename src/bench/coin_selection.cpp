// Copyright (c) 2012-present The Bitcoin Core developers
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
    NodeContext node;
    auto chain = interfaces::MakeChain(node);
    CWallet wallet(chain.get(), "", CreateMockableWalletDatabase());
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
    const CoinSelectionParams coin_selection_params{
        det_rand,
        /*change_output_size=*/31,
        /*change_spend_size=*/68,
        /*min_change_target=*/CHANGE_LOWER,
        /*effective_feerate=*/CFeeRate(20'000),
        /*long_term_feerate=*/CFeeRate(10'000),
        /*discard_feerate=*/CFeeRate(3000),
        /*tx_noinputs_size=*/72,
        /*avoid_partial=*/false,
    };
    auto group = wallet::GroupOutputs(wallet, available_coins, coin_selection_params, {{filter_standard}})[filter_standard];

    constexpr size_t NUM_TARGETS{10};
    std::vector<CAmount> targets;
    targets.reserve(NUM_TARGETS);
    for (size_t i{0}; i < NUM_TARGETS; ++i) {
        targets.push_back(10'000'000 + det_rand.randrange(90'000'000));
    }

    bench.batch(NUM_TARGETS).run([&] {
        for (const auto& target : targets) {
            auto result = AttemptSelection(wallet.chain(), target, group, coin_selection_params, /*allow_mixed_output_types=*/true);
            assert(result);
            assert(result->GetSelectedValue() >= target);
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

    CAmount target = make_hard_case(17, utxo_pool);
    bench.run([&] {
        (void)SelectCoinsBnB(utxo_pool, target, /*cost_of_change=*/0, MAX_STANDARD_TX_WEIGHT); // Should exhaust
    });
}

BENCHMARK(CoinSelection);
BENCHMARK(BnBExhaustion);
