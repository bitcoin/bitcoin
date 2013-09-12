// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include "main.h"
#include "wallet.h"

using namespace std;
using namespace ::boost;
using namespace ::boost::multi_index;

CTxMemPool mempool;

static const char* MEMPOOL_FILENAME="mempool.dat";

// CMinerPolicyEstimator is told when transactions exit the
// memory pool because they are included in blocks, and uses
// that information to estimate the priority needed for
// free transactions to be included in blocks and the
// fee needed for fee-paying transactions.

struct TimeValue
{
    int64 t;
    double v;
    TimeValue(int64 _t, double _v) : t(_t), v(_v) { }
};
typedef multi_index_container<
    TimeValue,
    indexed_by<
        // Sort by time inserted
        ordered_non_unique<member<TimeValue,int64,&TimeValue::t > >,

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
    // from the memory pool and because the transaction relaying code
    // calls estimate on every transaction to decide whether or not it
    // should be relayed.
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
            int64 t;
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

    void add(const CTxMemPoolEntry& entry, int nBlockHeight)
    {
        if (nBlockHeight < 0 || entry.getTxSize() == 0) return;
        double dFeePerByte = entry.getFee() / (double)entry.getTxSize();
        double dPriority = entry.getPriority(nBlockHeight);
        if (dPriority == 0 && dFeePerByte > 0)
        {
            byFee.insert(TimeValue(GetTimeMillis(), dFeePerByte));
            resize(byFee, nMax);
            byFeeCache.clear();
        }
        else if (dFeePerByte == 0 && dPriority > 0)
        {
            byPriority.insert(TimeValue(GetTimeMillis(), dPriority));
            resize(byPriority, nMax);
            byPriorityCache.clear();
        }
        // Ignore transactions with both fee and priority > 0,
        // because we can't tell why miners included them (might
        // have been priority, might have been fee)
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

CTxMemPoolEntry::CTxMemPoolEntry(const CTransaction& _tx, int64 _nFee, double _dPriority,
                                 unsigned int _nHeight):
    tx(_tx), nFee(_nFee), dPriority(_dPriority), nHeight(_nHeight)
{
    nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
}

CTxMemPoolEntry::CTxMemPoolEntry(const CTxMemPoolEntry& other)
{
    *this = other;
}

double
CTxMemPoolEntry::getPriority(unsigned int currentHeight) const
{
    int64 nValueIn = tx.GetValueOut()+nFee;
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
    // well in practice.
    minerPolicyEstimator = new CMinerPolicyEstimator(100, 10000);
}

CTxMemPool::~CTxMemPool()
{
    delete minerPolicyEstimator;
}

void CTxMemPool::pruneSpent(const uint256 &hashTx, CCoins &coins)
{
    LOCK(cs);

    map<COutPoint, CInPoint>::iterator it = mapNextTx.lower_bound(COutPoint(hashTx, 0));

    // iterate over all COutPoints in mapNextTx whose hash equals the provided hashTx
    while (it != mapNextTx.end() && it->first.hash == hashTx) {
        coins.Spend(it->first.n); // and remove those outputs from coins
        it++;
    }
}

bool CTxMemPool::accept(CValidationState &state, const CTransaction &tx, bool fLimitFree,
                        bool* pfMissingInputs, bool fRejectInsaneFee)
{
    if (pfMissingInputs)
        *pfMissingInputs = false;

    if (!CheckTransaction(tx, state))
        return error("CTxMemPool::accept() : CheckTransaction failed");

    // Coinbase is only valid in a block, not as a loose transaction
    if (tx.IsCoinBase())
        return state.DoS(100, error("CTxMemPool::accept() : coinbase as individual tx"));

    // Rather not work on nonstandard transactions (unless -testnet/-regtest)
    string reason;
    if (Params().NetworkID() == CChainParams::MAIN && !IsStandardTx(tx, reason))
        return error("CTxMemPool::accept() : nonstandard transaction: %s",
                     reason.c_str());

    // is it already in the memory pool?
    uint256 hash = tx.GetHash();
    {
        LOCK(cs);
        if (mapTx.count(hash))
            return false;
    }

    // Check for conflicts with in-memory transactions
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        COutPoint outpoint = tx.vin[i].prevout;
        if (mapNextTx.count(outpoint))
        {
            // Disable replacement feature for now
            return false;
        }
    }

    unsigned int nBestBlockHeight = 0;
    CCoinsView dummy;
    CCoinsViewCache view(dummy);
    {
        LOCK(cs);
        CCoinsViewMemPool viewMemPool(*pcoinsTip, *this);
        view.SetBackend(viewMemPool);

        // do we already have it?
        if (view.HaveCoins(hash))
            return false;

        // do all inputs exist?
        // Note that this does not check for the presence of actual outputs (see the next check for that),
        // only helps filling in pfMissingInputs (to determine missing vs spent).
        BOOST_FOREACH(const CTxIn txin, tx.vin) {
            if (!view.HaveCoins(txin.prevout.hash)) {
                if (pfMissingInputs)
                    *pfMissingInputs = true;
                return false;
            }
        }

        // are the actual inputs available?
        if (!view.HaveInputs(tx))
            return state.Invalid(error("CTxMemPool::accept() : inputs already spent"));

        // Bring the best block into scope
        nBestBlockHeight = view.GetBestBlock()->nHeight;

        // we have all inputs cached now, so switch back to dummy, so we don't need to keep lock on mempool
        view.SetBackend(dummy);
    }
    {
        // Check for non-standard pay-to-script-hash in inputs
        if (Params().NetworkID() == CChainParams::MAIN && !AreInputsStandard(tx, view))
            return error("CTxMemPool::accept() : nonstandard transaction input");

        // Note: if you modify this code to accept non-standard transactions, then
        // you should add code here to check that the transaction does a
        // reasonable number of ECDSA signature verifications.

        int64 nValueIn = view.GetValueIn(tx);
        int64 nValueOut = tx.GetValueOut();
        int64 nFees = nValueIn-nValueOut;
        double dPriority = view.GetPriority(tx, nBestBlockHeight);

        CTxMemPoolEntry entry(tx, nFees, dPriority, nBestBlockHeight);
        unsigned int nSize = entry.getTxSize();

        // Don't accept it if it can't get into a block
        int64 txMinFee = GetMinFee(tx, true, GMF_RELAY);
        if (fLimitFree && nFees < txMinFee)
            return error("CTxMemPool::accept() : not enough fees %s, %"PRI64d" < %"PRI64d,
                         hash.ToString().c_str(),
                         nFees, txMinFee);

        // Continuously rate-limit free transactions
        // This mitigates 'penny-flooding' -- sending thousands of free transactions just to
        // be annoying or make others' transactions take longer to confirm.
        if (fLimitFree && nFees < CTransaction::nMinRelayTxFee)
        {
            static double dFreeCount;
            static int64 nLastTime;
            int64 nNow = GetTime();

            LOCK(cs);

            // Use an exponentially decaying ~10-minute window:
            dFreeCount *= pow(1.0 - 1.0/600.0, (double)(nNow - nLastTime));
            nLastTime = nNow;
            // -limitfreerelay unit is thousand-bytes-per-minute
            // At default rate it would take over a month to fill 1GB
            if (dFreeCount >= GetArg("-limitfreerelay", 15)*10*1000)
                return error("CTxMemPool::accept() : free transaction rejected by rate limiter");
            LogPrint("mempool", "Rate limit dFreeCount: %g => %g\n", dFreeCount, dFreeCount+nSize);
            dFreeCount += nSize;
        }

        if (fRejectInsaneFee && nFees > CTransaction::nMinRelayTxFee * 10000)
            return error("CTxMemPool::accept() : insane fees %s, %"PRI64d" > %"PRI64d,
                         hash.ToString().c_str(),
                         nFees, CTransaction::nMinRelayTxFee * 10000);

        // Check against previous transactions
        // This is done last to help prevent CPU exhaustion denial-of-service attacks.
        if (!CheckInputs(tx, state, view, true, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC))
        {
            return error("CTxMemPool::accept() : ConnectInputs failed %s", hash.ToString().c_str());
        }
        // Store transaction in memory
        addUnchecked(hash, entry);
    }

    SyncWithWallets(hash, tx, NULL, true);

    LogPrint("mempool", "CTxMemPool::accept() : accepted %s (poolsz %"PRIszu")\n",
             hash.ToString().c_str(),
             mapTx.size());
    return true;
}


