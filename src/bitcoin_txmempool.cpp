// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoin_core.h"
#include "bitcoin_txmempool.h"

using namespace std;

Bitcoin_CTxMemPoolEntry::Bitcoin_CTxMemPoolEntry()
{
    nHeight = BITCOIN_MEMPOOL_HEIGHT;
}

Bitcoin_CTxMemPoolEntry::Bitcoin_CTxMemPoolEntry(const Bitcoin_CTransaction& _tx, int64_t _nFee,
                                 int64_t _nTime, double _dPriority,
                                 unsigned int _nHeight):
    tx(_tx), nFee(_nFee), nTime(_nTime), dPriority(_dPriority), nHeight(_nHeight)
{
    nTxSize = ::GetSerializeSize(tx, SER_NETWORK, BITCOIN_PROTOCOL_VERSION);
}

Bitcoin_CTxMemPoolEntry::Bitcoin_CTxMemPoolEntry(const Bitcoin_CTxMemPoolEntry& other)
{
    *this = other;
}

double
Bitcoin_CTxMemPoolEntry::GetPriority(unsigned int currentHeight) const
{
    int64_t nValueIn = tx.GetValueOut()+nFee;
    double deltaPriority = ((double)(currentHeight-nHeight)*nValueIn)/nTxSize;
    double dResult = dPriority + deltaPriority;
    return dResult;
}

Bitcoin_CTxMemPool::Bitcoin_CTxMemPool()
{
    // Sanity checks off by default for performance, because otherwise
    // accepting transactions becomes O(N^2) where N is the number
    // of transactions in the pool
    fSanityCheck = false;
}

void Bitcoin_CTxMemPool::pruneSpent(const uint256 &hashTx, Bitcoin_CCoins &coins)
{
    LOCK(cs);

    std::map<COutPoint, Bitcoin_CInPoint>::iterator it = mapNextTx.lower_bound(COutPoint(hashTx, 0));

    // iterate over all COutPoints in mapNextTx whose hash equals the provided hashTx
    while (it != mapNextTx.end() && it->first.hash == hashTx) {
        coins.Bitcoin_Spend(it->first); // and remove those outputs from coins
        it++;
    }
}

unsigned int Bitcoin_CTxMemPool::GetTransactionsUpdated() const
{
    LOCK(cs);
    return nTransactionsUpdated;
}

void Bitcoin_CTxMemPool::AddTransactionsUpdated(unsigned int n)
{
    LOCK(cs);
    nTransactionsUpdated += n;
}


bool Bitcoin_CTxMemPool::addUnchecked(const uint256& hash, const Bitcoin_CTxMemPoolEntry &entry)
{
    // Add to memory pool without checking anything.
    // Used by main.cpp AcceptToMemoryPool(), which DOES do
    // all the appropriate checks.
    LOCK(cs);
    {
        mapTx[hash] = entry;
        const Bitcoin_CTransaction& tx = mapTx[hash].GetTx();
        for (unsigned int i = 0; i < tx.vin.size(); i++)
            mapNextTx[tx.vin[i].prevout] = Bitcoin_CInPoint(&tx, i);
        nTransactionsUpdated++;
    }
    return true;
}


void Bitcoin_CTxMemPool::remove(const Bitcoin_CTransaction &tx, std::list<Bitcoin_CTransaction>& removed, bool fRecursive)
{
    // Remove transaction from memory pool
    {
        LOCK(cs);
        uint256 hash = tx.GetHash();
        if (fRecursive) {
            for (unsigned int i = 0; i < tx.vout.size(); i++) {
                std::map<COutPoint, Bitcoin_CInPoint>::iterator it = mapNextTx.find(COutPoint(hash, i));
                if (it == mapNextTx.end())
                    continue;
                remove(*it->second.ptx, removed, true);
            }
        }
        if (mapTx.count(hash))
        {
            removed.push_front(tx);
            BOOST_FOREACH(const Bitcoin_CTxIn& txin, tx.vin)
                mapNextTx.erase(txin.prevout);
            mapTx.erase(hash);
            nTransactionsUpdated++;
        }
    }
}

void Bitcoin_CTxMemPool::removeConflicts(const Bitcoin_CTransaction &tx, std::list<Bitcoin_CTransaction>& removed)
{
    // Remove transactions which depend on inputs of tx, recursively
    list<Bitcoin_CTransaction> result;
    LOCK(cs);
    BOOST_FOREACH(const Bitcoin_CTxIn &txin, tx.vin) {
        std::map<COutPoint, Bitcoin_CInPoint>::iterator it = mapNextTx.find(txin.prevout);
        if (it != mapNextTx.end()) {
            const Bitcoin_CTransaction &txConflict = *it->second.ptx;
            if (txConflict != tx)
            {
                remove(txConflict, removed, true);
            }
        }
    }
}

