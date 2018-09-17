// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <coins.h>
#include <policy/policy.h>
#include <wallet/crypter.h>

#include <vector>

// FIXME: Dedup with SetupDummyInputs in test/transaction_tests.cpp.
//
// Helper: create two dummy transactions, each with
// two outputs.  The first has 11 and 50 COIN outputs
// paid to a TX_PUBKEY, the second 21 and 22 COIN outputs
// paid to a TX_PUBKEYHASH.
//
static std::vector<CMutableTransaction>
SetupDummyInputs(CBasicKeyStore& keystoreRet, CCoinsViewCache& coinsRet)
{
    std::vector<CMutableTransaction> dummyTransactions;
    dummyTransactions.resize(2);

    // Add some keys to the keystore:
    CKey key[4];
    for (int i = 0; i < 4; i++) {
        key[i].MakeNewKey(i % 2);
        keystoreRet.AddKey(key[i]);
    }

    // Create some dummy input transactions
    dummyTransactions[0].vout.resize(2);
    dummyTransactions[0].vout[0].nValue = 11 * COIN;
    dummyTransactions[0].vout[0].scriptPubKey << ToByteVector(key[0].GetPubKey()) << OP_CHECKSIG;
    dummyTransactions[0].vout[1].nValue = 50 * COIN;
    dummyTransactions[0].vout[1].scriptPubKey << ToByteVector(key[1].GetPubKey()) << OP_CHECKSIG;
    AddCoins(coinsRet, dummyTransactions[0], 0);

    dummyTransactions[1].vout.resize(2);
    dummyTransactions[1].vout[0].nValue = 21 * COIN;
    dummyTransactions[1].vout[0].scriptPubKey = GetScriptForDestination(key[2].GetPubKey().GetID());
    dummyTransactions[1].vout[1].nValue = 22 * COIN;
    dummyTransactions[1].vout[1].scriptPubKey = GetScriptForDestination(key[3].GetPubKey().GetID());
    AddCoins(coinsRet, dummyTransactions[1], 0);

    return dummyTransactions;
}

// Microbenchmark for simple accesses to a CCoinsViewCache database. Note from
// laanwj, "replicating the actual usage patterns of the client is hard though,
// many times micro-benchmarks of the database showed completely different
// characteristics than e.g. reindex timings. But that's not a requirement of
// every benchmark."
// (https://github.com/bitcoin/bitcoin/issues/7883#issuecomment-224807484)
static void CCoinsCaching(benchmark::State& state)
{
    CBasicKeyStore keystore;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    std::vector<CMutableTransaction> dummyTransactions = SetupDummyInputs(keystore, coins);

    CMutableTransaction t1;
    t1.vin.resize(3);
    t1.vin[0].prevout.hash = dummyTransactions[0].GetHash();
    t1.vin[0].prevout.n = 1;
    t1.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
    t1.vin[1].prevout.hash = dummyTransactions[1].GetHash();
    t1.vin[1].prevout.n = 0;
    t1.vin[1].scriptSig << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4);
    t1.vin[2].prevout.hash = dummyTransactions[1].GetHash();
    t1.vin[2].prevout.n = 1;
    t1.vin[2].scriptSig << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4);
    t1.vout.resize(2);
    t1.vout[0].nValue = 90 * COIN;
    t1.vout[0].scriptPubKey << OP_1;

    // Benchmark.
    while (state.KeepRunning()) {
        bool success = AreInputsStandard(t1, coins);
        assert(success);
        CAmount value = coins.GetValueIn(t1);
        assert(value == (50 + 21 + 22) * COIN);
    }
}

BENCHMARK(CCoinsCaching, 170 * 1000);
