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
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN, {.mock_steady_clock = true});
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
    node.validation_signals->BlockConnected({true, false}, nullptr, nullptr);
    int32_t max_mempool_weight = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(0, DEFAULT_BLOCK_MAX_WEIGHT * 10);
    PopulateRandTransactionsToMempool(fuzzed_data_provider, node.mempool.get(), max_mempool_weight);

    // Establish baseline template
    auto now = MockableSteadyClock::INITIAL_MOCK_TIME; // In milliseconds
    MockableSteadyClock::SetMockTime(now += 1ms);
    node::BlockAssembler::Options base_options;
    base_options.test_block_validity = false;
    auto tmpl = template_cache->GetBlockTemplate(base_options, MillisecondsDouble{0ms});
    assert(tmpl);
    auto prev_time = tmpl->m_creation_time;
    auto CheckCache = [&](bool expect_hit,
                          const node::BlockAssembler::Options& opts,
                          std::optional<MillisecondsDouble> age_limit = std::nullopt,
                          std::chrono::milliseconds advance = 1ms) {
        MockableSteadyClock::SetMockTime(now += advance);
        auto t = template_cache->GetBlockTemplate(opts, age_limit);
        assert(t);

        if (expect_hit) {
            assert(t->m_creation_time == prev_time);
        } else {
            assert(t->m_creation_time > prev_time);
            if (age_limit) prev_time = t->m_creation_time;
        }
    };
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes(), 10)
    {
        // template age-based behavior
        const auto advance_ms =
            std::chrono::milliseconds{fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(1, 100000)};
        const auto template_age_limit_ms =
            std::chrono::milliseconds{fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(0, 100000)};
        const auto age_limit = MillisecondsDouble{template_age_limit_ms};

        // Expected hit iff block template age limit exceeds the elapsed time.
        const bool expect_hit = advance_ms < template_age_limit_ms;
        CheckCache(expect_hit, base_options, age_limit, advance_ms);

        // historical chainstate role should not clear cache
        kernel::ChainstateRole background_role{true, true};
        node.validation_signals->BlockConnected(background_role, nullptr, nullptr);
        CheckCache(true, base_options, MillisecondsDouble::max());

        node::BlockAssembler::Options modified = base_options;
        modified.block_reserved_weight =
            fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(
                MINIMUM_BLOCK_RESERVED_WEIGHT, DEFAULT_BLOCK_RESERVED_WEIGHT - 1);

        const CAmount fee_paid =
            fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, COIN);
        const int32_t tx_size =
            fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(
                1, MAX_STANDARD_TX_WEIGHT / WITNESS_SCALE_FACTOR);

        modified.blockMinFeeRate = CFeeRate(fee_paid, tx_size);

        if (fuzzed_data_provider.ConsumeBool()) {
            modified.nBlockMaxWeight =
                fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(
                    modified.block_reserved_weight, DEFAULT_BLOCK_MAX_WEIGHT);
        }

        // Different options must miss cache
        CheckCache(false, modified, MillisecondsDouble::max());

        // std::nullopt max_template age should not hit a cache and not clear.
        CheckCache(false, modified);
        CheckCache(true, modified, MillisecondsDouble::max());

        // Sanity check after all these insertions
        template_cache->SanityCheck();

        // non-historical chainstate role role should clear cache
        kernel::ChainstateRole normal_role{true, false};
        node.validation_signals->BlockConnected(normal_role, nullptr, nullptr);
        CheckCache(false, base_options, MillisecondsDouble::max());

        // All block disconnection should clear cache
        node.validation_signals->BlockDisconnected(nullptr, nullptr);
        CheckCache(false, base_options, MillisecondsDouble::max());
    }
    template_cache->SanityCheck();
}
