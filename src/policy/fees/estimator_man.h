// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license. See the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_FEES_ESTIMATOR_MAN_H
#define BITCOIN_POLICY_FEES_ESTIMATOR_MAN_H

#include <policy/fees/estimator.h>
#include <primitives/transaction.h>
#include <util/fees.h>
#include <validationinterface.h>

#include <chrono>
#include <memory>
#include <unordered_map>

class CFeeRate;

struct EstimationResult;
struct NewMempoolTransactionInfo;
struct RemovedMempoolTransactionInfo;

enum class FeeEstimateHorizon;
enum class MempoolRemovalReason;

// How often to flush data to disk
static constexpr std::chrono::hours FEE_FLUSH_INTERVAL{1};

/** \class FeeRateEstimatorManager
 * Manages multiple fee rate estimators.
 */
class FeeRateEstimatorManager : public CValidationInterface
{
private:
    //! Map of all registered estimators to their unique pointers.
    std::unordered_map<FeeRateEstimatorType, std::unique_ptr<FeeRateEstimator>> estimators;

    /**
     * Return a pointer to a fee rate estimator given an estimator type.
     */
    template <class T>
    T* GetFeeRateEstimator(FeeRateEstimatorType estimator_type)
    {
        FeeRateEstimator* estimator_ptr = estimators.find(estimator_type)->second.get();
        return dynamic_cast<T*>(estimator_ptr);
    }
public:
    virtual ~FeeRateEstimatorManager();

    /**
     * Register a fee rate estimator.
     * @param[in] estimator unique pointer to a FeeRateEstimator instance.
     */
    void RegisterFeeRateEstimator(std::unique_ptr<FeeRateEstimator> estimator);

    /* Get a fee rate estimate from block policy estimator.
     * @param[in] target The target within which the transaction should be confirmed.
     * @param[in] conservative True if the package cannot be fee bumped later.
     * @return fee rate estimator result.
     */

    virtual FeeRateEstimatorResult GetFeeRateEstimate(int target, bool conservative);

    /* Flush recorded data to disk. */
    void IntervalFlush();

    /* Flush recorded data to disk as part of shutdown sequence*/
    void ShutdownFlush();

    /**
     * @brief Returns the maximum supported confirmation target from all feerate estimator.
     */
    virtual unsigned int MaximumTarget() const;

    /**
     * @brief call block policy estimator estimaterawfee (test-only).
     *
     **/
    CFeeRate BlockPolicyEstimateRawFee(unsigned int target, double threshold, FeeEstimateHorizon horizon, EstimationResult* buckets);

    /**
     * @brief Returns the maximum supported confirmation target of block policy estimator (test-only).
     *
     */
    unsigned int BlockPolicyHighestTargetTracked(FeeEstimateHorizon horizon);

protected:
    /** Overridden from CValidationInterface. */
    void TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t /*unused*/) override;
    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason /*unused*/, uint64_t /*unused*/) override;
    void MempoolTransactionsRemovedForBlock(const std::vector<CTransactionRef>& block_txs, const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, unsigned int nBlockHeight) override;
};

#endif // BITCOIN_POLICY_FEES_ESTIMATOR_MAN_H
