// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txmempool.h"

#include "clientversion.h"
#include "consensus/consensus.h"
#include "consensus/validation.h"
#include "main.h"
#include "policy/fees.h"
#include "streams.h"
#include "util.h"
#include "utilmoneystr.h"
#include "version.h"

using namespace std;

CTxMemPoolEntry::CTxMemPoolEntry():
    nFee(0), nTxSize(0), nModSize(0), nUsageSize(0), nTime(0), dPriority(0.0), hadNoDependencies(false)
{
    nHeight = MEMPOOL_HEIGHT;
}

CTxMemPoolEntry::CTxMemPoolEntry(const CTransaction& _tx, const CAmount& _nFee,
                                 int64_t _nTime, double _dPriority,
                                 unsigned int _nHeight, bool poolHasNoInputsOf):
    tx(_tx), nFee(_nFee), nTime(_nTime), dPriority(_dPriority), nHeight(_nHeight),
    hadNoDependencies(poolHasNoInputsOf)
{
    nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
    nModSize = tx.CalculateModifiedSize(nTxSize);
    nUsageSize = RecursiveDynamicUsage(tx);
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

CTxMemPool::CTxMemPool(const CFeeRate& _minRelayFee)
{
    clear();

    // Sanity checks off by default for performance, because otherwise
    // accepting transactions becomes O(N^2) where N is the number
    // of transactions in the pool
    fSanityCheck = false;

    minerPolicyEstimator = new CBlockPolicyEstimator(_minRelayFee);
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


bool CTxMemPool::addUnchecked(const uint256& hash, const CTxMemPoolEntry &entry, bool fCurrentEstimate)
{
    // Add to memory pool without checking anything.
    // Used by main.cpp AcceptToMemoryPool(), which DOES do
    // all the appropriate checks.
    LOCK(cs);
    mapTx.insert(entry);
    const CTransaction& tx = mapTx.find(hash)->GetTx();
    for (unsigned int i = 0; i < tx.vin.size(); i++)
        mapNextTx[tx.vin[i].prevout] = CInPoint(&tx, i);
    nTransactionsUpdated++;
    totalTxSize += entry.GetTxSize();
    cachedInnerUsage += entry.DynamicMemoryUsage();
    minerPolicyEstimator->processTransaction(entry, fCurrentEstimate);

    return true;
}

void CTxMemPool::removeUnchecked(const uint256& hash)
{
    indexed_transaction_set::iterator it = mapTx.find(hash);

    BOOST_FOREACH(const CTxIn& txin, it->GetTx().vin)
        mapNextTx.erase(txin.prevout);

    totalTxSize -= it->GetTxSize();
    cachedInnerUsage -= it->DynamicMemoryUsage();
    mapTx.erase(it);
    nTransactionsUpdated++;
    minerPolicyEstimator->removeTx(hash);
}

void CTxMemPool::remove(const CTransaction &origTx, std::list<CTransaction>& removed, bool fRecursive)
{
    // Remove transaction from memory pool
    {
        LOCK(cs);
        std::deque<uint256> txToRemove;
        txToRemove.push_back(origTx.GetHash());
        if (fRecursive && !mapTx.count(origTx.GetHash())) {
            // If recursively removing but origTx isn't in the mempool
            // be sure to remove any children that are in the pool. This can
            // happen during chain re-orgs if origTx isn't re-accepted into
            // the mempool for any reason.
            for (unsigned int i = 0; i < origTx.vout.size(); i++) {
                std::map<COutPoint, CInPoint>::iterator it = mapNextTx.find(COutPoint(origTx.GetHash(), i));
                if (it == mapNextTx.end())
                    continue;
                txToRemove.push_back(it->second.ptx->GetHash());
            }
        }
        while (!txToRemove.empty())
        {
            uint256 hash = txToRemove.front();
            txToRemove.pop_front();
            if (!mapTx.count(hash))
                continue;
            const CTransaction& tx = mapTx.find(hash)->GetTx();
            if (fRecursive) {
                for (unsigned int i = 0; i < tx.vout.size(); i++) {
                    std::map<COutPoint, CInPoint>::iterator it = mapNextTx.find(COutPoint(hash, i));
                    if (it == mapNextTx.end())
                        continue;
                    txToRemove.push_back(it->second.ptx->GetHash());
                }
            }
            removed.push_back(tx);
            removeUnchecked(hash);
        }
    }
}

void CTxMemPool::removeCoinbaseSpends(const CCoinsViewCache *pcoins, unsigned int nMemPoolHeight)
{
    // Remove transactions spending a coinbase which are now immature
    LOCK(cs);
    list<CTransaction> transactionsToRemove;
    for (indexed_transaction_set::const_iterator it = mapTx.begin(); it != mapTx.end(); it++) {
        const CTransaction& tx = it->GetTx();
        BOOST_FOREACH(const CTxIn& txin, tx.vin) {
            indexed_transaction_set::const_iterator it2 = mapTx.find(txin.prevout.hash);
            if (it2 != mapTx.end())
                continue;
            const CCoins *coins = pcoins->AccessCoins(txin.prevout.hash);
            if (fSanityCheck) assert(coins);
            if (!coins || (coins->IsCoinBase() && ((signed long)nMemPoolHeight) - coins->nHeight < COINBASE_MATURITY)) {
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
                                std::list<CTransaction>& conflicts, bool fCurrentEstimate)
{
    LOCK(cs);
    std::vector<CTxMemPoolEntry> entries;
    BOOST_FOREACH(const CTransaction& tx, vtx)
    {
        uint256 hash = tx.GetHash();

        indexed_transaction_set::iterator i = mapTx.find(hash);
        if (i != mapTx.end())
            entries.push_back(*i);
    }
    BOOST_FOREACH(const CTransaction& tx, vtx)
    {
        std::list<CTransaction> dummy;
        remove(tx, dummy, false);
        removeConflicts(tx, conflicts);
        ClearPrioritisation(tx.GetHash());
    }
    // After the txs in the new block have been removed from the mempool, update policy estimates
    minerPolicyEstimator->processBlock(nBlockHeight, entries, fCurrentEstimate);
}

void CTxMemPool::clear()
{
    LOCK(cs);
    mapTx.clear();
    mapNextTx.clear();
    totalTxSize = 0;
    cachedInnerUsage = 0;
    bypassedSize = 0;
    ++nTransactionsUpdated;
}

void CTxMemPool::check(const CCoinsViewCache *pcoins) const
{
    if (!fSanityCheck)
        return;

    LogPrint("mempool", "Checking mempool with %u transactions and %u inputs\n", (unsigned int)mapTx.size(), (unsigned int)mapNextTx.size());

    uint64_t checkTotal = 0;
    uint64_t innerUsage = 0;

    CCoinsViewCache mempoolDuplicate(const_cast<CCoinsViewCache*>(pcoins));

    LOCK(cs);
    list<const CTxMemPoolEntry*> waitingOnDependants;
    for (indexed_transaction_set::const_iterator it = mapTx.begin(); it != mapTx.end(); it++) {
        unsigned int i = 0;
        checkTotal += it->GetTxSize();
        innerUsage += it->DynamicMemoryUsage();
        const CTransaction& tx = it->GetTx();
        bool fDependsWait = false;
        BOOST_FOREACH(const CTxIn &txin, tx.vin) {
            // Check that every mempool transaction's inputs refer to available coins, or other mempool tx's.
            indexed_transaction_set::const_iterator it2 = mapTx.find(txin.prevout.hash);
            if (it2 != mapTx.end()) {
                const CTransaction& tx2 = it2->GetTx();
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
            waitingOnDependants.push_back(&(*it));
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
        indexed_transaction_set::const_iterator it2 = mapTx.find(hash);
        const CTransaction& tx = it2->GetTx();
        assert(it2 != mapTx.end());
        assert(&tx == it->second.ptx);
        assert(tx.vin.size() > it->second.n);
        assert(it->first == it->second.ptx->vin[it->second.n].prevout);
    }

    assert(totalTxSize == checkTotal);
    assert(innerUsage == cachedInnerUsage);
}

void CTxMemPool::queryHashes(vector<uint256>& vtxid)
{
    vtxid.clear();

    LOCK(cs);
    vtxid.reserve(mapTx.size());
    for (indexed_transaction_set::iterator mi = mapTx.begin(); mi != mapTx.end(); ++mi)
        vtxid.push_back(mi->GetTx().GetHash());
}

bool CTxMemPool::lookup(uint256 hash, CTransaction& result) const
{
    LOCK(cs);
    indexed_transaction_set::const_iterator i = mapTx.find(hash);
    if (i == mapTx.end()) return false;
    result = i->GetTx();
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
        fileout << 109900; // version required to read: 0.10.99 or later
        fileout << CLIENT_VERSION; // version that wrote the file
        minerPolicyEstimator->Write(fileout);
    }
    catch (const std::exception&) {
        LogPrintf("CTxMemPool::WriteFeeEstimates(): unable to write policy estimator data (non-fatal)\n");
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
            return error("CTxMemPool::ReadFeeEstimates(): up-version (%d) fee estimate file", nVersionRequired);

        LOCK(cs);
        minerPolicyEstimator->Read(filein);
    }
    catch (const std::exception&) {
        LogPrintf("CTxMemPool::ReadFeeEstimates(): unable to read policy estimator data (non-fatal)\n");
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

bool CTxMemPool::HasNoInputsOf(const CTransaction &tx) const
{
    LOCK(cs);
    for (unsigned int i = 0; i < tx.vin.size(); i++)
        if (exists(tx.vin[i].prevout.hash))
            return false;
    return true;
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

size_t CTxMemPool::DynamicMemoryUsage() const {
    LOCK(cs);
    // Estimate the overhead of mapTx to be 9 pointers + an allocation, as no exact formula for boost::multi_index_contained is implemented.
    return memusage::MallocUsage(sizeof(CTxMemPoolEntry) + 9 * sizeof(void*)) * mapTx.size() + memusage::DynamicUsage(mapNextTx) + memusage::DynamicUsage(mapDeltas) + cachedInnerUsage;
}

size_t CTxMemPool::GuessDynamicMemoryUsage(const CTxMemPoolEntry& entry) const {
    return memusage::MallocUsage(sizeof(CTxMemPoolEntry) + 9 * sizeof(void*)) + entry.DynamicMemoryUsage() + memusage::IncrementalDynamicUsage(mapNextTx) * entry.GetTx().vin.size();
}

bool CTxMemPool::StageTrimToSize(size_t sizelimit, const CTxMemPoolEntry& toadd, CAmount nFeesReserved,
                                 std::set<uint256>& stage, CAmount& nFeesRemoved) {
    std::set<uint256> protect;
    BOOST_FOREACH(const CTxIn& in, toadd.GetTx().vin) {
        protect.insert(in.prevout.hash);
    }

    size_t incUsage = GuessDynamicMemoryUsage(toadd);
    size_t expsize = DynamicMemoryUsage() + incUsage; // Track the expected resulting memory usage of the mempool.
    if (expsize <= sizelimit) {
        return true;
    }
    size_t sizeToTrim = std::min(expsize - sizelimit, incUsage);
    return TrimMempool(sizeToTrim, protect, nFeesReserved, toadd.GetTxSize(), toadd.GetFee(), true, 10, stage, nFeesRemoved);
}

void CTxMemPool::SurplusTrim(int multiplier, CFeeRate minRelayRate, size_t usageToTrim) {
    CFeeRate excessRate(multiplier * minRelayRate.GetFeePerK());
    std::set<uint256> noprotect;
    CAmount nFeesRemoved = 0;
    std::set<uint256> stageTrimDelete;
    size_t sizeToTrim = usageToTrim / 4;  //Conservatively assume we have transactions at least 1/4 the size of the mempool space they've taken
    if (TrimMempool(usageToTrim, noprotect, 0, sizeToTrim, excessRate.GetFee(sizeToTrim), false, 100, stageTrimDelete, nFeesRemoved)) {
        size_t oldUsage = DynamicMemoryUsage();
        size_t txsToDelete = stageTrimDelete.size();
        RemoveStaged(stageTrimDelete);
        size_t curUsage = DynamicMemoryUsage();
        LogPrint("mempool", "Removing %u transactions (%ld total usage) using periodic trim from reserve size\n", txsToDelete, oldUsage - curUsage);
    }
}

bool CTxMemPool::TrimMempool(size_t sizeToTrim, std::set<uint256> &protect, CAmount nFeesReserved, size_t sizeToUse, CAmount feeToUse,
                             bool mustTrimAllSize, int iterextra, std::set<uint256>& stage, CAmount &nFeesRemoved) {
    size_t usageRemoved = 0;
    indexed_transaction_set::nth_index<1>::type::reverse_iterator it = mapTx.get<1>().rbegin();
    int fails = 0; // Number of mempool transactions iterated over that were not included in the stage.
    int iterperfail = 10;
    int itertotal = 0;
    int failmax = 10; //Try no more than 10 starting transactions
    seed_insecure_rand();
    // Iterate from lowest feerate to highest feerate in the mempool:
    while (usageRemoved < sizeToTrim && it != mapTx.get<1>().rend()) {
        if (insecure_rand()%10) {
            // Only try 1/10 of the transactions so we don't get stuck on the same long chains
            it++;
            continue;
        }
        const uint256& hash = it->GetTx().GetHash();
        if (stage.count(hash)) {
            // If the transaction is already staged for deletion, we know its descendants are already processed, so skip it.
            it++;
            continue;
        }
        if ((double)it->GetFee() * sizeToUse > (double)feeToUse * it->GetTxSize()) {
            // If the transaction's feerate is worse than what we're looking for, nothing else we will iterate over
            // could improve the staged set. If we don't have an acceptable solution by now, bail out.
            break;
        }
        std::deque<uint256> todo; // List of hashes that we still need to process (descendants of 'hash').
        std::set<uint256> now; // Set of hashes that will need to be added to stage if 'hash' is included.
        CAmount nowfee = 0; // Sum of the fees in 'now'.
        size_t nowsize = 0; // Sum of the tx sizes in 'now'.
        size_t nowusage = 0; // Sum of the memory usages of transactions in 'now'.
        todo.push_back(it->GetTx().GetHash()); // Add 'hash' to the todo list, to initiate processing its children.
        bool good = true; // Whether including 'hash' (and all its descendants) is a good idea.
        // Iterate breadth-first over all descendants of transaction with hash 'hash'.
        while (!todo.empty()) {
            uint256 hashnow = todo.front();
            if (protect.count(hashnow)) {
                // If this transaction is in the protected set, we're done with 'hash'.
                good = false;
                break;
            }
            itertotal++; // We only count transactions we actually had to go find in the mempool.
            if (itertotal > iterextra + iterperfail*(fails+1)) {
                good = false;
                break;
            }
            const CTxMemPoolEntry* origTx = &*mapTx.find(hashnow);
            nowfee += origTx->GetFee();
            if (nFeesReserved + nFeesRemoved + nowfee > feeToUse) {
                // If this pushes up to the total fees deleted too high, we're done with 'hash'.
                good = false;
                break;
            }
            todo.pop_front();
            // Add 'hashnow' to the 'now' set, and update its statistics.
            now.insert(hashnow);
            nowusage += GuessDynamicMemoryUsage(*origTx);
            nowsize += origTx->GetTxSize();
            // Find dependencies of 'hashnow' and them to todo.
            std::map<COutPoint, CInPoint>::iterator iter = mapNextTx.lower_bound(COutPoint(hashnow, 0));
            while (iter != mapNextTx.end() && iter->first.hash == hashnow) {
                const uint256& nexthash = iter->second.ptx->GetHash();
                if (!(stage.count(nexthash) || now.count(nexthash))) {
                    todo.push_back(nexthash);
                }
                iter++;
            }
        }
        if (good && (double)nowfee * sizeToUse > (double)feeToUse * nowsize) {
            // The new transaction's feerate is below that of the set we're removing.
            good = false;
        }
        if (good) {
            stage.insert(now.begin(), now.end());
            nFeesRemoved += nowfee;
            usageRemoved += nowusage;
        } else {
            fails++;
            if (fails > failmax) {
                // Bail out after traversing failmax transactions that are not acceptable.
                break;
            }
        }
        it++;
    }
    //We've added all we can.  Is it enough?
    if (mustTrimAllSize && usageRemoved < sizeToTrim)
        return false;
    if (stage.empty() && sizeToTrim > 0)
        return false;
    return true;
}

void CTxMemPool::RemoveStaged(std::set<uint256>& stage) {
    BOOST_FOREACH(const uint256& hash, stage) {
        removeUnchecked(hash);
    }
}

int CTxMemPool::Expire(int64_t time) {
    LOCK(cs);
    indexed_transaction_set::nth_index<2>::type::iterator it = mapTx.get<2>().begin();
    std::set<uint256> toremove;
    int ret = 0;
    while (it != mapTx.get<2>().end() && it->GetTime() < time) {
        toremove.insert(it->GetTx().GetHash());
        it++;
    }
    while (!toremove.empty()) {
        std::set<uint256>::iterator ite = toremove.begin();
        std::map<COutPoint, CInPoint>::iterator iter = mapNextTx.lower_bound(COutPoint(*ite, 0));
        while (iter != mapNextTx.end() && iter->first.hash == *ite) {
            toremove.insert(iter->second.ptx->GetHash());
            iter++;
        }
        ret++;
        removeUnchecked(*ite);
        toremove.erase(ite);
    }
    return ret;
}
