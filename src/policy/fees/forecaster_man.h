// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_FORECASTER_MAN_H
#define BITCOIN_POLICY_FEES_FORECASTER_MAN_H

#include <primitives/transaction.h>
#include <sync.h>
#include <threadsafety.h>
#include <validationinterface.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class CBlockPolicyEstimator;
class Forecaster;

enum class ForecastType;
enum class MemPoolRemovalReason;

struct RemovedMempoolTransactionInfo;
struct NewMempoolTransactionInfo;
struct ForecastResult;

// Constants for mempool sanity checks.
constexpr size_t NUMBER_OF_BLOCKS = 6;
constexpr double HEALTHY_BLOCK_PERCENTILE = 0.75;

/** \class FeeRateForecasterManager
 * Manages multiple fee rate forecasters.
 */
class FeeRateForecasterManager : public CValidationInterface
{
private:
    //! Map of all registered forecasters to their shared pointers.
    std::unordered_map<ForecastType, std::shared_ptr<Forecaster>> forecasters GUARDED_BY(cs);

    mutable RecursiveMutex cs;

    //! Structure to track the health of mined blocks.
    struct BlockData {
        size_t m_height;                      //!< Block height.
        bool empty{false};                    //!< Whether the block is empty.
        size_t m_removed_block_txs_weight;    //!< Removed transaction weight from the mempool.
        std::optional<size_t> m_block_weight; //!< Weight of the block.

        BlockData(size_t height, size_t removed_block_txs_weight)
            : m_height(height), m_removed_block_txs_weight(removed_block_txs_weight) {}
    };

    //! Tracks the statistics of previously mined blocks.
    std::vector<BlockData> prev_mined_blocks GUARDED_BY(cs);

    //! Checks if recent mined blocks indicate a healthy mempool state.
    bool IsMempoolHealthy() const EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Computes the total weight of transactions in a block.
    size_t CalculateBlockWeight(const std::vector<CTransactionRef>& txs) const EXCLUSIVE_LOCKS_REQUIRED(cs);

protected:
    /** Overridden from CValidationInterface. */
    void TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t /*unused*/) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason /*unused*/, uint64_t /*unused*/) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void MempoolTransactionsRemovedForBlock(const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, unsigned int nBlockHeight) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void BlockConnected(ChainstateRole /*unused*/, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);


public:
    FeeRateForecasterManager() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    virtual ~FeeRateForecasterManager();

    //! Registers a new fee forecaster.
    void RegisterForecaster(std::shared_ptr<Forecaster> forecaster) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    //! Returns a pointer to the block policy estimator.
    CBlockPolicyEstimator* GetBlockPolicyEstimator() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    //! Retrieves block data as a human-readable string.
    std::vector<std::string> GetPreviouslyMinedBlockDataStr() const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    /**
     * Estimates a fee rate using registered forecasters for a given confirmation target.
     *
     * Iterates through all registered forecasters and selects the lowest viable fee estimate
     * with acceptable confidence.
     *
     * @param[in] target The target within which the transaction should be confirmed.
     * @param[in] conservative If true, returns a higher fee rate for greater confirmation probability.
     * @return A pair consisting of the forecast result and a vector of error messages.
     */
    std::pair<ForecastResult, std::vector<std::string>> ForecastFeeRateFromForecasters(int target, bool conservative) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    /**
     * @brief Returns the maximum supported confirmation target from all forecasters.
     */
    unsigned int MaximumTarget() const EXCLUSIVE_LOCKS_REQUIRED(!cs);
};

#endif // BITCOIN_POLICY_FEES_FORECASTER_MAN_H
