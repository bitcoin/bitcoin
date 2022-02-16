// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/coinstats.h>

#include <coins.h>
#include <index/coinstatsindex.h>
#include <optional>
#include <validation.h>

namespace node {
std::optional<CCoinsStats> GetUTXOStats(CCoinsView* view, BlockManager& blockman, CoinStatsHashType hash_type, const std::function<void()>& interruption_point, const CBlockIndex* pindex, bool index_requested)
{
    // Use CoinStatsIndex if it is requested and available and a hash_type of Muhash or None was requested
    if ((hash_type == CoinStatsHashType::MUHASH || hash_type == CoinStatsHashType::NONE) && g_coin_stats_index && index_requested) {
        if (pindex) {
            return g_coin_stats_index->LookUpStats(pindex);
        } else {
            CBlockIndex* block_index = WITH_LOCK(::cs_main, return blockman.LookupBlockIndex(view->GetBestBlock()));
            return g_coin_stats_index->LookUpStats(block_index);
        }
    }

    // If the coinstats index isn't requested or is otherwise not usable, the
    // pindex should either be null or equal to the view's best block. This is
    // because without the coinstats index we can only get coinstats about the
    // best block.
    assert(!pindex || pindex->GetBlockHash() == view->GetBestBlock());

    return ComputeUTXOStats(hash_type, view, blockman, interruption_point);
}
} // namespace node
