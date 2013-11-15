// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "compat.h"
#include "core.h"
#include "txmempool.h"

// These must come after compat.h to avoid windows
// cross-compiler build errors.
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

using namespace std;
using namespace boost;
using namespace boost::multi_index;

static const char* MEMPOOL_FILENAME="mempool.dat";

// CMinerPolicyEstimator is told when transactions exit the
// memory pool because they are included in blocks, and uses
// that information to estimate the priority needed for
// free transactions to be included in blocks and the
// fee needed for fee-paying transactions.

struct TimeValue
{
    int64_t t;
    double v;
    TimeValue(int64_t _t, double _v) : t(_t), v(_v) { }
};
typedef multi_index_container<
    TimeValue,
    indexed_by<
        // Sort by time inserted
        ordered_non_unique<member<TimeValue,int64_t,&TimeValue::t > >,

        // Sort by value
        ordered_non_unique<member<TimeValue,double,&TimeValue::v > >
        >
    > SortedValues ;

class CMinerPolicyEstimator
{
private:
    size_t nMin, nMax;
    SortedValues byPriority;
    SortedValues byFee;

    // Results of estimate() are cached between new blocks, because
    // the estimate doesn't change until a new block pulls transactions
    // from the memory pool and because estimation is O(N) where N
    // is the number of data points; that becomes O(N^2) if an estimate
    // is done for every transaction
    map<double, double> byPriorityCache;
    map<double, double> byFeeCache;

    // Estimate what value is required to be chosen above
    // fraction of other transactions (fraction is 0.0 to 1.0)
    // Returns -1.0 if not enough data has been collected to
    // give a good estimate.
    double estimate(const SortedValues& values, double fraction, map<double,double>& cache)
    {
        assert(fraction >= 0.0 && fraction <= 1.0);
        if (values.size() < nMin) return -1.0;

        map<double,double>::iterator cached = cache.find(fraction);
        if (cached != cache.end())
            return cached->second;

        size_t n = size_t(values.size()*fraction);
        if (n > 0) --n;
        SortedValues::nth_index<1>::type::iterator it=values.get<1>().begin();
        std::advance(it, n);
        cache[fraction] = it->v;
        return it->v;
    }

    bool Write(CAutoFile& fileout, const SortedValues& values)
    {
        fileout << values.size();
        SortedValues::nth_index<1>::type::iterator it=values.get<1>().begin();
        while (it != values.get<1>().end())
        {
            fileout << it->t << it->v;
            it++;
        }
        return true;
    }
    bool Read(CAutoFile& filein, SortedValues& values)
    {
        size_t n;
        filein >> n;
        for (size_t i = 0; i < n; i++)
        {
            int64_t t;
            double v;
            filein >> t >> v;
            values.insert(TimeValue(t, v));
        }
        return true;
    }

public:
    CMinerPolicyEstimator(size_t _nMin, size_t _nMax) : nMin(_nMin), nMax(_nMax)
    {
    }

    void resize(SortedValues& what, size_t n)
    {
        while (what.size() > n)
            what.erase(what.begin());
    }

    void add(const CTxMemPoolEntry& entry, unsigned int nBlockHeight)
    {
        if (nBlockHeight == 0 || entry.GetTxSize() == 0) return;
        double dFeePerByte = entry.GetFee() / (double)entry.GetTxSize();
        double dPriority = entry.GetPriority(nBlockHeight);
        if (dPriority == 0 && dFeePerByte > 0)
        {
            byFee.insert(TimeValue(GetTimeMillis(), dFeePerByte));
            resize(byFee, nMax);
            byFeeCache.clear();
            LogPrint("estimatefee", "est.fee+ %g\n", dFeePerByte);
        }
        else if (dFeePerByte == 0 && dPriority > 0)
        {
            byPriority.insert(TimeValue(GetTimeMillis(), dPriority));
            resize(byPriority, nMax);
            byPriorityCache.clear();
            LogPrint("estimatefee", "est.pri+ %g\n", dPriority);
        }
        else
        {
            LogPrint("estimatefee", "est.ignore: %g %g\n", dFeePerByte, dPriority);
            // Ignore transactions with both fee and priority > 0,
            // because we can't tell why miners included them (might
            // have been priority, might have been fee)
        }
    }

    double estimatePriority(double fraction)
    {
        return estimate(byPriority, fraction, byPriorityCache);
    }
    double estimateFee(double fraction)
    {
        return estimate(byFee, fraction, byFeeCache);
    }
    bool Write(CAutoFile& fileout)
    {
        return Write(fileout, byPriority) && Write(fileout, byFee);
    }
    bool Read(CAutoFile& filein)
    {
        return Read(filein, byPriority) && Read(filein, byFee);
    }
};

CTxMemPoolEntry::CTxMemPoolEntry()
{
    nHeight = MEMPOOL_HEIGHT;
}

