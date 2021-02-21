// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_COINSTATS_H
#define BITCOIN_NODE_COINSTATS_H

#include <amount.h>
#include <uint256.h>

#include <cstdint>
#include <functional>

class BlockManager;
class CCoinsView;

enum class CoinStatsHashType {
    HASH_SERIALIZED,
    MUHASH,
    NONE,
};

struct CCoinsStats
{
    CoinStatsHashType m_hash_type;
    int nHeight{0};
    uint256 hashBlock{};
    uint64_t nTransactions{0};
    uint64_t nTransactionOutputs{0};
    uint64_t nBogoSize{0};
    uint256 hashSerialized{};
    uint64_t nDiskSize{0};
    CAmount nTotalAmount{0};

    //! The number of coins contained.
    uint64_t coins_count{0};

    CCoinsStats(CoinStatsHashType hash_type) : m_hash_type(hash_type) {}
};

//! Calculate statistics about the unspent transaction output set
bool GetUTXOStats(CCoinsView* view, BlockManager& blockman, CCoinsStats& stats, const std::function<void()>& interruption_point = {});

#endif // BITCOIN_NODE_COINSTATS_H
