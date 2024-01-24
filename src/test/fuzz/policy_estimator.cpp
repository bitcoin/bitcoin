// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/mempool_entry.h>
#include <policy/fees.h>
#include <policy/fees_args.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/setup_common.h>

#include <memory>
#include <optional>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;
} // namespace

void initialize_policy_estimator()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(policy_estimator, .init = initialize_policy_estimator)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    bool good_data{true};

    CBlockPolicyEstimator block_policy_estimator{FeeestPath(*g_setup->m_node.args), DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};

    uint32_t current_height{0};
    const auto advance_height{
        [&] { current_height = fuzzed_data_provider.ConsumeIntegralInRange<decltype(current_height)>(current_height, 1 << 30); },
    };
    advance_height();
    LIMITED_WHILE(good_data && fuzzed_data_provider.ConsumeBool(), 10'000)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const std::optional<CMutableTransaction> mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
                if (!mtx) {
                    good_data = false;
                    return;
                }
                const CTransaction tx{*mtx};
                const auto entry{ConsumeTxMemPoolEntry(fuzzed_data_provider, tx, current_height)};
                const auto tx_submitted_in_package = fuzzed_data_provider.ConsumeBool();
                const auto tx_has_mempool_parents = fuzzed_data_provider.ConsumeBool();
                const auto tx_info = NewMempoolTransactionInfo(entry.GetSharedTx(), entry.GetFee(),
                                                               entry.GetTxSize(), entry.GetHeight(),
                                                               /*mempool_limit_bypassed=*/false,
                                                               tx_submitted_in_package,
                                                               /*chainstate_is_current=*/true,
                                                               tx_has_mempool_parents);
                block_policy_estimator.processTransaction(tx_info);
                if (fuzzed_data_provider.ConsumeBool()) {
                    (void)block_policy_estimator.removeTx(tx.GetHash());
                }
            },
            [&] {
                std::list<CTxMemPoolEntry> mempool_entries;
                LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
                {
                    const std::optional<CMutableTransaction> mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
                    if (!mtx) {
                        good_data = false;
                        break;
                    }
                    const CTransaction tx{*mtx};
                    mempool_entries.emplace_back(CTxMemPoolEntry::ExplicitCopy, ConsumeTxMemPoolEntry(fuzzed_data_provider, tx, current_height));
                }
                std::vector<RemovedMempoolTransactionInfo> txs;
                txs.reserve(mempool_entries.size());
                for (const CTxMemPoolEntry& mempool_entry : mempool_entries) {
                    txs.emplace_back(mempool_entry);
                }
                advance_height();
                block_policy_estimator.processBlock(txs, current_height);
            },
            [&] {
                (void)block_policy_estimator.removeTx(ConsumeUInt256(fuzzed_data_provider));
            },
            [&] {
                block_policy_estimator.FlushUnconfirmed();
            });
        (void)block_policy_estimator.estimateFee(fuzzed_data_provider.ConsumeIntegral<int>());
        EstimationResult result;
        auto conf_target = fuzzed_data_provider.ConsumeIntegral<int>();
        auto success_threshold = fuzzed_data_provider.ConsumeFloatingPoint<double>();
        auto horizon = fuzzed_data_provider.PickValueInArray(ALL_FEE_ESTIMATE_HORIZONS);
        auto* result_ptr = fuzzed_data_provider.ConsumeBool() ? &result : nullptr;
        (void)block_policy_estimator.estimateRawFee(conf_target, success_threshold, horizon, result_ptr);

        FeeCalculation fee_calculation;
        conf_target = fuzzed_data_provider.ConsumeIntegral<int>();
        auto* fee_calc_ptr = fuzzed_data_provider.ConsumeBool() ? &fee_calculation : nullptr;
        auto conservative = fuzzed_data_provider.ConsumeBool();
        (void)block_policy_estimator.estimateSmartFee(conf_target, fee_calc_ptr, conservative);

        (void)block_policy_estimator.HighestTargetTracked(fuzzed_data_provider.PickValueInArray(ALL_FEE_ESTIMATE_HORIZONS));
    }
    {
        FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
        AutoFile fuzzed_auto_file{fuzzed_file_provider.open()};
        block_policy_estimator.Write(fuzzed_auto_file);
        block_policy_estimator.Read(fuzzed_auto_file);
        (void)fuzzed_auto_file.fclose();
    }
}
