// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_POLICY_FEES_H
#define BITCOIN_POLICY_FEES_H

#include <consensus/amount.h>
#include <policy/feerate.h>
#include <random.h>
#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>
#include <util/fs.h>
#include <validationinterface.h>

#include <array>
#include <chrono>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>


// How often to flush fee estimates to fee_estimates.dat.
static constexpr std::chrono::hours FEE_FLUSH_INTERVAL{1};

/** fee_estimates.dat that are more than 60 hours (2.5 days) old will not be read,
 * as fee estimates are based on historical data and may be inaccurate if
 * network activity has changed.
 */
static constexpr std::chrono::hours MAX_FILE_AGE{60};

// Whether we allow importing a fee_estimates file older than MAX_FILE_AGE.
static constexpr bool DEFAULT_ACCEPT_STALE_FEE_ESTIMATES{false};

class AutoFile;
class TxConfirmStats;
struct RemovedMempoolTransactionInfo;
struct NewMempoolTransactionInfo;

/* Identifier for each of the 3 different TxConfirmStats which will track
 * history over different time horizons. */
enum class FeeEstimateHorizon {
    SHORT_HALFLIFE,
    MED_HALFLIFE,
    LONG_HALFLIFE,
};

static constexpr auto ALL_FEE_ESTIMATE_HORIZONS = std::array{
    FeeEstimateHorizon::SHORT_HALFLIFE,
    FeeEstimateHorizon::MED_HALFLIFE,
    FeeEstimateHorizon::LONG_HALFLIFE,
};

std::string StringForFeeEstimateHorizon(FeeEstimateHorizon horizon);

/* Enumeration of reason for returned fee estimate */
enum class FeeReason {
    NONE,
    HALF_ESTIMATE,
    FULL_ESTIMATE,
    DOUBLE_ESTIMATE,
    CONSERVATIVE,
    MEMPOOL_MIN,
    PAYTXFEE,
    FALLBACK,
    REQUIRED,
};

/* Used to return detailed information about a feerate bucket */
struct EstimatorBucket
{
    double start = -1;
    double end = -1;
    double withinTarget = 0;
    double totalConfirmed = 0;
    double inMempool = 0;
    double leftMempool = 0;
};

/* Used to return detailed information about a fee estimate calculation */
struct EstimationResult
{
    EstimatorBucket pass;
    EstimatorBucket fail;
    double decay = 0;
    unsigned int scale = 0;
};

struct FeeCalculation
{
    EstimationResult est;
    FeeReason reason = FeeReason::NONE;
    int desiredTarget = 0;
    int returnedTarget = 0;
};

/** \class CBlockPolicyEstimator
 * The BlockPolicyEstimator is used for estimating the feerate needed
 * for a transaction to be included in a block within a certain number of
 * blocks.
 *
 * At a high level the algorithm works by grouping transactions into buckets
 * based on having similar feerates and then tracking how long it
 * takes transactions in the various buckets to be mined.  It operates under
 * the assumption that in general transactions of higher feerate will be
 * included in blocks before transactions of lower feerate.   So for
 * example if you wanted to know what feerate you should put on a transaction to
 * be included in a block within the next 5 blocks, you would start by looking
 * at the bucket with the highest feerate transactions and verifying that a
 * sufficiently high percentage of them were confirmed within 5 blocks and
 * then you would look at the next highest feerate bucket, and so on, stopping at
 * the last bucket to pass the test.   The average feerate of transactions in this
 * bucket will give you an indication of the lowest feerate you can put on a
 * transaction and still have a sufficiently high chance of being confirmed
 * within your desired 5 blocks.
 *
 * Here is a brief description of the implementation:
 * When a transaction enters the mempool, we track the height of the block chain
 * at entry.  All further calculations are conducted only on this set of "seen"
 * transactions. Whenever a block comes in, we count the number of transactions
 * in each bucket and the total amount of feerate paid in each bucket. Then we
 * calculate how many blocks Y it took each transaction to be mined.  We convert
 * from a number of blocks to a number of periods Y' each encompassing "scale"
 * blocks.  This is tracked in 3 different data sets each up to a maximum
 * number of periods. Within each data set we have an array of counters in each
 * feerate bucket and we increment all the counters from Y' up to max periods
 * representing that a tx was successfully confirmed in less than or equal to
 * that many periods. We want to save a history of this information, so at any
 * time we have a counter of the total number of transactions that happened in a
 * given feerate bucket and the total number that were confirmed in each of the
 * periods or less for any bucket.  We save this history by keeping an
 * exponentially decaying moving average of each one of these stats.  This is
 * done for a different decay in each of the 3 data sets to keep relevant data
 * from different time horizons.  Furthermore we also keep track of the number
 * unmined (in mempool or left mempool without being included in a block)
 * transactions in each bucket and for how many blocks they have been
 * outstanding and use both of these numbers to increase the number of transactions
 * we've seen in that feerate bucket when calculating an estimate for any number
 * of confirmations below the number of blocks they've been outstanding.
 *
 *  We want to be able to estimate feerates that are needed on tx's to be included in
 * a certain number of blocks.  Every time a block is added to the best chain, this class records
 * stats on the transactions included in that block
 */
