// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/block_template_manager.h>

#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <interfaces/types.h>
#include <kernel/chain.h>
#include <kernel/mempool_entry.h>
#include <kernel/types.h>
#include <node/miner.h>
#include <node/mining_args.h>
#include <node/mining_types.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/mining.h>
#include <test/util/random.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/feefrac.h>
#include <util/hasher.h>
#include <util/time.h>
#include <validationinterface.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using node::BlockCreateOptions;

namespace {
struct SpendableCoin {
    COutPoint outpoint;
    CAmount amount;
    bool is_coinbase;
};
const TestingSetup* g_setup;
std::vector<SpendableCoin> g_mature_coinbase_outpoints;

void initialize_block_template_manager()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
    SetMockTime(WITH_LOCK(g_setup->m_node.chainman->GetMutex(),
                          return g_setup->m_node.chainman->ActiveTip()->Time()));
    const BlockCreateOptions assembler_options{.coinbase_output_script = P2WSH_OP_TRUE};
    for (int i{0}; i < 2 * COINBASE_MATURITY; ++i) {
        COutPoint prevout{MineBlock(g_setup->m_node, assembler_options)};
        if (i < COINBASE_MATURITY) {
            LOCK(::cs_main);
            const auto coin{g_setup->m_node.chainman->ActiveChainstate().CoinsTip().GetCoin(prevout)};
            assert(coin);
            g_mature_coinbase_outpoints.push_back({prevout, coin->out.nValue, /*is_coinbase=*/true});
        }
    }
    g_setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();
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

/** Add transactions with existing inputs and valid scripts, bypassing mempool validation. */
void PopulateResolvableTransactionsToMempool(FuzzedDataProvider& fuzzed_data_provider,
                                             CTxMemPool& mempool,
                                             int weight_budget,
                                             std::vector<SpendableCoin>& spendable_outpoints)
{
    while (weight_budget > 0 && !spendable_outpoints.empty() && fuzzed_data_provider.remaining_bytes() > 0) {
        CMutableTransaction tx_mut;
        tx_mut.nLockTime = fuzzed_data_provider.ConsumeBool() ? 0 : fuzzed_data_provider.ConsumeIntegral<uint32_t>();
        // Small fan-in keeps the outpoint pool alive, so many transactions form layered clusters.
        const auto num_in = fuzzed_data_provider.ConsumeIntegralInRange<int>(1, std::min<size_t>(5, spendable_outpoints.size()));
        const auto num_out = fuzzed_data_provider.ConsumeIntegralInRange<int>(1, num_in * 2);
        std::vector<SpendableCoin> consumed_outpoints;
        CAmount amount_in{0};
        bool spends_coinbase{false};
        for (int i{0}; i < num_in; ++i) {
            const size_t outpoint_index{
                fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, spendable_outpoints.size() - 1)};
            const auto [prevout, amount, is_coinbase]{spendable_outpoints[outpoint_index]};
            spendable_outpoints.erase(spendable_outpoints.begin() + outpoint_index);
            consumed_outpoints.push_back({prevout, amount, is_coinbase});
            amount_in += amount;
            spends_coinbase |= is_coinbase;
            CTxIn in;
            in.prevout = prevout;
            // The assembler checks locktime finality but not BIP68, so keep relative locktime disabled.
            in.nSequence = ConsumeSequence(fuzzed_data_provider) | CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG;
            in.scriptWitness.stack = {WITNESS_STACK_ELEM_OP_TRUE};
            tx_mut.vin.push_back(in);
        }
        // A bare P2PK output adds legacy sigops without needing a valid signature.
        const bool add_sigops{fuzzed_data_provider.ConsumeBool()};
        const CAmount fee{fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, amount_in)};
        const CAmount amount_out{(amount_in - fee) / num_out};
        for (int i{0}; i < num_out; ++i) {
            if (i == 0 && add_sigops) {
                tx_mut.vout.emplace_back(amount_out, CScript() << std::vector<unsigned char>(33, 0x02) << OP_CHECKSIG);
            } else {
                tx_mut.vout.emplace_back(amount_out, P2WSH_OP_TRUE);
            }
        }
        CTransactionRef tx{MakeTransactionRef(tx_mut)};
        // The output division remainder also goes to fees.
        const CAmount actual_fee{amount_in - amount_out * num_out};
        TestMemPoolEntryHelper entry;
        CTxMemPoolEntry mempool_entry{
            entry.Fee(actual_fee).SpendsCoinbase(spends_coinbase).SigOpsCost(add_sigops ? WITNESS_SCALE_FACTOR : 0).FromTx(tx)};
        const int32_t tx_weight{mempool_entry.GetTxWeight()};
        if (tx_weight > weight_budget) {
            spendable_outpoints.insert(spendable_outpoints.end(), consumed_outpoints.begin(), consumed_outpoints.end());
            break;
        }
        TryAddToMempool(mempool, mempool_entry);
        // Only offer the outputs for spending when the mempool accepted the transaction.
        if (!mempool.exists(tx->GetHash())) {
            spendable_outpoints.insert(spendable_outpoints.end(), consumed_outpoints.begin(), consumed_outpoints.end());
            continue;
        }
        weight_budget -= tx_weight;
        for (uint32_t n{add_sigops ? 1u : 0u}; n < tx->vout.size(); ++n) {
            spendable_outpoints.push_back({COutPoint{tx->GetHash(), n}, tx->vout[n].nValue, /*is_coinbase=*/false});
        }
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
    const bool use_valid_transactions{fuzzed_data_provider.ConsumeBool()};
    std::vector<SpendableCoin> spendable_outpoints;
    int weight_budget = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, DEFAULT_BLOCK_MAX_WEIGHT * 10);
    if (use_valid_transactions) {
        spendable_outpoints = g_mature_coinbase_outpoints;
        PopulateResolvableTransactionsToMempool(fuzzed_data_provider, mempool, weight_budget, spendable_outpoints);
    } else {
        PopulateRandTransactionsToMempool(fuzzed_data_provider, mempool, weight_budget);
    }
    LIMITED_WHILE (fuzzed_data_provider.remaining_bytes() > 0, 10) {
        BlockCreateOptions options;
        options.test_block_validity = use_valid_transactions;
        if (fuzzed_data_provider.ConsumeBool()) {
            options.use_mempool = fuzzed_data_provider.ConsumeBool();
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            options.block_reserved_weight = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            const CAmount fee_amount = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, COIN);
            const int32_t fee_size = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(1, DEFAULT_BLOCK_MAX_WEIGHT / WITNESS_SCALE_FACTOR);
            options.block_min_fee_rate = CFeeRate(fee_amount, fee_size);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            options.block_max_weight = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            options.print_modified_fee = fuzzed_data_provider.ConsumeBool();
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            options.coinbase_output_max_additional_sigops = fuzzed_data_provider.ConsumeIntegral<size_t>();
        }
        // An arbitrary coinbase script can exceed consensus weight/sigops limits and fail TestBlockValidity.
        if (!use_valid_transactions && fuzzed_data_provider.ConsumeBool()) {
            options.coinbase_output_script = ConsumeScript(fuzzed_data_provider);
        }
        const auto merged_options{node::MergeMiningOptions(options, block_template_manager.BlockCreateArgs())};
        const auto options_check{node::CheckMiningOptions(merged_options, /*use_argnames=*/false)};
        std::unique_ptr<node::CBlockTemplate> block_template;
        uint64_t template_id{0};
        try {
            block_template = block_template_manager.CreateNewTemplate(options, &template_id);
        } catch (const std::runtime_error& e) {
            if (!options_check) {
                assert(e.what() == util::ErrorString(options_check).original);
                continue;
            }
            // test_block_validity is set only for valid transactions, so TestBlockValidity
            // should not fail.
            throw;
        }
        assert(options_check);
        assert(block_template);
        const auto resolved{node::FlattenMiningOptions(merged_options)};
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

        const uint256 tip_hash{block.hashPrevBlock};
        // The template builds on the current tip.
        {
            const std::optional<interfaces::BlockRef> tip{block_template_manager.GetTip()};
            assert(tip && tip->hash == tip_hash);
            assert(tip->height == WITH_LOCK(node.chainman->GetMutex(), return node.chainman->ActiveHeight()));
        }

        // Zero-timeout waits still check their predicates once.
        {
            // The wait consumes and resets the interrupt flag, so keep a copy of the request.
            const bool interrupted{fuzzed_data_provider.ConsumeBool()};
            bool interrupt{false};
            if (interrupted) block_template_manager.InterruptWait(interrupt);
            assert(interrupt == interrupted);
            const uint256 current_tip{fuzzed_data_provider.ConsumeBool() ? tip_hash : ConsumeUInt256(fuzzed_data_provider)};
            MillisecondsDouble timeout{0};
            const std::optional<interfaces::BlockRef> wait_tip{block_template_manager.WaitTipChanged(current_tip, timeout, interrupt)};
            assert(!interrupt);
            if (interrupted) {
                assert(!wait_tip);
            } else {
                assert(wait_tip && wait_tip->hash == tip_hash);
            }
        }
        {
            // Fees or testnet min-difficulty can produce a template without a tip change.
            if (fuzzed_data_provider.ConsumeBool()) {
                const int extra_weight_budget{
                    fuzzed_data_provider.ConsumeIntegralInRange<int>(0, DEFAULT_BLOCK_MAX_WEIGHT)};
                if (use_valid_transactions) {
                    PopulateResolvableTransactionsToMempool(fuzzed_data_provider, mempool, extra_weight_budget, spendable_outpoints);
                } else {
                    PopulateRandTransactionsToMempool(fuzzed_data_provider, mempool, extra_weight_budget);
                }
            }
            const std::chrono::seconds mock_offset{fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, 30 * 60)};
            SetMockTime(WITH_LOCK(node.chainman->GetMutex(),
                                  return node.chainman->ActiveTip()->Time()) +
                        mock_offset);
            const bool min_difficulty_window{mock_offset > std::chrono::minutes{20}};
            node::BlockWaitOptions wait_options;
            wait_options.timeout = MillisecondsDouble{0};
            if (fuzzed_data_provider.ConsumeBool()) {
                wait_options.fee_threshold = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, MAX_MONEY);
            }
            const bool interrupted{fuzzed_data_provider.ConsumeBool()};
            bool interrupt{false};
            if (interrupted) block_template_manager.InterruptWait(interrupt);
            assert(interrupt == interrupted);
            uint64_t new_template_id{0};
            const auto next_template{block_template_manager.WaitAndCreateNewBlock(tip_hash, template_id, wait_options, options, interrupt, new_template_id)};
            assert(!interrupt);
            if (interrupted) {
                assert(!next_template);
            } else if (min_difficulty_window) {
                assert(next_template && next_template->block.hashPrevBlock == tip_hash);
            } else if (next_template) {
                assert(next_template->block.hashPrevBlock == tip_hash);
                const CAmount old_fees{std::accumulate(block_template->vTxFees.begin(), block_template->vTxFees.end(), CAmount{0})};
                const CAmount new_fees{std::accumulate(next_template->vTxFees.begin(), next_template->vTxFees.end(), CAmount{0})};
                assert(wait_options.fee_threshold < MAX_MONEY);
                assert(new_fees >= old_fees + wait_options.fee_threshold);
            }
        }
        {
            // With no headers ahead of the tip the cooldown returns
            // immediately and leaves the interrupt flag untouched.
            bool interrupt{fuzzed_data_provider.ConsumeBool()};
            const bool interrupted{interrupt};
            assert(block_template_manager.CooldownIfHeadersAhead(*block_template_manager.GetTip(), interrupt));
            assert(interrupt == interrupted);
        }
        if (fuzzed_data_provider.ConsumeBool()) {
            // An unsolved header can pass the regtest target by chance.
            if (!CheckProofOfWork(block.GetHash(), block.nBits, node.chainman->GetConsensus())) {
                std::string reason;
                std::string debug;
                assert(!block_template_manager.SubmitBlock(std::make_shared<const CBlock>(block), reason, debug));
                assert(reason == "high-hash");
                assert(debug == "proof of work failed");
            }
        }
    }
}

FUZZ_TARGET(block_template_manager_staleness, .init = initialize_block_template_manager)
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
    const auto resolved{node::FlattenMiningOptions(node::MergeMiningOptions(options, block_template_manager.BlockCreateArgs()))};
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
