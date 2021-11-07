// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/coinstats.h>

#include <coins.h>
#include <crypto/muhash.h>
#include <hash.h>
#include <index/coinstatsindex.h>
#include <serialize.h>
#include <uint256.h>
#include <util/system.h>
#include <validation.h>

#include <map>

// Database-independent metric indicating the UTXO set size
uint64_t GetBogoSize(const CScript& script_pub_key)
{
    return 32 /* txid */ +
           4 /* vout index */ +
           4 /* height + coinbase */ +
           8 /* amount */ +
           2 /* scriptPubKey len */ +
           script_pub_key.size() /* scriptPubKey */;
}

CDataStream TxOutSer(const COutPoint& outpoint, const Coin& coin) {
    CDataStream ss(SER_DISK, PROTOCOL_VERSION);
    ss << outpoint;
    ss << static_cast<uint32_t>(coin.nHeight * 2 + coin.fCoinBase);
    ss << coin.out;
    return ss;
}

//! Warning: be very careful when changing this! assumeutxo and UTXO snapshot
//! validation commitments are reliant on the hash constructed by this
//! function.
//!
//! If the construction of this hash is changed, it will invalidate
//! existing UTXO snapshots. This will not result in any kind of consensus
//! failure, but it will force clients that were expecting to make use of
//! assumeutxo to do traditional IBD instead.
//!
//! It is also possible, though very unlikely, that a change in this
//! construction could cause a previously invalid (and potentially malicious)
//! UTXO snapshot to be considered valid.
static void ApplyHash(CHashWriter& ss, const uint256& hash, const std::map<uint32_t, Coin>& outputs)
{
    for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        if (it == outputs.begin()) {
            ss << hash;
            ss << VARINT(it->second.nHeight * 2 + it->second.fCoinBase ? 1u : 0u);
        }

        ss << VARINT(it->first + 1);
        ss << it->second.out.scriptPubKey;
        ss << VARINT_MODE(it->second.out.nValue, VarIntMode::NONNEGATIVE_SIGNED);

        if (it == std::prev(outputs.end())) {
            ss << VARINT(0u);
        }
    }
}

static void ApplyHash(std::nullptr_t, const uint256& hash, const std::map<uint32_t, Coin>& outputs) {}

static void ApplyHash(MuHash3072& muhash, const uint256& hash, const std::map<uint32_t, Coin>& outputs)
{
    for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        COutPoint outpoint = COutPoint(hash, it->first);
        Coin coin = it->second;
        muhash.Insert(MakeUCharSpan(TxOutSer(outpoint, coin)));
    }
}

static void ApplyStats(CCoinsStats& stats, const uint256& hash, const std::map<uint32_t, Coin>& outputs)
{
    assert(!outputs.empty());
    stats.nTransactions++;
    for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        stats.nTransactionOutputs++;
        stats.nTotalAmount += it->second.out.nValue;
        stats.nBogoSize += GetBogoSize(it->second.out.scriptPubKey);
    }
}

//! Calculate statistics about the unspent transaction output set
template <typename T>
static bool GetUTXOStats(CCoinsView* view, BlockManager& blockman, CCoinsStats& stats, T hash_obj, const std::function<void()>& interruption_point, const CBlockIndex* pindex)
{
    std::unique_ptr<CCoinsViewCursor> pcursor(view->Cursor());
    assert(pcursor);

    if (!pindex) {
        {
            LOCK(cs_main);
            pindex = blockman.LookupBlockIndex(view->GetBestBlock());
        }
    }
    stats.nHeight = Assert(pindex)->nHeight;
    stats.hashBlock = pindex->GetBlockHash();

    // Use CoinStatsIndex if it is requested and available and a hash_type of Muhash or None was requested
    if ((stats.m_hash_type == CoinStatsHashType::MUHASH || stats.m_hash_type == CoinStatsHashType::NONE) && g_coin_stats_index && stats.index_requested) {
        stats.index_used = true;
        return g_coin_stats_index->LookUpStats(pindex, stats);
    }

    PrepareHash(hash_obj, stats);

    uint256 prevkey;
    std::map<uint32_t, Coin> outputs;
    while (pcursor->Valid()) {
        interruption_point();
        COutPoint key;
        Coin coin;
        if (pcursor->GetKey(key) && pcursor->GetValue(coin)) {
            if (!outputs.empty() && key.hash != prevkey) {
                ApplyStats(stats, prevkey, outputs);
                ApplyHash(hash_obj, prevkey, outputs);
                outputs.clear();
            }
            prevkey = key.hash;
            outputs[key.n] = std::move(coin);
            stats.coins_count++;
        } else {
            return error("%s: unable to read value", __func__);
        }
        pcursor->Next();
    }
    if (!outputs.empty()) {
        ApplyStats(stats, prevkey, outputs);
        ApplyHash(hash_obj, prevkey, outputs);
    }

    FinalizeHash(hash_obj, stats);

    stats.nDiskSize = view->EstimateSize();
    return true;
}

bool GetUTXOStats(CCoinsView* view, BlockManager& blockman, CCoinsStats& stats, const std::function<void()>& interruption_point, const CBlockIndex* pindex)
{
    switch (stats.m_hash_type) {
    case(CoinStatsHashType::HASH_SERIALIZED): {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        return GetUTXOStats(view, blockman, stats, ss, interruption_point, pindex);
    }
    case(CoinStatsHashType::MUHASH): {
        MuHash3072 muhash;
        return GetUTXOStats(view, blockman, stats, muhash, interruption_point, pindex);
    }
    case(CoinStatsHashType::NONE): {
        return GetUTXOStats(view, blockman, stats, nullptr, interruption_point, pindex);
    }
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

// The legacy hash serializes the hashBlock
static void PrepareHash(CHashWriter& ss, const CCoinsStats& stats)
{
    ss << stats.hashBlock;
}
// MuHash does not need the prepare step
static void PrepareHash(MuHash3072& muhash, CCoinsStats& stats) {}
static void PrepareHash(std::nullptr_t, CCoinsStats& stats) {}

static void FinalizeHash(CHashWriter& ss, CCoinsStats& stats)
{
    stats.hashSerialized = ss.GetHash();
}
static void FinalizeHash(MuHash3072& muhash, CCoinsStats& stats)
{
    uint256 out;
    muhash.Finalize(out);
    stats.hashSerialized = out;
}
static void FinalizeHash(std::nullptr_t, CCoinsStats& stats) {}
