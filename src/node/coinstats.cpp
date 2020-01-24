// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/coinstats.h>

#include <crypto/muhash.h>
#include <hash.h>
#include <index/coinstatsindex.h>
#include <serialize.h>
#include <uint256.h>
#include <util/system.h>
#include <validation.h>

#include <map>

static void ApplyStats(CCoinsStats &stats, MuHash3072& muhash, const uint256& hash, const std::map<uint32_t, Coin>& outputs)
{
    assert(!outputs.empty());
    for (const auto& output : outputs) {
        COutPoint outpoint = COutPoint(hash, output.first);
        Coin coin = output.second;

        muhash *= MuHash3072(GetTruncatedSHA512Hash(outpoint, coin).begin());

        stats.nTransactionOutputs++;
        stats.nTotalAmount += output.second.out.nValue;
        stats.nBogoSize += 32 /* txid */ + 4 /* vout index */ + 4 /* height + coinbase */ + 8 /* amount */ +
                           2 /* scriptPubKey len */ + output.second.out.scriptPubKey.size() /* scriptPubKey */;
    }
}

uint256 GetTruncatedSHA512Hash(const COutPoint& outpoint, const Coin& coin) {
    TruncatedSHA512Writer ss(SER_DISK, 0);
    ss << outpoint;
    ss << (uint32_t)(coin.nHeight * 2 + coin.fCoinBase);
    ss << coin.out;
    return ss.GetHash();
}

//! Calculate statistics about the unspent transaction output set
bool GetUTXOStats(CCoinsView *view, CCoinsStats &stats)
{
    stats = CCoinsStats();
    std::unique_ptr<CCoinsViewCursor> pcursor(view->Cursor());
    assert(pcursor);
    stats.hashBlock = pcursor->GetBestBlock();

    const CBlockIndex* block_index;
    {
        LOCK(cs_main);
        block_index = LookupBlockIndex(pcursor->GetBestBlock());
    }
    stats.nHeight = block_index->nHeight;

    // Use CoinStatsIndex if it is available
    if (g_coin_stats_index) {
        if (g_coin_stats_index->LookupStats(block_index, stats)) {
            return true;
        } else {
            return false;
        }
    }

    MuHash3072 muhash;
    uint256 prevkey;
    std::map<uint32_t, Coin> outputs;
    while (pcursor->Valid()) {
        COutPoint key;
        Coin coin;
        if (pcursor->GetKey(key) && pcursor->GetValue(coin)) {
            if (!outputs.empty() && key.hash != prevkey) {
                ApplyStats(stats, muhash, prevkey, outputs);
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
        ApplyStats(stats, muhash, prevkey, outputs);
    }

    unsigned char out[384];
    muhash.Finalize(out);
    stats.hashSerialized = (TruncatedSHA512Writer(SER_DISK, 0) << out).GetHash();

    stats.nDiskSize = view->EstimateSize();
    return true;
}
