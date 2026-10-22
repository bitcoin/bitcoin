// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <coins.h>
#include <consensus/amount.h>
#include <consensus/validation.h>
#include <key.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <test/util/transaction_utils.h>
#include <util/check.h>

#include <array>
#include <span>
#include <vector>

// Microbenchmark for simple accesses to a CCoinsViewCache database. Note from
// laanwj, "replicating the actual usage patterns of the client is hard though,
// many times micro-benchmarks of the database showed completely different
// characteristics than e.g. reindex timings. But that's not a requirement of
// every benchmark."
// (https://github.com/bitcoin/bitcoin/issues/7883#issuecomment-224807484)
static void CCoinsCaching(benchmark::Bench& bench)
{
    ECC_Context ecc_context{};

    FillableSigningProvider keystore;
    CCoinsViewCache coins{&CoinsViewEmpty::Get()};
    std::vector<CMutableTransaction> dummyTransactions =
        SetupDummyInputs(keystore, coins, {11 * COIN, 50 * COIN, 21 * COIN, 22 * COIN});

    CMutableTransaction t1;
    t1.vin = {
        CTxIn{COutPoint{dummyTransactions[0].GetHash(), 1}, CScript{} << std::vector<unsigned char>(65, 0)},
        CTxIn{COutPoint{dummyTransactions[1].GetHash(), 0}, CScript{} << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4)},
        CTxIn{COutPoint{dummyTransactions[1].GetHash(), 1}, CScript{} << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4)},
    };
    t1.vout = {
        CTxOut{90 * COIN, CScript{} << OP_1},
        CTxOut{},
    };

    // Benchmark.
    const CTransaction tx_1(t1);
    bench.run([&] {
        assert(ValidateInputsStandardness(tx_1, coins).IsValid());
    });
}

BENCHMARK(CCoinsCaching);
