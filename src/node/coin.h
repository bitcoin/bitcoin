// Copyright (c) 2019 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WIDECOIN_NODE_COIN_H
#define WIDECOIN_NODE_COIN_H

#include <map>

class COutPoint;
class Coin;
struct NodeContext;

/**
 * Look up unspent output information. Returns coins in the mempool and in the
 * current chain UTXO set. Iterates through all the keys in the map and
 * populates the values.
 *
 * @param[in] node The node context to use for lookup
 * @param[in,out] coins map to fill
 */
void FindCoins(const NodeContext& node, std::map<COutPoint, Coin>& coins);

#endif // WIDECOIN_NODE_COIN_H