class CBlockPolicyEstimator : public CValidationInterface
{
private:
    /** Track confirm delays up to 12 blocks for short horizon */
    static constexpr unsigned int SHORT_BLOCK_PERIODS = 12;
    static constexpr unsigned int SHORT_SCALE = 1;
    /** Track confirm delays up to 48 blocks for medium horizon */
    static constexpr unsigned int MED_BLOCK_PERIODS = 24;
    static constexpr unsigned int MED_SCALE = 2;
    /** Track confirm delays up to 1008 blocks for long horizon */
    static constexpr unsigned int LONG_BLOCK_PERIODS = 42;
    static constexpr unsigned int LONG_SCALE = 24;
    /** Historical estimates that are older than this aren't valid */
    static const unsigned int OLDEST_ESTIMATE_HISTORY = 6 * 1008;

    /** Decay of .962 is a half-life of 18 blocks or about 3 hours */
    static constexpr double SHORT_DECAY = .962;
    /** Decay of .9952 is a half-life of 144 blocks or about 1 day */
    static constexpr double MED_DECAY = .9952;
    /** Decay of .99931 is a half-life of 1008 blocks or about 1 week */
    static constexpr double LONG_DECAY = .99931;

    /** Require greater than 60% of X feerate transactions to be confirmed within Y/2 blocks*/
    static constexpr double HALF_SUCCESS_PCT = .6;
    /** Require greater than 85% of X feerate transactions to be confirmed within Y blocks*/
    static constexpr double SUCCESS_PCT = .85;
    /** Require greater than 95% of X feerate transactions to be confirmed within 2 * Y blocks*/
    static constexpr double DOUBLE_SUCCESS_PCT = .95;

    /** Require an avg of 0.1 tx in the combined feerate bucket per block to have stat significance */
    static constexpr double SUFFICIENT_FEETXS = 0.1;
    /** Require an avg of 0.5 tx when using short decay since there are fewer blocks considered*/
    static constexpr double SUFFICIENT_TXS_SHORT = 0.5;

    /** Minimum and Maximum values for tracking feerates
     * The MIN_BUCKET_FEERATE should just be set to the lowest reasonable feerate we
     * might ever want to track.  Historically this has been 1000 since it was
     * inheriting DEFAULT_MIN_RELAY_TX_FEE and changing it is disruptive as it
     * invalidates old estimates files. So leave it at 1000 unless it becomes
     * necessary to lower it, and then lower it substantially.
     */
    static constexpr double MIN_BUCKET_FEERATE = 1000;
    static constexpr double MAX_BUCKET_FEERATE = 1e7;

    /** Spacing of FeeRate buckets
     * We have to lump transactions into buckets based on feerate, but we want to be able
     * to give accurate estimates over a large range of potential feerates
     * Therefore it makes sense to exponentially space the buckets
     */
    static constexpr double FEE_SPACING = 1.05;

    const fs::path m_estimation_filepath;
public:
    /** Create new BlockPolicyEstimator and initialize stats tracking classes with default values */
    CBlockPolicyEstimator(const fs::path& estimation_filepath, const bool read_stale_estimates);
    virtual ~CBlockPolicyEstimator();

    /** Process all the transactions that have been included in a block */
    void processBlock(const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block,
                      unsigned int nBlockHeight)
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);

    /** Process a transaction accepted to the mempool*/
    void processTransaction(const NewMempoolTransactionInfo& tx)
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);

    /** Remove a transaction from the mempool tracking stats for non BLOCK removal reasons*/
    bool removeTx(uint256 hash)
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);

    /** DEPRECATED. Return a feerate estimate */
    CFeeRate estimateFee(int confTarget) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);

    /** Estimate feerate needed to get be included in a block within confTarget
     *  blocks. If no answer can be given at confTarget, return an estimate at
     *  the closest target where one can be given.  'conservative' estimates are
     *  valid over longer time horizons also.
     */
    CFeeRate estimateSmartFee(int confTarget, FeeCalculation *feeCalc, bool conservative) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);

    /** Return a specific fee estimate calculation with a given success
     * threshold and time horizon, and optionally return detailed data about
     * calculation
     */
    CFeeRate estimateRawFee(int confTarget, double successThreshold, FeeEstimateHorizon horizon,
                            EstimationResult* result = nullptr) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);

    /** Write estimation data to a file */
    bool Write(AutoFile& fileout) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);

    /** Read estimation data from a file */
    bool Read(AutoFile& filein)
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);

    /** Empty mempool transactions on shutdown to record failure to confirm for txs still in mempool */
    void FlushUnconfirmed()
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);

    /** Calculation of highest target that estimates are tracked for */
    unsigned int HighestTargetTracked(FeeEstimateHorizon horizon) const
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);

    /** Drop still unconfirmed transactions and record current estimations, if the fee estimation file is present. */
    void Flush()
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);

    /** Record current fee estimations. */
    void FlushFeeEstimates()
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);

    /** Calculates the age of the file, since last modified */
    std::chrono::hours GetFeeEstimatorFileAge();

