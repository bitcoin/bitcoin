// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <kernel/chain.h>
#include <node/miner.h>
#include <policy/policy.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <test/util/txmempool.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <util/time.h>
#include <validationinterface.h>

#include <optional>

namespace {
const ChainTestingSetup* g_setup;
} // namespace

void initialize_block_template_cache()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

void PopulateRandTransactionsToMempool(FuzzedDataProvider& fuzzed_data_provider, CTxMemPool* mempool, int total_weight)
{
    // Populate mempool with varying transaction load to test cache under different conditions
    while (total_weight > 0 && fuzzed_data_provider.remaining_bytes() > 0) {
        const std::optional<CMutableTransaction> mutable_tx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
        if (!mutable_tx) break;
        auto tx = MakeTransactionRef(*mutable_tx);
        CTxMemPoolEntry mempool_entry{ConsumeTxMemPoolEntry(fuzzed_data_provider, *tx)};
        const Txid txid = tx->GetHash();
        if (mempool->exists(txid)) continue;
        AddToMempool(*mempool, mempool_entry);
        const int32_t tx_weight = mempool_entry.GetTxWeight();
        if (tx_weight > total_weight) break;
        total_weight -= tx_weight;
    }
}

FUZZ_TARGET(block_template_cache, .init = initialize_block_template_cache)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const auto& node = g_setup->m_node;
    auto& template_cache = node.block_template_cache;
    node.validation_signals->RegisterSharedValidationInterface(template_cache);

    SeedRandomStateForTest(SeedRand::ZEROS);
    SetMockTime(WITH_LOCK(g_setup->m_node.chainman->GetMutex(),
                          return g_setup->m_node.chainman->ActiveTip()->Time()));

    int32_t max_mempool_weight = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(0, DEFAULT_BLOCK_MAX_WEIGHT * 10);
    PopulateRandTransactionsToMempool(fuzzed_data_provider, node.mempool.get(), max_mempool_weight);
    auto num_iterations = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, 10);
    LIMITED_WHILE (fuzzed_data_provider.remaining_bytes(), num_iterations) {
        // Test cache hit/miss behavior based on time intervals
        node::BlockAssembler::Options base_options;
        base_options.test_block_validity = false;
        auto first_template = template_cache->GetBlockTemplate(base_options, std::chrono::seconds{0});

        auto time_advance_seconds = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(1, 100);
        SetMockTime(GetMockTime() + std::chrono::seconds{time_advance_seconds});
        auto cache_interval_seconds = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(1, 100);
        auto second_template = template_cache->GetBlockTemplate(base_options, std::chrono::seconds{cache_interval_seconds});

        // Cache should return same template if cache interval strictly exceeds elapsed time
        if (cache_interval_seconds > time_advance_seconds) {
            assert(second_template.second == first_template.second);
        } else { // Cache should create a new template when cache interval is less than or equal to elapsed time
            assert(second_template.second > first_template.second);
        }

        // Test cache differentiation with modified block assembly options
        node::BlockAssembler::Options modified_options;
        modified_options.test_block_validity = false;
        auto max_reserved_weight = DEFAULT_BLOCK_RESERVED_WEIGHT - 1;
        modified_options.block_reserved_weight = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(
            MINIMUM_BLOCK_RESERVED_WEIGHT, max_reserved_weight);

        CAmount fee_paid = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, COIN);
        int32_t tx_size = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(1, MAX_STANDARD_TX_WEIGHT / WITNESS_SCALE_FACTOR);
        modified_options.blockMinFeeRate = CFeeRate(fee_paid, tx_size);
        if (fuzzed_data_provider.ConsumeBool()) {
            modified_options.nBlockMaxWeight = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(
                modified_options.block_reserved_weight, DEFAULT_BLOCK_MAX_WEIGHT);
        }

        auto additional_time_advance = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(1, 100);
        SetMockTime(GetMockTime() + std::chrono::seconds{additional_time_advance});

        // Different options should produce newer template despite long cache interval
        auto third_template = template_cache->GetBlockTemplate(modified_options, std::chrono::seconds::max());
        assert(third_template.second > second_template.second);

        // Same original options must hit the second template in the cache
        auto fourth_template = template_cache->GetBlockTemplate(base_options, std::chrono::seconds::max());
        assert(fourth_template.second == second_template.second);

        // BlockConnected signal should invalidate entire cache
        node.validation_signals->BlockConnected(ChainstateRole::NORMAL, nullptr, nullptr);
        SetMockTime(GetMockTime() + std::chrono::seconds{1});
        auto current_time = NodeClock::now();

        auto fifth_template = template_cache->GetBlockTemplate(base_options, std::chrono::seconds::max());
        // Post-invalidation template must be fresh regardless of cache interval
        assert(fifth_template.second >= current_time);
        node.validation_signals->BlockConnected(ChainstateRole::NORMAL, nullptr, nullptr);
    }
    node.validation_signals->UnregisterSharedValidationInterface(template_cache);
}
