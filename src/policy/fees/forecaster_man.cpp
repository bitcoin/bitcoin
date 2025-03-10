// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <kernel/mempool_entry.h>
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
    auto it = forecasters.find(ForecastType::BLOCK_POLICY);
    return (it != forecasters.end()) ? dynamic_cast<CBlockPolicyEstimator*>(it->second.get()) : nullptr;
}

// Estimates the fee rate using registered forecasters
std::pair<std::optional<ForecastResult>, std::vector<std::string>> FeeRateForecasterManager::GetFeeEstimateFromForecasters(ConfirmationTarget& target)
{
    LOCK(cs);
    std::vector<std::string> err_messages;
    std::optional<ForecastResult> selected_forecast;

    if (prev_mined_blocks.size() < NUMBER_OF_BLOCKS) {
        err_messages.emplace_back(strprintf("Insufficient data (at least %d blocks needs to be processed after startup) only %d was processed", NUMBER_OF_BLOCKS, prev_mined_blocks.size()));
        return {std::nullopt, err_messages};
    }

    const bool mempool_healthy = IsMempoolHealthy();
    if (!mempool_healthy) {
        err_messages.emplace_back("Mempool is unreliable for fee rate forecasting.");
    }

    for (const auto& [type, forecaster] : forecasters) {
        if (type == ForecastType::MEMPOOL_FORECAST) {
            if (!mempool_healthy) continue;
            target.transactions_to_ignore = GetTransactionsToIgnore();
        }

        auto forecast = forecaster->EstimateFee(target);
        if (forecast.GetError()) {
            err_messages.emplace_back(strprintf("%s: %s", forecastTypeToString(type), *forecast.GetError()));
        }

        // Select the lowest fee estimate among available (sane) forecasters
        if (!forecast.Empty() && (!selected_forecast || forecast < *selected_forecast)) {
            selected_forecast = forecast;
        }
    }

    if (selected_forecast && !selected_forecast->Empty()) {
        LogDebug(BCLog::ESTIMATEFEE,
                 "FeeEst %s: Block height %s, low priority fee rate %s %s/kvB, high priority fee rate %s %s/kvB.\n",
                 forecastTypeToString(selected_forecast->GetResponse().forecaster),
                 selected_forecast->GetResponse().current_block_height,
                 CFeeRate(selected_forecast->GetResponse().low_priority.fee, selected_forecast->GetResponse().low_priority.size).GetFeePerK(),
                 CURRENCY_ATOM,
                 CFeeRate(selected_forecast->GetResponse().high_priority.fee, selected_forecast->GetResponse().high_priority.size).GetFeePerK(),
                 CURRENCY_ATOM);
    }
    return {selected_forecast, err_messages};
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
        // Ensure the node has seen most of the transactions in the block in it's mempool.
        // TODO: handle cases where the block is just sparse
        if (current.m_removed_block_txs_weight / *current.m_block_weight < HEALTHY_BLOCK_PERCENTILE) return false;
    }
    return true;
}

// Calculates total block weight excluding the coinbase transaction
size_t FeeRateForecasterManager::CalculateBlockWeight(const std::vector<CTransactionRef>& txs) const
{
    AssertLockHeld(cs);
    return std::accumulate(txs.begin() + 1, txs.end(), 0u, [](size_t acc, const CTransactionRef& tx) {
        return acc + tx->GetTotalWeight();
    });
}

// Updates previously mined block data when a new block is connected
void FeeRateForecasterManager::BlockConnected(ChainstateRole /*unused*/, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex)
{
    LOCK(cs);
    size_t height = pindex->nHeight;
    auto it = std::find_if(prev_mined_blocks.begin(), prev_mined_blocks.end(), [height](const auto& blk) {
        return blk.m_height == height;
    });

    Assume(it != prev_mined_blocks.end());
    it->m_block_weight = static_cast<double>(CalculateBlockWeight(block->vtx));
    it->empty = it->m_block_weight.value() == 0.0;
}

std::set<Txid> FeeRateForecasterManager::GetTransactionsToIgnore()
{
    std::set<Txid> transactions_to_ignore;
    // Iterate through sorted transactions (sorted by mining count in descending order)
    for (auto tx = sorted_txs.begin(); tx != sorted_txs.end(); ++tx) {
        // Stop once we reach transactions below the ignore threshold
        if (tx->first < NUMBER_OF_BLOCKS) break;
        // Add transactions that have been seen multiple times without mining
        transactions_to_ignore.insert(tx->second);
    }
    return transactions_to_ignore;
}

