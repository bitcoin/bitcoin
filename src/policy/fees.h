// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_POLICYESTIMATOR_H
#define BITCOIN_POLICYESTIMATOR_H

#include "amount.h"
#include "uint256.h"

#include <map>
#include <string>
#include <vector>

class CAutoFile;
class CFeeRate;
class CTxMemPoolEntry;
class CTxMemPool;

/** \class CBlockPolicyEstimator
 * The BlockPolicyEstimator is used for estimating the fee or priority needed
 * for a transaction to be included in a block within a certain number of
 * blocks.
 *
 * At a high level the algorithm works by grouping transactions into buckets
 * based on having similar priorities or fees and then tracking how long it
 * takes transactions in the various buckets to be mined.  It operates under
 * the assumption that in general transactions of higher fee/priority will be
 * included in blocks before transactions of lower fee/priority.   So for
 * example if you wanted to know what fee you should put on a transaction to
 * be included in a block within the next 5 blocks, you would start by looking
 * at the bucket with the highest fee transactions and verifying that a
 * sufficiently high percentage of them were confirmed within 5 blocks and
 * then you would look at the next highest fee bucket, and so on, stopping at
 * the last bucket to pass the test.   The average fee of transactions in this
 * bucket will give you an indication of the lowest fee you can put on a
 * transaction and still have a sufficiently high chance of being confirmed
 * within your desired 5 blocks.
 *
 * When a transaction enters the mempool or is included within a block we
 * decide whether it can be used as a data point for fee estimation, priority
 * estimation or neither.  If the value of exactly one of those properties was
 * below the required minimum it can be used to estimate the other.  In
 * addition, if a priori our estimation code would indicate that the
 * transaction would be much more quickly included in a block because of one
 * of the properties compared to the other, we can also decide to use it as
 * an estimate for that property.
 *
 * Here is a brief description of the implementation for fee estimation.
 * When a transaction that counts for fee estimation enters the mempool, we
 * track the height of the block chain at entry.  Whenever a block comes in,
 * we count the number of transactions in each bucket and the total amount of fee
 * paid in each bucket. Then we calculate how many blocks Y it took each
 * transaction to be mined and we track an array of counters in each bucket
 * for how long it to took transactions to get confirmed from 1 to a max of 25
 * and we increment all the counters from Y up to 25. This is because for any
 * number Z>=Y the transaction was successfully mined within Z blocks.  We
 * want to save a history of this information, so at any time we have a
 * counter of the total number of transactions that happened in a given fee
 * bucket and the total number that were confirmed in each number 1-25 blocks
 * or less for any bucket.   We save this history by keeping an exponentially
 * decaying moving average of each one of these stats.  Furthermore we also
 * keep track of the number unmined (in mempool) transactions in each bucket
 * and for how many blocks they have been outstanding and use that to increase
 * the number of transactions we've seen in that fee bucket when calculating
 * an estimate for any number of confirmations below the number of blocks
 * they've been outstanding.
 */

/**
 * We will instantiate two instances of this class, one to track transactions
 * that were included in a block due to fee, and one for tx's included due to
 * priority.  We will lump transactions into a bucket according to their approximate
 * fee or priority and then track how long it took for those txs to be included in a block
 *
 * The tracking of unconfirmed (mempool) transactions is completely independent of the
 * historical tracking of transactions that have been confirmed in a block.
 */
class TxConfirmStats
{
private:
    //Define the buckets we will group transactions into (both fee buckets and priority buckets)
    std::vector<double> buckets;              // The upper-bound of the range for the bucket (inclusive)
    std::map<double, unsigned int> bucketMap; // Map of bucket upper-bound to index into all vectors by bucket

    // For each bucket X:
    // Count the total # of txs in each bucket
    // Track the historical moving average of this total over blocks
    std::vector<double> txCtAvg;
    // and calculate the total for the current block to update the moving average
    std::vector<int> curBlockTxCt;

    // Count the total # of txs confirmed within Y blocks in each bucket
    // Track the historical moving average of theses totals over blocks
    std::vector<std::vector<double> > confAvg; // confAvg[Y][X]
    // and calculate the totals for the current block to update the moving averages
    std::vector<std::vector<int> > curBlockConf; // curBlockConf[Y][X]

    // Sum the total priority/fee of all tx's in each bucket
    // Track the historical moving average of this total over blocks
    std::vector<double> avg;
    // and calculate the total for the current block to update the moving average
    std::vector<double> curBlockVal;

    // Combine the conf counts with tx counts to calculate the confirmation % for each Y,X
    // Combine the total value with the tx counts to calculate the avg fee/priority per bucket

    std::string dataTypeString;
    double decay;

    // Mempool counts of outstanding transactions
    // For each bucket X, track the number of transactions in the mempool
    // that are unconfirmed for each possible confirmation value Y
    std::vector<std::vector<int> > unconfTxs;  //unconfTxs[Y][X]
    // transactions still unconfirmed after MAX_CONFIRMS for each bucket
    std::vector<int> oldUnconfTxs;

public:
    /**
     * Initialize the data structures.  This is called by BlockPolicyEstimator's
     * constructor with default values.
     * @param defaultBuckets contains the upper limits for the bucket boundaries
     * @param maxConfirms max number of confirms to track
     * @param decay how much to decay the historical moving average per block
     * @param dataTypeString for logging purposes
     */
    void Initialize(std::vector<double>& defaultBuckets, unsigned int maxConfirms, double decay, std::string dataTypeString);

    /** Clear the state of the curBlock variables to start counting for the new block */
    void ClearCurrent(unsigned int nBlockHeight);

