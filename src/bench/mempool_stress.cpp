// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/amount.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/script.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <validation.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class CCoinsViewCache;

static void AddTx(const CTransactionRef& tx, CTxMemPool& pool, FastRandomContext& det_rand) EXCLUSIVE_LOCKS_REQUIRED(cs_main, pool.cs)
{
    int64_t nTime = 0;
    unsigned int nHeight = 1;
    uint64_t sequence = 0;
    bool spendsCoinbase = false;
    unsigned int sigOpCost = 4;
    LockPoints lp;
    AddToMempool(pool, CTxMemPoolEntry(tx, det_rand.randrange(10000)+1000, nTime, nHeight, sequence, spendsCoinbase, sigOpCost, lp));
}

struct Available {
    CTransactionRef ref;
    size_t vin_left{0};
    size_t tx_count;
    Available(CTransactionRef& ref, size_t tx_count) : ref(ref), tx_count(tx_count){}
};

// Create a cluster of transactions, randomly.
static std::vector<CTransactionRef> CreateCoinCluster(FastRandomContext& det_rand, int childTxs, int min_ancestors)
{
    std::vector<Available> available_coins;
    std::vector<CTransactionRef> ordered_coins;
    // Create some base transactions
    size_t tx_counter = 1;
    for (auto x = 0; x < 10; ++x) {
        CMutableTransaction tx = CMutableTransaction();
        tx.vin.resize(1);
        tx.vin[0].prevout = COutPoint(Txid::FromUint256(GetRandHash()), 1);
        tx.vin[0].scriptSig = CScript() << CScriptNum(tx_counter);
        tx.vin[0].scriptWitness.stack.push_back(CScriptNum(x).getvch());
        tx.vout.resize(det_rand.randrange(10)+2);
        for (auto& out : tx.vout) {
            out.scriptPubKey = CScript() << CScriptNum(tx_counter) << OP_EQUAL;
            out.nValue = 10 * COIN;
        }
        ordered_coins.emplace_back(MakeTransactionRef(tx));
        available_coins.emplace_back(ordered_coins.back(), tx_counter++);
    }
    for (auto x = 0; x < childTxs && !available_coins.empty(); ++x) {
        CMutableTransaction tx = CMutableTransaction();
        size_t n_ancestors = det_rand.randrange(10)+1;
        for (size_t ancestor = 0; ancestor < n_ancestors && !available_coins.empty(); ++ancestor){
            size_t idx = det_rand.randrange(available_coins.size());
            Available coin = available_coins[idx];
            Txid hash = coin.ref->GetHash();
            // biased towards taking min_ancestors parents, but maybe more
            size_t n_to_take = det_rand.randrange(2) == 0 ?
                               min_ancestors :
                               min_ancestors + det_rand.randrange(coin.ref->vout.size() - coin.vin_left);
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
            tx.vout.resize(det_rand.randrange(10)+2);
            for (auto& out : tx.vout) {
                out.scriptPubKey = CScript() << CScriptNum(tx_counter) << OP_EQUAL;
                out.nValue = 10 * COIN;
            }
        }
        ordered_coins.emplace_back(MakeTransactionRef(tx));
        available_coins.emplace_back(ordered_coins.back(), tx_counter++);
    }
    return ordered_coins;
}

static void MemPoolAddTransactions(benchmark::Bench& bench)
{
    FastRandomContext det_rand{true};
    int childTxs = 90;
    if (bench.complexityN() > 1) {
        childTxs = static_cast<int>(bench.complexityN());
    }
    const auto testing_setup = MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN);
    CTxMemPool& pool = *testing_setup.get()->m_node.mempool;

    std::vector<CTransactionRef> transactions;
    // Create 1000 clusters of 100 transactions each
    for (int i=0; i<100; i++) {
        auto new_txs = CreateCoinCluster(det_rand, childTxs, /*min_ancestors*/ 1);
        transactions.insert(transactions.end(), new_txs.begin(), new_txs.end());
    }

    LOCK2(cs_main, pool.cs);

    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {
        for (auto& tx : transactions) {
            AddTx(tx, pool, det_rand);
        }
        pool.TrimToSize(0, nullptr);
    });
}

static void ComplexMemPool(benchmark::Bench& bench)
{
    FastRandomContext det_rand{true};
    int childTxs = 90;
    if (bench.complexityN() > 1) {
        childTxs = static_cast<int>(bench.complexityN());
    }
    const auto testing_setup = MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN);
    CTxMemPool& pool = *testing_setup.get()->m_node.mempool;

    std::vector<CTransactionRef> tx_remove_for_block;
    std::vector<uint256> hashes_remove_for_block;

    LOCK2(cs_main, pool.cs);

    for (int i=0; i<1000; i++) {
        std::vector<CTransactionRef> transactions = CreateCoinCluster(det_rand, childTxs, /*min_ancestors=*/1);

        // Add all transactions to the mempool.
        // Also store the first 10 transactions from each cluster as the
        // transactions we'll "mine" in the the benchmark.
        int tx_count = 0;
        for (auto& tx : transactions) {
            if (tx_count < 10) {
                tx_remove_for_block.push_back(tx);
                ++tx_count;
                hashes_remove_for_block.emplace_back(tx->GetHash());
            }
            AddTx(tx, pool, det_rand);
        }
    }

    // Since the benchmark will be run repeatedly, we have to leave the mempool
    // in the same state at the end of the function, so we benchmark both
    // mining a block and reorging the block's contents back into the mempool.
    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {
        pool.removeForBlock(tx_remove_for_block, /*nBlockHeight*/100);
        for (auto& tx: tx_remove_for_block) {
            AddTx(tx, pool, det_rand);
        }
        pool.UpdateTransactionsFromBlock(hashes_remove_for_block);
    });
}

