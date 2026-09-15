// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_COIN_H
#define BITCOIN_NODE_COIN_H

#include <map>
#include <set>

class COutPoint;
class Coin;
class CScript;
class uint256;

namespace node {
struct NodeContext;

/**
 * Look up unspent output information. Returns coins in the mempool and in the
 * current chain UTXO set. Iterates through all the keys in the map and
 * populates the values.
 *
 * @param[in] node The node context to use for lookup
 * @param[in,out] coins map to fill
 */
void FindCoins(const node::NodeContext& node, std::map<COutPoint, Coin>& coins);

/**
 * Scan a chainstate snapshot for coins belonging to output_scripts.
 *
 * @param[in] node The node context to use for lookup
 * @param[in] output_scripts The scripts to scan for coins
 * @param[in,out] coins Map to fill
 * @param[out] best_block The block hash of the scanned snapshot
 * @return Whether the scan completed successfully
 */
bool FindCoinsByScript(const NodeContext& node, const std::set<CScript>& output_scripts, std::map<COutPoint, Coin>& coins, uint256& best_block);
} // namespace node

#endif // BITCOIN_NODE_COIN_H
