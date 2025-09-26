// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <kernel/mempool_entry.h>
#include <kernel/mempool_removal_reason.h>
#include <logging.h>
#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/forecaster.h>
#include <policy/fees/forecaster_man.h>
#include <policy/fees/forecaster_util.h>
#include <policy/policy.h>
#include <validationinterface.h>

#include <algorithm>
#include <utility>

FeeRateForecasterManager::FeeRateForecasterManager()
{
    LOCK(cs);
    prev_mined_blocks.reserve(NUMBER_OF_BLOCKS);
}

void FeeRateForecasterManager::RegisterForecaster(std::shared_ptr<Forecaster> forecaster)
{
    LOCK(cs);
    forecasters.emplace(forecaster->GetForecastType(), std::move(forecaster));
}

// Retrieves the block policy estimator if available
CBlockPolicyEstimator* FeeRateForecasterManager::GetBlockPolicyEstimator()
{
    LOCK(cs);
    Assert(forecasters.contains(ForecastType::BLOCK_POLICY));
    Forecaster* block_policy_estimator = forecasters.find(ForecastType::BLOCK_POLICY)->second.get();
    return dynamic_cast<CBlockPolicyEstimator*>(block_policy_estimator);
}

std::pair<ForecastResult, std::vector<std::string>> FeeRateForecasterManager::ForecastFeeRateFromForecasters(
    int target, bool conservative) const
{
    LOCK(cs);
    std::vector<std::string> err_messages;
    ForecastResult feerate_forecast;

    const bool mempool_healthy = IsMempoolHealthy();
    if (!mempool_healthy) {
        err_messages.emplace_back("Mempool is unreliable for fee rate forecasting.");
    }

    for (const auto& [type, forecaster] : forecasters) {
        if (!mempool_healthy && type == ForecastType::MEMPOOL_FORECAST) continue;
        auto curr_forecast = forecaster->ForecastFeeRate(target, conservative);

        if (curr_forecast.error.has_value()) {
            err_messages.emplace_back(
                strprintf("%s: %s", forecastTypeToString(type), curr_forecast.error.value()));
        }

        // Handle case where the block policy forecaster does not have enough data.
        if (type == ForecastType::BLOCK_POLICY && curr_forecast.feerate.IsEmpty()) {
            return {ForecastResult(), err_messages};
        }

        if (!curr_forecast.feerate.IsEmpty()) {
            if (feerate_forecast.feerate.IsEmpty()) {
                // If there's no selected forecast, choose curr_forecast as feerate_forecast.
                feerate_forecast = curr_forecast;
            } else {
                // Otherwise, choose the smaller as feerate_forecast.
                feerate_forecast = std::min(feerate_forecast, curr_forecast);
            }
        }
    }

    if (!feerate_forecast.feerate.IsEmpty()) {
        Assume(feerate_forecast.forecaster);
        LogInfo("Fee rate Forecaster %s: Block height %s, fee rate %s %s/kvB.\n",
                forecastTypeToString(feerate_forecast.forecaster.value()),
                feerate_forecast.current_block_height,
                CFeeRate(feerate_forecast.feerate.fee, feerate_forecast.feerate.size).GetFeePerK(),
                CURRENCY_ATOM);
    }

    return {feerate_forecast, err_messages};
}

unsigned int FeeRateForecasterManager::MaximumTarget() const
{
    LOCK(cs);
    unsigned int maximum_target{0};
    for (const auto& forecaster : forecasters) {
        maximum_target = std::max(maximum_target, forecaster.second->MaximumTarget());
    }
    return maximum_target;
}

