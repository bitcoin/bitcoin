// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <validation.h>

#include <array>
#include <vector>
#include <script/signingprovider.h>
#include <test/util/transaction_utils.h>



static void BenchCheckInputScripts(benchmark::Bench& bench)
{
    ECC_Start();

    FillableSigningProvider keystore;

    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    std::vector<CMutableTransaction> dummyTransactions =
        SetupDummyInputs(keystore, coins, {11 * COIN, 50 * COIN, 21 * COIN, 22 * COIN});

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

    const CTransactionRef tx1_r{MakeTransactionRef(t1)};

    TxValidationState state_dummy;


    PrecomputedTransactionData m_precomputed_txdata;

    bench.unit("script").run([&] {
        CheckInputScripts(*tx1_r, state_dummy, coins, 0, true, true, m_precomputed_txdata);
    });
    ECC_Stop();
}


BENCHMARK(BenchCheckInputScripts, benchmark::PriorityLevel::HIGH);

