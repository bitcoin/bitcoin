// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txmempool.h"

#include "clientversion.h"
#include "main.h"
#include "streams.h"
#include "util.h"
#include "utilmoneystr.h"
#include "version.h"

#include <boost/circular_buffer.hpp>

using namespace std;

CTxMemPoolEntry::CTxMemPoolEntry():
    nFee(0), nTxSize(0), nModSize(0), nTime(0), dPriority(0.0)
{
    nHeight = MEMPOOL_HEIGHT;
}

CTxMemPoolEntry::CTxMemPoolEntry(const CTransaction& _tx, const CAmount& _nFee,
                                 int64_t _nTime, double _dPriority,
                                 unsigned int _nHeight):
    tx(_tx), nFee(_nFee), nTime(_nTime), dPriority(_dPriority), nHeight(_nHeight)
{
    nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);

    nModSize = tx.CalculateModifiedSize(nTxSize);
}

CTxMemPoolEntry::CTxMemPoolEntry(const CTxMemPoolEntry& other)
{
    *this = other;
}

double
CTxMemPoolEntry::GetPriority(unsigned int currentHeight) const
{
    CAmount nValueIn = tx.GetValueOut()+nFee;
    double deltaPriority = ((double)(currentHeight-nHeight)*nValueIn)/nModSize;
    double dResult = dPriority + deltaPriority;
    return dResult;
}

/**
 * Keep track of fee/priority for transactions confirmed within N blocks
 */
class CBlockAverage
{
private:
    boost::circular_buffer<CFeeRate> feeSamples;
    boost::circular_buffer<double> prioritySamples;

    template<typename T> std::vector<T> buf2vec(boost::circular_buffer<T> buf) const
    {
        std::vector<T> vec(buf.begin(), buf.end());
        return vec;
    }

public:
    CBlockAverage() : feeSamples(100), prioritySamples(100) { }

    void RecordFee(const CFeeRate& feeRate) {
        feeSamples.push_back(feeRate);
    }

    void RecordPriority(double priority) {
        prioritySamples.push_back(priority);
    }

    size_t FeeSamples() const { return feeSamples.size(); }
    size_t GetFeeSamples(std::vector<CFeeRate>& insertInto) const
    {
        BOOST_FOREACH(const CFeeRate& f, feeSamples)
            insertInto.push_back(f);
        return feeSamples.size();
    }
    size_t PrioritySamples() const { return prioritySamples.size(); }
    size_t GetPrioritySamples(std::vector<double>& insertInto) const
    {
        BOOST_FOREACH(double d, prioritySamples)
            insertInto.push_back(d);
        return prioritySamples.size();
    }

    /**
     * Used as belt-and-suspenders check when reading to detect
     * file corruption
     */
    static bool AreSane(const CFeeRate fee, const CFeeRate& minRelayFee)
    {
        if (fee < CFeeRate(0))
            return false;
        if (fee.GetFeePerK() > minRelayFee.GetFeePerK() * 10000)
            return false;
        return true;
    }
    static bool AreSane(const std::vector<CFeeRate>& vecFee, const CFeeRate& minRelayFee)
    {
        BOOST_FOREACH(CFeeRate fee, vecFee)
        {
            if (!AreSane(fee, minRelayFee))
                return false;
        }
        return true;
    }
    static bool AreSane(const double priority)
    {
        return priority >= 0;
    }
    static bool AreSane(const std::vector<double> vecPriority)
    {
        BOOST_FOREACH(double priority, vecPriority)
        {
            if (!AreSane(priority))
                return false;
        }
        return true;
    }

    void Write(CAutoFile& fileout) const
    {
        std::vector<CFeeRate> vecFee = buf2vec(feeSamples);
        fileout << vecFee;
        std::vector<double> vecPriority = buf2vec(prioritySamples);
        fileout << vecPriority;
    }

