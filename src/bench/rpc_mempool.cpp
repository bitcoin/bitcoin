// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/amount.h>
#include <kernel/cs_main.h>
#include <primitives/transaction.h>
#include <rpc/mempool.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <univalue.h>
#include <util/check.h>

#include <cstddef>
#include <memory>
#include <vector>


static void AddTx(const CTransactionRef& tx, const CAmount& fee, CTxMemPool& pool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, pool.cs)
{
    LockPoints lp;
    TryAddToMempool(pool, CTxMemPoolEntry(tx, fee, /*time=*/0, /*entry_height=*/1, /*entry_sequence=*/0, /*spends_coinbase=*/false, /*sigops_cost=*/4, lp));
}

static void RpcMempool(benchmark::Bench& bench)
{
    const auto testing_setup = MakeNoLogFileContext<const ChainTestingSetup>(ChainType::MAIN);
    CTxMemPool& pool = *Assert(testing_setup->m_node.mempool);
    LOCK2(cs_main, pool.cs);

    for (int i = 0; i < 1000; ++i) {
        CMutableTransaction tx = CMutableTransaction();
        tx.vin.resize(1);
        tx.vin[0].scriptSig = CScript() << OP_1;
        tx.vin[0].scriptWitness.stack.push_back({1});
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey = CScript() << OP_1 << OP_EQUAL;
        tx.vout[0].nValue = i;
        const CTransactionRef tx_r{MakeTransactionRef(tx)};
        AddTx(tx_r, /*fee=*/i, pool);
    }

    bench.run([&] {
        (void)MempoolToJSON(pool, /*verbose=*/true);
    });
}

BENCHMARK(RpcMempool);

/** getrawmempool without verbose on chains of CHAIN_LEN small transactions that each spend the previous one.
 *  The fees are not monotonic along a chain, so that each chain is split into several chunks. */
static void RpcMempoolTxids(benchmark::Bench& bench)
{
    static constexpr int NUM_TXS{10000};
    static constexpr int CHAIN_LEN{25};
    static constexpr CAmount FEE_STEP{7919};
    static constexpr CAmount FEE_RANGE{5000};

    const auto testing_setup = MakeNoLogFileContext<const ChainTestingSetup>(ChainType::MAIN);
    CTxMemPool& pool = *Assert(testing_setup->m_node.mempool);
    LOCK2(cs_main, pool.cs);

    CTransactionRef prev;
    for (int i = 0; i < NUM_TXS; ++i) {
        CMutableTransaction tx;
        tx.vin.resize(1);
        if (i % CHAIN_LEN != 0) tx.vin[0].prevout = COutPoint{prev->GetHash(), 0};
        tx.vin[0].scriptSig = CScript() << OP_1;
        tx.vin[0].scriptWitness.stack.push_back({1});
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey = CScript() << OP_1 << OP_EQUAL;
        tx.vout[0].nValue = i;
        prev = MakeTransactionRef(tx);
        AddTx(prev, /*fee=*/(i * FEE_STEP) % FEE_RANGE, pool);
    }
    Assert(pool.size() == static_cast<size_t>(NUM_TXS));

    bench.run([&] {
        (void)MempoolToJSON(pool, /*verbose=*/false);
    });
}

BENCHMARK(RpcMempoolTxids);
