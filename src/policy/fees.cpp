// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "policy/fees.h"
#include "policy/policy.h"

#include "amount.h"
#include "clientversion.h"
#include "primitives/transaction.h"
#include "streams.h"
#include "txmempool.h"
#include "util.h"

static constexpr double INF_FEERATE = 1e99;

std::string StringForFeeEstimateHorizon(FeeEstimateHorizon horizon) {
    static const std::map<FeeEstimateHorizon, std::string> horizon_strings = {
        {FeeEstimateHorizon::SHORT_HALFLIFE, "short"},
        {FeeEstimateHorizon::MED_HALFLIFE, "medium"},
        {FeeEstimateHorizon::LONG_HALFLIFE, "long"},
    };
    auto horizon_string = horizon_strings.find(horizon);

    if (horizon_string == horizon_strings.end()) return "unknown";

    return horizon_string->second;
}

std::string StringForFeeReason(FeeReason reason) {
    static const std::map<FeeReason, std::string> fee_reason_strings = {
        {FeeReason::NONE, "None"},
        {FeeReason::HALF_ESTIMATE, "Half Target 60% Threshold"},
        {FeeReason::FULL_ESTIMATE, "Target 85% Threshold"},
        {FeeReason::DOUBLE_ESTIMATE, "Double Target 95% Threshold"},
        {FeeReason::CONSERVATIVE, "Conservative Double Target longer horizon"},
        {FeeReason::MEMPOOL_MIN, "Mempool Min Fee"},
        {FeeReason::PAYTXFEE, "PayTxFee set"},
        {FeeReason::FALLBACK, "Fallback fee"},
        {FeeReason::REQUIRED, "Minimum Required Fee"},
        {FeeReason::MAXTXFEE, "MaxTxFee limit"}
    };
    auto reason_string = fee_reason_strings.find(reason);

    if (reason_string == fee_reason_strings.end()) return "Unknown";

    return reason_string->second;
}

bool FeeModeFromString(const std::string& mode_string, FeeEstimateMode& fee_estimate_mode) {
    static const std::map<std::string, FeeEstimateMode> fee_modes = {
        {"UNSET", FeeEstimateMode::UNSET},
        {"ECONOMICAL", FeeEstimateMode::ECONOMICAL},
        {"CONSERVATIVE", FeeEstimateMode::CONSERVATIVE},
    };
    auto mode = fee_modes.find(mode_string);

    if (mode == fee_modes.end()) return false;

    fee_estimate_mode = mode->second;
    return true;
}

/**
 * We will instantiate an instance of this class to track transactions that were
 * included in a block. We will lump transactions into a bucket according to their
 * approximate feerate and then track how long it took for those txs to be included in a block
 *
 * The tracking of unconfirmed (mempool) transactions is completely independent of the
 * historical tracking of transactions that have been confirmed in a block.
 */
class TxConfirmStats
{
private:
    //Define the buckets we will group transactions into
    const std::vector<double>& buckets;              // The upper-bound of the range for the bucket (inclusive)
    const std::map<double, unsigned int>& bucketMap; // Map of bucket upper-bound to index into all vectors by bucket

    // For each bucket X:
    // Count the total # of txs in each bucket
    // Track the historical moving average of this total over blocks
    std::vector<double> txCtAvg;

    // Count the total # of txs confirmed within Y blocks in each bucket
    // Track the historical moving average of theses totals over blocks
    std::vector<std::vector<double>> confAvg; // confAvg[Y][X]

    // Track moving avg of txs which have been evicted from the mempool
    // after failing to be confirmed within Y blocks
    std::vector<std::vector<double>> failAvg; // failAvg[Y][X]

    // Sum the total feerate of all tx's in each bucket
    // Track the historical moving average of this total over blocks
    std::vector<double> avg;

    // Combine the conf counts with tx counts to calculate the confirmation % for each Y,X
    // Combine the total value with the tx counts to calculate the avg feerate per bucket

    double decay;

    // Resolution (# of blocks) with which confirmations are tracked
    unsigned int scale;

    // Mempool counts of outstanding transactions
    // For each bucket X, track the number of transactions in the mempool
    // that are unconfirmed for each possible confirmation value Y
    std::vector<std::vector<int> > unconfTxs;  //unconfTxs[Y][X]
    // transactions still unconfirmed after GetMaxConfirms for each bucket
    std::vector<int> oldUnconfTxs;

    void resizeInMemoryCounters(size_t newbuckets);

public:
    /**
     * Create new TxConfirmStats. This is called by BlockPolicyEstimator's
     * constructor with default values.
     * @param defaultBuckets contains the upper limits for the bucket boundaries
     * @param maxPeriods max number of periods to track
     * @param decay how much to decay the historical moving average per block
     */
    TxConfirmStats(const std::vector<double>& defaultBuckets, const std::map<double, unsigned int>& defaultBucketMap,
                   unsigned int maxPeriods, double decay, unsigned int scale);

    /** Roll the circular buffer for unconfirmed txs*/
    void ClearCurrent(unsigned int nBlockHeight);

    /**
     * Record a new transaction data point in the current block stats
     * @param blocksToConfirm the number of blocks it took this transaction to confirm
     * @param val the feerate of the transaction
     * @warning blocksToConfirm is 1-based and has to be >= 1
     */
    void Record(int blocksToConfirm, double val);

    /** Record a new transaction entering the mempool*/
    unsigned int NewTx(unsigned int nBlockHeight, double val);

    /** Remove a transaction from mempool tracking stats*/
    void removeTx(unsigned int entryHeight, unsigned int nBestSeenHeight,
                  unsigned int bucketIndex, bool inBlock);