bool CTxMemPool::addUnchecked(const uint256& hash, const CTxMemPoolEntry &entry)
{
    // Add to memory pool without checking anything.  Don't call this directly,
    // call CTxMemPool::accept to properly check the transaction first.
    {
        LOCK(cs);
        mapTx[hash] = entry;
        const CTransaction& tx = mapTx[hash].getTx();
        for (unsigned int i = 0; i < tx.vin.size(); i++)
            mapNextTx[tx.vin[i].prevout] = CInPoint(&tx, i);
        nTransactionsUpdated++;
    }
    return true;
}


bool CTxMemPool::remove(const uint256& hash, bool fRecursive, int nBlockHeight)
{
    // Remove transaction from memory pool
    {
        LOCK(cs);
        if (mapTx.count(hash) == 0)
            return false;
        const CTxMemPoolEntry& entry = mapTx[hash];
        const CTransaction& tx = entry.getTx();
        if (fRecursive) {
            for (unsigned int i = 0; i < tx.vout.size(); i++) {
                map<COutPoint, CInPoint>::iterator it = mapNextTx.find(COutPoint(hash, i));
                if (it != mapNextTx.end())
                    remove(it->second.ptx->GetHash(), true, nBlockHeight);
            }
        }
        minerPolicyEstimator->add(entry, nBlockHeight);
        BOOST_FOREACH(const CTxIn& txin, tx.vin)
            mapNextTx.erase(txin.prevout);
        mapTx.erase(hash);
        nTransactionsUpdated++;
    }
    return true;
}

