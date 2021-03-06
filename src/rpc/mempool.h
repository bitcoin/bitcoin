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

/** Mempool information to JSON */
UniValue MempoolInfoToJSON(const CTxMemPool& pool, const std::optional<MempoolHistogramFeeRates>& histogram_floors);

/** Mempool to JSON */
UniValue MempoolToJSON(const CTxMemPool& pool, bool verbose = false, bool include_mempool_sequence = false);

#endif // BITCOIN_RPC_MEMPOOL_H