const std::vector<FeeRateForecasterManager::BlockData>& FeeRateForecasterManager::GetPreviouslyMinedBlockData() const
{
    LOCK(cs);
    return prev_mined_blocks;
}

std::vector<std::string> FeeRateForecasterManager::GetPreviouslyMinedBlockDataStr() const
{
    LOCK(cs);
    std::vector<std::string> block_data_strings;
    block_data_strings.reserve(prev_mined_blocks.size() + sorted_txs.size() + 2);
    block_data_strings.emplace_back(strprintf("Tracked %d most-recent blocks.", prev_mined_blocks.size()));

    for (const auto& block : prev_mined_blocks) {
        std::string block_str = strprintf("Block height: %d, %dWU txs were seen in the mempool", block.m_height, block.m_removed_block_txs_weight);

        if (!block.m_block_weight.has_value()) {
            block_str += ", Block weight: Unknown.";
        } else {
            double mempool_percentage = (block.m_removed_block_txs_weight > 0) ? (block.m_removed_block_txs_weight / block.m_block_weight.value()) * 100 : 0;
            block_str += strprintf(", Block weight: %d WU (%.2f%% from mempool).", block.m_block_weight.value(), mempool_percentage);
        }
        block_data_strings.emplace_back(block_str);
    }

    block_data_strings.emplace_back(strprintf("We suspect %d txs are been filtered by miners", sorted_txs.size()));
    for (auto& tx : sorted_txs) {
        block_data_strings.emplace_back(strprintf("%s; count %d", tx.second.ToString(), tx.first));
    }
    return block_data_strings;
}

// Updates mempool data for removed transactions during block connection
void FeeRateForecasterManager::MempoolTransactionsRemovedForBlock(const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block,
                                                                  const std::vector<CTransactionRef>& expected_block_txs, unsigned int nBlockHeight)
{
    LOCK(cs);
    // Track removed transaction IDs and compute their total virtual weight.
    std::set<Txid> removed_transactions;
    size_t removed_weight = 0;

    for (const auto& tx : txs_removed_for_block) {
        removed_weight += tx.info.m_virtual_transaction_size * WITNESS_SCALE_FACTOR;
        const Txid& txid = tx.info.m_tx->GetHash();
        removed_transactions.insert(txid);

        // Remove from tracking structures if we were tracking it
        auto it = tx_mine_count.find(txid);
        if (it != tx_mine_count.end()) {
            sorted_txs.erase({it->second, txid});
            tx_mine_count.erase(it);
        }
    }

    // TODO: Store all block health data and then use only the latest NUMBER_OF_BLOCKS for mempool health assessment
    if (prev_mined_blocks.size() == NUMBER_OF_BLOCKS) {
        prev_mined_blocks.erase(prev_mined_blocks.begin());
    }
    prev_mined_blocks.emplace_back(nBlockHeight, static_cast<double>(removed_weight));

    // Process transactions that should have been mined but weren't
    if (!expected_block_txs.empty()) {
        for (auto it = expected_block_txs.begin() + 1; it != expected_block_txs.end(); ++it) { // Skip coinbase
            const Txid& txid = (*it)->GetHash();

            // Skip transactions that were already removed
            if (removed_transactions.count(txid)) continue;

            // Remove old count entry from `sorted_txs` before updating count
            auto existing = tx_mine_count.find(txid);
            if (existing != tx_mine_count.end()) {
                sorted_txs.erase({existing->second, txid});
                existing->second++; // Increment count
            } else {
                tx_mine_count[txid] = 1;
            }

            // Insert updated count entry into `sorted_txs`
            sorted_txs.insert({tx_mine_count[txid], txid});
            LogDebug(BCLog::ESTIMATEFEE, "%s expected in block \n", txid.ToString());
        }
    }
}

void FeeRateForecasterManager::RemoveTransaction(const Txid& txid)
{
    // Remove from tracking structures if we were tracking it
    auto it = tx_mine_count.find(txid);
    if (it != tx_mine_count.end()) {
        sorted_txs.erase({it->second, txid});
        tx_mine_count.erase(it);
    }
}

void FeeRateForecasterManager::TransactionRemovedFromMempool(const CTransactionRef& tx,
                                                             MemPoolRemovalReason /*unused*/, uint64_t /*unused*/)
{
    RemoveTransaction(tx->GetHash());
}

FeeRateForecasterManager::~FeeRateForecasterManager() = default;
