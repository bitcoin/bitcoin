// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "policy/estimator.h"

#include "policy/policy.h"
#include "streams.h"
#include "util.h"
#include "version.h"

#include <boost/foreach.hpp>

using namespace std;

void CBlockAverage::RecordFee(const CFeeRate& feeRate) {
    feeSamples.push_back(feeRate);
}

void CBlockAverage::RecordPriority(double priority) {
    prioritySamples.push_back(priority);
}

size_t CBlockAverage::FeeSamples() const { return feeSamples.size(); }

size_t CBlockAverage::GetFeeSamples(std::vector<CFeeRate>& insertInto) const
{
    BOOST_FOREACH(const CFeeRate& f, feeSamples)
        insertInto.push_back(f);
    return feeSamples.size();
}

size_t CBlockAverage::PrioritySamples() const { return prioritySamples.size(); }

size_t CBlockAverage::GetPrioritySamples(std::vector<double>& insertInto) const
{
    BOOST_FOREACH(double d, prioritySamples)
        insertInto.push_back(d);
    return prioritySamples.size();
}

bool CBlockAverage::AreSane(const CFeeRate fee, const CFeeRate& minRelayFee)
{
    if (fee < CFeeRate(0))
        return false;
    if (fee.GetFeePerK() > minRelayFee.GetFeePerK() * 10000)
        return false;
    return true;
}

bool CBlockAverage::AreSane(const std::vector<CFeeRate>& vecFee, const CFeeRate& minRelayFee)
{
    BOOST_FOREACH(CFeeRate fee, vecFee)
    {
        if (!AreSane(fee, minRelayFee))
            return false;
    }
    return true;
}

bool CBlockAverage::AreSane(const double priority)
{
    return priority >= 0;
}

bool CBlockAverage::AreSane(const std::vector<double> vecPriority)
{
    BOOST_FOREACH(double priority, vecPriority)
    {
        if (!AreSane(priority))
            return false;
    }
    return true;
}

void CBlockAverage::Write(CAutoFile& fileout) const
{
    std::vector<CFeeRate> vecFee = buf2vec(feeSamples);
    fileout << vecFee;
    std::vector<double> vecPriority = buf2vec(prioritySamples);
    fileout << vecPriority;
}

void CBlockAverage::Read(CAutoFile& filein, const CFeeRate& minRelayFee)
{
    std::vector<CFeeRate> vecFee;
    filein >> vecFee;
    if (AreSane(vecFee, minRelayFee))
        feeSamples.insert(feeSamples.end(), vecFee.begin(), vecFee.end());
    else
        throw runtime_error("Corrupt fee value in estimates file.");
    std::vector<double> vecPriority;
    filein >> vecPriority;
    if (AreSane(vecPriority))
        prioritySamples.insert(prioritySamples.end(), vecPriority.begin(), vecPriority.end());
    else
        throw runtime_error("Corrupt priority value in estimates file.");
    if (feeSamples.size() + prioritySamples.size() > 0)
        LogPrint("estimatefee", "Read %d fee samples and %d priority samples\n",
                 feeSamples.size(), prioritySamples.size());
}

CMinerPolicyEstimator::CMinerPolicyEstimator(int nEntries) : nBestSeenHeight(0)
{
    history.resize(nEntries);
}

void CMinerPolicyEstimator::seenTxConfirm(const CFeeRate& feeRate, const CFeeRate& minRelayFee, double dPriority, int nBlocksAgo)
{
    // Last entry records "everything else".
    int nBlocksTruncated = min(nBlocksAgo, (int) history.size() - 1);
    assert(nBlocksTruncated >= 0);

    // We need to guess why the transaction was included in a block-- either
    // because it is high-priority or because it has sufficient fees.
    bool sufficientFee = (feeRate > minRelayFee);
    bool sufficientPriority = AllowFree(dPriority);
    const char* assignedTo = "unassigned";
    if (sufficientFee && !sufficientPriority && CBlockAverage::AreSane(feeRate, minRelayFee))
    {
        history[nBlocksTruncated].RecordFee(feeRate);
        assignedTo = "fee";
    }
    else if (sufficientPriority && !sufficientFee && CBlockAverage::AreSane(dPriority))
    {
        history[nBlocksTruncated].RecordPriority(dPriority);
        assignedTo = "priority";
    }
    else {
        // Neither or both fee and priority sufficient to get confirmed:
        // don't know why they got confirmed.
    }
    LogPrint("estimatefee", "Seen TX confirm: %s: %s fee/%g priority, took %d blocks\n",
             assignedTo, feeRate.ToString(), dPriority, nBlocksAgo);
}

