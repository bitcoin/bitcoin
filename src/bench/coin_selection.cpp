// Copyright (c) 2012-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <interfaces/chain.h>
#include <node/context.h>
#include <wallet/coinselection.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>

#include <set>

static void addCoin(const CAmount& nValue, const CWallet& wallet, std::vector<std::unique_ptr<CWalletTx>>& wtxs)
{
    static int nextLockTime = 0;
    CMutableTransaction tx;
    tx.nLockTime = nextLockTime++; // so all transactions get different hashes
    tx.vout.resize(1);
    tx.vout[0].nValue = nValue;
    wtxs.push_back(std::make_unique<CWalletTx>(MakeTransactionRef(std::move(tx))));
}

// Simple benchmark for wallet coin selection. Note that it maybe be necessary
// to build up more complicated scenarios in order to get meaningful
// measurements of performance. From laanwj, "Wallet coin selection is probably
// the hardest, as you need a wider selection of scenarios, just testing the
// same one over and over isn't too useful. Generating random isn't useful
// either for measurements."
// (https://github.com/bitcoin/bitcoin/issues/7883#issuecomment-224807484)
static void CoinSelection(benchmark::Bench& bench)
{
    NodeContext node;
    auto chain = interfaces::MakeChain(node);
    CWallet wallet(chain.get(), "", CreateDummyWalletDatabase());
    wallet.SetupLegacyScriptPubKeyMan();
    std::vector<std::unique_ptr<CWalletTx>> wtxs;
    LOCK(wallet.cs_wallet);

    // Add coins.
    for (int i = 0; i < 1000; ++i) {
        addCoin(1000 * COIN, wallet, wtxs);
    }
    addCoin(3 * COIN, wallet, wtxs);

    // Create coins
    std::vector<COutput> coins;
    for (const auto& wtx : wtxs) {
        coins.emplace_back(wallet, *wtx, 0 /* iIn */, 6 * 24 /* nDepthIn */, true /* spendable */, true /* solvable */, true /* safe */);
    }

    const CoinEligibilityFilter filter_standard(1, 6, 0);
    const CoinSelectionParams coin_selection_params(/* change_output_size= */ 34,
                                                    /* change_spend_size= */ 148, /* effective_feerate= */ CFeeRate(0),
                                                    /* long_term_feerate= */ CFeeRate(0), /* discard_feerate= */ CFeeRate(0),
                                                    /* tx_no_inputs_size= */ 0, /* avoid_partial= */ false);
    bench.run([&] {
        std::set<CInputCoin> setCoinsRet;
        CAmount nValueRet;
        bool success = AttemptSelection(wallet, 1003 * COIN, filter_standard, coins, setCoinsRet, nValueRet, coin_selection_params);
        assert(success);
        assert(nValueRet == 1003 * COIN);
        assert(setCoinsRet.size() == 2);
    });
}

typedef std::set<CInputCoin> CoinSet;
static NodeContext testNode;
static auto testChain = interfaces::MakeChain(testNode);
static CWallet testWallet(testChain.get(), "", CreateDummyWalletDatabase());
std::vector<std::unique_ptr<CWalletTx>> wtxn;

// Copied from src/wallet/test/coinselector_tests.cpp
static void add_coin(const CAmount& nValue, int nInput, std::vector<OutputGroup>& set)
{
    CMutableTransaction tx;
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    std::unique_ptr<CWalletTx> wtx = std::make_unique<CWalletTx>(MakeTransactionRef(std::move(tx)));
    set.emplace_back();
    set.back().Insert(COutput(testWallet, *wtx, nInput, 0, true, true, true).GetInputCoin(), 0, true, 0, 0, false);
    wtxn.emplace_back(std::move(wtx));
}
// Copied from src/wallet/test/coinselector_tests.cpp
static CAmount make_hard_case(int utxos, std::vector<OutputGroup>& utxo_pool)
{
    utxo_pool.clear();
    CAmount target = 0;
    for (int i = 0; i < utxos; ++i) {
        target += (CAmount)1 << (utxos+i);
        add_coin((CAmount)1 << (utxos+i), 2*i, utxo_pool);
        add_coin(((CAmount)1 << (utxos+i)) + ((CAmount)1 << (utxos-1-i)), 2*i + 1, utxo_pool);
    }
    return target;
}

static void BnBExhaustion(benchmark::Bench& bench)
{
    // Setup
    testWallet.SetupLegacyScriptPubKeyMan();
    std::vector<OutputGroup> utxo_pool;
    CoinSet selection;
    CAmount value_ret = 0;

    bench.run([&] {
        // Benchmark
        CAmount target = make_hard_case(17, utxo_pool);
        SelectCoinsBnB(utxo_pool, target, 0, selection, value_ret); // Should exhaust

        // Cleanup
        utxo_pool.clear();
        selection.clear();
    });
}

BENCHMARK(CoinSelection);
BENCHMARK(BnBExhaustion);
