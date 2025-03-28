// Copyright (c) 2020-present The Bitcoin Core developers
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
#include <util/check.h>
#include <util/translation.h>

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
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    std::optional<CMutableTransaction> mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
    if (!mtx) {
        return;
    }

    bilingual_str error;
    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node), error};
    Assert(error.empty());

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
        if (!pool.GetIter(another_tx.GetHash())) {
            AddToMempool(pool, ConsumeTxMemPoolEntry(fuzzed_data_provider, another_tx));
        }
    }
    const CTransaction tx{*mtx};
    if (fuzzed_data_provider.ConsumeBool()) {
        LOCK2(cs_main, pool.cs);
        if (!pool.GetIter(tx.GetHash())) {
            AddToMempool(pool, ConsumeTxMemPoolEntry(fuzzed_data_provider, tx));
        }
    }
    {
        LOCK(pool.cs);
        (void)IsRBFOptIn(tx, pool);
    }
}

FUZZ_TARGET(package_rbf, .init = initialize_package_rbf)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));

    // "Real" virtual size is not important for this test since ConsumeTxMemPoolEntry generates its own virtual size values
    // so we construct small transactions for performance reasons. Child simply needs an input for later to perhaps connect to parent.
    CMutableTransaction child;
    child.vin.resize(1);

    bilingual_str error;
    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node), error};
    Assert(error.empty());

    // Add a bunch of parent-child pairs to the mempool, and remember them.
    std::vector<CTransaction> mempool_txs;
    uint32_t iter{0};

    // Keep track of the total vsize of CTxMemPoolEntry's being added to the mempool to avoid overflow
    // Add replacement_vsize since this is added to new diagram during RBF check
    std::optional<CMutableTransaction> replacement_tx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
    if (!replacement_tx) {
        return;
    }
    replacement_tx->vin.resize(1);
    replacement_tx->vin[0].prevout = g_outpoints.at(iter++);
    CTransaction replacement_tx_final{*replacement_tx};
    auto replacement_entry = ConsumeTxMemPoolEntry(fuzzed_data_provider, replacement_tx_final);
    int32_t replacement_vsize = replacement_entry.GetTxSize();
    int64_t running_vsize_total{replacement_vsize};

    LOCK2(cs_main, pool.cs);

    while (fuzzed_data_provider.ConsumeBool()) {
        if (iter >= NUM_ITERS) break;

        // Make sure txns only have one input, and that a unique input is given to avoid circular references
        CMutableTransaction parent;
        parent.vin.resize(1);
        parent.vin[0].prevout = g_outpoints.at(iter++);
        parent.vout.emplace_back(0, CScript());

        mempool_txs.emplace_back(parent);
        const auto parent_entry = ConsumeTxMemPoolEntry(fuzzed_data_provider, mempool_txs.back());
        running_vsize_total += parent_entry.GetTxSize();
        if (running_vsize_total > std::numeric_limits<int32_t>::max()) {
            // We aren't adding this final tx to mempool, so we don't want to conflict with it
            mempool_txs.pop_back();
            break;
        }
        assert(!pool.GetIter(parent_entry.GetTx().GetHash()));
        AddToMempool(pool, parent_entry);
        if (fuzzed_data_provider.ConsumeBool()) {
            child.vin[0].prevout = COutPoint{mempool_txs.back().GetHash(), 0};
        }
        mempool_txs.emplace_back(child);
        const auto child_entry = ConsumeTxMemPoolEntry(fuzzed_data_provider, mempool_txs.back());
        running_vsize_total += child_entry.GetTxSize();
        if (running_vsize_total > std::numeric_limits<int32_t>::max()) {
            // We aren't adding this final tx to mempool, so we don't want to conflict with it
            mempool_txs.pop_back();
            break;
        }
        if (!pool.GetIter(child_entry.GetTx().GetHash())) {
            AddToMempool(pool, child_entry);
        }

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

    CAmount replacement_fees = ConsumeMoney(fuzzed_data_provider);
    auto changeset = pool.GetChangeSet();
    for (auto& txiter : all_conflicts) {
        changeset->StageRemoval(txiter);
    }
    changeset->StageAddition(replacement_entry.GetSharedTx(), replacement_fees,
            replacement_entry.GetTime().count(), replacement_entry.GetHeight(),
            replacement_entry.GetSequence(), replacement_entry.GetSpendsCoinbase(),
            replacement_entry.GetSigOpCost(), replacement_entry.GetLockPoints());
    // Calculate the chunks for a replacement.
    auto calc_results{changeset->CalculateChunksForRBF()};

    if (calc_results.has_value()) {
        // Sanity checks on the chunks.

        // Feerates are monotonically decreasing.
        FeeFrac first_sum;
        for (size_t i = 0; i < calc_results->first.size(); ++i) {
            first_sum += calc_results->first[i];
            if (i) assert(!(calc_results->first[i - 1] << calc_results->first[i]));
        }
        FeeFrac second_sum;
        for (size_t i = 0; i < calc_results->second.size(); ++i) {
            second_sum += calc_results->second[i];
            if (i) assert(!(calc_results->second[i - 1] << calc_results->second[i]));
        }

        FeeFrac replaced;
        for (auto txiter : all_conflicts) {
            replaced.fee += txiter->GetModifiedFee();
            replaced.size += txiter->GetTxSize();
        }
        // The total fee & size of the new diagram minus replaced fee & size should be the total
        // fee & size of the old diagram minus replacement fee & size.
        assert((first_sum - replaced) == (second_sum - FeeFrac{replacement_fees, replacement_vsize}));
    }

    // If internals report error, wrapper should too
    auto err_tuple{ImprovesFeerateDiagram(*changeset)};
    if (!calc_results.has_value()) {
         assert(err_tuple.value().first == DiagramCheckError::UNCALCULABLE);
    } else {
        // Diagram check succeeded
        auto old_sum = std::accumulate(calc_results->first.begin(), calc_results->first.end(), FeeFrac{});
        auto new_sum = std::accumulate(calc_results->second.begin(), calc_results->second.end(), FeeFrac{});
        if (!err_tuple.has_value()) {
            // New diagram's final fee should always match or exceed old diagram's
            assert(old_sum.fee <= new_sum.fee);
        } else if (old_sum.fee > new_sum.fee) {
            // Or it failed, and if old diagram had higher fees, it should be a failure
            assert(err_tuple.value().first == DiagramCheckError::FAILURE);
        }
    }
}