    /** Update our estimates by decaying our historical moving average and updating
        with the data gathered from the current block */
    void UpdateMovingAverages();

    /**
     * Calculate a feerate estimate.  Find the lowest value bucket (or range of buckets
     * to make sure we have enough data points) whose transactions still have sufficient likelihood
     * of being confirmed within the target number of confirmations
     * @param confTarget target number of confirmations
     * @param sufficientTxVal required average number of transactions per block in a bucket range
     * @param minSuccess the success probability we require
     * @param requireGreater return the lowest feerate such that all higher values pass minSuccess OR
     *        return the highest feerate such that all lower values fail minSuccess
     * @param nBlockHeight the current block height
     */
    double EstimateMedianVal(int confTarget, double sufficientTxVal,
                             double minSuccess, bool requireGreater, unsigned int nBlockHeight,
                             EstimationResult *result = nullptr) const;

    /** Return the max number of confirms we're tracking */
    unsigned int GetMaxConfirms() const { return scale * confAvg.size(); }

    /** Write state of estimation data to a file*/
    void Write(CAutoFile& fileout) const;

    /**
     * Read saved state of estimation data from a file and replace all internal data structures and
     * variables with this state.
     */
    void Read(CAutoFile& filein, int nFileVersion, size_t numBuckets);
};


TxConfirmStats::TxConfirmStats(const std::vector<double>& defaultBuckets,
                                const std::map<double, unsigned int>& defaultBucketMap,
                               unsigned int maxPeriods, double _decay, unsigned int _scale)
    : buckets(defaultBuckets), bucketMap(defaultBucketMap)
{
    decay = _decay;
    scale = _scale;
    confAvg.resize(maxPeriods);
    for (unsigned int i = 0; i < maxPeriods; i++) {
        confAvg[i].resize(buckets.size());
    }
    failAvg.resize(maxPeriods);
    for (unsigned int i = 0; i < maxPeriods; i++) {
        failAvg[i].resize(buckets.size());
    }

    txCtAvg.resize(buckets.size());
    avg.resize(buckets.size());

    resizeInMemoryCounters(buckets.size());
}

void TxConfirmStats::resizeInMemoryCounters(size_t newbuckets) {
    // newbuckets must be passed in because the buckets referred to during Read have not been updated yet.
    unconfTxs.resize(GetMaxConfirms());
    for (unsigned int i = 0; i < unconfTxs.size(); i++) {
        unconfTxs[i].resize(newbuckets);
    }
    oldUnconfTxs.resize(newbuckets);
}

// Roll the unconfirmed txs circular buffer
void TxConfirmStats::ClearCurrent(unsigned int nBlockHeight)
{
    for (unsigned int j = 0; j < buckets.size(); j++) {
        oldUnconfTxs[j] += unconfTxs[nBlockHeight%unconfTxs.size()][j];
        unconfTxs[nBlockHeight%unconfTxs.size()][j] = 0;
    }
}


void TxConfirmStats::Record(int blocksToConfirm, double val)
{
    // blocksToConfirm is 1-based
    if (blocksToConfirm < 1)
        return;
    int periodsToConfirm = (blocksToConfirm + scale - 1)/scale;
    unsigned int bucketindex = bucketMap.lower_bound(val)->second;
    for (size_t i = periodsToConfirm; i <= confAvg.size(); i++) {
        confAvg[i - 1][bucketindex]++;
    }
    txCtAvg[bucketindex]++;
    avg[bucketindex] += val;
}

void TxConfirmStats::UpdateMovingAverages()
{
    for (unsigned int j = 0; j < buckets.size(); j++) {
        for (unsigned int i = 0; i < confAvg.size(); i++)
            confAvg[i][j] = confAvg[i][j] * decay;
        for (unsigned int i = 0; i < failAvg.size(); i++)
            failAvg[i][j] = failAvg[i][j] * decay;
        avg[j] = avg[j] * decay;
        txCtAvg[j] = txCtAvg[j] * decay;
    }
}

