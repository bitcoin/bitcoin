// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <kernel/chain.h>
#include <kernel/types.h>
#include <node/miner.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
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
        TryAddToMempool(*mempool, mempool_entry);
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
    SeedRandomStateForTest(SeedRand::ZEROS);
    SetMockTime(WITH_LOCK(g_setup->m_node.chainman->GetMutex(),
                          return g_setup->m_node.chainman->ActiveTip()->Time()));
    auto current_time = MockableSteadyClock::INITIAL_MOCK_TIME; // In milliseconds
    int32_t max_mempool_weight = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(0, DEFAULT_BLOCK_MAX_WEIGHT * 10);
    PopulateRandTransactionsToMempool(fuzzed_data_provider, node.mempool.get(), max_mempool_weight);
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes(), 10)
    {
        // Test cache hit/miss behavior based on block template age.
        node::BlockAssembler::Options base_options;
        base_options.test_block_validity = false;
        MockableSteadyClock::SetMockTime(current_time += std::chrono::milliseconds{1});
        auto block_template = template_cache->GetBlockTemplate(base_options);
        auto prev_time = block_template->m_creation_time;
        auto time_advance = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(1, 100000); // milliseconds
        auto max_age = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(0, 100000);      // milliseconds

        MockableSteadyClock::SetMockTime(current_time += std::chrono::milliseconds{time_advance});
        auto max_template_age = MillisecondsDouble{max_age};
        block_template = template_cache->GetBlockTemplate(base_options, max_template_age);

        auto time_delta = max_age - time_advance;
        if (time_delta > 0) {
            // Cache should return the same template when the configured maximum block age strictly exceeds the time delta since last
            // block creation (with the same constraints)
            assert(!node::TimeIntervalElapsed(prev_time, max_template_age));
            assert(block_template->m_creation_time == prev_time);
        } else {
            // Cache should create a new template when the configured maximum block age is less than
            // or equal to the time delta since last block creation.
            assert(node::TimeIntervalElapsed(prev_time, max_template_age));
            assert(block_template->m_creation_time > prev_time);
        }
        prev_time = block_template->m_creation_time;

        // Test cache differentiation with some modified block assembly options
        node::BlockAssembler::Options modified_options = base_options;
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
        MockableSteadyClock::SetMockTime(current_time += std::chrono::milliseconds{additional_time_advance});

        {
            // Different options should produce a newer template despite long maximum block age
            auto long_max_template_age = MillisecondsDouble::max();
            block_template = template_cache->GetBlockTemplate(modified_options, long_max_template_age);
            assert(block_template->m_creation_time > prev_time);

            // Same original options must hit the previous template in the cache
            block_template = template_cache->GetBlockTemplate(base_options, long_max_template_age);
            assert(block_template->m_creation_time == prev_time);

            // BlockConnected signal with background chainstate role should not invalidate cache
            kernel::ChainstateRole background_role{true, true};
            node.validation_signals->BlockConnected(background_role, nullptr, nullptr);
            MockableSteadyClock::SetMockTime(current_time += std::chrono::milliseconds{1});
            block_template = template_cache->GetBlockTemplate(base_options, long_max_template_age);
            assert(block_template->m_creation_time == prev_time);

            // BlockConnected signal with normal chainstate role should invalidate entire cache
            kernel::ChainstateRole normal_role{true, false};
            node.validation_signals->BlockConnected(normal_role, nullptr, nullptr);
            MockableSteadyClock::SetMockTime(current_time += std::chrono::milliseconds{1});
            block_template = template_cache->GetBlockTemplate(base_options, long_max_template_age);
            assert(block_template->m_creation_time > prev_time);
            prev_time = block_template->m_creation_time;

            // BlockDisconnected signal should also invalidate cache
            node.validation_signals->BlockDisconnected(nullptr, nullptr);
            MockableSteadyClock::SetMockTime(current_time += std::chrono::milliseconds{1});
            block_template = template_cache->GetBlockTemplate(base_options, long_max_template_age);
            assert(block_template->m_creation_time > prev_time);
            node.validation_signals->BlockConnected(normal_role, nullptr, nullptr);
        }
    }
}