static void MemPoolAncestorsDescendants(benchmark::Bench& bench)
{
    FastRandomContext det_rand{true};
    int childTxs = 90;
    if (bench.complexityN() > 1) {
        childTxs = static_cast<int>(bench.complexityN());
    }
    const auto testing_setup = MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN);
    CTxMemPool& pool = *testing_setup.get()->m_node.mempool;

    LOCK2(cs_main, pool.cs);

    std::vector<CTransactionRef> transactions = CreateCoinCluster(det_rand, childTxs, /*min_ancestors=*/1);
    for (auto& tx : transactions) {
        AddTx(tx, pool, det_rand);
    }

    CTxMemPool::txiter first_tx = *pool.GetIter(transactions[0]->GetHash());
    CTxMemPool::txiter last_tx = *pool.GetIter(transactions.back()->GetHash());

    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {
        ankerl::nanobench::doNotOptimizeAway(pool.CalculateDescendants({first_tx}));
        ankerl::nanobench::doNotOptimizeAway(pool.CalculateMemPoolAncestors(*last_tx, false));
    });
}


static void MempoolCheck(benchmark::Bench& bench)
{
    FastRandomContext det_rand{true};
    auto testing_setup = MakeNoLogFileContext<TestChain100Setup>(ChainType::REGTEST, {.extra_args = {"-checkmempool=1"}});
    CTxMemPool& pool = *testing_setup.get()->m_node.mempool;
    LOCK2(cs_main, pool.cs);
    testing_setup->PopulateMempool(det_rand, 400, true);
    const CCoinsViewCache& coins_tip = testing_setup.get()->m_node.chainman->ActiveChainstate().CoinsTip();

    CTxMemPool::Limits limits(kernel::MemPoolLimits::NoLimits());

    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {
        // Bump up the spendheight so we don't hit premature coinbase spend errors.
        pool.check(coins_tip, /*spendheight=*/300, &limits);
    });
}

#if 0
static void MemPoolMiningScoreCheck(benchmark::Bench& bench)
{
    // Default test: each cluster is of size 20, and we'll try to RBF with a
    // transaction that merges 10 clusters, evicting 10 transactions from each.

    FastRandomContext det_rand{true};
    int childTxs = 10;
    if (bench.complexityN() > 1) {
        childTxs = static_cast<int>(bench.complexityN());
    }
    const auto testing_setup = MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN);
    CTxMemPool& pool = *testing_setup.get()->m_node.mempool;

    LOCK2(cs_main, pool.cs);

    std::vector<CTransactionRef> parent_txs_for_rbf;
    std::set<uint256> child_txs_to_conflict_with;

    for (int i=0; i<10; i++) {
        std::vector<CTransactionRef> transactions = CreateCoinCluster(det_rand, childTxs, /*min_ancestors=*/1);
        parent_txs_for_rbf.push_back(transactions[0]);
        // Conflict with everything after the first 10 transactions
        for (size_t j=10; j<transactions.size(); ++j) {
            child_txs_to_conflict_with.insert(transactions[j]->GetHash());
        }

        // Add all transactions to the mempool.
        for (auto& tx : transactions) {
            AddTx(tx, pool, det_rand);
        }
    }

    // Construct a transaction that spends from each of the parent transactions
    // selected.
    CMutableTransaction tx = CMutableTransaction();
    tx.vin.resize(10);
    for (size_t i=0; i<parent_txs_for_rbf.size(); ++i) {
        tx.vin[i].prevout = COutPoint(parent_txs_for_rbf[i]->GetHash(), 0);
        tx.vin[i].scriptSig = CScript() << i;
    }
    tx.vout.resize(1);
    for (auto& out : tx.vout) {
        out.scriptPubKey = CScript() << CScriptNum(det_rand.randrange(19)+1) << OP_EQUAL;
        out.nValue = 10 * COIN;
    }

    CTxMemPool::setEntries all_conflicts = pool.GetIterSet(child_txs_to_conflict_with);
    CTxMemPoolEntry entry(MakeTransactionRef(tx), det_rand.randrange(10000)+1000, 0, 1, 0, false, 4, LockPoints());

    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {
        CTxMemPool::Limits limits(pool.m_limits);
        pool.CalculateMiningScoreOfReplacementTx(entry, det_rand.randrange(30000)+1000, all_conflicts, limits);
    });
}
#endif

BENCHMARK(MemPoolAncestorsDescendants, benchmark::PriorityLevel::HIGH);
BENCHMARK(MemPoolAddTransactions, benchmark::PriorityLevel::HIGH);
BENCHMARK(ComplexMemPool, benchmark::PriorityLevel::HIGH);
BENCHMARK(MempoolCheck, benchmark::PriorityLevel::HIGH);
//BENCHMARK(MemPoolMiningScoreCheck, benchmark::PriorityLevel::HIGH);
