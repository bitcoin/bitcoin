// Copyright (c) 2012-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <wallet/wallet.h>
#include <wallet/coinselection.h>

#include <set>

static void addCoin(const CAmount& nValue, const CWallet& wallet, std::vector<COutput>& vCoins)
{
    int nInput = 0;

    static int nextLockTime = 0;
    CMutableTransaction tx;
    tx.nLockTime = nextLockTime++; // so all transactions get different hashes
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    CWalletTx* wtx = new CWalletTx(&wallet, MakeTransactionRef(std::move(tx)));

    int nAge = 6 * 24;
    COutput output(wtx, nInput, nAge, true /* spendable */, true /* solvable */, true /* safe */);
    vCoins.push_back(output);
}

// Simple benchmark for wallet coin selection. Note that it maybe be necessary
// to build up more complicated scenarios in order to get meaningful
// measurements of performance. From laanwj, "Wallet coin selection is probably
// the hardest, as you need a wider selection of scenarios, just testing the
// same one over and over isn't too useful. Generating random isn't useful
// either for measurements."
// (https://github.com/bitcoin/bitcoin/issues/7883#issuecomment-224807484)
static void CoinSelection(benchmark::State& state)
{
    const CWallet wallet("dummy", WalletDatabase::CreateDummy());
    LOCK(wallet.cs_wallet);

    // Add coins.
    std::vector<COutput> vCoins;
    for (int i = 0; i < 1000; ++i) {
        addCoin(1000 * COIN, wallet, vCoins);
    }
    addCoin(3 * COIN, wallet, vCoins);

    const CoinEligibilityFilter filter_standard(1, 6, 0);
    const CoinSelectionParams coin_selection_params(true, 34, 148, CFeeRate(0), 0);
    while (state.KeepRunning()) {
        std::set<CInputCoin> setCoinsRet;
        CAmount nValueRet;
        bool bnb_used;
        bool success = wallet.SelectCoinsMinConf(1003 * COIN, filter_standard, vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used);
        assert(success);
        assert(nValueRet == 1003 * COIN);
        assert(setCoinsRet.size() == 2);
    }
}

typedef std::set<CInputCoin> CoinSet;

// Copied from src/wallet/test/coinselector_tests.cpp
static void add_coin(const CAmount& nValue, int nInput, std::vector<CInputCoin>& set)
{
    CMutableTransaction tx;
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    set.emplace_back(MakeTransactionRef(tx), nInput);
}
// Copied from src/wallet/test/coinselector_tests.cpp
static CAmount make_hard_case(int utxos, std::vector<CInputCoin>& utxo_pool)
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

static void BnBExhaustion(benchmark::State& state)
{
    // Setup
    std::vector<CInputCoin> utxo_pool;
    CoinSet selection;
    CAmount value_ret = 0;
    CAmount not_input_fees = 0;

    while (state.KeepRunning()) {
        // Benchmark
        CAmount target = make_hard_case(17, utxo_pool);
        SelectCoinsBnB(utxo_pool, target, 0, selection, value_ret, not_input_fees); // Should exhaust

        // Cleanup
        utxo_pool.clear();
        selection.clear();
    }
}

BENCHMARK(CoinSelection, 650);
BENCHMARK(BnBExhaustion, 650);