// returns -1 on error conditions
double TxConfirmStats::EstimateMedianVal(int confTarget, double sufficientTxVal,
                                         double successBreakPoint, bool requireGreater,
                                         unsigned int nBlockHeight, EstimationResult *result) const
{
    // Counters for a bucket (or range of buckets)
    double nConf = 0; // Number of tx's confirmed within the confTarget
    double totalNum = 0; // Total number of tx's that were ever confirmed
    int extraNum = 0;  // Number of tx's still in mempool for confTarget or longer
    double failNum = 0; // Number of tx's that were never confirmed but removed from the mempool after confTarget
    int periodTarget = (confTarget + scale - 1)/scale;

    int maxbucketindex = buckets.size() - 1;

    // requireGreater means we are looking for the lowest feerate such that all higher
    // values pass, so we start at maxbucketindex (highest feerate) and look at successively
    // smaller buckets until we reach failure.  Otherwise, we are looking for the highest
    // feerate such that all lower values fail, and we go in the opposite direction.
    unsigned int startbucket = requireGreater ? maxbucketindex : 0;
    int step = requireGreater ? -1 : 1;

    // We'll combine buckets until we have enough samples.
    // The near and far variables will define the range we've combined
    // The best variables are the last range we saw which still had a high
    // enough confirmation rate to count as success.
    // The cur variables are the current range we're counting.
    unsigned int curNearBucket = startbucket;
    unsigned int bestNearBucket = startbucket;
    unsigned int curFarBucket = startbucket;
    unsigned int bestFarBucket = startbucket;

    bool foundAnswer = false;
    unsigned int bins = unconfTxs.size();
    bool newBucketRange = true;
    bool passing = true;
    EstimatorBucket passBucket;
    EstimatorBucket failBucket;

    // Start counting from highest(default) or lowest feerate transactions
    for (int bucket = startbucket; bucket >= 0 && bucket <= maxbucketindex; bucket += step) {
        if (newBucketRange) {
            curNearBucket = bucket;
            newBucketRange = false;
        }
        curFarBucket = bucket;
        nConf += confAvg[periodTarget - 1][bucket];
        totalNum += txCtAvg[bucket];
        failNum += failAvg[periodTarget - 1][bucket];
        for (unsigned int confct = confTarget; confct < GetMaxConfirms(); confct++)
            extraNum += unconfTxs[(nBlockHeight - confct)%bins][bucket];
        extraNum += oldUnconfTxs[bucket];
        // If we have enough transaction data points in this range of buckets,
        // we can test for success
        // (Only count the confirmed data points, so that each confirmation count
        // will be looking at the same amount of data and same bucket breaks)
        if (totalNum >= sufficientTxVal / (1 - decay)) {
            double curPct = nConf / (totalNum + failNum + extraNum);

            // Check to see if we are no longer getting confirmed at the success rate
            if ((requireGreater && curPct < successBreakPoint) || (!requireGreater && curPct > successBreakPoint)) {
                if (passing == true) {
                    // First time we hit a failure record the failed bucket
                    unsigned int failMinBucket = std::min(curNearBucket, curFarBucket);
                    unsigned int failMaxBucket = std::max(curNearBucket, curFarBucket);
                    failBucket.start = failMinBucket ? buckets[failMinBucket - 1] : 0;
                    failBucket.end = buckets[failMaxBucket];
                    failBucket.withinTarget = nConf;
                    failBucket.totalConfirmed = totalNum;
                    failBucket.inMempool = extraNum;
                    failBucket.leftMempool = failNum;
                    passing = false;
                }
                continue;
            }
            // Otherwise update the cumulative stats, and the bucket variables
            // and reset the counters
            else {
                failBucket = EstimatorBucket(); // Reset any failed bucket, currently passing
                foundAnswer = true;
                passing = true;
                passBucket.withinTarget = nConf;
                nConf = 0;
                passBucket.totalConfirmed = totalNum;
                totalNum = 0;
                passBucket.inMempool = extraNum;
                passBucket.leftMempool = failNum;
                failNum = 0;
                extraNum = 0;
                bestNearBucket = curNearBucket;
                bestFarBucket = curFarBucket;
                newBucketRange = true;
            }
        }
    }

    double median = -1;
    double txSum = 0;

    // Calculate the "average" feerate of the best bucket range that met success conditions
    // Find the bucket with the median transaction and then report the average feerate from that bucket
    // This is a compromise between finding the median which we can't since we don't save all tx's
    // and reporting the average which is less accurate
    unsigned int minBucket = std::min(bestNearBucket, bestFarBucket);
    unsigned int maxBucket = std::max(bestNearBucket, bestFarBucket);
    for (unsigned int j = minBucket; j <= maxBucket; j++) {
        txSum += txCtAvg[j];
    }
    if (foundAnswer && txSum != 0) {
        txSum = txSum / 2;
        for (unsigned int j = minBucket; j <= maxBucket; j++) {
            if (txCtAvg[j] < txSum)
                txSum -= txCtAvg[j];
            else { // we're in the right bucket
                median = avg[j] / txCtAvg[j];
                break;
            }
        }

        passBucket.start = minBucket ? buckets[minBucket-1] : 0;
        passBucket.end = buckets[maxBucket];
    }

    // If we were passing until we reached last few buckets with insufficient data, then report those as failed
    if (passing && !newBucketRange) {
        unsigned int failMinBucket = std::min(curNearBucket, curFarBucket);
        unsigned int failMaxBucket = std::max(curNearBucket, curFarBucket);
        failBucket.start = failMinBucket ? buckets[failMinBucket - 1] : 0;
        failBucket.end = buckets[failMaxBucket];
        failBucket.withinTarget = nConf;
        failBucket.totalConfirmed = totalNum;
        failBucket.inMempool = extraNum;
        failBucket.leftMempool = failNum;
    }

    LogPrint(BCLog::ESTIMATEFEE, "FeeEst: %d %s%.0f%% decay %.5f: feerate: %g from (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out) Fail: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out)\n",
             confTarget, requireGreater ? ">" : "<", 100.0 * successBreakPoint, decay,
             median, passBucket.start, passBucket.end,
             100 * passBucket.withinTarget / (passBucket.totalConfirmed + passBucket.inMempool + passBucket.leftMempool),
             passBucket.withinTarget, passBucket.totalConfirmed, passBucket.inMempool, passBucket.leftMempool,
             failBucket.start, failBucket.end,
             100 * failBucket.withinTarget / (failBucket.totalConfirmed + failBucket.inMempool + failBucket.leftMempool),
             failBucket.withinTarget, failBucket.totalConfirmed, failBucket.inMempool, failBucket.leftMempool);


    if (result) {
        result->pass = passBucket;
        result->fail = failBucket;
        result->decay = decay;
        result->scale = scale;
    }
    return median;
}

void TxConfirmStats::Write(CAutoFile& fileout) const
{
    fileout << decay;
    fileout << scale;
    fileout << avg;
    fileout << txCtAvg;
    fileout << confAvg;
    fileout << failAvg;
}

