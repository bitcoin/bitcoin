// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <kernel/chain.h>
#include <kernel/mempool_entry.h>
#include <kernel/types.h>
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
#include <util/feefrac.h>
#include <util/hasher.h>
#include <util/time.h>
#include <validationinterface.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
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
    // Keep template creation from consuming bytes needed by the state machine.
    FuzzedDataProvider template_creation_provider{buffer.data(), buffer.size()};
    const auto& node = g_setup->m_node;
    auto& block_template_manager = *Assert(node.block_template_manager);
    auto& mempool = *Assert(node.mempool);
    SeedRandomStateForTest(SeedRand::ZEROS);
    SetMockTime(WITH_LOCK(node.chainman->GetMutex(),
                          return node.chainman->ActiveTip()->Time()));
    node.validation_signals->SyncWithValidationInterfaceQueue();
    const auto chain_tip = WITH_LOCK(node.chainman->GetMutex(), return node.chainman->ActiveTip());
    const auto block = std::make_shared<const CBlock>();
    node.validation_signals->BlockConnected(kernel::ChainstateRole{}, block, chain_tip);
    node.validation_signals->SyncWithValidationInterfaceQueue();
    {
        LOCK(mempool.cs);
        mempool.TrimToSize(0);
    }
    const int weight_budget = template_creation_provider.ConsumeIntegralInRange<int>(0, DEFAULT_BLOCK_MAX_WEIGHT * 10);
    PopulateRandTransactionsToMempool(template_creation_provider, mempool, weight_budget);

    BlockCreateOptions options;
    options.test_block_validity = false;
    if (template_creation_provider.ConsumeBool()) {
        options.use_mempool = template_creation_provider.ConsumeBool();
    }
    if (template_creation_provider.ConsumeBool()) {
        options.block_reserved_weight = template_creation_provider.ConsumeIntegralInRange<uint64_t>(
            MINIMUM_BLOCK_RESERVED_WEIGHT, DEFAULT_BLOCK_RESERVED_WEIGHT);
    }
    if (template_creation_provider.ConsumeBool()) {
        const CAmount fee_amount = template_creation_provider.ConsumeIntegralInRange<CAmount>(0, COIN);
        const int32_t fee_size = template_creation_provider.ConsumeIntegralInRange<int32_t>(1, MAX_STANDARD_TX_WEIGHT / WITNESS_SCALE_FACTOR);
        options.block_min_fee_rate = CFeeRate(fee_amount, fee_size);
    }
    if (template_creation_provider.ConsumeBool()) {
        const uint64_t reserved = options.block_reserved_weight.value_or(DEFAULT_BLOCK_RESERVED_WEIGHT);
        options.block_max_weight = template_creation_provider.ConsumeIntegralInRange<uint64_t>(reserved, DEFAULT_BLOCK_MAX_WEIGHT);
    }
    if (template_creation_provider.ConsumeBool()) {
        options.print_modified_fee = template_creation_provider.ConsumeBool();
    }
    if (template_creation_provider.ConsumeBool()) {
        options.coinbase_output_max_additional_sigops = template_creation_provider.ConsumeIntegralInRange<size_t>(0, MAX_BLOCK_SIGOPS_COST);
    }
    if (template_creation_provider.ConsumeBool()) {
        options.coinbase_output_script = ConsumeScript(template_creation_provider);
    }
    auto block_template = block_template_manager.CreateNewTemplate(options);
    assert(block_template);
    const auto resolved{node::FlattenMiningOptions(node::MergeMiningOptions(options, block_template_manager.GetInitBlockCreateOptions()))};
    const CBlock& template_block{block_template->block};
    // Coinbase is first; the per-tx vectors exclude it and track the block.
    assert(!template_block.vtx.empty() && template_block.vtx[0]->IsCoinBase());
    assert(block_template->vTxFees.size() == template_block.vtx.size() - 1);
    assert(block_template->vTxSigOpsCost.size() == template_block.vtx.size() - 1);
    // Reserved weight plus selected transactions stays within the limit. The
    // coinbase is covered by the reserved weight, so it is not counted here.
    uint64_t weight{*resolved.block_reserved_weight};
    for (size_t i{1}; i < template_block.vtx.size(); ++i) {
        weight += GetTransactionWeight(*template_block.vtx[i]);
    }
    assert(weight <= *resolved.block_max_weight);
    // Reserved coinbase sigops plus selected transactions stays within the limit.
    int64_t sigops = resolved.coinbase_output_max_additional_sigops;
    for (const int64_t tx_sigops : block_template->vTxSigOpsCost) {
        sigops += tx_sigops;
    }
    assert(sigops <= MAX_BLOCK_SIGOPS_COST);
    // Tracked fees are non-negative; without the mempool only the coinbase is included.
    for (const CAmount fee : block_template->vTxFees) {
        assert(fee >= 0);
    }
    if (!resolved.use_mempool) assert(template_block.vtx.size() == 1);

    // Use an empty mempool so fee inflow can be modeled independently.
    {
        LOCK(mempool.cs);
        mempool.TrimToSize(0);
    }
    node.validation_signals->SyncWithValidationInterfaceQueue();
    node.validation_signals->BlockConnected(kernel::ChainstateRole{}, block, chain_tip);
    node.validation_signals->SyncWithValidationInterfaceQueue();

    BlockCreateOptions staleness_options;
    staleness_options.test_block_validity = false;
    // Model the public staleness state independently from TemplateSnapshot.
    // tracked_template_id keys the lookups.
    uint64_t tracked_template_id{0};
    uint256 tracked_tip_hash;
    bool expect_tracking{false};
    std::map<uint256, CAmount> expected_chunk_fees;
    CAmount expected_fee_inflow{0};
    CAmount previous_fee_threshold{0};

    const auto clear_expected_tracking = [&] {
        expect_tracking = false;
        expected_chunk_fees.clear();
        expected_fee_inflow = 0;
    };
    const auto start_tracking = [&] {
        const auto new_template = block_template_manager.CreateNewTemplate(staleness_options, &tracked_template_id);
        // The empty mempool makes every accepted chunk post-creation fee inflow.
        assert(new_template && new_template->m_template_chunks.empty());
        tracked_tip_hash = new_template->block.hashPrevBlock;
        expect_tracking = true;
        expected_chunk_fees.clear();
        expected_fee_inflow = 0;
    };
    const auto assert_staleness = [&](CAmount fee_threshold) {
        const bool expected_stale = expect_tracking && expected_fee_inflow >= fee_threshold;
        assert(block_template_manager.IsTrackingFeeInflow(tracked_template_id) == expect_tracking);
        assert(block_template_manager.IsStale(staleness_options, tracked_template_id, fee_threshold) == expected_stale);
        assert(!block_template_manager.IsStale(staleness_options, /*template_id=*/0, fee_threshold));
        BlockCreateOptions different_options{staleness_options};
        different_options.use_mempool = !different_options.use_mempool;
        assert(!block_template_manager.IsStale(different_options, tracked_template_id, fee_threshold));
    };
    const auto update_mempool = [&] {
        const uint8_t transaction_nonce = fuzzed_data_provider.ConsumeIntegral<uint8_t>();
        CMutableTransaction mutable_tx;
        mutable_tx.version = 2;
        mutable_tx.vout.emplace_back(0, CScript{} << OP_RETURN << int64_t{transaction_nonce});
        const auto tx = MakeTransactionRef(std::move(mutable_tx));
        const auto fee = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(1, COIN);
        const FeePerWeight feerate{fee, 1};
        const uint256 chunk_hash = GetHashFromWitnesses({tx->GetWitnessHash()});
        const bool remove_chunk = fuzzed_data_provider.ConsumeBool();
        const bool add_chunk = fuzzed_data_provider.ConsumeBool();
        const auto removal_reason = fuzzed_data_provider.PickValueInArray({
            MemPoolRemovalReason::EXPIRY,
            MemPoolRemovalReason::SIZELIMIT,
            MemPoolRemovalReason::REORG,
            MemPoolRemovalReason::BLOCK,
            MemPoolRemovalReason::CONFLICT,
            MemPoolRemovalReason::REPLACED,
        });
        MemPoolChunksUpdate update;
        update.reason = removal_reason;
        if (remove_chunk) update.old_chunks.push_back({feerate, {tx}, 0, std::nullopt});
        if (add_chunk) update.new_chunks.push_back({feerate, {tx}, 0, std::nullopt});
        node.validation_signals->MempoolUpdated(std::move(update));
        node.validation_signals->SyncWithValidationInterfaceQueue();
        // BLOCK and CONFLICT updates are ignored; the corresponding chain
        // transition prunes templates built on its old tip.
        if (!expect_tracking ||
            removal_reason == MemPoolRemovalReason::BLOCK ||
            removal_reason == MemPoolRemovalReason::CONFLICT) {
            return;
        }
        if (remove_chunk) {
            if (const auto it = expected_chunk_fees.find(chunk_hash); it != expected_chunk_fees.end()) {
                expected_fee_inflow -= it->second;
                expected_chunk_fees.erase(it);
            }
        }
        if (add_chunk && !expected_chunk_fees.contains(chunk_hash)) {
            expected_fee_inflow += fee;
            expected_chunk_fees[chunk_hash] = fee;
        }
    };
    const auto stop_or_restart_tracking = [&] {
        block_template_manager.StopTrackingFeeInflow(tracked_template_id);
        clear_expected_tracking();
        if (fuzzed_data_provider.ConsumeBool()) start_tracking();
    };
    const auto send_block_notification = [&] {
        const bool historical = fuzzed_data_provider.ConsumeBool();
        const bool connect = fuzzed_data_provider.ConsumeBool();
        CBlock notification_block{*block};
        if (connect && fuzzed_data_provider.ConsumeBool()) {
            notification_block.hashPrevBlock = tracked_tip_hash;
        }
        const auto notification = std::make_shared<const CBlock>(std::move(notification_block));
        if (connect) {
            node.validation_signals->BlockConnected(kernel::ChainstateRole{.historical = historical}, notification, chain_tip);
        } else {
            node.validation_signals->BlockDisconnected(notification, chain_tip);
        }
        node.validation_signals->SyncWithValidationInterfaceQueue();
        const uint256 old_tip_hash = connect ? notification->hashPrevBlock : notification->GetHash();
        if ((!connect || !historical) && tracked_tip_hash == old_tip_hash) clear_expected_tracking();
    };
    const auto create_untracked_template = [&] {
        const auto untracked_template = block_template_manager.CreateNewTemplate(staleness_options);
        // Untracked templates carry no id; the reserved id 0 is never tracked.
        assert(untracked_template && !block_template_manager.IsTrackingFeeInflow(0));
    };

    start_tracking();
    node.validation_signals->BlockDisconnected(block, chain_tip);
    node.validation_signals->SyncWithValidationInterfaceQueue();
    assert(block_template_manager.IsTrackingFeeInflow(tracked_template_id));
    CBlock matching_connect;
    matching_connect.hashPrevBlock = tracked_tip_hash;
    node.validation_signals->BlockConnected(kernel::ChainstateRole{}, std::make_shared<const CBlock>(matching_connect), chain_tip);
    node.validation_signals->SyncWithValidationInterfaceQueue();
    clear_expected_tracking();
    assert(!block_template_manager.IsTrackingFeeInflow(tracked_template_id));
    start_tracking();
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes() > 0, 100)
    {
        CallOneOf(fuzzed_data_provider, update_mempool, stop_or_restart_tracking, send_block_notification, create_untracked_template);
        const CAmount fee_threshold = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, 10 * COIN);
        assert_staleness(fee_threshold);
        // Re-query old and exact fee thresholds to ensure IsStale has no query state
        // and treats equality as sufficient fee inflow.
        assert_staleness(previous_fee_threshold);
        assert_staleness(expected_fee_inflow);
        assert_staleness(expected_fee_inflow + 1);
        previous_fee_threshold = fee_threshold;
        block_template_manager.SanityCheck();
    }
    node.validation_signals->BlockDisconnected(block, chain_tip);
    node.validation_signals->SyncWithValidationInterfaceQueue();
}