bool CTxMemPool::removeConflicts(const CTransaction &tx)
{
    // Remove transactions which depend on inputs of tx, recursively
    LOCK(cs);
    BOOST_FOREACH(const CTxIn &txin, tx.vin) {
        map<COutPoint, CInPoint>::iterator it = mapNextTx.find(txin.prevout);
        if (it != mapNextTx.end()) {
            const CTransaction &txConflict = *it->second.ptx;
            if (txConflict != tx)
                remove(txConflict.GetHash(), true);
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
    for (map<uint256, CTxMemPoolEntry>::const_iterator it = mapTx.begin(); it != mapTx.end(); it++) {
        unsigned int i = 0;
        const CTransaction& tx = it->second.getTx();
        BOOST_FOREACH(const CTxIn &txin, tx.vin) {
            // Check that every mempool transaction's inputs refer to available coins, or other mempool tx's.
            map<uint256, CTxMemPoolEntry>::const_iterator it2 = mapTx.find(txin.prevout.hash);
            if (it2 != mapTx.end()) {
                const CTransaction& tx2 = it2->second.getTx();
                assert(tx2.vout.size() > txin.prevout.n && !tx2.vout[txin.prevout.n].IsNull());
            } else {
                CCoins &coins = pcoins->GetCoins(txin.prevout.hash);
                assert(coins.IsAvailable(txin.prevout.n));
            }
            // Check whether its inputs are marked in mapNextTx.
            map<COutPoint, CInPoint>::const_iterator it3 = mapNextTx.find(txin.prevout);
            assert(it3 != mapNextTx.end());
            assert(it3->second.ptx == &tx);
            assert(it3->second.n == i);
            i++;
        }
    }
    for (map<COutPoint, CInPoint>::const_iterator it = mapNextTx.begin(); it != mapNextTx.end(); it++) {
        uint256 hash = it->second.ptx->GetHash();
        map<uint256, CTxMemPoolEntry>::const_iterator it2 = mapTx.find(hash);
        const CTransaction& tx = it2->second.getTx();
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
    result = i->second.getTx();
    return true;
}

void CTxMemPool::estimateFees(double dPriorityMedian, double& dPriority, double dFeeMedian, double& dFee)
{
    LOCK(cs);
    dPriority = minerPolicyEstimator->estimatePriority(dPriorityMedian);
    dFee = minerPolicyEstimator->estimateFee(dFeeMedian);
}

void CTxMemPool::writeEntry(CAutoFile& file, const uint256& txid, std::set<uint256>& alreadyWritten)
{
    if (alreadyWritten.count(txid)) return;
    alreadyWritten.insert(txid);
    CTxMemPoolEntry& entry = mapTx[txid];
    // Write txns we depend on first:
    BOOST_FOREACH(const CTxIn txin, entry.getTx().vin)
    {
        const uint256& prevout = txin.prevout.hash;
        if (mapTx.count(prevout))
            writeEntry(file, prevout, alreadyWritten);
    }
    file << entry.getTx();
}

//
// Format of the mempool.dat file:
//  32-bit versionRequiredToRead
//  32-bit versionThatWrote
//  32-bit-number of priority data points
//  [ (time,priority) ]
//  32-bit-number of fee data points
//  [ (time,fee) ]
//  32-bit-number of transactions
//  [ serialized transactions ]
//
bool CTxMemPool::Write()
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
        minerPolicyEstimator->Write(fileout);
        fileout << mapTx.size();
        for (map<uint256, CTxMemPoolEntry>::const_iterator it = mapTx.begin();
             it != mapTx.end(); it++)
        {
            writeEntry(fileout, it->first, alreadyWritten);
        }
    }
    catch (std::exception &e) {
        return error("CTxMemPool::Write() : I/O error");
    }

    return true;
}

bool CTxMemPool::Read()
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

        minerPolicyEstimator->Read(filein);
        size_t nTx;
        filein >> nTx;

        for (size_t i = 0; i < nTx; i++)
        {
            CTransaction tx;
            filein >> tx;
        
            CValidationState state;
            bool fMissingInputs;
            accept(state, tx, false, &fMissingInputs);
            assert(state.IsValid() && !fMissingInputs);
        }
    }
    catch (std::exception &e) {
        return error("CTxMemPool::Read() : I/O error or stream data corrupted");
    }

    return true;
}