void TxConfirmStats::Read(CAutoFile& filein, int nFileVersion, size_t numBuckets)
{
    // Read data file and do some very basic sanity checking
    // buckets and bucketMap are not updated yet, so don't access them
    // If there is a read failure, we'll just discard this entire object anyway
    size_t maxConfirms, maxPeriods;

    // The current version will store the decay with each individual TxConfirmStats and also keep a scale factor
    if (nFileVersion >= 140100) {
        filein >> decay;
        if (decay <= 0 || decay >= 1) {
            throw std::runtime_error("Corrupt estimates file. Decay must be between 0 and 1 (non-inclusive)");
        }
        filein >> scale;
    }

    filein >> avg;
    if (avg.size() != numBuckets) {
        throw std::runtime_error("Corrupt estimates file. Mismatch in feerate average bucket count");
    }
    filein >> txCtAvg;
    if (txCtAvg.size() != numBuckets) {
        throw std::runtime_error("Corrupt estimates file. Mismatch in tx count bucket count");
    }
    filein >> confAvg;
    maxPeriods = confAvg.size();
    maxConfirms = scale * maxPeriods;

    if (maxConfirms <= 0 || maxConfirms > 6 * 24 * 7) { // one week
        throw std::runtime_error("Corrupt estimates file.  Must maintain estimates for between 1 and 1008 (one week) confirms");
    }
    for (unsigned int i = 0; i < maxPeriods; i++) {
        if (confAvg[i].size() != numBuckets) {
            throw std::runtime_error("Corrupt estimates file. Mismatch in feerate conf average bucket count");
        }
    }

    if (nFileVersion >= 140100) {
        filein >> failAvg;
        if (maxPeriods != failAvg.size()) {
            throw std::runtime_error("Corrupt estimates file. Mismatch in confirms tracked for failures");
        }
        for (unsigned int i = 0; i < maxPeriods; i++) {
            if (failAvg[i].size() != numBuckets) {
                throw std::runtime_error("Corrupt estimates file. Mismatch in one of failure average bucket counts");
            }
        }
    } else {
        failAvg.resize(confAvg.size());
        for (unsigned int i = 0; i < failAvg.size(); i++) {
            failAvg[i].resize(numBuckets);
        }
    }

    // Resize the current block variables which aren't stored in the data file
    // to match the number of confirms and buckets
    resizeInMemoryCounters(numBuckets);

    LogPrint(BCLog::ESTIMATEFEE, "Reading estimates: %u buckets counting confirms up to %u blocks\n",
             numBuckets, maxConfirms);
}

unsigned int TxConfirmStats::NewTx(unsigned int nBlockHeight, double val)
{
    unsigned int bucketindex = bucketMap.lower_bound(val)->second;
    unsigned int blockIndex = nBlockHeight % unconfTxs.size();
    unconfTxs[blockIndex][bucketindex]++;
    return bucketindex;
}

void TxConfirmStats::removeTx(unsigned int entryHeight, unsigned int nBestSeenHeight, unsigned int bucketindex, bool inBlock)
{
    //nBestSeenHeight is not updated yet for the new block
    int blocksAgo = nBestSeenHeight - entryHeight;
    if (nBestSeenHeight == 0)  // the BlockPolicyEstimator hasn't seen any blocks yet
        blocksAgo = 0;
    if (blocksAgo < 0) {
        LogPrint(BCLog::ESTIMATEFEE, "Blockpolicy error, blocks ago is negative for mempool tx\n");
        return;  //This can't happen because we call this with our best seen height, no entries can have higher
    }

    if (blocksAgo >= (int)unconfTxs.size()) {
        if (oldUnconfTxs[bucketindex] > 0) {
            oldUnconfTxs[bucketindex]--;
        } else {
            LogPrint(BCLog::ESTIMATEFEE, "Blockpolicy error, mempool tx removed from >25 blocks,bucketIndex=%u already\n",
                     bucketindex);
        }
    }
    else {
        unsigned int blockIndex = entryHeight % unconfTxs.size();
        if (unconfTxs[blockIndex][bucketindex] > 0) {
            unconfTxs[blockIndex][bucketindex]--;
        } else {
            LogPrint(BCLog::ESTIMATEFEE, "Blockpolicy error, mempool tx removed from blockIndex=%u,bucketIndex=%u already\n",
                     blockIndex, bucketindex);
        }
    }
    if (!inBlock && (unsigned int)blocksAgo >= scale) { // Only counts as a failure if not confirmed for entire period
        unsigned int periodsAgo = blocksAgo / scale;
        for (size_t i = 0; i < periodsAgo && i < failAvg.size(); i++) {
            failAvg[i][bucketindex]++;
        }
    }
}

// This function is called from CTxMemPool::removeUnchecked to ensure
// txs removed from the mempool for any reason are no longer
// tracked. Txs that were part of a block have already been removed in
// processBlockTx to ensure they are never double tracked, but it is
// of no harm to try to remove them again.
bool CBlockPolicyEstimator::removeTx(uint256 hash, bool inBlock)
{
    LOCK(cs_feeEstimator);
    std::map<uint256, TxStatsInfo>::iterator pos = mapMemPoolTxs.find(hash);
    if (pos != mapMemPoolTxs.end()) {
        feeStats->removeTx(pos->second.blockHeight, nBestSeenHeight, pos->second.bucketIndex, inBlock);
        shortStats->removeTx(pos->second.blockHeight, nBestSeenHeight, pos->second.bucketIndex, inBlock);
        longStats->removeTx(pos->second.blockHeight, nBestSeenHeight, pos->second.bucketIndex, inBlock);
        mapMemPoolTxs.erase(hash);
        return true;
    } else {
        return false;
    }
}

