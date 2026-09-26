// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_MEMPOOL_H
#define BITCOIN_RPC_MEMPOOL_H

#include <core_io.h>

class CTxMemPool;
class UniValue;

/** Mempool information to JSON */
UniValue MempoolInfoToJSON(const CTxMemPool& pool, FeeRateUnit feerate_units = FeeRateUnit::BTC_KVB);

/** Mempool to JSON */
UniValue MempoolToJSON(const CTxMemPool& pool, bool verbose = false, bool include_mempool_sequence = false);

#endif // BITCOIN_RPC_MEMPOOL_H