protected:
    /** Overridden from CValidationInterface. */
    void TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t /*unused*/) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);
    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason /*unused*/, uint64_t /*unused*/) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);
    void MempoolTransactionsRemovedForBlock(const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, unsigned int nBlockHeight) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_cs_fee_estimator);

private:
    mutable Mutex m_cs_fee_estimator;

    unsigned int nBestSeenHeight GUARDED_BY(m_cs_fee_estimator){0};
    unsigned int firstRecordedHeight GUARDED_BY(m_cs_fee_estimator){0};
    unsigned int historicalFirst GUARDED_BY(m_cs_fee_estimator){0};
    unsigned int historicalBest GUARDED_BY(m_cs_fee_estimator){0};

    struct TxStatsInfo
    {
        unsigned int blockHeight{0};
        unsigned int bucketIndex{0};
        TxStatsInfo() = default;
    };

    // map of txids to information about that transaction
    std::map<uint256, TxStatsInfo> mapMemPoolTxs GUARDED_BY(m_cs_fee_estimator);

    /** Classes to track historical data on transaction confirmations */
    std::unique_ptr<TxConfirmStats> feeStats PT_GUARDED_BY(m_cs_fee_estimator);
    std::unique_ptr<TxConfirmStats> shortStats PT_GUARDED_BY(m_cs_fee_estimator);
    std::unique_ptr<TxConfirmStats> longStats PT_GUARDED_BY(m_cs_fee_estimator);

    unsigned int trackedTxs GUARDED_BY(m_cs_fee_estimator){0};
    unsigned int untrackedTxs GUARDED_BY(m_cs_fee_estimator){0};

    std::vector<double> buckets GUARDED_BY(m_cs_fee_estimator); // The upper-bound of the range for the bucket (inclusive)
    std::map<double, unsigned int> bucketMap GUARDED_BY(m_cs_fee_estimator); // Map of bucket upper-bound to index into all vectors by bucket

    /** Process a transaction confirmed in a block*/
    bool processBlockTx(unsigned int nBlockHeight, const RemovedMempoolTransactionInfo& tx) EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);

    /** Helper for estimateSmartFee */
    double estimateCombinedFee(unsigned int confTarget, double successThreshold, bool checkShorterHorizon, EstimationResult *result) const EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);
    /** Helper for estimateSmartFee */
    double estimateConservativeFee(unsigned int doubleTarget, EstimationResult *result) const EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);
    /** Number of blocks of data recorded while fee estimates have been running */
    unsigned int BlockSpan() const EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);
    /** Number of blocks of recorded fee estimate data represented in saved data file */
    unsigned int HistoricalBlockSpan() const EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);
    /** Calculation of highest target that reasonable estimate can be provided for */
    unsigned int MaxUsableEstimate() const EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);

    /** A non-thread-safe helper for the removeTx function */
    bool _removeTx(const uint256& hash, bool inBlock)
        EXCLUSIVE_LOCKS_REQUIRED(m_cs_fee_estimator);
};

class FeeFilterRounder
{
private:
    static constexpr double MAX_FILTER_FEERATE = 1e7;
    /** FEE_FILTER_SPACING is just used to provide some quantization of fee
     * filter results.  Historically it reused FEE_SPACING, but it is completely
     * unrelated, and was made a separate constant so the two concepts are not
     * tied together */
    static constexpr double FEE_FILTER_SPACING = 1.1;

public:
    /** Create new FeeFilterRounder */
    explicit FeeFilterRounder(const CFeeRate& min_incremental_fee, FastRandomContext& rng);

    /** Quantize a minimum fee for privacy purpose before broadcast. */
    CAmount round(CAmount currentMinFee) EXCLUSIVE_LOCKS_REQUIRED(!m_insecure_rand_mutex);

private:
    const std::set<double> m_fee_set;
    Mutex m_insecure_rand_mutex;
    FastRandomContext& insecure_rand GUARDED_BY(m_insecure_rand_mutex);
};

#endif // BITCOIN_POLICY_FEES_H
