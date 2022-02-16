// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_COINSTATS_H
#define BITCOIN_NODE_COINSTATS_H

#include <kernel/coinstats.h>

#include <chain.h>
#include <coins.h>
#include <consensus/amount.h>
#include <streams.h>
#include <uint256.h>

#include <cstdint>
#include <functional>

class CCoinsView;
namespace node {
class BlockManager;
} // namespace node

namespace node {
/**
 * Calculate statistics about the unspent transaction output set
 *
 * @param[in] index_requested Signals if the coinstatsindex should be used (when available).
 */
std::optional<kernel::CCoinsStats> GetUTXOStats(CCoinsView* view, node::BlockManager& blockman,
                                        kernel::CoinStatsHashType hash_type,
                                        const std::function<void()>& interruption_point = {},
                                        const CBlockIndex* pindex = nullptr,
                                        bool index_requested = true);
} // namespace node

#endif // BITCOIN_NODE_COINSTATS_H