CTxMemPoolEntry::CTxMemPoolEntry(const CTransaction& _tx, int64_t _nFee,
                                 int64_t _nTime, double _dPriority,
                                 unsigned int _nHeight):
    tx(_tx), nFee(_nFee), nTime(_nTime), dPriority(_dPriority), nHeight(_nHeight)
{
    nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
}

CTxMemPoolEntry::CTxMemPoolEntry(const CTxMemPoolEntry& other)
{
    *this = other;
}

double
CTxMemPoolEntry::GetPriority(unsigned int currentHeight) const
{
    int64_t nValueIn = tx.GetValueOut()+nFee;
    double deltaPriority = ((double)(currentHeight-nHeight)*nValueIn)/nTxSize;
    double dResult = dPriority + deltaPriority;
    return dResult;
}

CTxMemPool::CTxMemPool()
{
    // Sanity checks off by default for performance, because otherwise
    // accepting transactions becomes O(N^2) where N is the number
    // of transactions in the pool
    fSanityCheck = false;
    // 100 and 10,000 values here are arbitrary, but work
    // well in practice, giving reasonable estimates within a few
    // blocks and stable estimates over time.
    minerPolicyEstimator = new CMinerPolicyEstimator(100, 10000);
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
    }
    return true;
}


bool CTxMemPool::remove(const CTransaction &tx, bool fRecursive, unsigned int nBlockHeight)
{
    // Remove transaction from memory pool
    {
        LOCK(cs);
        uint256 hash = tx.GetHash();
        if (fRecursive) {
            for (unsigned int i = 0; i < tx.vout.size(); i++) {
                std::map<COutPoint, CInPoint>::iterator it = mapNextTx.find(COutPoint(hash, i));
                if (it != mapNextTx.end())
                    remove(*it->second.ptx, true);
            }
        }
        if (mapTx.count(hash))
        {
            minerPolicyEstimator->add(mapTx[hash], nBlockHeight);
            BOOST_FOREACH(const CTxIn& txin, tx.vin)
                mapNextTx.erase(txin.prevout);
            mapTx.erase(hash);
            nTransactionsUpdated++;
        }
    }
    return true;
}

bool CTxMemPool::removeConflicts(const CTransaction &tx)
{
    // Remove transactions which depend on inputs of tx, recursively
    LOCK(cs);
    BOOST_FOREACH(const CTxIn &txin, tx.vin) {
        std::map<COutPoint, CInPoint>::iterator it = mapNextTx.find(txin.prevout);
        if (it != mapNextTx.end()) {
            const CTransaction &txConflict = *it->second.ptx;
            if (txConflict != tx)
                remove(txConflict, true);
        }
    }
    return true;
}

void CTxMemPool::clear()
{
    LOCK(cs);
    mapTx.clear();
    mapNextTx.clear();
    ++nTransactionsUpdated;
}