    void Read(CAutoFile& filein, const CFeeRate& minRelayFee) {
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
};

class CMinerPolicyEstimator
{
private:
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
    void seenTxConfirm(const CFeeRate& feeRate, const CFeeRate& minRelayFee, double dPriority, int nBlocksAgo)
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
        else
        {
            // Neither or both fee and priority sufficient to get confirmed:
            // don't know why they got confirmed.
        }
        LogPrint("estimatefee", "Seen TX confirm: %s : %s fee/%g priority, took %d blocks\n",
                 assignedTo, feeRate.ToString(), dPriority, nBlocksAgo);
    }

public:
    CMinerPolicyEstimator(int nEntries) : nBestSeenHeight(0)
    {
        history.resize(nEntries);
    }

    void seenBlock(const std::vector<CTxMemPoolEntry>& entries, int nBlockHeight, const CFeeRate minRelayFee)
    {
        if (nBlockHeight <= nBestSeenHeight)
        {
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
            if (delta <= 0)
            {
                // Re-org made us lose height, this should only happen if we happen
                // to re-org on a difficulty transition point: very rare!
                continue;
            }
            if ((delta-1) >= (int)history.size())
                delta = history.size(); // Last bucket is catch-all
            entriesByConfirmations.at(delta-1).push_back(&entry);
        }
        for (size_t i = 0; i < entriesByConfirmations.size(); i++)
        {
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

    /**
     * Can return CFeeRate(0) if we don't have any data for that many blocks back. nBlocksToConfirm is 1 based.
     */
    CFeeRate estimateFee(int nBlocksToConfirm)
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
    double estimatePriority(int nBlocksToConfirm)
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

    void Write(CAutoFile& fileout) const
    {
        fileout << nBestSeenHeight;
        fileout << history.size();
        BOOST_FOREACH(const CBlockAverage& entry, history)
        {
            entry.Write(fileout);
        }
    }

    void Read(CAutoFile& filein, const CFeeRate& minRelayFee)
    {
        int nFileBestSeenHeight;
        filein >> nFileBestSeenHeight;
        size_t numEntries;
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
};


CTxMemPool::CTxMemPool(const CFeeRate& _minRelayFee) :
    nTransactionsUpdated(0),
    minRelayFee(_minRelayFee)
{
    // Sanity checks off by default for performance, because otherwise
    // accepting transactions becomes O(N^2) where N is the number
    // of transactions in the pool
    fSanityCheck = false;

    // 25 blocks is a compromise between using a lot of disk/memory and
    // trying to give accurate estimates to people who might be willing
    // to wait a day or two to save a fraction of a penny in fees.
    // Confirmation times for very-low-fee transactions that take more
    // than an hour or three to confirm are highly variable.
    minerPolicyEstimator = new CMinerPolicyEstimator(25);
}

CTxMemPool::~CTxMemPool()
{
    delete minerPolicyEstimator;
}

void CTxMemPool::pruneSpent(const uint256 &hashTx, CCoins &coins)
{
    LOCK(cs);

    std::map<COutPoint, CInPoint>::iterator it = mapNextTx.lower_bound(COutPoint(hashTx, 0));

    // iterate over all COutPoints in mapNextTx whose hash equals the provided hashTx
    while (it != mapNextTx.end() && it->first.hash == hashTx) {
        coins.Spend(it->first.n); // and remove those outputs from coins
        it++;
    }
}

unsigned int CTxMemPool::GetTransactionsUpdated() const
{
    LOCK(cs);
    return nTransactionsUpdated;
}

void CTxMemPool::AddTransactionsUpdated(unsigned int n)
{
    LOCK(cs);
    nTransactionsUpdated += n;
}


bool CTxMemPool::addUnchecked(const uint256& hash, const CTxMemPoolEntry &entry)
{
    // Add to memory pool without checking anything.
    // Used by main.cpp AcceptToMemoryPool(), which DOES do
    // all the appropriate checks.
    LOCK(cs);
    {
        mapTx[hash] = entry;
        const CTransaction& tx = mapTx[hash].GetTx();
        for (unsigned int i = 0; i < tx.vin.size(); i++)
            mapNextTx[tx.vin[i].prevout] = CInPoint(&tx, i);
        nTransactionsUpdated++;
        totalTxSize += entry.GetTxSize();
    }
    return true;
}


void CTxMemPool::remove(const CTransaction &origTx, std::list<CTransaction>& removed, bool fRecursive)
{
    // Remove transaction from memory pool
    {
        LOCK(cs);
        std::deque<uint256> txToRemove;
        txToRemove.push_back(origTx.GetHash());
        while (!txToRemove.empty())
        {
            uint256 hash = txToRemove.front();
            txToRemove.pop_front();
            if (!mapTx.count(hash))
                continue;
            const CTransaction& tx = mapTx[hash].GetTx();
            if (fRecursive) {
                for (unsigned int i = 0; i < tx.vout.size(); i++) {
                    std::map<COutPoint, CInPoint>::iterator it = mapNextTx.find(COutPoint(hash, i));
                    if (it == mapNextTx.end())
                        continue;
                    txToRemove.push_back(it->second.ptx->GetHash());
                }
            }
            BOOST_FOREACH(const CTxIn& txin, tx.vin)
                mapNextTx.erase(txin.prevout);

            removed.push_back(tx);
            totalTxSize -= mapTx[hash].GetTxSize();
            mapTx.erase(hash);
            nTransactionsUpdated++;
        }
    }
}

void CTxMemPool::removeCoinbaseSpends(const CCoinsViewCache *pcoins, unsigned int nMemPoolHeight)
{
    // Remove transactions spending a coinbase which are now immature
    LOCK(cs);
    list<CTransaction> transactionsToRemove;
    for (std::map<uint256, CTxMemPoolEntry>::const_iterator it = mapTx.begin(); it != mapTx.end(); it++) {
        const CTransaction& tx = it->second.GetTx();
        BOOST_FOREACH(const CTxIn& txin, tx.vin) {
            std::map<uint256, CTxMemPoolEntry>::const_iterator it2 = mapTx.find(txin.prevout.hash);
            if (it2 != mapTx.end())
                continue;
            const CCoins *coins = pcoins->AccessCoins(txin.prevout.hash);
            if (fSanityCheck) assert(coins);
            if (!coins || (coins->IsCoinBase() && nMemPoolHeight - coins->nHeight < COINBASE_MATURITY)) {
                transactionsToRemove.push_back(tx);
                break;
            }
        }
    }
    BOOST_FOREACH(const CTransaction& tx, transactionsToRemove) {
        list<CTransaction> removed;
        remove(tx, removed, true);
    }
}

void CTxMemPool::removeConflicts(const CTransaction &tx, std::list<CTransaction>& removed)
{
    // Remove transactions which depend on inputs of tx, recursively
    list<CTransaction> result;
    LOCK(cs);
    BOOST_FOREACH(const CTxIn &txin, tx.vin) {
        std::map<COutPoint, CInPoint>::iterator it = mapNextTx.find(txin.prevout);
        if (it != mapNextTx.end()) {
            const CTransaction &txConflict = *it->second.ptx;
            if (txConflict != tx)
            {
                remove(txConflict, removed, true);
            }
        }
    }
}

/**
 * Called when a block is connected. Removes from mempool and updates the miner fee estimator.
 */
void CTxMemPool::removeForBlock(const std::vector<CTransaction>& vtx, unsigned int nBlockHeight,
                                std::list<CTransaction>& conflicts)
{
    LOCK(cs);
    std::vector<CTxMemPoolEntry> entries;
    BOOST_FOREACH(const CTransaction& tx, vtx)
    {
        uint256 hash = tx.GetHash();
        if (mapTx.count(hash))
            entries.push_back(mapTx[hash]);
    }
    minerPolicyEstimator->seenBlock(entries, nBlockHeight, minRelayFee);
    BOOST_FOREACH(const CTransaction& tx, vtx)
    {
        std::list<CTransaction> dummy;
        remove(tx, dummy, false);
        removeConflicts(tx, conflicts);
        ClearPrioritisation(tx.GetHash());
    }
}


void CTxMemPool::clear()
{
    LOCK(cs);
    mapTx.clear();
    mapNextTx.clear();
    totalTxSize = 0;
    ++nTransactionsUpdated;
}

void CTxMemPool::check(const CCoinsViewCache *pcoins) const
{
    if (!fSanityCheck)
        return;

    LogPrint("mempool", "Checking mempool with %u transactions and %u inputs\n", (unsigned int)mapTx.size(), (unsigned int)mapNextTx.size());

    uint64_t checkTotal = 0;

    CCoinsViewCache mempoolDuplicate(const_cast<CCoinsViewCache*>(pcoins));

    LOCK(cs);
    list<const CTxMemPoolEntry*> waitingOnDependants;
    for (std::map<uint256, CTxMemPoolEntry>::const_iterator it = mapTx.begin(); it != mapTx.end(); it++) {
        unsigned int i = 0;
        checkTotal += it->second.GetTxSize();
        const CTransaction& tx = it->second.GetTx();
        bool fDependsWait = false;
        BOOST_FOREACH(const CTxIn &txin, tx.vin) {
            // Check that every mempool transaction's inputs refer to available coins, or other mempool tx's.
            std::map<uint256, CTxMemPoolEntry>::const_iterator it2 = mapTx.find(txin.prevout.hash);
            if (it2 != mapTx.end()) {
                const CTransaction& tx2 = it2->second.GetTx();
                assert(tx2.vout.size() > txin.prevout.n && !tx2.vout[txin.prevout.n].IsNull());
                fDependsWait = true;
            } else {
                const CCoins* coins = pcoins->AccessCoins(txin.prevout.hash);
                assert(coins && coins->IsAvailable(txin.prevout.n));
            }
            // Check whether its inputs are marked in mapNextTx.
            std::map<COutPoint, CInPoint>::const_iterator it3 = mapNextTx.find(txin.prevout);
            assert(it3 != mapNextTx.end());
            assert(it3->second.ptx == &tx);
            assert(it3->second.n == i);
            i++;
        }
        if (fDependsWait)
            waitingOnDependants.push_back(&it->second);
        else {
            CValidationState state;
            assert(CheckInputs(tx, state, mempoolDuplicate, false, 0, false, NULL));
            UpdateCoins(tx, state, mempoolDuplicate, 1000000);
        }
    }
    unsigned int stepsSinceLastRemove = 0;
    while (!waitingOnDependants.empty()) {
        const CTxMemPoolEntry* entry = waitingOnDependants.front();
        waitingOnDependants.pop_front();
        CValidationState state;
        if (!mempoolDuplicate.HaveInputs(entry->GetTx())) {
            waitingOnDependants.push_back(entry);
            stepsSinceLastRemove++;
            assert(stepsSinceLastRemove < waitingOnDependants.size());
        } else {
            assert(CheckInputs(entry->GetTx(), state, mempoolDuplicate, false, 0, false, NULL));
            UpdateCoins(entry->GetTx(), state, mempoolDuplicate, 1000000);
            stepsSinceLastRemove = 0;
        }
    }
    for (std::map<COutPoint, CInPoint>::const_iterator it = mapNextTx.begin(); it != mapNextTx.end(); it++) {
        uint256 hash = it->second.ptx->GetHash();
        map<uint256, CTxMemPoolEntry>::const_iterator it2 = mapTx.find(hash);
        const CTransaction& tx = it2->second.GetTx();
        assert(it2 != mapTx.end());
        assert(&tx == it->second.ptx);
        assert(tx.vin.size() > it->second.n);
        assert(it->first == it->second.ptx->vin[it->second.n].prevout);
    }

    assert(totalTxSize == checkTotal);
}

void CTxMemPool::queryHashes(vector<uint256>& vtxid)
{
    vtxid.clear();

    LOCK(cs);
    vtxid.reserve(mapTx.size());
    for (map<uint256, CTxMemPoolEntry>::iterator mi = mapTx.begin(); mi != mapTx.end(); ++mi)
        vtxid.push_back((*mi).first);
}

bool CTxMemPool::lookup(uint256 hash, CTransaction& result) const
{
    LOCK(cs);
    map<uint256, CTxMemPoolEntry>::const_iterator i = mapTx.find(hash);
    if (i == mapTx.end()) return false;
    result = i->second.GetTx();
    return true;
}

CFeeRate CTxMemPool::estimateFee(int nBlocks) const
{
    LOCK(cs);
    return minerPolicyEstimator->estimateFee(nBlocks);
}
double CTxMemPool::estimatePriority(int nBlocks) const
{
    LOCK(cs);
    return minerPolicyEstimator->estimatePriority(nBlocks);
}

bool
CTxMemPool::WriteFeeEstimates(CAutoFile& fileout) const
{
    try {
        LOCK(cs);
        fileout << 99900; // version required to read: 0.9.99 or later
        fileout << CLIENT_VERSION; // version that wrote the file
        minerPolicyEstimator->Write(fileout);
    }
    catch (const std::exception&) {
        LogPrintf("CTxMemPool::WriteFeeEstimates() : unable to write policy estimator data (non-fatal)");
        return false;
    }
    return true;
}

bool
CTxMemPool::ReadFeeEstimates(CAutoFile& filein)
{
    try {
        int nVersionRequired, nVersionThatWrote;
        filein >> nVersionRequired >> nVersionThatWrote;
        if (nVersionRequired > CLIENT_VERSION)
            return error("CTxMemPool::ReadFeeEstimates() : up-version (%d) fee estimate file", nVersionRequired);

        LOCK(cs);
        minerPolicyEstimator->Read(filein, minRelayFee);
    }
    catch (const std::exception&) {
        LogPrintf("CTxMemPool::ReadFeeEstimates() : unable to read policy estimator data (non-fatal)");
        return false;
    }
    return true;
}

void CTxMemPool::PrioritiseTransaction(const uint256 hash, const string strHash, double dPriorityDelta, const CAmount& nFeeDelta)
{
    {
        LOCK(cs);
        std::pair<double, CAmount> &deltas = mapDeltas[hash];
        deltas.first += dPriorityDelta;
        deltas.second += nFeeDelta;
    }
    LogPrintf("PrioritiseTransaction: %s priority += %f, fee += %d\n", strHash, dPriorityDelta, FormatMoney(nFeeDelta));
}

void CTxMemPool::ApplyDeltas(const uint256 hash, double &dPriorityDelta, CAmount &nFeeDelta)
{
    LOCK(cs);
    std::map<uint256, std::pair<double, CAmount> >::iterator pos = mapDeltas.find(hash);
    if (pos == mapDeltas.end())
        return;
    const std::pair<double, CAmount> &deltas = pos->second;
    dPriorityDelta += deltas.first;
    nFeeDelta += deltas.second;
}

void CTxMemPool::ClearPrioritisation(const uint256 hash)
{
    LOCK(cs);
    mapDeltas.erase(hash);
}


CCoinsViewMemPool::CCoinsViewMemPool(CCoinsView *baseIn, CTxMemPool &mempoolIn) : CCoinsViewBacked(baseIn), mempool(mempoolIn) { }

bool CCoinsViewMemPool::GetCoins(const uint256 &txid, CCoins &coins) const {
    // If an entry in the mempool exists, always return that one, as it's guaranteed to never
    // conflict with the underlying cache, and it cannot have pruned entries (as it contains full)
    // transactions. First checking the underlying cache risks returning a pruned entry instead.
    CTransaction tx;
    if (mempool.lookup(txid, tx)) {
        coins = CCoins(tx, MEMPOOL_HEIGHT);
        return true;
    }
    return (base->GetCoins(txid, coins) && !coins.IsPruned());
}

bool CCoinsViewMemPool::HaveCoins(const uint256 &txid) const {
    return mempool.exists(txid) || base->HaveCoins(txid);
}
