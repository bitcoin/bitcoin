// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/coin.h>

#include <node/context.h>
#include <txmempool.h>
#include <validation.h>

namespace node {
/**
 * Find and update Coin objects in the given map, using the current chain and mempool state.
 *
 * For each COutPoint in the map, try to get the Coin from the combined view of chain and mempool.
 * If the Coin exists, update it in the map. If not, clear it (means spent or not found).
 *
 * @param node Node context, must have mempool and chainman set.
 * @param coins Map of COutPoint to Coin to update.
 */
void FindCoins(const NodeContext& node, std::map<COutPoint, Coin>& coins)
{
    // Check that node context has mempool and chainman
    assert(node.mempool);
    assert(node.chainman);

    // Lock global resources for safe access to chain and mempool
    LOCK2(cs_main, node.mempool->cs);

    // Get current chain view (UTXO set)
    CCoinsViewCache& chain_view = node.chainman->ActiveChainstate().CoinsTip();
    // Create combined view with mempool
    CCoinsViewMemPool mempool_view(&chain_view, *node.mempool);

    // Loop through each COutPoint in the map and update Coin
    for (auto& [outpoint, coin] : coins) {
        // Try to get Coin from combined view
        if (auto c{mempool_view.GetCoin(outpoint)}) {
            coin = std::move(*c); // Coin found, update value
        } else {
            coin.Clear(); // Coin not found or spent, clear value
        }
    }
}
} // namespace node
