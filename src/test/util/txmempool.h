// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_TXMEMPOOL_H
#define BITCOIN_TEST_UTIL_TXMEMPOOL_H

#include <txmempool.h>

struct TxMemPoolClearable : public CTxMemPool {
    /** Clear added transactions */
    void clearTxs()
    {
        LOCK(cs);
        mapTx.clear();
        mapNextTx.clear();
        totalTxSize = 0;
        cachedInnerUsage = 0;
        ++nTransactionsUpdated;
    }
};

#endif // BITCOIN_TEST_UTIL_TXMEMPOOL_H
