// Copyright (c) 2011-2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/validation.h>
#include <node/mini_miner.h>
#include <policy/ancestor_packages.h>
#include <random.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>

#include <deque>

static void AncestorPackageRandom(benchmark::Bench& bench)
{
    FastRandomContext rand{true};
    Package txns;

    int cluster_size = 100;
    if (bench.complexityN() > 1) {
        cluster_size = static_cast<int>(bench.complexityN());
    }

    std::deque<COutPoint> available_coins;
    for (uint32_t i = 0; i < uint32_t{100}; ++i) {
        available_coins.emplace_back(uint256::ZERO, i);
    }

    for (uint32_t count = cluster_size - 1; !available_coins.empty() && count; --count)
    {
        CMutableTransaction mtx = CMutableTransaction();
        const size_t num_inputs = rand.randrange(available_coins.size()) + 1;
        const size_t num_outputs = rand.randrange(50) + 1;
        for (size_t n{0}; n < num_inputs; ++n) {
            auto prevout = available_coins.front();
            mtx.vin.emplace_back(prevout, CScript());
            available_coins.pop_front();
        }
        for (uint32_t n{0}; n < num_outputs; ++n) {
            mtx.vout.emplace_back(100, P2WSH_OP_TRUE);
        }
        CTransactionRef tx = MakeTransactionRef(mtx);
        txns.emplace_back(tx);

        // At least one output is available for spending by descendant
        for (uint32_t n{0}; n < num_outputs; ++n) {
            if (n == 0 || rand.randbool()) {
                available_coins.emplace_back(tx->GetHash(), n);
            }
        }
    }

    assert(!available_coins.empty());

    // Now spend all available coins to make sure it's an ancestor package
    size_t num_coins = available_coins.size();
    CMutableTransaction child_mtx;
    for (size_t avail = 0; avail < num_coins; avail++) {
        auto prevout = available_coins[0];
        child_mtx.vin.emplace_back(prevout, CScript());
        available_coins.pop_front();
    }
    child_mtx.vout.emplace_back(100, P2WSH_OP_TRUE);
    CTransactionRef child_tx = MakeTransactionRef(child_mtx);
    txns.emplace_back(child_tx);

    bench.run([&]() NO_THREAD_SAFETY_ANALYSIS {
        AncestorPackage ancestor_package(txns);
        assert(ancestor_package.IsAncestorPackage());
        for (size_t i = 0; i < txns.size(); i++) {
            ancestor_package.AddFeeAndVsize(txns[i]->GetHash(), rand.randrange(1000000), rand.randrange(1000));
        }
        ancestor_package.LinearizeWithFees();
    });
}

static void AncestorPackageChain(benchmark::Bench& bench)
{
    FastRandomContext det_rand{true};

    // A chain of 25 97-in-1-out transactions, where the nth tx spends the n-1th.  We are trying to
    // maximize the number of transactions in the package (within MAX_PACKAGE_COUNT) and number of
    // inputs to look up (while staying within MAX_PACKAGE_WEIGHT) for the "worst case" package.
    const uint32_t num_txns{25};
    int32_t total_weight{0};
    Package txns;
    txns.reserve(num_txns);

    uint256 starting_txid{det_rand.rand256()};
    uint256& last_txid = starting_txid;
    for (uint32_t count{0}; count < num_txns; ++count) {
        CMutableTransaction mtx = CMutableTransaction();
        mtx.vin.reserve(97);
        // First input is the output from the last tx.
        mtx.vin.emplace_back(last_txid, 0);
        // The rest are random.
        for (uint32_t i{1}; i < 97; ++i) {
            mtx.vin.emplace_back(det_rand.rand256(), count * 100 + i);
        }

        mtx.vout.emplace_back(1000, P2WSH_OP_TRUE);
        CTransactionRef tx = MakeTransactionRef(mtx);
        txns.emplace_back(tx);
        last_txid = tx->GetHash();
        total_weight += GetTransactionWeight(*tx);
    }

    // Just barely below MAX_PACKAGE_WEIGHT.
    assert(total_weight == 403000);

    // Pass the transactions in backwards for worst-case sorting.
    Package reversed_txns(txns.rbegin(), txns.rend());

    bench.minEpochIterations(10).run([&]() NO_THREAD_SAFETY_ANALYSIS {
        AncestorPackage ancestor_package(reversed_txns);
        assert(ancestor_package.IsAncestorPackage());
        for (size_t i = 0; i < txns.size(); ++i) {
            // Decreasing feerate so that "mining" each transaction requires updating all the rest.
            // Make each transaction 100vB bigger than the previous one.
            ancestor_package.AddFeeAndVsize(txns.at(i)->GetHash(), 10000000, 100 * (i + 1));
        }
        ancestor_package.LinearizeWithFees();
    });
}

BENCHMARK(AncestorPackageRandom, benchmark::PriorityLevel::HIGH);
BENCHMARK(AncestorPackageChain, benchmark::PriorityLevel::HIGH);