void Bitcoin_CTxMemPool::clear()
{
    LOCK(cs);
    mapTx.clear();
    mapNextTx.clear();
    ++nTransactionsUpdated;
}

void Bitcoin_CTxMemPool::check(Bitcoin_CCoinsViewCache *pcoins) const
{
    if (!fSanityCheck)
        return;

    LogPrint("mempool", "Checking mempool with %u transactions and %u inputs\n", (unsigned int)mapTx.size(), (unsigned int)mapNextTx.size());

    LOCK(cs);
    for (std::map<uint256, Bitcoin_CTxMemPoolEntry>::const_iterator it = mapTx.begin(); it != mapTx.end(); it++) {
        unsigned int i = 0;
        const Bitcoin_CTransaction& tx = it->second.GetTx();
        BOOST_FOREACH(const Bitcoin_CTxIn &txin, tx.vin) {
            // Check that every mempool transaction's inputs refer to available coins, or other mempool tx's.
            std::map<uint256, Bitcoin_CTxMemPoolEntry>::const_iterator it2 = mapTx.find(txin.prevout.hash);
            if (it2 != mapTx.end()) {
                const Bitcoin_CTransaction& tx2 = it2->second.GetTx();
                assert(tx2.vout.size() > txin.prevout.n && !tx2.vout[txin.prevout.n].IsNull());
            } else {
            	Bitcoin_CCoins &coins = pcoins->GetCoins(txin.prevout.hash);
                assert(coins.Original_IsAvailable(txin.prevout.n));
            }
            // Check whether its inputs are marked in mapNextTx.
            std::map<COutPoint, Bitcoin_CInPoint>::const_iterator it3 = mapNextTx.find(txin.prevout);
            assert(it3 != mapNextTx.end());
            assert(it3->second.ptx == &tx);
            assert(it3->second.n == i);
            i++;
        }
    }
    for (std::map<COutPoint, Bitcoin_CInPoint>::const_iterator it = mapNextTx.begin(); it != mapNextTx.end(); it++) {
        uint256 hash = it->second.ptx->GetHash();
        map<uint256, Bitcoin_CTxMemPoolEntry>::const_iterator it2 = mapTx.find(hash);
        const Bitcoin_CTransaction& tx = it2->second.GetTx();
        assert(it2 != mapTx.end());
        assert(&tx == it->second.ptx);
        assert(tx.vin.size() > it->second.n);
        assert(it->first == it->second.ptx->vin[it->second.n].prevout);
    }
}

void Bitcoin_CTxMemPool::queryHashes(vector<uint256>& vtxid)
{
    vtxid.clear();

    LOCK(cs);
    vtxid.reserve(mapTx.size());
    for (map<uint256, Bitcoin_CTxMemPoolEntry>::iterator mi = mapTx.begin(); mi != mapTx.end(); ++mi)
        vtxid.push_back((*mi).first);
}

bool Bitcoin_CTxMemPool::lookup(uint256 hash, Bitcoin_CTransaction& result) const
{
    LOCK(cs);
    map<uint256, Bitcoin_CTxMemPoolEntry>::const_iterator i = mapTx.find(hash);
    if (i == mapTx.end()) return false;
    result = i->second.GetTx();
    return true;
}

Bitcoin_CCoinsViewMemPool::Bitcoin_CCoinsViewMemPool(Bitcoin_CCoinsView &baseIn, Bitcoin_CTxMemPool &mempoolIn) : Bitcoin_CCoinsViewBacked(baseIn), mempool(mempoolIn) { }

bool Bitcoin_CCoinsViewMemPool::GetCoins(const uint256 &txid, Bitcoin_CCoins &coins) {
    if (base->GetCoins(txid, coins))
        return true;
    Bitcoin_CTransaction tx;
    if (mempool.lookup(txid, tx)) {
        coins = Bitcoin_CCoins(tx, BITCOIN_MEMPOOL_HEIGHT);
        return true;
    }
    return false;
}

bool Bitcoin_CCoinsViewMemPool::HaveCoins(const uint256 &txid) {
    return mempool.exists(txid) || base->HaveCoins(txid);
}

