// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <wallet/wallet.h>
#include <wallet/coinselection.h>

#include <set>

static void addCoin(const CAmount& nValue, const CWallet& wallet, std::vector<OutputGroup>& groups)
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
    groups.emplace_back(output.GetInputCoin(), 0, false, 0, 0);
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
    const CWallet wallet(WalletLocation(), WalletDatabase::CreateDummy());
    std::vector<OutputGroup> groups;
    LOCK(wallet.cs_wallet);

    while (state.KeepRunning()) {
        // Add coins.
        for (int i = 0; i < 1000; i++)
            addCoin(1000 * COIN, wallet, groups);
        addCoin(3 * COIN, wallet, groups);

        std::set<CInputCoin> setCoinsRet;
        CAmount nValueRet;
        bool bnb_used;
        CoinEligibilityFilter filter_standard(1, 6, 0);
        CoinSelectionParams coin_selection_params(false, 34, 148, CFeeRate(0), 0);
        bool success = wallet.SelectCoinsMinConf(1003 * COIN, filter_standard, groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used)
                       || wallet.SelectCoinsMinConf(1003 * COIN, filter_standard, groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used);
        assert(success);
        assert(nValueRet == 1003 * COIN);
        assert(setCoinsRet.size() == 2);

        groups.clear();
    }
}

typedef std::set<CInputCoin> CoinSet;
static const CWallet testWallet(WalletLocation(), WalletDatabase::CreateDummy());
std::vector<std::unique_ptr<CWalletTx>> wtxn;

// Copied from src/wallet/test/coinselector_tests.cpp
static void add_coin(const CAmount& nValue, int nInput, std::vector<OutputGroup>& set)
{
    CMutableTransaction tx;
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    std::unique_ptr<CWalletTx> wtx(new CWalletTx(&testWallet, MakeTransactionRef(std::move(tx))));
    set.emplace_back(COutput(wtx.get(), nInput, 0, true, true, true).GetInputCoin(), 0, true, 0, 0);
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

static void BnBExhaustion(benchmark::State& state)
{
    // Setup
    std::vector<OutputGroup> utxo_pool;
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