CBlockPolicyEstimator::CBlockPolicyEstimator()
    : nBestSeenHeight(0), firstRecordedHeight(0), historicalFirst(0), historicalBest(0), trackedTxs(0), untrackedTxs(0)
{
    static_assert(MIN_BUCKET_FEERATE > 0, "Min feerate must be nonzero");
    size_t bucketIndex = 0;
    for (double bucketBoundary = MIN_BUCKET_FEERATE; bucketBoundary <= MAX_BUCKET_FEERATE; bucketBoundary *= FEE_SPACING, bucketIndex++) {
        buckets.push_back(bucketBoundary);
        bucketMap[bucketBoundary] = bucketIndex;
    }
    buckets.push_back(INF_FEERATE);
    bucketMap[INF_FEERATE] = bucketIndex;
    assert(bucketMap.size() == buckets.size());

    feeStats = new TxConfirmStats(buckets, bucketMap, MED_BLOCK_PERIODS, MED_DECAY, MED_SCALE);
    shortStats = new TxConfirmStats(buckets, bucketMap, SHORT_BLOCK_PERIODS, SHORT_DECAY, SHORT_SCALE);
    longStats = new TxConfirmStats(buckets, bucketMap, LONG_BLOCK_PERIODS, LONG_DECAY, LONG_SCALE);
}

CBlockPolicyEstimator::~CBlockPolicyEstimator()
{
    delete feeStats;
    delete shortStats;
    delete longStats;
}

void CBlockPolicyEstimator::processTransaction(const CTxMemPoolEntry& entry, bool validFeeEstimate)
{
    LOCK(cs_feeEstimator);
    unsigned int txHeight = entry.GetHeight();
    uint256 hash = entry.GetTx().GetHash();
    if (mapMemPoolTxs.count(hash)) {
        LogPrint(BCLog::ESTIMATEFEE, "Blockpolicy error mempool tx %s already being tracked\n", hash.ToString());
        return;
    }

    if (txHeight != nBestSeenHeight) {
        // Ignore side chains and re-orgs; assuming they are random they don't
        // affect the estimate.  We'll potentially double count transactions in 1-block reorgs.
        // Ignore txs if BlockPolicyEstimator is not in sync with chainActive.Tip().
        // It will be synced next time a block is processed.
        return;
    }

    // Only want to be updating estimates when our blockchain is synced,
    // otherwise we'll miscalculate how many blocks its taking to get included.
    if (!validFeeEstimate) {
        untrackedTxs++;
        return;
    }
    trackedTxs++;

    // Feerates are stored and reported as BTC-per-kb:
    CFeeRate feeRate(entry.GetFee(), entry.GetTxSize());

    mapMemPoolTxs[hash].blockHeight = txHeight;
    unsigned int bucketIndex = feeStats->NewTx(txHeight, (double)feeRate.GetFeePerK());
    mapMemPoolTxs[hash].bucketIndex = bucketIndex;
    unsigned int bucketIndex2 = shortStats->NewTx(txHeight, (double)feeRate.GetFeePerK());
    assert(bucketIndex == bucketIndex2);
    unsigned int bucketIndex3 = longStats->NewTx(txHeight, (double)feeRate.GetFeePerK());
    assert(bucketIndex == bucketIndex3);
}

bool CBlockPolicyEstimator::processBlockTx(unsigned int nBlockHeight, const CTxMemPoolEntry* entry)
{
    if (!removeTx(entry->GetTx().GetHash(), true)) {
        // This transaction wasn't being tracked for fee estimation
        return false;
    }

    // How many blocks did it take for miners to include this transaction?
    // blocksToConfirm is 1-based, so a transaction included in the earliest
    // possible block has confirmation count of 1
    int blocksToConfirm = nBlockHeight - entry->GetHeight();
    if (blocksToConfirm <= 0) {
        // This can't happen because we don't process transactions from a block with a height
        // lower than our greatest seen height
        LogPrint(BCLog::ESTIMATEFEE, "Blockpolicy error Transaction had negative blocksToConfirm\n");
        return false;
    }

    // Feerates are stored and reported as BTC-per-kb:
    CFeeRate feeRate(entry->GetFee(), entry->GetTxSize());

    feeStats->Record(blocksToConfirm, (double)feeRate.GetFeePerK());
    shortStats->Record(blocksToConfirm, (double)feeRate.GetFeePerK());
    longStats->Record(blocksToConfirm, (double)feeRate.GetFeePerK());
    return true;
}

