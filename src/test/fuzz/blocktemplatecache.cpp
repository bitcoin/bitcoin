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
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/time.h>
#include <validationinterface.h>

#include <optional>

using node::BlockCreateOptions;

namespace {
const TestingSetup* g_setup;

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
        const int32_t tx_weight = mempool_entry.GetTxWeight();
        if (tx_weight > total_weight) break;
        TryAddToMempool(*mempool, mempool_entry);
        total_weight -= tx_weight;
    }
}
} // namespace

FUZZ_TARGET(block_template_cache, .init = initialize_block_template_cache)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const auto& node = g_setup->m_node;
    auto& template_cache = node.block_template_cache;
    SteadyClockContext steady_clock{};
    SeedRandomStateForTest(SeedRand::ZEROS);
    SetMockTime(WITH_LOCK(g_setup->m_node.chainman->GetMutex(),
                          return g_setup->m_node.chainman->ActiveTip()->Time()));
    node.validation_signals->BlockConnected(kernel::ChainstateRole{}, /*block=*/nullptr, /*pindex=*/nullptr);
    int max_mempool_weight = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, DEFAULT_BLOCK_MAX_WEIGHT * 10);
    PopulateRandTransactionsToMempool(fuzzed_data_provider, node.mempool.get(), max_mempool_weight);
    // Establish baseline template
    steady_clock += 1ms;
    BlockCreateOptions base_options;
    base_options.test_block_validity = false;
    auto tmpl = template_cache->GetBlockTemplate(base_options, MillisecondsDouble{0});
    assert(tmpl);
    auto prev_time = tmpl->m_creation_time;
    auto CheckCache = [&](bool expect_hit,
                          const BlockCreateOptions& opts,
                          std::optional<MillisecondsDouble> max_age = std::nullopt,
                          std::chrono::milliseconds advance = 1ms) {
        steady_clock += advance;
        auto t = template_cache->GetBlockTemplate(opts, max_age);
        assert(t);
        if (expect_hit) {
            assert(t->m_creation_time == prev_time);
        } else {
            assert(t->m_creation_time > prev_time);
            if (max_age) prev_time = t->m_creation_time;
        }
    };
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes(), 10)
    {
        // template age-based behavior
        const auto advance_ms =
            std::chrono::milliseconds{fuzzed_data_provider.ConsumeIntegralInRange<int>(1, 100000)};
        const auto template_max_age_ms =
            std::chrono::milliseconds{fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 100000)};
        const auto max_age = MillisecondsDouble{template_max_age_ms};
        // Expect a hit iff block template isn't older than max_age.
        const bool expect_hit = advance_ms <= template_max_age_ms;
        CheckCache(expect_hit, base_options, max_age, advance_ms);
        // historical chainstate role should not clear cache
        node.validation_signals->BlockConnected(kernel::ChainstateRole{.historical = true}, /*block=*/nullptr, /*pindex=*/nullptr);
        CheckCache(true, base_options, MillisecondsDouble::max());
        BlockCreateOptions modified = base_options;
        modified.block_reserved_weight =
            fuzzed_data_provider.ConsumeIntegralInRange<int>(
                MINIMUM_BLOCK_RESERVED_WEIGHT, DEFAULT_BLOCK_RESERVED_WEIGHT - 1);
        const CAmount fee_paid =
            fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, COIN);
        const int32_t tx_size =
            fuzzed_data_provider.ConsumeIntegralInRange<int>(
                1, MAX_STANDARD_TX_WEIGHT / WITNESS_SCALE_FACTOR);
        modified.block_min_fee_rate = CFeeRate(fee_paid, tx_size);
        if (fuzzed_data_provider.ConsumeBool()) {
            modified.block_max_weight =
                fuzzed_data_provider.ConsumeIntegralInRange<int>(
                    modified.block_reserved_weight.value(), DEFAULT_BLOCK_MAX_WEIGHT);
        }
        // Different options must miss cache
        CheckCache(false, modified, MillisecondsDouble::max());
        // std::nullopt max_template age should not hit a cache and not clear.
        CheckCache(false, modified);
        CheckCache(true, modified, MillisecondsDouble::max());
        // Sanity check after all these insertions
        template_cache->SanityCheck();
        // non-historical chainstate role should clear cache
        node.validation_signals->BlockConnected(kernel::ChainstateRole{}, /*block=*/nullptr, /*pindex=*/nullptr);
        CheckCache(false, base_options, MillisecondsDouble::max());
        // All block disconnection should clear cache
        node.validation_signals->BlockDisconnected(/*block=*/nullptr, /*pindex=*/nullptr);
        CheckCache(false, base_options, MillisecondsDouble::max());
    }
    template_cache->SanityCheck();
}