void CTxMemPool::check(CCoinsViewCache *pcoins) const
{
    if (!fSanityCheck)
        return;

    LogPrint("mempool", "Checking mempool with %u transactions and %u inputs\n", (unsigned int)mapTx.size(), (unsigned int)mapNextTx.size());

    LOCK(cs);
    for (std::map<uint256, CTxMemPoolEntry>::const_iterator it = mapTx.begin(); it != mapTx.end(); it++) {
        unsigned int i = 0;
        const CTransaction& tx = it->second.GetTx();
        BOOST_FOREACH(const CTxIn &txin, tx.vin) {
            // Check that every mempool transaction's inputs refer to available coins, or other mempool tx's.
            std::map<uint256, CTxMemPoolEntry>::const_iterator it2 = mapTx.find(txin.prevout.hash);
            if (it2 != mapTx.end()) {
                const CTransaction& tx2 = it2->second.GetTx();
                assert(tx2.vout.size() > txin.prevout.n && !tx2.vout[txin.prevout.n].IsNull());
            } else {
                CCoins &coins = pcoins->GetCoins(txin.prevout.hash);
                assert(coins.IsAvailable(txin.prevout.n));
            }
            // Check whether its inputs are marked in mapNextTx.
            std::map<COutPoint, CInPoint>::const_iterator it3 = mapNextTx.find(txin.prevout);
            assert(it3 != mapNextTx.end());
            assert(it3->second.ptx == &tx);
            assert(it3->second.n == i);
            i++;
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

double CTxMemPool::estimateFreePriority(double dPriorityMedian, bool fUseHardCoded)
{
    LOCK(cs);
    double dPriority = minerPolicyEstimator->estimatePriority(dPriorityMedian);
    // Hard-coded priority is 1-BTC, 144-confirmation old, 250-byte txn:
    if (dPriority < 0 && fUseHardCoded) dPriority = COIN*144 / 250;
    return dPriority;
}

double CTxMemPool::estimateFee(double dFeeMedian, bool fUseHardCoded)
{
    LOCK(cs);
    double dFee = minerPolicyEstimator->estimateFee(dFeeMedian);
    if (dFee < 0 && fUseHardCoded) dFee = CTransaction::nMinTxFee / 1000.0;
    return dFee;
}

void CTxMemPool::writeEntry(CAutoFile& file, const uint256& txid, std::set<uint256>& alreadyWritten) const
{
    if (alreadyWritten.count(txid)) return;
    alreadyWritten.insert(txid);
    const CTxMemPoolEntry& entry = mapTx.at(txid);
    // Write txns we depend on first:
    BOOST_FOREACH(const CTxIn txin, entry.GetTx().vin)
    {
        const uint256& prevout = txin.prevout.hash;
        if (mapTx.count(prevout))
            writeEntry(file, prevout, alreadyWritten);
    }
    unsigned int nHeight = entry.GetHeight();
    file << entry.GetTx() << entry.GetFee() << entry.GetTime() << entry.GetPriority(nHeight) << nHeight;
}

//
// Format of the mempool.dat file:
//  32-bit versionRequiredToRead
//  32-bit versionThatWrote
//  32-bit-number of transactions
//  [ serialized: transaction / fee / time / priority / height ]
//    ... then the miner policy estimation data:
//  32-bit-number of priority data points
//  [ (time,priority) ]
//  32-bit-number of fee data points
//  [ (time,fee) ]
//
bool CTxMemPool::Write() const
{
    boost::filesystem::path path = GetDataDir() / MEMPOOL_FILENAME;
    FILE *file = fopen(path.string().c_str(), "wb"); // Overwrites any older mempool (which is fine)
    CAutoFile fileout = CAutoFile(file, SER_DISK, CLIENT_VERSION);
    if (!fileout)
        return error("CTxMemPool::Write() : open failed");

    fileout << CLIENT_VERSION; // version required to read
    fileout << CLIENT_VERSION; // version that wrote the file

    std::set<uint256> alreadyWritten; // Used to write parents before dependents
    try {
        LOCK(cs);
        fileout << mapTx.size();
        for (map<uint256, CTxMemPoolEntry>::const_iterator it = mapTx.begin();
             it != mapTx.end(); it++)
        {
            writeEntry(fileout, it->first, alreadyWritten);
        }
        minerPolicyEstimator->Write(fileout);
    }
    catch (std::exception &e) {
        // We don't care much about errors; saving
        // and restoring the memory pool is mostly an
        // optimization for cases where a mining node shuts down
        // briefly (maybe to change an option), and it is better
        // to restart with a full memory pool of transactions to mine.
        return error("CTxMemPool::Write() : unable to write (non-fatal)");
    }

    return true;
}

bool CTxMemPool::Read(std::list<CTxMemPoolEntry>& vecEntries) const
{
    boost::filesystem::path path = GetDataDir() / MEMPOOL_FILENAME;
    FILE *file = fopen(path.string().c_str(), "rb");
    if (!file) return true; // No mempool.dat: OK
    CAutoFile filein = CAutoFile(file, SER_DISK, CLIENT_VERSION);
    if (!filein)
        return error("CTxMemPool::Read() : open failed");

    try {
        int nVersionRequired, nVersionThatWrote;
        filein >> nVersionRequired >> nVersionThatWrote;

        if (nVersionRequired > CLIENT_VERSION)
            return error("CTxMemPool::Read() : up-version (%d) mempool.dat", nVersionRequired);

        size_t nTx;
        filein >> nTx;

        for (size_t i = 0; i < nTx; i++)
        {
            CTransaction tx;
            int64_t nFee;
            int64_t nTime;
            double dPriority;
            unsigned int nHeight;
            filein >> tx >> nFee >> nTime >> dPriority >> nHeight;
            CTxMemPoolEntry e(tx, 0, nTime, dPriority, nHeight);
            vecEntries.push_back(e);
        }
        minerPolicyEstimator->Read(filein);
    }
    catch (std::exception &e) {
        // Not a big deal if mempool.dat gets corrupted:
        return error("CTxMemPool::Read() : unable to read (non-fatal)");
    }

    return true;
}

CCoinsViewMemPool::CCoinsViewMemPool(CCoinsView &baseIn, CTxMemPool &mempoolIn) : CCoinsViewBacked(baseIn), mempool(mempoolIn) { }

bool CCoinsViewMemPool::GetCoins(const uint256 &txid, CCoins &coins) {
    if (base->GetCoins(txid, coins))
        return true;
    CTransaction tx;
    if (mempool.lookup(txid, tx)) {
        coins = CCoins(tx, MEMPOOL_HEIGHT);
        return true;
    }
    return false;
}

bool CCoinsViewMemPool::HaveCoins(const uint256 &txid) {
    return mempool.exists(txid) || base->HaveCoins(txid);
}

