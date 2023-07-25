// Copyright (c) 2011-2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <deque>
#include <kernel/mempool_entry.h>
#include <node/mini_miner.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>

static void MiniMinerBench(benchmark::Bench& bench)
{

    FastRandomContext det_rand{true};

    int max_cluster_size = 500;
    if (bench.complexityN() > 1) {
        max_cluster_size = static_cast<int>(bench.complexityN());
    }

    std::deque<COutPoint> available_coins;
    for (uint32_t i = 0; i < uint32_t{100}; ++i) {
        available_coins.push_back(COutPoint{uint256::ZERO, i});
    }

    CTxMemPool pool{CTxMemPool::Options{}};
    std::vector<COutPoint> outpoints;
    LOCK2(::cs_main, pool.cs);
    // Cluster size cannot exceed 500, or specified amount
    for (uint32_t count = max_cluster_size; !available_coins.empty() && count; --count)
    {
        CMutableTransaction mtx = CMutableTransaction();
        const size_t num_inputs = det_rand.randrange(available_coins.size()) + 1;
        const size_t num_outputs = det_rand.randrange(50) + 1;
        for (size_t n{0}; n < num_inputs; ++n) {
            auto prevout = available_coins.front();
            mtx.vin.push_back(CTxIn(prevout, CScript()));
            available_coins.pop_front();
        }
        for (uint32_t n{0}; n < num_outputs; ++n) {
            mtx.vout.push_back(CTxOut(100, P2WSH_OP_TRUE));
        }
        CTransactionRef tx = MakeTransactionRef(mtx);
        TestMemPoolEntryHelper entry;
        const CAmount fee{(CAmount) det_rand.randrange(MAX_MONEY/100000)};
        assert(MoneyRange(fee));
        pool.addUnchecked(entry.Fee(fee).FromTx(tx));

        // All outputs are available to spend
        for (uint32_t n{0}; n < num_outputs; ++n) {
            if (det_rand.randbool()) {
                available_coins.push_back(COutPoint{tx->GetHash(), n});
            }
        }

        if (det_rand.randbool() && !tx->vout.empty()) {
            // Add outpoint from this tx (may or not be spent by a later tx)
            outpoints.push_back(COutPoint{tx->GetHash(),
                                          (uint32_t) det_rand.randrange(tx->vout.size())});
        } else {
            // Add some random outpoint (will be interpreted as confirmed or not yet submitted
            // to mempool).
            COutPoint outpoint;
            outpoint.hash = det_rand.rand256();
            outpoint.n = det_rand.randrange(50);
            if (std::find(outpoints.begin(), outpoints.end(), outpoint) == outpoints.end()) {
                outpoints.push_back(outpoint);
            }
        }

    }

    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {
        node::MiniMiner mini_miner{pool, outpoints};
    });
}

BENCHMARK(MiniMinerBench, benchmark::PriorityLevel::HIGH);