// Checks if the mempool is in a healthy state for fee rate forecasting
bool FeeRateForecasterManager::IsMempoolHealthy() const
{
    AssertLockHeld(cs);
    if (prev_mined_blocks.size() < NUMBER_OF_BLOCKS) return false;

    for (size_t i = 1; i < prev_mined_blocks.size(); ++i) {
        const auto& current = prev_mined_blocks[i];
        const auto& previous = prev_mined_blocks[i - 1];

        // TODO: Instead of just returning false; prevent all cases where blocks are disconnected (i.e reorg)
        // then assume this should not happen
        if (current.m_height != previous.m_height + 1) return false;

        // TODO: Ensure validation interface sync before calling this, then throw if missing block weight.
        if (!current.m_block_weight) return false;

        // Handle when the node has seen none of the block transactions then return false.
        if (current.m_removed_block_txs_weight == 0 && !current.empty) return false;
        // When the block is empty then continue to next iteration.
        if (current.m_removed_block_txs_weight == 0 && current.empty) continue;
        // Ensure the node has seen most of the transactions in the block in its mempool.
        // TODO: handle cases where the block is just sparse
        if (static_cast<double>(current.m_removed_block_txs_weight) / static_cast<double>(*current.m_block_weight) < HEALTHY_BLOCK_PERCENTILE) return false;
    }
    return true;
}

// Calculates total block weight excluding the coinbase transaction
size_t FeeRateForecasterManager::CalculateBlockWeight(const std::vector<CTransactionRef>& txs) const
{
    AssertLockHeld(cs);
    return std::accumulate(txs.begin() + 1, txs.end(), 0u, [](size_t acc, const CTransactionRef& tx) {
        return acc + GetTransactionWeight(*tx);
    });
}

// Updates mempool data for removed transactions during block connection
void FeeRateForecasterManager::MempoolTransactionsRemovedForBlock(const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, unsigned int nBlockHeight)
{
    GetBlockPolicyEstimator()->processBlock(txs_removed_for_block, nBlockHeight);
    LOCK(cs);
    size_t removed_weight = std::accumulate(txs_removed_for_block.begin(), txs_removed_for_block.end(), 0u,
                                            [](size_t acc, const auto& tx) { return acc + GetTransactionWeight(*tx.info.m_tx); });

    // TODO: Store all block health data and then use only the latest NUMBER_OF_BLOCKS for mempool health assessment
    if (prev_mined_blocks.size() == NUMBER_OF_BLOCKS) {
        prev_mined_blocks.erase(prev_mined_blocks.begin());
    }
    prev_mined_blocks.emplace_back(nBlockHeight, removed_weight);
}

// Updates previously mined block data when a new block is connected
void FeeRateForecasterManager::BlockConnected(ChainstateRole /*unused*/, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex)
{
    LOCK(cs);
    size_t height = pindex->nHeight;
    auto it = std::find_if(prev_mined_blocks.begin(), prev_mined_blocks.end(), [height](const auto& blk) {
        return blk.m_height == height;
    });
    if (it != prev_mined_blocks.end()) {
        it->m_block_weight = CalculateBlockWeight(block->vtx);
        it->empty = it->m_block_weight.value() == 0;
    }
}

void FeeRateForecasterManager::TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t /*unused*/)
{
    GetBlockPolicyEstimator()->processTransaction(tx);
}

void FeeRateForecasterManager::TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason /*unused*/, uint64_t /*unused*/)
{
    GetBlockPolicyEstimator()->removeTx(tx->GetHash());
}

std::vector<std::string> FeeRateForecasterManager::GetPreviouslyMinedBlockDataStr() const
{
    LOCK(cs);
    std::vector<std::string> block_data_strings;
    block_data_strings.reserve(prev_mined_blocks.size() + 1);
    block_data_strings.emplace_back(strprintf("Tracked %d most-recent blocks.", prev_mined_blocks.size()));
    for (const auto& block : prev_mined_blocks) {
        std::string block_str;
        if (block.m_block_weight.has_value()) {
            double percentage = (block.m_removed_block_txs_weight > 0.0) ? (static_cast<double>(block.m_removed_block_txs_weight) / static_cast<double>(block.m_block_weight.value())) * 100 : 0;
            block_str = strprintf("Block height: %d, Block weight: %d WU, total weight of txs seen in mempool %d WU (excluding coinbase tx) (%.2f%% of total weight)", block.m_height, block.m_block_weight.value(), block.m_removed_block_txs_weight, percentage);
        } else {
            block_str = strprintf("Block height: %d, Block weight: Unknown yet, total weight of txs seen in mempool %d WU (excluding coinbase tx)", block.m_height, block.m_removed_block_txs_weight);
        }
        block_data_strings.emplace_back(block_str);
    }
    return block_data_strings;
}

FeeRateForecasterManager::~FeeRateForecasterManager() = default;
