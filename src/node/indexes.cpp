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

void BlockFilterIndexDeleter::operator()(BlockFilterIndex* index) const noexcept { delete index; }
void CoinStatsIndexDeleter::operator()(CoinStatsIndex* index) const noexcept { delete index; }
void TxIndexDeleter::operator()(TxIndex* index) const noexcept { delete index; }
void TxoSpenderIndexDeleter::operator()(TxoSpenderIndex* index) const noexcept { delete index; }

BlockFilterIndex* GetBlockFilterIndex(NodeContext& node, BlockFilterType filter_type)
{
    for (const auto& index : node.block_filter_indexes) {
        if (index->GetFilterType() == filter_type) return index.get();
    }
    return nullptr;
}

const BlockFilterIndex* GetBlockFilterIndex(const NodeContext& node, BlockFilterType filter_type)
{
    for (const auto& index : node.block_filter_indexes) {
        if (index->GetFilterType() == filter_type) return index.get();
    }
    return nullptr;
}

void ForEachBlockFilterIndex(NodeContext& node, std::function<void(BlockFilterIndex&)> fn)
{
    for (auto& index : node.block_filter_indexes) fn(*index);
}

bool InitBlockFilterIndex(NodeContext& node, std::function<std::unique_ptr<interfaces::Chain>()> make_chain, BlockFilterType filter_type,
                          size_t n_cache_size, bool f_memory, bool f_wipe)
{
    if (GetBlockFilterIndex(node, filter_type)) return false;
    node.block_filter_indexes.emplace_back(new BlockFilterIndex(make_chain(), filter_type, n_cache_size, f_memory, f_wipe));
    return true;
}

bool DestroyBlockFilterIndex(NodeContext& node, BlockFilterType filter_type)
{
    for (auto it{node.block_filter_indexes.begin()}; it != node.block_filter_indexes.end(); ++it) {
        if ((*it)->GetFilterType() == filter_type) {
            node.block_filter_indexes.erase(it);
            return true;
        }
    }
    return false;
}

void DestroyAllBlockFilterIndexes(NodeContext& node)
{
    node.block_filter_indexes.clear();
}

} // namespace node
