// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_ESTIMATOR_MAN_H
#define BITCOIN_POLICY_FEES_ESTIMATOR_MAN_H

#include <policy/fees/block_policy_estimator.h>
#include <primitives/transaction.h>
#include <util/fees.h>
#include <util/fs.h>
#include <validationinterface.h>

#include <chrono>
#include <memory>

class CFeeRate;

enum class MempoolRemovalReason;

// How often to flush data to disk
static constexpr std::chrono::hours FEE_FLUSH_INTERVAL{1};

/** \class FeeRateEstimatorManager
 * Manages fee rate estimators.
 */
class FeeRateEstimatorManager : public CValidationInterface
{
public:
    /**
     * @param[in] block_policy_path    Path to the block policy fee estimates file.
     * @param[in] read_stale_estimates Whether to load stale estimates from disk.
     */
    FeeRateEstimatorManager(const fs::path& block_policy_path, bool read_stale_estimates);

    virtual ~FeeRateEstimatorManager() = default;

    /**
     * @brief Get a fee rate estimate from block policy estimator.
     * @param[in] target The target within which the transaction should be confirmed.
     * @param[in] conservative True if the package cannot be fee bumped later.
     * @return fee rate estimator result.
     */
    virtual FeeRateEstimatorResult GetFeeRateEstimate(int target, bool conservative) const;

    /** Flush recorded data to disk. */
    void IntervalFlush();

    /** Flush recorded data to disk as part of shutdown sequence. */
    void ShutdownFlush();

    /**
     * @brief Returns the maximum supported confirmation target from all fee rate estimators.
     */
    virtual unsigned int MaximumTarget() const;

    /**
     * @brief Call block policy estimator estimaterawfee (test-only).
     */
    CFeeRate BlockPolicyEstimateRawFee(unsigned int target, double threshold, FeeEstimateHorizon horizon, EstimationResult* buckets);

    /**
     * @brief Returns the maximum supported confirmation target of block policy estimator (test-only).
     */
    unsigned int BlockPolicyHighestTargetTracked(FeeEstimateHorizon horizon);

protected:
    /** Overridden from CValidationInterface. */
    void TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t /*unused*/) override;
    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason /*unused*/, uint64_t /*unused*/) override;
    void MempoolTransactionsRemovedForBlock(const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, unsigned int nBlockHeight) override;

private:
    std::unique_ptr<CBlockPolicyEstimator> m_block_policy_estimator;
};

#endif // BITCOIN_POLICY_FEES_ESTIMATOR_MAN_H
