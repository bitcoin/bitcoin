// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "policy/fees.h"
#include "policy/policy.h"

#include "amount.h"
#include "clientversion.h"
#include "primitives/transaction.h"
#include "random.h"
#include "streams.h"
#include "txmempool.h"
#include "util.h"

static constexpr double INF_FEERATE = 1e99;

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

    std::vector<std::vector<double>> failAvg; // future use

    // Sum the total feerate of all tx's in each bucket
    // Track the historical moving average of this total over blocks
    std::vector<double> avg;

    // Combine the conf counts with tx counts to calculate the confirmation % for each Y,X
    // Combine the total value with the tx counts to calculate the avg feerate per bucket

    double decay;

    unsigned int scale;

    // Mempool counts of outstanding transactions
    // For each bucket X, track the number of transactions in the mempool
    // that are unconfirmed for each possible confirmation value Y
    std::vector<std::vector<int> > unconfTxs;  //unconfTxs[Y][X]
    // transactions still unconfirmed after MAX_CONFIRMS for each bucket
    std::vector<int> oldUnconfTxs;

    void resizeInMemoryCounters(size_t newbuckets);

public:
    /**
     * Create new TxConfirmStats. This is called by BlockPolicyEstimator's
     * constructor with default values.
     * @param defaultBuckets contains the upper limits for the bucket boundaries
     * @param maxConfirms max number of confirms to track
     * @param decay how much to decay the historical moving average per block
     */
    TxConfirmStats(const std::vector<double>& defaultBuckets, const std::map<double, unsigned int>& defaultBucketMap,
                   unsigned int maxConfirms, double decay);

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
                  unsigned int bucketIndex);

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
                             double minSuccess, bool requireGreater, unsigned int nBlockHeight) const;

    /** Return the max number of confirms we're tracking */
    unsigned int GetMaxConfirms() const { return confAvg.size(); }

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
                               unsigned int maxConfirms, double _decay)
    : buckets(defaultBuckets), bucketMap(defaultBucketMap)
{
    decay = _decay;
    scale = 1;
    confAvg.resize(maxConfirms);
    for (unsigned int i = 0; i < maxConfirms; i++) {
        confAvg[i].resize(buckets.size());
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
    unsigned int bucketindex = bucketMap.lower_bound(val)->second;
    for (size_t i = blocksToConfirm; i <= confAvg.size(); i++) {
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
        avg[j] = avg[j] * decay;
        txCtAvg[j] = txCtAvg[j] * decay;
    }
}

// returns -1 on error conditions
double TxConfirmStats::EstimateMedianVal(int confTarget, double sufficientTxVal,
                                         double successBreakPoint, bool requireGreater,
                                         unsigned int nBlockHeight) const
{
    // Counters for a bucket (or range of buckets)
    double nConf = 0; // Number of tx's confirmed within the confTarget
    double totalNum = 0; // Total number of tx's that were ever confirmed
    int extraNum = 0;  // Number of tx's still in mempool for confTarget or longer

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

    // Start counting from highest(default) or lowest feerate transactions
    for (int bucket = startbucket; bucket >= 0 && bucket <= maxbucketindex; bucket += step) {
        curFarBucket = bucket;
        nConf += confAvg[confTarget - 1][bucket];
        totalNum += txCtAvg[bucket];
        for (unsigned int confct = confTarget; confct < GetMaxConfirms(); confct++)
            extraNum += unconfTxs[(nBlockHeight - confct)%bins][bucket];
        extraNum += oldUnconfTxs[bucket];
        // If we have enough transaction data points in this range of buckets,
        // we can test for success
        // (Only count the confirmed data points, so that each confirmation count
        // will be looking at the same amount of data and same bucket breaks)
        if (totalNum >= sufficientTxVal / (1 - decay)) {
            double curPct = nConf / (totalNum + extraNum);

            // Check to see if we are no longer getting confirmed at the success rate
            if ((requireGreater && curPct < successBreakPoint) || (!requireGreater && curPct > successBreakPoint)) {
                continue;
            }
            // Otherwise update the cumulative stats, and the bucket variables
            // and reset the counters
            else {
                foundAnswer = true;
                nConf = 0;
                totalNum = 0;
                extraNum = 0;
                bestNearBucket = curNearBucket;
                bestFarBucket = curFarBucket;
                curNearBucket = bucket + step;
            }
        }
    }

    double median = -1;
    double txSum = 0;

    // Calculate the "average" feerate of the best bucket range that met success conditions
    // Find the bucket with the median transaction and then report the average feerate from that bucket
    // This is a compromise between finding the median which we can't since we don't save all tx's
    // and reporting the average which is less accurate
    unsigned int minBucket = bestNearBucket < bestFarBucket ? bestNearBucket : bestFarBucket;
    unsigned int maxBucket = bestNearBucket > bestFarBucket ? bestNearBucket : bestFarBucket;
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
    }

    LogPrint(BCLog::ESTIMATEFEE, "%3d: For conf success %s %4.2f need feerate %s: %12.5g from buckets %8g - %8g  Cur Bucket stats %6.2f%%  %8.1f/(%.1f+%d mempool)\n",
             confTarget, requireGreater ? ">" : "<", successBreakPoint,
             requireGreater ? ">" : "<", median, buckets[minBucket], buckets[maxBucket],
             100 * nConf / (totalNum + extraNum), nConf, totalNum, extraNum);

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
    size_t maxConfirms;

    // The current version will store the decay with each individual TxConfirmStats and also keep a scale factor
    if (nFileVersion >= 149900) {
        filein >> decay;
        if (decay <= 0 || decay >= 1) {
            throw std::runtime_error("Corrupt estimates file. Decay must be between 0 and 1 (non-inclusive)");
        }
        filein >> scale; //Unused for now
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
    maxConfirms = confAvg.size();
    if (maxConfirms <= 0 || maxConfirms > 6 * 24 * 7) { // one week
        throw std::runtime_error("Corrupt estimates file.  Must maintain estimates for between 1 and 1008 (one week) confirms");
    }
    for (unsigned int i = 0; i < maxConfirms; i++) {
        if (confAvg[i].size() != numBuckets) {
            throw std::runtime_error("Corrupt estimates file. Mismatch in feerate conf average bucket count");
        }
    }

    if (nFileVersion >= 149900) {
        filein >> failAvg;
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

void TxConfirmStats::removeTx(unsigned int entryHeight, unsigned int nBestSeenHeight, unsigned int bucketindex)
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
}

// This function is called from CTxMemPool::removeUnchecked to ensure
// txs removed from the mempool for any reason are no longer
// tracked. Txs that were part of a block have already been removed in
// processBlockTx to ensure they are never double tracked, but it is
// of no harm to try to remove them again.
bool CBlockPolicyEstimator::removeTx(uint256 hash)
{
    LOCK(cs_feeEstimator);
    std::map<uint256, TxStatsInfo>::iterator pos = mapMemPoolTxs.find(hash);
    if (pos != mapMemPoolTxs.end()) {
        feeStats->removeTx(pos->second.blockHeight, nBestSeenHeight, pos->second.bucketIndex);
        shortStats->removeTx(pos->second.blockHeight, nBestSeenHeight, pos->second.bucketIndex);
        longStats->removeTx(pos->second.blockHeight, nBestSeenHeight, pos->second.bucketIndex);
        mapMemPoolTxs.erase(hash);
        return true;
    } else {
        return false;
    }
}

CBlockPolicyEstimator::CBlockPolicyEstimator()
    : nBestSeenHeight(0), trackedTxs(0), untrackedTxs(0)
{
    static_assert(MIN_BUCKET_FEERATE > 0, "Min feerate must be nonzero");
    minTrackedFee = CFeeRate(MIN_BUCKET_FEERATE);
    size_t bucketIndex = 0;
    for (double bucketBoundary = minTrackedFee.GetFeePerK(); bucketBoundary <= MAX_BUCKET_FEERATE; bucketBoundary *= FEE_SPACING, bucketIndex++) {
        buckets.push_back(bucketBoundary);
        bucketMap[bucketBoundary] = bucketIndex;
    }
    buckets.push_back(INF_FEERATE);
    bucketMap[INF_FEERATE] = bucketIndex;
    assert(bucketMap.size() == buckets.size());

    feeStats = new TxConfirmStats(buckets, bucketMap, MED_BLOCK_CONFIRMS, MED_DECAY);
    shortStats = new TxConfirmStats(buckets, bucketMap, SHORT_BLOCK_CONFIRMS, SHORT_DECAY);
    longStats = new TxConfirmStats(buckets, bucketMap, LONG_BLOCK_CONFIRMS, LONG_DECAY);
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
        LogPrint(BCLog::ESTIMATEFEE, "Blockpolicy error mempool tx %s already being tracked\n",
                 hash.ToString().c_str());
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
    if (!removeTx(entry->GetTx().GetHash())) {
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
    for (unsigned int i = 0; i < entries.size(); i++) {
        if (processBlockTx(nBlockHeight, entries[i]))
            countedTxs++;
    }


    LogPrint(BCLog::ESTIMATEFEE, "Blockpolicy after updating estimates for %u of %u txs in block, since last block %u of %u tracked, new mempool map size %u\n",
             countedTxs, entries.size(), trackedTxs, trackedTxs + untrackedTxs, mapMemPoolTxs.size());

    trackedTxs = 0;
    untrackedTxs = 0;
}

CFeeRate CBlockPolicyEstimator::estimateFee(int confTarget) const
{
    LOCK(cs_feeEstimator);
    // Return failure if trying to analyze a target we're not tracking
    // It's not possible to get reasonable estimates for confTarget of 1
    if (confTarget <= 1 || (unsigned int)confTarget > feeStats->GetMaxConfirms())
        return CFeeRate(0);

    double median = feeStats->EstimateMedianVal(confTarget, SUFFICIENT_FEETXS, DOUBLE_SUCCESS_PCT, true, nBestSeenHeight);

    if (median < 0)
        return CFeeRate(0);

    return CFeeRate(median);
}

CFeeRate CBlockPolicyEstimator::estimateSmartFee(int confTarget, int *answerFoundAtTarget, const CTxMemPool& pool) const
{
    if (answerFoundAtTarget)
        *answerFoundAtTarget = confTarget;

    double median = -1;

    {
        LOCK(cs_feeEstimator);

        // Return failure if trying to analyze a target we're not tracking
        if (confTarget <= 0 || (unsigned int)confTarget > feeStats->GetMaxConfirms())
            return CFeeRate(0);

        // It's not possible to get reasonable estimates for confTarget of 1
        if (confTarget == 1)
            confTarget = 2;

        while (median < 0 && (unsigned int)confTarget <= feeStats->GetMaxConfirms()) {
            median = feeStats->EstimateMedianVal(confTarget++, SUFFICIENT_FEETXS, DOUBLE_SUCCESS_PCT, true, nBestSeenHeight);
        }
    } // Must unlock cs_feeEstimator before taking mempool locks

    if (answerFoundAtTarget)
        *answerFoundAtTarget = confTarget - 1;

    // If mempool is limiting txs , return at least the min feerate from the mempool
    CAmount minPoolFee = pool.GetMinFee(GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000).GetFeePerK();
    if (minPoolFee > 0 && minPoolFee > median)
        return CFeeRate(minPoolFee);

    if (median < 0)
        return CFeeRate(0);

    return CFeeRate(median);
}

bool CBlockPolicyEstimator::Write(CAutoFile& fileout) const
{
    try {
        LOCK(cs_feeEstimator);
        fileout << 149900; // version required to read: 0.14.99 or later
        fileout << CLIENT_VERSION; // version that wrote the file
        fileout << nBestSeenHeight;
        unsigned int future1 = 0, future2 = 0;
        fileout << future1 << future2;
        fileout << buckets;
        feeStats->Write(fileout);
        shortStats->Write(fileout);
        longStats->Write(fileout);
    }
    catch (const std::exception&) {
        LogPrintf("CBlockPolicyEstimator::Write(): unable to read policy estimator data (non-fatal)\n");
        return false;
    }
    return true;
}

bool CBlockPolicyEstimator::Read(CAutoFile& filein)
{
    try {
        LOCK(cs_feeEstimator);
        int nVersionRequired, nVersionThatWrote;
        unsigned int nFileBestSeenHeight;
        filein >> nVersionRequired >> nVersionThatWrote;
        if (nVersionRequired > CLIENT_VERSION)
            return error("CBlockPolicyEstimator::Read(): up-version (%d) fee estimate file", nVersionRequired);

        // Read fee estimates file into temporary variables so existing data
        // structures aren't corrupted if there is an exception.
        filein >> nFileBestSeenHeight;

        if (nVersionThatWrote < 149900) {
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

            std::unique_ptr<TxConfirmStats> tempFeeStats(new TxConfirmStats(tempBuckets, tempMap, MED_BLOCK_CONFIRMS, tempDecay));
            tempFeeStats->Read(filein, nVersionThatWrote, tempNum);
            // if nVersionThatWrote < 139900 then another TxConfirmStats (for priority) follows but can be ignored.

            tempMap.clear();
            for (unsigned int i = 0; i < tempBuckets.size(); i++) {
                tempMap[tempBuckets[i]] = i;
            }
        }
        else { // nVersionThatWrote >= 149900
            unsigned int future1, future2;
            filein >> future1 >> future2;

            std::vector<double> fileBuckets;
            filein >> fileBuckets;
            size_t numBuckets = fileBuckets.size();
            if (numBuckets <= 1 || numBuckets > 1000)
                throw std::runtime_error("Corrupt estimates file. Must have between 2 and 1000 feerate buckets");

            std::unique_ptr<TxConfirmStats> fileFeeStats(new TxConfirmStats(buckets, bucketMap, MED_BLOCK_CONFIRMS, MED_DECAY));
            std::unique_ptr<TxConfirmStats> fileShortStats(new TxConfirmStats(buckets, bucketMap, SHORT_BLOCK_CONFIRMS, SHORT_DECAY));
            std::unique_ptr<TxConfirmStats> fileLongStats(new TxConfirmStats(buckets, bucketMap, LONG_BLOCK_CONFIRMS, LONG_DECAY));
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
        }
    }
    catch (const std::exception& e) {
        LogPrintf("CBlockPolicyEstimator::Read(): unable to read policy estimator data (non-fatal): %s\n",e.what());
        return false;
    }
    return true;
}

FeeFilterRounder::FeeFilterRounder(const CFeeRate& minIncrementalFee)
{
    CAmount minFeeLimit = std::max(CAmount(1), minIncrementalFee.GetFeePerK() / 2);
    feeset.insert(0);
    for (double bucketBoundary = minFeeLimit; bucketBoundary <= MAX_FILTER_FEERATE; bucketBoundary *= FEE_FILTER_SPACING) {
        feeset.insert(bucketBoundary);
    }
}

CAmount FeeFilterRounder::round(CAmount currentMinFee)
{
    std::set<double>::iterator it = feeset.lower_bound(currentMinFee);
    if ((it != feeset.begin() && insecure_rand.rand32() % 3 != 0) || it == feeset.end()) {
        it--;
    }
    return *it;
}