void CBlockPolicyEstimator::processBlock(unsigned int nBlockHeight,
                                         std::vector<const CTxMemPoolEntry*>& entries)
{
    LOCK(cs_feeEstimator);
    if (nBlockHeight <= nBestSeenHeight) {
        // Ignore side chains and re-orgs; assuming they are random
        // they don't affect the estimate.
        // And if an attacker can re-org the chain at will, then
        // you've got much bigger problems than "attacker can influence
        // transaction fees."
        return;
    }

    // Must update nBestSeenHeight in sync with ClearCurrent so that
    // calls to removeTx (via processBlockTx) correctly calculate age
    // of unconfirmed txs to remove from tracking.
    nBestSeenHeight = nBlockHeight;

    // Update unconfirmed circular buffer
    feeStats->ClearCurrent(nBlockHeight);
    shortStats->ClearCurrent(nBlockHeight);
    longStats->ClearCurrent(nBlockHeight);

    // Decay all exponential averages
    feeStats->UpdateMovingAverages();
    shortStats->UpdateMovingAverages();
    longStats->UpdateMovingAverages();

    unsigned int countedTxs = 0;
    // Update averages with data points from current block
    for (const auto& entry : entries) {
        if (processBlockTx(nBlockHeight, entry))
            countedTxs++;
    }

    if (firstRecordedHeight == 0 && countedTxs > 0) {
        firstRecordedHeight = nBestSeenHeight;
        LogPrint(BCLog::ESTIMATEFEE, "Blockpolicy first recorded height %u\n", firstRecordedHeight);
    }


    LogPrint(BCLog::ESTIMATEFEE, "Blockpolicy estimates updated by %u of %u block txs, since last block %u of %u tracked, mempool map size %u, max target %u from %s\n",
             countedTxs, entries.size(), trackedTxs, trackedTxs + untrackedTxs, mapMemPoolTxs.size(),
             MaxUsableEstimate(), HistoricalBlockSpan() > BlockSpan() ? "historical" : "current");

    trackedTxs = 0;
    untrackedTxs = 0;
}

CFeeRate CBlockPolicyEstimator::estimateFee(int confTarget) const
{
    // It's not possible to get reasonable estimates for confTarget of 1
    if (confTarget <= 1)
        return CFeeRate(0);

    return estimateRawFee(confTarget, DOUBLE_SUCCESS_PCT, FeeEstimateHorizon::MED_HALFLIFE);
}

CFeeRate CBlockPolicyEstimator::estimateRawFee(int confTarget, double successThreshold, FeeEstimateHorizon horizon, EstimationResult* result) const
{
    TxConfirmStats* stats;
    double sufficientTxs = SUFFICIENT_FEETXS;
    switch (horizon) {
    case FeeEstimateHorizon::SHORT_HALFLIFE: {
        stats = shortStats;
        sufficientTxs = SUFFICIENT_TXS_SHORT;
        break;
    }
    case FeeEstimateHorizon::MED_HALFLIFE: {
        stats = feeStats;
        break;
    }
    case FeeEstimateHorizon::LONG_HALFLIFE: {
        stats = longStats;
        break;
    }
    default: {
        throw std::out_of_range("CBlockPolicyEstimator::estimateRawFee unknown FeeEstimateHorizon");
    }
    }

    LOCK(cs_feeEstimator);
    // Return failure if trying to analyze a target we're not tracking
    if (confTarget <= 0 || (unsigned int)confTarget > stats->GetMaxConfirms())
        return CFeeRate(0);
    if (successThreshold > 1)
        return CFeeRate(0);

    double median = stats->EstimateMedianVal(confTarget, sufficientTxs, successThreshold, true, nBestSeenHeight, result);

    if (median < 0)
        return CFeeRate(0);

    return CFeeRate(median);
}

unsigned int CBlockPolicyEstimator::HighestTargetTracked(FeeEstimateHorizon horizon) const
{
    switch (horizon) {
    case FeeEstimateHorizon::SHORT_HALFLIFE: {
        return shortStats->GetMaxConfirms();
    }
    case FeeEstimateHorizon::MED_HALFLIFE: {
        return feeStats->GetMaxConfirms();
    }
    case FeeEstimateHorizon::LONG_HALFLIFE: {
        return longStats->GetMaxConfirms();
    }
    default: {
        throw std::out_of_range("CBlockPolicyEstimator::HighestTargetTracked unknown FeeEstimateHorizon");
    }
    }
}

unsigned int CBlockPolicyEstimator::BlockSpan() const
{
    if (firstRecordedHeight == 0) return 0;
    assert(nBestSeenHeight >= firstRecordedHeight);

    return nBestSeenHeight - firstRecordedHeight;
}

unsigned int CBlockPolicyEstimator::HistoricalBlockSpan() const
{
    if (historicalFirst == 0) return 0;
    assert(historicalBest >= historicalFirst);

    if (nBestSeenHeight - historicalBest > OLDEST_ESTIMATE_HISTORY) return 0;

    return historicalBest - historicalFirst;
}

unsigned int CBlockPolicyEstimator::MaxUsableEstimate() const
{
    // Block spans are divided by 2 to make sure there are enough potential failing data points for the estimate
    return std::min(longStats->GetMaxConfirms(), std::max(BlockSpan(), HistoricalBlockSpan()) / 2);
}

/** Return a fee estimate at the required successThreshold from the shortest
 * time horizon which tracks confirmations up to the desired target.  If
 * checkShorterHorizon is requested, also allow short time horizon estimates
 * for a lower target to reduce the given answer */
double CBlockPolicyEstimator::estimateCombinedFee(unsigned int confTarget, double successThreshold, bool checkShorterHorizon, EstimationResult *result) const
{
    double estimate = -1;
    if (confTarget >= 1 && confTarget <= longStats->GetMaxConfirms()) {
        // Find estimate from shortest time horizon possible
        if (confTarget <= shortStats->GetMaxConfirms()) { // short horizon
            estimate = shortStats->EstimateMedianVal(confTarget, SUFFICIENT_TXS_SHORT, successThreshold, true, nBestSeenHeight, result);
        }
        else if (confTarget <= feeStats->GetMaxConfirms()) { // medium horizon
            estimate = feeStats->EstimateMedianVal(confTarget, SUFFICIENT_FEETXS, successThreshold, true, nBestSeenHeight, result);
        }
        else { // long horizon
            estimate = longStats->EstimateMedianVal(confTarget, SUFFICIENT_FEETXS, successThreshold, true, nBestSeenHeight, result);
        }
        if (checkShorterHorizon) {
            EstimationResult tempResult;
            // If a lower confTarget from a more recent horizon returns a lower answer use it.
            if (confTarget > feeStats->GetMaxConfirms()) {
                double medMax = feeStats->EstimateMedianVal(feeStats->GetMaxConfirms(), SUFFICIENT_FEETXS, successThreshold, true, nBestSeenHeight, &tempResult);
                if (medMax > 0 && (estimate == -1 || medMax < estimate)) {
                    estimate = medMax;
                    if (result) *result = tempResult;
                }
            }
            if (confTarget > shortStats->GetMaxConfirms()) {
                double shortMax = shortStats->EstimateMedianVal(shortStats->GetMaxConfirms(), SUFFICIENT_TXS_SHORT, successThreshold, true, nBestSeenHeight, &tempResult);
                if (shortMax > 0 && (estimate == -1 || shortMax < estimate)) {
                    estimate = shortMax;
                    if (result) *result = tempResult;
                }
            }
        }
    }
    return estimate;
}

