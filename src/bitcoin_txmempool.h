// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_BITCOIN_TXMEMPOOL_H
#define BITCOIN_BITCOIN_TXMEMPOOL_H

#include <list>

#include "bitcoin_coins.h"
#include "bitcoin_core.h"
#include "sync.h"

/** Fake height value used in CCoins to signify they are only in the memory pool (since 0.8) */
static const unsigned int BITCOIN_MEMPOOL_HEIGHT = 0x7FFFFFFF;

/*
 * CTxMemPool stores these:
 */
class Bitcoin_CTxMemPoolEntry
{
private:
    Bitcoin_CTransaction tx;
    int64_t nFee; // Cached to avoid expensive parent-transaction lookups
    size_t nTxSize; // ... and avoid recomputing tx size
    int64_t nTime; // Local time when entering the mempool
    double dPriority; // Priority when entering the mempool
    unsigned int nHeight; // Chain height when entering the mempool

public:
    Bitcoin_CTxMemPoolEntry(const Bitcoin_CTransaction& _tx, int64_t _nFee,
                    int64_t _nTime, double _dPriority, unsigned int _nHeight);
    Bitcoin_CTxMemPoolEntry();
    Bitcoin_CTxMemPoolEntry(const Bitcoin_CTxMemPoolEntry& other);

    const Bitcoin_CTransaction& GetTx() const { return this->tx; }
    double GetPriority(unsigned int currentHeight) const;
    int64_t GetFee() const { return nFee; }
    size_t GetTxSize() const { return nTxSize; }
    int64_t GetTime() const { return nTime; }
    unsigned int GetHeight() const { return nHeight; }
};

/*
 * CTxMemPool stores valid-according-to-the-current-best-chain
 * transactions that may be included in the next block.
 *
 * Transactions are added when they are seen on the network
 * (or created by the local node), but not all transactions seen
 * are added to the pool: if a new transaction double-spends
 * an input of a transaction in the pool, it is dropped,
 * as are non-standard transactions.
 */
class Bitcoin_CTxMemPool
{
private:
    bool fSanityCheck; // Normally false, true if -checkmempool or -regtest
    unsigned int nTransactionsUpdated;

public:
    mutable CCriticalSection cs;
    std::map<uint256, Bitcoin_CTxMemPoolEntry> mapTx;
    std::map<COutPoint, Bitcoin_CInPoint> mapNextTx;

    Bitcoin_CTxMemPool();

    /*
     * If sanity-checking is turned on, check makes sure the pool is
     * consistent (does not contain two transactions that spend the same inputs,
     * all inputs are in the mapNextTx array). If sanity-checking is turned off,
     * check does nothing.
     */
    void check(Bitcoin_CCoinsViewCache *pcoins) const;
    void setSanityCheck(bool _fSanityCheck) { fSanityCheck = _fSanityCheck; }

    bool addUnchecked(const uint256& hash, const Bitcoin_CTxMemPoolEntry &entry);
    void remove(const Bitcoin_CTransaction &tx, std::list<Bitcoin_CTransaction>& removed, bool fRecursive = false);
    void removeConflicts(const Bitcoin_CTransaction &tx, std::list<Bitcoin_CTransaction>& removed);
    void clear();
    void queryHashes(std::vector<uint256>& vtxid);
    void pruneSpent(const uint256& hash, Bitcoin_CCoins &coins);
    unsigned int GetTransactionsUpdated() const;
    void AddTransactionsUpdated(unsigned int n);

    unsigned long size()
    {
        LOCK(cs);
        return mapTx.size();
    }

    bool exists(uint256 hash)
    {
        LOCK(cs);
        return (mapTx.count(hash) != 0);
    }

    bool lookup(uint256 hash, Bitcoin_CTransaction& result) const;
};

/** Bitcoin_CCoinsView that brings transactions from a memorypool into view.
    It does not check for spendings by memory pool transactions. */
class Bitcoin_CCoinsViewMemPool : public Bitcoin_CCoinsViewBacked
{
protected:
    Bitcoin_CTxMemPool &mempool;

public:
    Bitcoin_CCoinsViewMemPool(Bitcoin_CCoinsView &baseIn, Bitcoin_CTxMemPool &mempoolIn);
    bool GetCoins(const uint256 &txid, Bitcoin_CCoins &coins);
    bool HaveCoins(const uint256 &txid);
};

#endif /* BITCOIN_TXMEMPOOL_H */
