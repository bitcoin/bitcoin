// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/nanobench.h>

#include <net.h>
#include <net_processing.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/script.h>
#include <sync.h>

#include <vector>

bool AddOrphanTx(const CTransactionRef& tx, NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);
unsigned int LimitOrphanTxSize(unsigned int nMaxOrphans);

static void OrphanTxPool(benchmark::Bench& bench)
{
    FastRandomContext rand;
    std::vector<CTransactionRef> txs;
    for (unsigned int i = 0; i < DEFAULT_MAX_ORPHAN_TRANSACTIONS; ++i) {
        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].prevout.n = 0;
        tx.vin[0].prevout.hash = rand.rand256();
        tx.vin[0].scriptSig << OP_1;
        tx.vout.resize(1);
        tx.vout[0].nValue = 0;
        tx.vout[0].scriptPubKey << OP_1;
        txs.emplace_back(MakeTransactionRef(tx));
    }

    bench.batch(txs.size()).unit("byte").run([&] {
        WITH_LOCK(g_cs_orphans, for (const auto& tx : txs) { AddOrphanTx(tx, 0); });
        LimitOrphanTxSize(0);
    });
}

BENCHMARK(OrphanTxPool);
