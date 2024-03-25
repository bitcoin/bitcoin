// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mempool_args.h>
#include <policy/rbf.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;
} // namespace

const int NUM_ITERS = 10000;

std::vector<COutPoint> g_outpoints;

void initialize_rbf()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    g_setup = testing_setup.get();
}

void initialize_package_rbf()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    g_setup = testing_setup.get();

    // Create a fixed set of unique "UTXOs" to source parents from
    // to avoid fuzzer giving circular references
    for (int i = 0; i < NUM_ITERS; ++i) {
        g_outpoints.emplace_back();
        g_outpoints.back().n = i;
    }

}

FUZZ_TARGET(rbf, .init = initialize_rbf)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    std::optional<CMutableTransaction> mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
    if (!mtx) {
        return;
    }

    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node)};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), NUM_ITERS)
    {
        const std::optional<CMutableTransaction> another_mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
        if (!another_mtx) {
            break;
        }
        const CTransaction another_tx{*another_mtx};
        if (fuzzed_data_provider.ConsumeBool() && !mtx->vin.empty()) {
            mtx->vin[0].prevout = COutPoint{another_tx.GetHash(), 0};
        }
        LOCK2(cs_main, pool.cs);
        pool.addUnchecked(ConsumeTxMemPoolEntry(fuzzed_data_provider, another_tx));
    }
    const CTransaction tx{*mtx};
    if (fuzzed_data_provider.ConsumeBool()) {
        LOCK2(cs_main, pool.cs);
        pool.addUnchecked(ConsumeTxMemPoolEntry(fuzzed_data_provider, tx));
    }
    {
        LOCK(pool.cs);
        (void)IsRBFOptIn(tx, pool);
    }
}

void CheckDiagramConcave(std::vector<FeeFrac>& diagram)
{
    // Diagrams are in monotonically-decreasing feerate order.
    FeeFrac last_chunk = diagram.front();
    for (size_t i = 1; i<diagram.size(); ++i) {
        FeeFrac next_chunk = diagram[i] - diagram[i-1];
        assert(next_chunk <= last_chunk);
        last_chunk = next_chunk;
    }
}

FUZZ_TARGET(package_rbf, .init = initialize_package_rbf)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));

    std::optional<CMutableTransaction> child = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
    if (!child) return;

    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node)};

    // Add a bunch of parent-child pairs to the mempool, and remember them.
    std::vector<CTransaction> mempool_txs;
    size_t iter{0};

    LOCK2(cs_main, pool.cs);

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), NUM_ITERS)
    {
        // Make sure txns only have one input, and that a unique input is given to avoid circular references
        std::optional<CMutableTransaction> parent = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
        if (!parent) {
            continue;
        }
        assert(iter <= g_outpoints.size());
        parent->vin.resize(1);
        parent->vin[0].prevout = g_outpoints[iter++];

        mempool_txs.emplace_back(*parent);
        pool.addUnchecked(ConsumeTxMemPoolEntry(fuzzed_data_provider, mempool_txs.back()));
        if (fuzzed_data_provider.ConsumeBool() && !child->vin.empty()) {
            child->vin[0].prevout = COutPoint{mempool_txs.back().GetHash(), 0};
        }
        mempool_txs.emplace_back(*child);
        pool.addUnchecked(ConsumeTxMemPoolEntry(fuzzed_data_provider, mempool_txs.back()));

        if (fuzzed_data_provider.ConsumeBool()) {
            pool.PrioritiseTransaction(mempool_txs.back().GetHash().ToUint256(), fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(-100000, 100000));
        }
    }

    // Pick some transactions at random to be the direct conflicts
    CTxMemPool::setEntries direct_conflicts;
    for (auto& tx : mempool_txs) {
        if (fuzzed_data_provider.ConsumeBool()) {
            direct_conflicts.insert(*pool.GetIter(tx.GetHash()));
        }
    }

    // Calculate all conflicts:
    CTxMemPool::setEntries all_conflicts;
    for (auto& txiter : direct_conflicts) {
        pool.CalculateDescendants(txiter, all_conflicts);
    }

    // Calculate the feerate diagrams for a replacement.
    CAmount replacement_fees = ConsumeMoney(fuzzed_data_provider);
    int64_t replacement_vsize = fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(1, 1000000);
    auto calc_results{pool.CalculateFeerateDiagramsForRBF(replacement_fees, replacement_vsize, direct_conflicts, all_conflicts)};

    if (calc_results.has_value()) {
        // Sanity checks on the diagrams.

        // Diagrams start at 0.
        assert(calc_results->first.front().size == 0);
        assert(calc_results->first.front().fee == 0);
        assert(calc_results->second.front().size == 0);
        assert(calc_results->second.front().fee == 0);

        CheckDiagramConcave(calc_results->first);
        CheckDiagramConcave(calc_results->second);

        CAmount replaced_fee{0};
        int64_t replaced_size{0};
        for (auto txiter : all_conflicts) {
            replaced_fee += txiter->GetModifiedFee();
            replaced_size += txiter->GetTxSize();
        }
        // The total fee of the new diagram should be the total fee of the old
        // diagram - replaced_fee + replacement_fees
        assert(calc_results->first.back().fee - replaced_fee + replacement_fees == calc_results->second.back().fee);
        assert(calc_results->first.back().size - replaced_size + replacement_vsize == calc_results->second.back().size);
    }

    // If internals report error, wrapper should too
    auto err_tuple{ImprovesFeerateDiagram(pool, direct_conflicts, all_conflicts, replacement_fees, replacement_vsize)};
    if (!calc_results.has_value()) {
         assert(err_tuple.value().first == DiagramCheckError::UNCALCULABLE);
    } else {
        // Diagram check succeeded
        if (!err_tuple.has_value()) {
            // New diagram's final fee should always match or exceed old diagram's
            assert(calc_results->first.back().fee <= calc_results->second.back().fee);
        } else if (calc_results->first.back().fee > calc_results->second.back().fee) {
            // Or it failed, and if old diagram had higher fees, it should be a failure
            assert(err_tuple.value().first == DiagramCheckError::FAILURE);
        }
    }
}
