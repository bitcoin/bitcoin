// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <kernel/mempool_entry.h>
#include <policy/policy.h>
#include <test/util/setup_common.h>
#include <txmempool.h>


static void AddTx(const CTransactionRef& tx, const CAmount& nFee, CTxMemPool& pool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, pool.cs)
{
    int64_t nTime = 0;
    unsigned int nHeight = 1;
    bool spendsCoinbase = false;
    unsigned int sigOpCost = 4;
    LockPoints lp;
    pool.addUnchecked(CTxMemPoolEntry(
        tx, nFee, nTime, nHeight,
        spendsCoinbase, sigOpCost, lp));
}

void add_parents_child(std::vector<CTransactionRef>& txns, size_t package_size, size_t set_num)
{
    // N+1 N-input-N-output txns, in a parents-and-child package
    // Where each tx takes an input from all prior txns
    const size_t num_puts = std::max(package_size - 1, (size_t)1);
    const size_t num_txns = package_size;
    for (size_t txn_index=0; txn_index<num_txns; txn_index++) {
        CMutableTransaction tx = CMutableTransaction();
        tx.vin.resize(num_puts);
        tx.vout.resize(num_puts);
        for (size_t put_index=0; put_index<num_puts; put_index++) {
            tx.vin[put_index].scriptSig = CScript() << CScriptNum(txn_index);
            tx.vin[put_index].scriptWitness.stack.push_back({1});
            tx.vout[put_index].scriptPubKey = CScript() << CScriptNum(set_num); // Ensures unique txid with set_num
            tx.vout[put_index].nValue = txn_index;

            if (put_index < txn_index) {
                // We spend put_index's txn's in the next available slot for each previous transaction
                assert(put_index + txn_index >= 1);
                tx.vin[put_index].prevout = COutPoint(txns[put_index]->GetHash(), put_index + txn_index - 1);
            }
        }
        txns.push_back(MakeTransactionRef(tx));
    }
}

static void MempoolEvictionInner(benchmark::Bench& bench, size_t num_packages, size_t package_size)
{
    const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();

    std::vector<CTransactionRef> txns;

    for (size_t i=0; i<num_packages; i++) {
        add_parents_child(txns, package_size, i);
    }

    CTxMemPool& pool = *Assert(testing_setup->m_node.mempool);
    LOCK2(cs_main, pool.cs);
    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {
        for (size_t i{0}; i < txns.size(); ++i) {
            // Monotonically decreasing fees
            AddTx(txns[i], (txns.size() - i)*1000LL, pool);
        }
        assert(pool.size() == txns.size());
        pool.TrimToSize(pool.DynamicMemoryUsage() - 1);
        assert(pool.size() == txns.size() - 1);
        pool.TrimToSize(0);
        assert(pool.size() == 0);
    });
}

static void MempoolConstructionInner(benchmark::Bench& bench, size_t num_packages, size_t package_size)
{
    const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();

    std::vector<CTransactionRef> txns;

    for (size_t i=0; i<num_packages; i++) {
        add_parents_child(txns, package_size, i);
    }

    /* Now we're ready */
    CTxMemPool& pool = *Assert(testing_setup->m_node.mempool);
    LOCK2(cs_main, pool.cs);
    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {
        for (size_t i=0; i<txns.size(); i++) {
            AddTx(txns[i], 1000LL, pool);
        }
    });
}

static void MempoolEviction(benchmark::Bench& bench)
{
    size_t package_size = 25;
    if (bench.complexityN() > 1) {
        package_size = static_cast<size_t>(bench.complexityN());
    }
    MempoolEvictionInner(bench, 2500 / package_size, package_size);
}

static void MempoolConstruction(benchmark::Bench& bench)
{
    size_t package_size = 25;
    if (bench.complexityN() > 1) {
        package_size = static_cast<size_t>(bench.complexityN());
    }
    MempoolConstructionInner(bench, 2500 / package_size, package_size);
}

BENCHMARK(MempoolEviction, benchmark::PriorityLevel::HIGH);
BENCHMARK(MempoolConstruction, benchmark::PriorityLevel::HIGH);