    /**
     * Record a new transaction data point in the current block stats
     * @param blocksToConfirm the number of blocks it took this transaction to confirm
     * @param val either the fee or the priority when entered of the transaction
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
     * Calculate a fee or priority estimate.  Find the lowest value bucket (or range of buckets
     * to make sure we have enough data points) whose transactions still have sufficient likelihood
     * of being confirmed within the target number of confirmations
     * @param confTarget target number of confirmations
     * @param sufficientTxVal required average number of transactions per block in a bucket range
     * @param minSuccess the success probability we require
     * @param requireGreater return the lowest fee/pri such that all higher values pass minSuccess OR
     *        return the highest fee/pri such that all lower values fail minSuccess
     * @param nBlockHeight the current block height
     */
    double EstimateMedianVal(int confTarget, double sufficientTxVal,
                             double minSuccess, bool requireGreater, unsigned int nBlockHeight);

    /** Return the max number of confirms we're tracking */
    unsigned int GetMaxConfirms() { return confAvg.size(); }

    /** Write state of estimation data to a file*/
    void Write(CAutoFile& fileout);

    /**
     * Read saved state of estimation data from a file and replace all internal data structures and
     * variables with this state.
     */
    void Read(CAutoFile& filein);
};



/** Track confirm delays up to 25 blocks, can't estimate beyond that */
static const unsigned int MAX_BLOCK_CONFIRMS = 25;

/** Decay of .998 is a half-life of 346 blocks or about 14.4 hours */
static const double DEFAULT_DECAY = .998;

/** Require greater than 95% of X fee transactions to be confirmed within Y blocks for X to be big enough */
static const double MIN_SUCCESS_PCT = .95;
static const double UNLIKELY_PCT = .5;

/** Require an avg of 1 tx in the combined fee bucket per block to have stat significance */
static const double SUFFICIENT_FEETXS = 1;

/** Require only an avg of 1 tx every 5 blocks in the combined pri bucket (way less pri txs) */
static const double SUFFICIENT_PRITXS = .2;

// Minimum and Maximum values for tracking fees and priorities
static const double MIN_FEERATE = 10;
static const double MAX_FEERATE = 1e7;
static const double INF_FEERATE = MAX_MONEY;
static const double MIN_PRIORITY = 10;
static const double MAX_PRIORITY = 1e16;
static const double INF_PRIORITY = 1e9 * MAX_MONEY;

// We have to lump transactions into buckets based on fee or priority, but we want to be able
// to give accurate estimates over a large range of potential fees and priorities
// Therefore it makes sense to exponentially space the buckets
/** Spacing of FeeRate buckets */
static const double FEE_SPACING = 1.1;

/** Spacing of Priority buckets */
static const double PRI_SPACING = 2;

/**
 *  We want to be able to estimate fees or priorities that are needed on tx's to be included in
 * a certain number of blocks.  Every time a block is added to the best chain, this class records
 * stats on the transactions included in that block
 */
class CBlockPolicyEstimator
{
public:
    /** Create new BlockPolicyEstimator and initialize stats tracking classes with default values */
    CBlockPolicyEstimator(const CFeeRate& minRelayFee);

    /** Process all the transactions that have been included in a block */
    void processBlock(unsigned int nBlockHeight,
                      std::vector<CTxMemPoolEntry>& entries, bool fCurrentEstimate);

    /** Process a transaction confirmed in a block*/
    void processBlockTx(unsigned int nBlockHeight, const CTxMemPoolEntry& entry);

    /** Process a transaction accepted to the mempool*/
    void processTransaction(const CTxMemPoolEntry& entry, bool fCurrentEstimate);

    /** Remove a transaction from the mempool tracking stats*/
    void removeTx(uint256 hash);

    /** Is this transaction likely included in a block because of its fee?*/
    bool isFeeDataPoint(const CFeeRate &fee, double pri);

    /** Is this transaction likely included in a block because of its priority?*/
    bool isPriDataPoint(const CFeeRate &fee, double pri);

    /** Return a fee estimate */
    CFeeRate estimateFee(int confTarget);

    /** Estimate fee rate needed to get be included in a block within
     *  confTarget blocks. If no answer can be given at confTarget, return an
     *  estimate at the lowest target where one can be given.
     */
    CFeeRate estimateSmartFee(int confTarget, int *answerFoundAtTarget, const CTxMemPool& pool);

    /** Return a priority estimate */
    double estimatePriority(int confTarget);

    /** Estimate priority needed to get be included in a block within
     *  confTarget blocks. If no answer can be given at confTarget, return an
     *  estimate at the lowest target where one can be given.
     */
    double estimateSmartPriority(int confTarget, int *answerFoundAtTarget, const CTxMemPool& pool);

    /** Write estimation data to a file */
    void Write(CAutoFile& fileout);

    /** Read estimation data from a file */
    void Read(CAutoFile& filein);

private:
    CFeeRate minTrackedFee; //! Passed to constructor to avoid dependency on main
    double minTrackedPriority; //! Set to AllowFreeThreshold
    unsigned int nBestSeenHeight;
    struct TxStatsInfo
    {
        TxConfirmStats *stats;
        unsigned int blockHeight;
        unsigned int bucketIndex;
        TxStatsInfo() : stats(NULL), blockHeight(0), bucketIndex(0) {}
    };

    // map of txids to information about that transaction
    std::map<uint256, TxStatsInfo> mapMemPoolTxs;

    /** Classes to track historical data on transaction confirmations */
    TxConfirmStats feeStats, priStats;

    /** Breakpoints to help determine whether a transaction was confirmed by priority or Fee */
    CFeeRate feeLikely, feeUnlikely;
    double priLikely, priUnlikely;
};
#endif /*BITCOIN_POLICYESTIMATOR_H */
