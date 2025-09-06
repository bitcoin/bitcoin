// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/amount.h>
#include <kernel/cs_main.h>
#include <policy/ephemeral_policy.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/check.h>

#include <cstdint>
#include <memory>
#include <vector>


static void AddTx(const CTransactionRef& tx, CTxMemPool& pool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, pool.cs)
{
    int64_t nTime{0};
    unsigned int nHeight{1};
    uint64_t sequence{0};
    bool spendsCoinbase{false};
    unsigned int sigOpCost{4};
    uint64_t fee{0};
    LockPoints lp;
    TryAddToMempool(pool, CTxMemPoolEntry(TxGraph::Ref(),
        tx, fee, nTime, nHeight, sequence,
        spendsCoinbase, sigOpCost, lp));
}

static void MempoolCheckEphemeralSpends(benchmark::Bench& bench)
{
    const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();

    int number_outputs{1000};
    if (bench.complexityN() > 1) {
        number_outputs = static_cast<int>(bench.complexityN());
    }

    // Tx with many outputs
    CMutableTransaction tx1;
    tx1.vin.resize(1);
    tx1.vout.resize(number_outputs);
    for (size_t i = 0; i < tx1.vout.size(); i++) {
        tx1.vout[i].scriptPubKey = CScript();
        // Each output progressively larger
        tx1.vout[i].nValue = i * CENT;
    }

    const auto& parent_txid = tx1.GetHash();

    // Spends all outputs of tx1, other details don't matter
    CMutableTransaction tx2;
    tx2.vin.resize(tx1.vout.size());
    for (size_t i = 0; i < tx2.vin.size(); i++) {
        tx2.vin[0].prevout.hash = parent_txid;
        tx2.vin[0].prevout.n = i;
    }
    tx2.vout.resize(1);

    CTxMemPool& pool = *Assert(testing_setup->m_node.mempool);
    LOCK2(cs_main, pool.cs);
    // Create transaction references outside the "hot loop"
    const CTransactionRef tx1_r{MakeTransactionRef(tx1)};
    const CTransactionRef tx2_r{MakeTransactionRef(tx2)};

    AddTx(tx1_r, pool);

    uint32_t iteration{0};

    TxValidationState dummy_state;
    Wtxid dummy_wtxid;

    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {

        CheckEphemeralSpends({tx2_r}, /*dust_relay_rate=*/CFeeRate(iteration * COIN / 10), pool, dummy_state, dummy_wtxid);
        iteration++;
    });
}

BENCHMARK(MempoolCheckEphemeralSpends, benchmark::PriorityLevel::HIGH);
