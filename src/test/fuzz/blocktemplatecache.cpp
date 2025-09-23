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
    SeedRandomStateForTest(SeedRand::ZEROS);
    SetMockTime(WITH_LOCK(g_setup->m_node.chainman->GetMutex(),
                          return g_setup->m_node.chainman->ActiveTip()->Time()));

    int32_t max_mempool_weight = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(0, DEFAULT_BLOCK_MAX_WEIGHT * 10);
    PopulateRandTransactionsToMempool(fuzzed_data_provider, node.mempool.get(), max_mempool_weight);
    LIMITED_WHILE (fuzzed_data_provider.remaining_bytes(), 10) {
        // Test cache hit/miss behavior based on time delta
        node::BlockAssembler::Options base_options;
        base_options.test_block_validity = false;
        auto block_template = template_cache->GetBlockTemplate(base_options);
        auto prev_time = block_template->m_creation_time;

        auto time_advance = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(1, 1000); // seconds
        auto block_max_age = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(1, 100000); // milliseconds
        SetMockTime(GetMockTime() + std::chrono::seconds{time_advance});

        base_options.max_template_age = MillisecondsDouble{block_max_age};
        block_template = template_cache->GetBlockTemplate(base_options);

        auto time_delta = block_max_age - (time_advance * 1000);
        if (time_delta > 0) {
            // Cache should return the same template when the configured maximum block age strictly exceeds the time delta since last
            // block creation (with the same constraints)
            assert(!node::TimeIntervalElapsed(prev_time, base_options.max_template_age));
            assert(block_template->m_creation_time == prev_time);
        } else {
            // Cache should create a new template when the configured maximum block age is less than
            // or equal to the time delta since last block creation (with the same constraints)
            assert(node::TimeIntervalElapsed(prev_time, base_options.max_template_age));
            assert(block_template->m_creation_time > prev_time);
        }
        prev_time = block_template->m_creation_time;

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

        {
            // Different options should produce a newer template despite long maximum block age
            modified_options.max_template_age = MillisecondsDouble::max();
            assert(!node::SimilarOptions(modified_options, base_options));
            block_template = template_cache->GetBlockTemplate(modified_options);
            assert(block_template->m_creation_time > prev_time);

            // Same original options must hit the previous template in the cache
            base_options.max_template_age = MillisecondsDouble::max();
            block_template = template_cache->GetBlockTemplate(base_options);
            assert(block_template->m_creation_time == prev_time);

            // BlockConnected signal with background chainstate role should not invalidate cache
            node.validation_signals->BlockConnected(ChainstateRole::BACKGROUND, nullptr, nullptr);
            SetMockTime(GetMockTime() + std::chrono::seconds{1});

            block_template = template_cache->GetBlockTemplate(base_options);
            assert(block_template->m_creation_time == prev_time);

            // BlockConnected signal with normal chainstate role should invalidate entire cache
            node.validation_signals->BlockConnected(ChainstateRole::NORMAL, nullptr, nullptr);
            SetMockTime(GetMockTime() + std::chrono::seconds{1});
            auto current_time = NodeClock::now();
            block_template = template_cache->GetBlockTemplate(base_options);
            assert(block_template->m_creation_time >= current_time);
            prev_time = block_template->m_creation_time;

            // BlockDisconnected signal should also invalidate cache
            node.validation_signals->BlockDisconnected(nullptr, nullptr);
            SetMockTime(GetMockTime() + std::chrono::seconds{1});
            block_template = template_cache->GetBlockTemplate(base_options);
            assert(block_template->m_creation_time > prev_time);
        }
        node.validation_signals->BlockConnected(ChainstateRole::NORMAL, nullptr, nullptr);
    }
}
