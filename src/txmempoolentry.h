// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXMEMPOOLENTRY_H
#define BITCOIN_TXMEMPOOLENTRY_H

#include "amount.h"
#include "primitives/transaction.h"

/** Fake height value used in CCoins to signify they are only in the memory pool (since 0.8) */
static const unsigned int MEMPOOL_HEIGHT = 0x7FFFFFFF;

/**
 * CTxMemPool stores these:
 */
class CTxMemPoolEntry
{
private:
    CTransaction tx;
    CAmount nFee; //! Cached to avoid expensive parent-transaction lookups
    size_t nTxSize; //! ... and avoid recomputing tx size
    size_t nModSize; //! ... and modified size for priority
    int64_t nTime; //! Local time when entering the mempool
    double dPriority; //! Priority when entering the mempool
    unsigned int nHeight; //! Chain height when entering the mempool

public:
    CTxMemPoolEntry(const CTransaction& _tx, const CAmount& _nFee,
                    int64_t _nTime, double _dPriority, unsigned int _nHeight);
    CTxMemPoolEntry();
    CTxMemPoolEntry(const CTxMemPoolEntry& other);

    const CTransaction& GetTx() const { return this->tx; }
    double GetPriority(unsigned int currentHeight) const;
    CAmount GetFee() const { return nFee; }
    size_t GetTxSize() const { return nTxSize; }
    int64_t GetTime() const { return nTime; }
    unsigned int GetHeight() const { return nHeight; }
};

#endif // BITCOIN_TXMEMPOOLENTRY_H
