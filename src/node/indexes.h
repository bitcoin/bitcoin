// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_INDEXES_H
#define BITCOIN_NODE_INDEXES_H

#include <cstdint>

class BlockFilterIndex;
enum class BlockFilterType : uint8_t;

namespace node {
struct NodeContext;

/**
 * Get a block filter index by type. Returns nullptr if index has not been initialized or was
 * already destroyed.
 */
BlockFilterIndex* GetBlockFilterIndex(const NodeContext& node, BlockFilterType filter_type);
} // namespace node

#endif // BITCOIN_NODE_INDEXES_H