/** Ensure that for a conservative estimate, the DOUBLE_SUCCESS_PCT is also met
 * at 2 * target for any longer time horizons.
 */
double CBlockPolicyEstimator::estimateConservativeFee(unsigned int doubleTarget, EstimationResult *result) const
{
    double estimate = -1;
    EstimationResult tempResult;
    if (doubleTarget <= shortStats->GetMaxConfirms()) {
        estimate = feeStats->EstimateMedianVal(doubleTarget, SUFFICIENT_FEETXS, DOUBLE_SUCCESS_PCT, true, nBestSeenHeight, result);
    }
    if (doubleTarget <= feeStats->GetMaxConfirms()) {
        double longEstimate = longStats->EstimateMedianVal(doubleTarget, SUFFICIENT_FEETXS, DOUBLE_SUCCESS_PCT, true, nBestSeenHeight, &tempResult);
        if (longEstimate > estimate) {
            estimate = longEstimate;
            if (result) *result = tempResult;
        }
    }
    return estimate;
}

/** estimateSmartFee returns the max of the feerates calculated with a 60%
 * threshold required at target / 2, an 85% threshold required at target and a
 * 95% threshold required at 2 * target.  Each calculation is performed at the
 * shortest time horizon which tracks the required target.  Conservative
 * estimates, however, required the 95% threshold at 2 * target be met for any
 * longer time horizons also.
 */
CFeeRate CBlockPolicyEstimator::estimateSmartFee(int confTarget, FeeCalculation *feeCalc, bool conservative) const
{
    LOCK(cs_feeEstimator);

    if (feeCalc) {
        feeCalc->desiredTarget = confTarget;
        feeCalc->returnedTarget = confTarget;
    }

    double median = -1;
    EstimationResult tempResult;

    // Return failure if trying to analyze a target we're not tracking
    if (confTarget <= 0 || (unsigned int)confTarget > longStats->GetMaxConfirms()) {
        return CFeeRate(0);  // error condition
    }

    // It's not possible to get reasonable estimates for confTarget of 1
    if (confTarget == 1) confTarget = 2;

    unsigned int maxUsableEstimate = MaxUsableEstimate();
    if ((unsigned int)confTarget > maxUsableEstimate) {
        confTarget = maxUsableEstimate;
    }
    if (feeCalc) feeCalc->returnedTarget = confTarget;

    if (confTarget <= 1) return CFeeRate(0); // error condition

    assert(confTarget > 0); //estimateCombinedFee and estimateConservativeFee take unsigned ints
    /** true is passed to estimateCombined fee for target/2 and target so
     * that we check the max confirms for shorter time horizons as well.
     * This is necessary to preserve monotonically increasing estimates.
     * For non-conservative estimates we do the same thing for 2*target, but
     * for conservative estimates we want to skip these shorter horizons
     * checks for 2*target because we are taking the max over all time
     * horizons so we already have monotonically increasing estimates and
     * the purpose of conservative estimates is not to let short term
     * fluctuations lower our estimates by too much.
     */
    double halfEst = estimateCombinedFee(confTarget/2, HALF_SUCCESS_PCT, true, &tempResult);
    if (feeCalc) {
        feeCalc->est = tempResult;
        feeCalc->reason = FeeReason::HALF_ESTIMATE;
    }
    median = halfEst;
    double actualEst = estimateCombinedFee(confTarget, SUCCESS_PCT, true, &tempResult);
    if (actualEst > median) {
        median = actualEst;
        if (feeCalc) {
            feeCalc->est = tempResult;
            feeCalc->reason = FeeReason::FULL_ESTIMATE;
        }
    }
    double doubleEst = estimateCombinedFee(2 * confTarget, DOUBLE_SUCCESS_PCT, !conservative, &tempResult);
    if (doubleEst > median) {
        median = doubleEst;
        if (feeCalc) {
            feeCalc->est = tempResult;
            feeCalc->reason = FeeReason::DOUBLE_ESTIMATE;
        }
    }

    if (conservative || median == -1) {
        double consEst =  estimateConservativeFee(2 * confTarget, &tempResult);
        if (consEst > median) {
            median = consEst;
            if (feeCalc) {
                feeCalc->est = tempResult;
                feeCalc->reason = FeeReason::CONSERVATIVE;
            }
        }
    }

    if (median < 0) return CFeeRate(0); // error condition

    return CFeeRate(median);
}


