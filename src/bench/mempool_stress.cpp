// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <policy/policy.h>
#include <txmempool.h>

#include <vector>

static void AddTx(const CTransactionRef& tx, CTxMemPool& pool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, pool.cs)
{
    int64_t nTime = 0;
    unsigned int nHeight = 1;
    bool spendsCoinbase = false;
    unsigned int sigOpCost = 4;
    LockPoints lp;
    pool.addUnchecked(CTxMemPoolEntry(tx, 1000, nTime, nHeight, spendsCoinbase, sigOpCost, lp));
}

struct Available {
    CTransactionRef ref;
    size_t vin_left{0};
    size_t tx_count;
    Available(CTransactionRef& ref, size_t tx_count) : ref(ref), tx_count(tx_count){}
};

static void ComplexMemPoolAsymptotic(benchmark::State& state, const std::vector<size_t>* asymptotic_factors)
{
    const std::vector<size_t> DEFAULTS {};
    const std::vector<size_t> scaling_factors = asymptotic_factors ? *asymptotic_factors : DEFAULTS;
    size_t CHILD_TXS = 800;
    size_t STARTING_TXS = 100;
    size_t DESCENDANTS_COUNT_MAX = 10;
    size_t ANCESTORS_COUNT_MAX = 10;
    if (scaling_factors.size() > 0) CHILD_TXS = scaling_factors[0];
    if (scaling_factors.size() > 1) STARTING_TXS = scaling_factors[1];
    if (scaling_factors.size() > 2) DESCENDANTS_COUNT_MAX = scaling_factors[2];
    if (scaling_factors.size() > 3) ANCESTORS_COUNT_MAX = scaling_factors[3];


    FastRandomContext det_rand{true};
    std::vector<Available> available_coins;
    std::vector<CTransactionRef> ordered_coins;
    // Create some base transactions
    size_t tx_counter = 1;
    for (size_t x = 0; x < STARTING_TXS; ++x) {
        CMutableTransaction tx = CMutableTransaction();
        tx.vin.resize(1);
        tx.vin[0].scriptSig = CScript() << CScriptNum(tx_counter);
        tx.vin[0].scriptWitness.stack.push_back(CScriptNum(x).getvch());
        tx.vout.resize(det_rand.randrange(DESCENDANTS_COUNT_MAX)+2);
        for (auto& out : tx.vout) {
            out.scriptPubKey = CScript() << CScriptNum(tx_counter) << OP_EQUAL;
            out.nValue = 10 * COIN;
        }
        ordered_coins.emplace_back(MakeTransactionRef(tx));
        available_coins.emplace_back(ordered_coins.back(), tx_counter++);
    }
    for (size_t x = 0; x < CHILD_TXS && !available_coins.empty(); ++x) {
        CMutableTransaction tx = CMutableTransaction();
        size_t n_ancestors = det_rand.randrange(ANCESTORS_COUNT_MAX)+1;
        for (size_t ancestor = 0; ancestor < n_ancestors && !available_coins.empty(); ++ancestor){
            size_t idx = det_rand.randrange(available_coins.size());
            Available coin = available_coins[idx];
            uint256 hash = coin.ref->GetHash();
            // biased towards taking just one ancestor, but maybe more
            size_t n_to_take = det_rand.randrange(2) == 0 ? 1 : 1+det_rand.randrange(coin.ref->vout.size() - coin.vin_left);
            for (size_t i = 0; i < n_to_take; ++i) {
                tx.vin.emplace_back();
                tx.vin.back().prevout = COutPoint(hash, coin.vin_left++);
                tx.vin.back().scriptSig = CScript() << coin.tx_count;
                tx.vin.back().scriptWitness.stack.push_back(CScriptNum(coin.tx_count).getvch());
            }
            if (coin.vin_left == coin.ref->vin.size()) {
                coin = available_coins.back();
                available_coins.pop_back();
            }
            tx.vout.resize(det_rand.randrange(DESCENDANTS_COUNT_MAX)+2);
            for (auto& out : tx.vout) {
                out.scriptPubKey = CScript() << CScriptNum(tx_counter) << OP_EQUAL;
                out.nValue = 10 * COIN;
            }
        }
        ordered_coins.emplace_back(MakeTransactionRef(tx));
        available_coins.emplace_back(ordered_coins.back(), tx_counter++);
    }
    CTxMemPool pool;
    LOCK2(cs_main, pool.cs);
    while (state.KeepRunning()) {
        for (auto& tx : ordered_coins) {
            AddTx(tx, pool);
        }
        pool.TrimToSize(pool.DynamicMemoryUsage() * 3 / 4);
        pool.TrimToSize(GetVirtualTransactionSize(*ordered_coins.front()));
    }
}
static void ComplexMemPool(benchmark::State& state) {
    ComplexMemPoolAsymptotic(state, nullptr);
}

BENCHMARK(ComplexMemPool, 1);
BENCHMARK_ASYMPTOTE(ComplexMemPoolAsymptotic);
