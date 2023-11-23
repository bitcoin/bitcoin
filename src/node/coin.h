// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_COIN_H
#define BITCOIN_NODE_COIN_H

#include <map>
#include <set>

class COutPoint;
class Coin;
class CScript;

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
 * Scans the UTXO set for coins belonging to output_scripts.
 *
 * @param[in] node The node context to use for lookup
 * @param[in] output_scripts The scripts to scan for coins
 * @param[in,out] coins map to fill
 */
void GetCoins(const NodeContext& node, std::set<CScript>& output_scripts, std::map<COutPoint, Coin>& coins);
} // namespace node

#endif // BITCOIN_NODE_COIN_H