bool CBlockPolicyEstimator::Write(CAutoFile& fileout) const
{
    try {
        LOCK(cs_feeEstimator);
        fileout << 140100; // version required to read: 0.14.1 or later
        fileout << CLIENT_VERSION; // version that wrote the file
        fileout << nBestSeenHeight;
        if (BlockSpan() > HistoricalBlockSpan()/2) {
            fileout << firstRecordedHeight << nBestSeenHeight;
        }
        else {
            fileout << historicalFirst << historicalBest;
        }
        fileout << buckets;
        feeStats->Write(fileout);
        shortStats->Write(fileout);
        longStats->Write(fileout);
    }
    catch (const std::exception&) {
        LogPrintf("CBlockPolicyEstimator::Write(): unable to write policy estimator data (non-fatal)\n");
        return false;
    }
    return true;
}

bool CBlockPolicyEstimator::Read(CAutoFile& filein)
{
    try {
        LOCK(cs_feeEstimator);
        int nVersionRequired, nVersionThatWrote;
        unsigned int nFileBestSeenHeight, nFileHistoricalFirst, nFileHistoricalBest;
        filein >> nVersionRequired >> nVersionThatWrote;
        if (nVersionRequired > CLIENT_VERSION)
            return error("CBlockPolicyEstimator::Read(): up-version (%d) fee estimate file", nVersionRequired);

        // Read fee estimates file into temporary variables so existing data
        // structures aren't corrupted if there is an exception.
        filein >> nFileBestSeenHeight;

        if (nVersionThatWrote < 140100) {
            // Read the old fee estimates file for temporary use, but then discard.  Will start collecting data from scratch.
            // decay is stored before buckets in old versions, so pre-read decay and pass into TxConfirmStats constructor
            double tempDecay;
            filein >> tempDecay;
            if (tempDecay <= 0 || tempDecay >= 1)
                throw std::runtime_error("Corrupt estimates file. Decay must be between 0 and 1 (non-inclusive)");

            std::vector<double> tempBuckets;
            filein >> tempBuckets;
            size_t tempNum = tempBuckets.size();
            if (tempNum <= 1 || tempNum > 1000)
                throw std::runtime_error("Corrupt estimates file. Must have between 2 and 1000 feerate buckets");

            std::map<double, unsigned int> tempMap;

            std::unique_ptr<TxConfirmStats> tempFeeStats(new TxConfirmStats(tempBuckets, tempMap, MED_BLOCK_PERIODS, tempDecay, 1));
            tempFeeStats->Read(filein, nVersionThatWrote, tempNum);
            // if nVersionThatWrote < 120300 then another TxConfirmStats (for priority) follows but can be ignored.

            tempMap.clear();
            for (unsigned int i = 0; i < tempBuckets.size(); i++) {
                tempMap[tempBuckets[i]] = i;
            }
        }
        else { // nVersionThatWrote >= 140100
            filein >> nFileHistoricalFirst >> nFileHistoricalBest;
            if (nFileHistoricalFirst > nFileHistoricalBest || nFileHistoricalBest > nFileBestSeenHeight) {
                throw std::runtime_error("Corrupt estimates file. Historical block range for estimates is invalid");
            }
            std::vector<double> fileBuckets;
            filein >> fileBuckets;
            size_t numBuckets = fileBuckets.size();
            if (numBuckets <= 1 || numBuckets > 1000)
                throw std::runtime_error("Corrupt estimates file. Must have between 2 and 1000 feerate buckets");

            std::unique_ptr<TxConfirmStats> fileFeeStats(new TxConfirmStats(buckets, bucketMap, MED_BLOCK_PERIODS, MED_DECAY, MED_SCALE));
            std::unique_ptr<TxConfirmStats> fileShortStats(new TxConfirmStats(buckets, bucketMap, SHORT_BLOCK_PERIODS, SHORT_DECAY, SHORT_SCALE));
            std::unique_ptr<TxConfirmStats> fileLongStats(new TxConfirmStats(buckets, bucketMap, LONG_BLOCK_PERIODS, LONG_DECAY, LONG_SCALE));
            fileFeeStats->Read(filein, nVersionThatWrote, numBuckets);
            fileShortStats->Read(filein, nVersionThatWrote, numBuckets);
            fileLongStats->Read(filein, nVersionThatWrote, numBuckets);

            // Fee estimates file parsed correctly
            // Copy buckets from file and refresh our bucketmap
            buckets = fileBuckets;
            bucketMap.clear();
            for (unsigned int i = 0; i < buckets.size(); i++) {
                bucketMap[buckets[i]] = i;
            }

            // Destroy old TxConfirmStats and point to new ones that already reference buckets and bucketMap
            delete feeStats;
            delete shortStats;
            delete longStats;
            feeStats = fileFeeStats.release();
            shortStats = fileShortStats.release();
            longStats = fileLongStats.release();

            nBestSeenHeight = nFileBestSeenHeight;
            historicalFirst = nFileHistoricalFirst;
            historicalBest = nFileHistoricalBest;
        }
    }
    catch (const std::exception& e) {
        LogPrintf("CBlockPolicyEstimator::Read(): unable to read policy estimator data (non-fatal): %s\n",e.what());
        return false;
    }
    return true;
}

void CBlockPolicyEstimator::FlushUnconfirmed(CTxMemPool& pool) {
    int64_t startclear = GetTimeMicros();
    std::vector<uint256> txids;
    pool.queryHashes(txids);
    LOCK(cs_feeEstimator);
    for (auto& txid : txids) {
        removeTx(txid, false);
    }
    int64_t endclear = GetTimeMicros();
    LogPrint(BCLog::ESTIMATEFEE, "Recorded %u unconfirmed txs from mempool in %ld micros\n",txids.size(), endclear - startclear);
}
