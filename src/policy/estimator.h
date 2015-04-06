// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MINER_POLICY_ESTIMATOR_H
#define BITCOIN_MINER_POLICY_ESTIMATOR_H

#include "policy/feerate.h"
#include "sync.h"
#include "txmempoolentry.h"

#include <boost/circular_buffer.hpp>

class CAutoFile;

/**
 * Keep track of fee/priority for transactions confirmed within N blocks
 */
class CBlockAverage
{
private:
    boost::circular_buffer<CFeeRate> feeSamples;
    boost::circular_buffer<double> prioritySamples;

    template<typename T> 
    std::vector<T> buf2vec(boost::circular_buffer<T> buf) const
    {
        std::vector<T> vec(buf.begin(), buf.end());
        return vec;
    }

public:
    CBlockAverage() : feeSamples(100), prioritySamples(100) { }

    void RecordFee(const CFeeRate& feeRate);
    void RecordPriority(double priority);
    size_t FeeSamples() const;
    size_t GetFeeSamples(std::vector<CFeeRate>& insertInto) const;
    size_t PrioritySamples() const;
    size_t GetPrioritySamples(std::vector<double>& insertInto) const;
    /**
     * Used as belt-and-suspenders check when reading to detect
     * file corruption
     */
    static bool AreSane(const CFeeRate fee, const CFeeRate& minRelayFee);
    static bool AreSane(const std::vector<CFeeRate>& vecFee, const CFeeRate& minRelayFee);
    static bool AreSane(const double priority);
    static bool AreSane(const std::vector<double> vecPriority);
    void Write(CAutoFile& fileout) const;
    void Read(CAutoFile& filein, const CFeeRate& minRelayFee);
};

class CMinerPolicyEstimator
{
private:
    mutable CCriticalSection cs;
    /**
     * Records observed averages transactions that confirmed within one block, two blocks,
     * three blocks etc.
     */
    std::vector<CBlockAverage> history;
    std::vector<CFeeRate> sortedFeeSamples;
    std::vector<double> sortedPrioritySamples;

    int nBestSeenHeight;

    /**
     * nBlocksAgo is 0 based, i.e. transactions that confirmed in the highest seen block are
     * nBlocksAgo == 0, transactions in the block before that are nBlocksAgo == 1 etc.
     */
    void seenTxConfirm(const CFeeRate& feeRate, const CFeeRate& minRelayFee, double dPriority, int nBlocksAgo);

public:
    CMinerPolicyEstimator(int nEntries);
    void seenBlock(const std::vector<CTxMemPoolEntry>& entries, int nBlockHeight);
    /**
     * Estimate fee rate needed to get into the next nBlocks.
     * Can return CFeeRate(0) if we don't have any data for that many blocks back. nBlocksToConfirm is 1 based.
     */
    CFeeRate estimateFee(int nBlocksToConfirm);
    /** Estimate priority needed to get into the next nBlocks */
    double estimatePriority(int nBlocksToConfirm);
    bool Write(CAutoFile& fileout) const;
    bool Read(CAutoFile& filein);
};

extern CMinerPolicyEstimator minerPolicyEstimator;

#endif // BITCOIN_MINER_POLICY_ESTIMATOR_H