void CMinerPolicyEstimator::seenBlock(const std::vector<CTxMemPoolEntry>& entries, int nBlockHeight, const CFeeRate minRelayFee)
{
    if (nBlockHeight <= nBestSeenHeight) {
        // Ignore side chains and re-orgs; assuming they are random
        // they don't affect the estimate.
        // And if an attacker can re-org the chain at will, then
        // you've got much bigger problems than "attacker can influence
        // transaction fees."
        return;
    }
    nBestSeenHeight = nBlockHeight;

    // Fill up the history buckets based on how long transactions took
    // to confirm.
    std::vector<std::vector<const CTxMemPoolEntry*> > entriesByConfirmations;
    entriesByConfirmations.resize(history.size());
    BOOST_FOREACH(const CTxMemPoolEntry& entry, entries)
    {
        // How many blocks did it take for miners to include this transaction?
        int delta = nBlockHeight - entry.GetHeight();
        if (delta <= 0) {
            // Re-org made us lose height, this should only happen if we happen
            // to re-org on a difficulty transition point: very rare!
            continue;
        }
        if ((delta-1) >= (int)history.size())
            delta = history.size(); // Last bucket is catch-all
        entriesByConfirmations.at(delta-1).push_back(&entry);
    }
    for (size_t i = 0; i < entriesByConfirmations.size(); i++) {

        std::vector<const CTxMemPoolEntry*> &e = entriesByConfirmations.at(i);
        // Insert at most 10 random entries per bucket, otherwise a single block
        // can dominate an estimate:
        if (e.size() > 10) {
            std::random_shuffle(e.begin(), e.end());
            e.resize(10);
        }
        BOOST_FOREACH(const CTxMemPoolEntry* entry, e)
        {
            // Fees are stored and reported as BTC-per-kb:
            CFeeRate feeRate(entry->GetFee(), entry->GetTxSize());
            double dPriority = entry->GetPriority(entry->GetHeight()); // Want priority when it went IN
            seenTxConfirm(feeRate, minRelayFee, dPriority, i);
        }
    }

    // After new samples are added, we have to clear the sorted lists,
    // so they'll be resorted the next time someone asks for an estimate
    sortedFeeSamples.clear();
    sortedPrioritySamples.clear();

    for (size_t i = 0; i < history.size(); i++) {
        if (history[i].FeeSamples() + history[i].PrioritySamples() > 0)
            LogPrint("estimatefee", "estimates: for confirming within %d blocks based on %d/%d samples, fee=%s, prio=%g\n", 
                     i,
                     history[i].FeeSamples(), history[i].PrioritySamples(),
                     estimateFee(i+1).ToString(), estimatePriority(i+1));
    }
}

CFeeRate CMinerPolicyEstimator::estimateFee(int nBlocksToConfirm)
{
    nBlocksToConfirm--;

    if (nBlocksToConfirm < 0 || nBlocksToConfirm >= (int)history.size())
        return CFeeRate(0);

    if (sortedFeeSamples.size() == 0)
    {
        for (size_t i = 0; i < history.size(); i++)
            history.at(i).GetFeeSamples(sortedFeeSamples);
        std::sort(sortedFeeSamples.begin(), sortedFeeSamples.end(),
                  std::greater<CFeeRate>());
    }
    if (sortedFeeSamples.size() < 11)
    {
        // Eleven is Gavin's Favorite Number
        // ... but we also take a maximum of 10 samples per block so eleven means
        // we're getting samples from at least two different blocks
        return CFeeRate(0);
    }

    int nBucketSize = history.at(nBlocksToConfirm).FeeSamples();

    // Estimates should not increase as number of confirmations goes up,
    // but the estimates are noisy because confirmations happen discretely
    // in blocks. To smooth out the estimates, use all samples in the history
    // and use the nth highest where n is (number of samples in previous bucket +
    // half the samples in nBlocksToConfirm bucket):
    size_t nPrevSize = 0;
    for (int i = 0; i < nBlocksToConfirm; i++)
        nPrevSize += history.at(i).FeeSamples();
    size_t index = min(nPrevSize + nBucketSize/2, sortedFeeSamples.size()-1);
    return sortedFeeSamples[index];
}

double CMinerPolicyEstimator::estimatePriority(int nBlocksToConfirm)
{
    nBlocksToConfirm--;

    if (nBlocksToConfirm < 0 || nBlocksToConfirm >= (int)history.size())
        return -1;

    if (sortedPrioritySamples.size() == 0)
    {
        for (size_t i = 0; i < history.size(); i++)
            history.at(i).GetPrioritySamples(sortedPrioritySamples);
        std::sort(sortedPrioritySamples.begin(), sortedPrioritySamples.end(),
                  std::greater<double>());
    }

    if (sortedPrioritySamples.size() < 11)
        return -1.0;

    int nBucketSize = history.at(nBlocksToConfirm).PrioritySamples();

    // Estimates should not increase as number of confirmations needed goes up,
    // but the estimates are noisy because confirmations happen discretely
    // in blocks. To smooth out the estimates, use all samples in the history
    // and use the nth highest where n is (number of samples in previous buckets +
    // half the samples in nBlocksToConfirm bucket).
    size_t nPrevSize = 0;
    for (int i = 0; i < nBlocksToConfirm; i++)
        nPrevSize += history.at(i).PrioritySamples();
    size_t index = min(nPrevSize + nBucketSize/2, sortedPrioritySamples.size()-1);
    return sortedPrioritySamples[index];
}

void CMinerPolicyEstimator::Write(CAutoFile& fileout) const
{
    fileout << nBestSeenHeight;
    fileout << (uint32_t)history.size();
    BOOST_FOREACH(const CBlockAverage& entry, history)
    {
        entry.Write(fileout);
    }
}

void CMinerPolicyEstimator::Read(CAutoFile& filein, const CFeeRate& minRelayFee)
{
    int nFileBestSeenHeight;
    filein >> nFileBestSeenHeight;
    uint32_t numEntries;
    filein >> numEntries;
    if (numEntries <= 0 || numEntries > 10000)
        throw runtime_error("Corrupt estimates file. Must have between 1 and 10k entries.");

    std::vector<CBlockAverage> fileHistory;
        
    for (size_t i = 0; i < numEntries; i++)
    {
        CBlockAverage entry;
        entry.Read(filein, minRelayFee);
        fileHistory.push_back(entry);
    }

    // Now that we've processed the entire fee estimate data file and not
    // thrown any errors, we can copy it to our history
    nBestSeenHeight = nFileBestSeenHeight;
    history = fileHistory;
    assert(history.size() > 0);
}
