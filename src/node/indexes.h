// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_INDEXES_H
#define BITCOIN_NODE_INDEXES_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

class BlockFilterIndex;
enum class BlockFilterType : uint8_t;

namespace interfaces {
class Chain;
}

namespace node {
struct NodeContext;

/**
 * Get a block filter index by type. Returns nullptr if index has not been initialized or was
 * already destroyed.
 */
BlockFilterIndex* GetBlockFilterIndex(NodeContext& node, BlockFilterType filter_type);
const BlockFilterIndex* GetBlockFilterIndex(const NodeContext& node, BlockFilterType filter_type);

/** Iterate over all running block filter indexes, invoking fn on each. */
void ForEachBlockFilterIndex(NodeContext& node, std::function<void(BlockFilterIndex&)> fn);

/**
 * Initialize a block filter index for the given type if one does not already exist. Returns true if
 * a new index is created and false if one has already been initialized.
 */
bool InitBlockFilterIndex(NodeContext& node, std::function<std::unique_ptr<interfaces::Chain>()> make_chain, BlockFilterType filter_type,
                          size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

/**
 * Destroy the block filter index with the given type. Returns false if no such index exists. This
 * just releases the allocated memory and closes the database connection, it does not delete the
 * index data.
 */
bool DestroyBlockFilterIndex(NodeContext& node, BlockFilterType filter_type);

/** Destroy all open block filter indexes. */
void DestroyAllBlockFilterIndexes(NodeContext& node);
} // namespace node

#endif // BITCOIN_NODE_INDEXES_H
