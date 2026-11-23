// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/indexes.h>

#include <index/blockfilterindex.h>
#include <index/coinstatsindex.h>
#include <index/txindex.h>
#include <index/txospenderindex.h>
#include <node/context.h>

namespace node {

void IndexDeleter::operator()(BlockFilterIndex* index) const noexcept { delete index; }
void IndexDeleter::operator()(CoinStatsIndex* index) const noexcept { delete index; }
void IndexDeleter::operator()(TxIndex* index) const noexcept { delete index; }
void IndexDeleter::operator()(TxoSpenderIndex* index) const noexcept { delete index; }

BlockFilterIndex* GetBlockFilterIndex(const NodeContext& node, BlockFilterType filter_type)
{
    for (const auto& index : node.block_filter_indexes) {
        if (index->GetFilterType() == filter_type) return index.get();
    }
    return nullptr;
}

} // namespace node
