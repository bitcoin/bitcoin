// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_MEMPOOL_H
#define BITCOIN_RPC_MEMPOOL_H

#include <consensus/amount.h>

#include <optional>
#include <vector>

class CTxMemPool;
class UniValue;

typedef std::vector<CAmount> MempoolHistogramFeeRates;

/* TODO: define log scale formular for dynamically creating the
 * feelimits but with the property of not constantly changing
 * (and thus screw up client implementations) */
static const MempoolHistogramFeeRates MempoolInfoToJSON_const_histogram_floors{
    1, 2, 3, 4, 5, 6, 7, 8, 10,
    12, 14, 17, 20, 25, 30, 40, 50, 60, 70, 80, 100,
    120, 140, 170, 200, 250, 300, 400, 500, 600, 700, 800, 1000,
    1200, 1400, 1700, 2000, 2500, 3000, 4000, 5000, 6000, 7000, 8000, 10000};

/** Mempool information to JSON */
UniValue MempoolInfoToJSON(const CTxMemPool& pool, const std::optional<MempoolHistogramFeeRates>& histogram_floors);

/** Mempool to JSON */
UniValue MempoolToJSON(const CTxMemPool& pool, bool verbose = false, bool include_mempool_sequence = false);

/** Mempool Txs to JSON */
UniValue MempoolTxsToJSON(const CTxMemPool& pool, bool verbose = false, uint64_t sequence_start = 0);

#endif // BITCOIN_RPC_MEMPOOL_H
