// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_FORECASTER_MAN_H
#define BITCOIN_POLICY_FEES_FORECASTER_MAN_H

#include <primitives/transaction.h>
#include <sync.h>
#include <threadsafety.h>

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

class CBlockPolicyEstimator;
class CValidationInterface;
class Forecaster;
class ForecastResult;

struct ConfirmationTarget;
struct RemovedMempoolTransactionInfo;

enum class ForecastType;

// Constants for mempool sanity checks.
constexpr size_t NUMBER_OF_BLOCKS = 6;
constexpr double HEALTHY_BLOCK_PERCENTILE = 0.75;

/**
 * \class FeeRateForecasterManager
 *
 * Manages multiple fee rate forecasters to estimate the safest and lowest possible
 * transaction fees based on recent block and mempool conditions.
 */
class FeeRateForecasterManager : public CValidationInterface
{
private:
    //! Map of all registered forecasters to their shared pointers.
    std::unordered_map<ForecastType, std::shared_ptr<Forecaster>> forecasters GUARDED_BY(cs);

    mutable Mutex cs;

    //! Structure to track the health of mined blocks.
    struct BlockData {
        size_t m_height;                      //!< Block height.
        bool empty{false};                    //!< Whether the block is empty.
        double m_removed_block_txs_weight;    //!< Removed transaction weight from the mempool.
        std::optional<double> m_block_weight; //!< Weight of the block.

        BlockData(size_t height, double removed_block_txs_weight)
            : m_height(height), m_removed_block_txs_weight(removed_block_txs_weight) {}
    };

    //! Tracks the statistics of previously mined blocks.
    std::vector<BlockData> prev_mined_blocks GUARDED_BY(cs);

    //! Map to track transaction mining count (fast lookup).
    std::map<Txid, size_t> tx_mine_count;

    //! Custom comparator: Sorts by count (descending).
    struct CompareTx {
        bool operator()(const std::pair<size_t, Txid>& a, const std::pair<size_t, Txid>& b) const
        {
            return a.first > b.first;
        }
    };

    //! Ordered set of transactions sorted by count.
    std::multiset<std::pair<size_t, Txid>, CompareTx> sorted_txs;

    //! Checks if recent mined blocks indicate a healthy mempool state.
    bool IsMempoolHealthy() const EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Computes the total weight of transactions in a block.
    size_t CalculateBlockWeight(const std::vector<CTransactionRef>& txs) const EXCLUSIVE_LOCKS_REQUIRED(cs);

protected:
    //! Handles transactions removed from the mempool due to a new block.
    void MempoolTransactionsRemovedForBlock(const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, const std::vector<CTransactionRef>& expected_block_txs, unsigned int nBlockHeight) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    //! Handles newly connected blocks.
    void BlockConnected(ChainstateRole /*unused*/, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

public:
    FeeRateForecasterManager() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    virtual ~FeeRateForecasterManager();

    //! Registers a new fee forecaster.
    void RegisterForecaster(std::shared_ptr<Forecaster> forecaster) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    //! Returns a pointer to the block policy estimator.
    CBlockPolicyEstimator* GetBlockPolicyEstimator() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    //! Provides access to historical block status.
    const std::vector<BlockData>& GetPreviouslyMinedBlockData() const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    //! Retrieves block data as a human-readable string.
    std::vector<std::string> GetPreviouslyMinedBlockDataStr() const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    /**
     * Estimates a fee rate using registered forecasters for a given confirmation target.
     *
     * Iterates through all registered forecasters and selects the lowest viable fee estimate
     * with acceptable confidence.
     *
     * @param[in] target The target confirmation window for the transaction.
     * @return A pair containing the forecast result (if available) and any relevant error messages.
     */
    std::pair<std::optional<ForecastResult>, std::vector<std::string>> GetFeeEstimateFromForecasters(ConfirmationTarget& target) EXCLUSIVE_LOCKS_REQUIRED(!cs);
};

#endif // BITCOIN_POLICY_FEES_FORECASTER_MAN_H
