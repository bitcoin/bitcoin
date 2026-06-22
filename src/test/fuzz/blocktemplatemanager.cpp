// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <kernel/mempool_entry.h>
#include <node/block_template_manager.h>
#include <node/miner.h>
#include <node/mining_args.h>
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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>

using node::BlockCreateOptions;

namespace {
const TestingSetup* g_setup;

void initialize_block_template_manager()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

/** Add random transactions to the mempool, bypassing validation. */
void PopulateRandTransactionsToMempool(FuzzedDataProvider& fuzzed_data_provider, CTxMemPool& mempool, int weight_budget)
{
    while (weight_budget > 0 && fuzzed_data_provider.remaining_bytes() > 0) {
        const std::optional<CMutableTransaction> mutable_tx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
        if (!mutable_tx) break;
        auto tx = MakeTransactionRef(*mutable_tx);
        CTxMemPoolEntry mempool_entry{ConsumeTxMemPoolEntry(fuzzed_data_provider, *tx)};
        const Txid txid = tx->GetHash();
        if (mempool.exists(txid)) continue;
        const int32_t tx_weight = mempool_entry.GetTxWeight();
        if (tx_weight > weight_budget) break;
        TryAddToMempool(mempool, mempool_entry);
        weight_budget -= tx_weight;
    }
}
} // namespace

FUZZ_TARGET(block_template_manager, .init = initialize_block_template_manager)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const auto& node = g_setup->m_node;
    auto& block_template_manager = *Assert(node.block_template_manager);
    auto& mempool = *Assert(node.mempool);
    SeedRandomStateForTest(SeedRand::ZEROS);
    SetMockTime(WITH_LOCK(node.chainman->GetMutex(),
                          return node.chainman->ActiveTip()->Time()));
    {
        LOCK(mempool.cs);
        mempool.TrimToSize(0);
    }
    int weight_budget = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, DEFAULT_BLOCK_MAX_WEIGHT * 10);
    PopulateRandTransactionsToMempool(fuzzed_data_provider, mempool, weight_budget);
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes() > 0, 10)
    {
        BlockCreateOptions options;
        options.test_block_validity = false;
        if (fuzzed_data_provider.ConsumeBool()) {
            options.use_mempool = fuzzed_data_provider.ConsumeBool();
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            options.block_reserved_weight = fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(
                MINIMUM_BLOCK_RESERVED_WEIGHT, DEFAULT_BLOCK_RESERVED_WEIGHT);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            const CAmount fee_amount = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, COIN);
            const int32_t fee_size = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(1, MAX_STANDARD_TX_WEIGHT / WITNESS_SCALE_FACTOR);
            options.block_min_fee_rate = CFeeRate(fee_amount, fee_size);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            const uint64_t reserved = options.block_reserved_weight.value_or(DEFAULT_BLOCK_RESERVED_WEIGHT);
            options.block_max_weight = fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(reserved, DEFAULT_BLOCK_MAX_WEIGHT);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            options.print_modified_fee = fuzzed_data_provider.ConsumeBool();
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            options.coinbase_output_max_additional_sigops = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, MAX_BLOCK_SIGOPS_COST);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            options.coinbase_output_script = ConsumeScript(fuzzed_data_provider);
        }
        auto block_template = block_template_manager.CreateNewTemplate(options);
        assert(block_template);
        const auto resolved{node::FlattenMiningOptions(node::MergeMiningOptions(options, block_template_manager.GetInitBlockCreateOptions()))};
        const CBlock& block{block_template->block};
        // Coinbase is first; the per-tx vectors exclude it and track the block.
        assert(!block.vtx.empty() && block.vtx[0]->IsCoinBase());
        assert(block_template->vTxFees.size() == block.vtx.size() - 1);
        assert(block_template->vTxSigOpsCost.size() == block.vtx.size() - 1);
        // Reserved weight plus selected transactions stays within the limit. The
        // coinbase is covered by the reserved weight, so it is not counted here.
        uint64_t weight{*resolved.block_reserved_weight};
        for (size_t i{1}; i < block.vtx.size(); ++i)
            weight += GetTransactionWeight(*block.vtx[i]);
        assert(weight <= *resolved.block_max_weight);
        // Reserved coinbase sigops plus selected transactions stays within the limit.
        int64_t sigops = resolved.coinbase_output_max_additional_sigops;
        for (const int64_t tx_sigops : block_template->vTxSigOpsCost)
            sigops += tx_sigops;
        assert(sigops <= MAX_BLOCK_SIGOPS_COST);
        // Tracked fees are non-negative; without the mempool only the coinbase is included.
        for (const CAmount fee : block_template->vTxFees)
            assert(fee >= 0);
        if (!resolved.use_mempool) assert(block.vtx.size() == 1);
    }
}
