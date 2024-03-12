// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/coinstats.h>

#include <chain.h>
#include <coins.h>
#include <crypto/muhash.h>
#include <hash.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <sync.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/check.h>
#include <util/overflow.h>
#include <validation.h>

#include <cassert>
#include <iosfwd>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace kernel {

CCoinsStats::CCoinsStats(int block_height, const uint256& block_hash)
    : nHeight(block_height),
      hashBlock(block_hash) {}

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

template <typename T>
static void TxOutSer(T& ss, const COutPoint& outpoint, const Coin& coin)
{
    ss << outpoint;
    ss << static_cast<uint32_t>((coin.nHeight << 1) + coin.fCoinBase);
    ss << coin.out;
}

static void ApplyCoinHash(HashWriter& ss, const COutPoint& outpoint, const Coin& coin)
{
    TxOutSer(ss, outpoint, coin);
}

void ApplyCoinHash(MuHash3072& muhash, const COutPoint& outpoint, const Coin& coin)
{
    DataStream ss{};
    TxOutSer(ss, outpoint, coin);
    muhash.Insert(MakeUCharSpan(ss));
}

void RemoveCoinHash(MuHash3072& muhash, const COutPoint& outpoint, const Coin& coin)
{
    DataStream ss{};
    TxOutSer(ss, outpoint, coin);
    muhash.Remove(MakeUCharSpan(ss));
}

static void ApplyCoinHash(std::nullptr_t, const COutPoint& outpoint, const Coin& coin) {}

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
template <typename T>
static void ApplyHash(T& hash_obj, const Txid& hash, const std::map<uint32_t, Coin>& outputs)
{
    for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        COutPoint outpoint = COutPoint(hash, it->first);
        Coin coin = it->second;
        ApplyCoinHash(hash_obj, outpoint, coin);
    }
}

static void ApplyStats(CCoinsStats& stats, const uint256& hash, const std::map<uint32_t, Coin>& outputs)
{
    assert(!outputs.empty());
    stats.nTransactions++;
    for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        stats.nTransactionOutputs++;
        if (stats.total_amount.has_value()) {
            stats.total_amount = CheckedAdd(*stats.total_amount, it->second.out.nValue);
        }
        stats.nBogoSize += GetBogoSize(it->second.out.scriptPubKey);
    }
}

//! Calculate statistics about the unspent transaction output set
template <typename T>
static bool ComputeUTXOStats(CCoinsView* view, CCoinsStats& stats, T hash_obj, const std::function<void()>& interruption_point)
{
    std::unique_ptr<CCoinsViewCursor> pcursor(view->Cursor());
    assert(pcursor);

    Txid prevkey;
    std::map<uint32_t, Coin> outputs;
    while (pcursor->Valid()) {
        if (interruption_point) interruption_point();
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
            LogError("%s: unable to read value\n", __func__);
            return false;
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

std::optional<CCoinsStats> ComputeUTXOStats(CoinStatsHashType hash_type, CCoinsView* view, node::BlockManager& blockman, const std::function<void()>& interruption_point)
{
    CBlockIndex* pindex = WITH_LOCK(::cs_main, return blockman.LookupBlockIndex(view->GetBestBlock()));
    CCoinsStats stats{Assert(pindex)->nHeight, pindex->GetBlockHash()};

    bool success = [&]() -> bool {
        switch (hash_type) {
        case(CoinStatsHashType::HASH_SERIALIZED): {
            HashWriter ss{};
            return ComputeUTXOStats(view, stats, ss, interruption_point);
        }
        case(CoinStatsHashType::MUHASH): {
            MuHash3072 muhash;
            return ComputeUTXOStats(view, stats, muhash, interruption_point);
        }
        case(CoinStatsHashType::NONE): {
            return ComputeUTXOStats(view, stats, nullptr, interruption_point);
        }
        } // no default case, so the compiler can warn about missing cases
        assert(false);
    }();

    if (!success) {
        return std::nullopt;
    }
    return stats;
}

static void FinalizeHash(HashWriter& ss, CCoinsStats& stats)
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

} // namespace kernel
