// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"
#include "wallet/wallet.h"

#include <boost/foreach.hpp>
#include <set>

using namespace std;

static void addCoin(const CAmount& nValue, const CWallet& wallet, vector<COutput>& vCoins)
{
    int nInput = 0;

    static int nextLockTime = 0;
    CMutableTransaction tx;
    tx.nLockTime = nextLockTime++; // so all transactions get different hashes
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    CWalletTx* wtx = new CWalletTx(&wallet, tx);

    int nAge = 6 * 24;
    COutput output(wtx, nInput, nAge, true, true);
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
    const CWallet wallet;
    vector<COutput> vCoins;
    LOCK(wallet.cs_wallet);

    while (state.KeepRunning()) {
        // Empty wallet.
        BOOST_FOREACH (COutput output, vCoins)
            delete output.tx;
        vCoins.clear();

        // Add coins.
        for (int i = 0; i < 1000; i++)
            addCoin(1000 * COIN, wallet, vCoins);
        addCoin(3 * COIN, wallet, vCoins);

        set<pair<const CWalletTx*, unsigned int> > setCoinsRet;
        CAmount nValueRet;
        bool success = wallet.SelectCoinsMinConf(1003 * COIN, 1, 6, vCoins, setCoinsRet, nValueRet);
        assert(success);
        assert(nValueRet == 1003 * COIN);
        assert(setCoinsRet.size() == 2);
    }
}

BENCHMARK(CoinSelection);
