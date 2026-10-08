// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/coin.h>

#include <node/context.h>
#include <txmempool.h>
#include <validation.h>

namespace node {
void FindCoins(const NodeContext& node, std::map<COutPoint, Coin>& coins)
{
    assert(node.mempool);
    assert(node.chainman);
    LOCK2(cs_main, node.mempool->cs);
    CCoinsViewCache& chain_view = node.chainman->ActiveChainstate().CoinsTip();
    CCoinsViewMemPool mempool_view(&chain_view, *node.mempool);
    for (auto& [outpoint, coin] : coins) {
        if (auto c{mempool_view.GetCoin(outpoint)}) {
            coin = std::move(*c);
        } else {
            coin.Clear(); // Either the coin is not in the CCoinsViewCache or is spent
        }
    }
}

bool FindCoinsByScript(const NodeContext& node, const std::set<CScript>& output_scripts, std::map<COutPoint, Coin>& coins, uint256& best_block)
{
    assert(node.chainman);
    std::unique_ptr<CCoinsViewCursor> cursor;
    {
        LOCK(cs_main);
        Chainstate& active_chainstate = node.chainman->ActiveChainstate();
        // Ensure on-disk coins DB is up-to-date so the cursor will see recent coins.
        active_chainstate.ForceFlushStateToDisk(/*wipe_cache=*/false);
        cursor = active_chainstate.CoinsDB().Cursor();
    }

    if (!cursor) {
        return false;
    }
    best_block = cursor->GetBestBlock();

    while (cursor->Valid()) {
        COutPoint key;
        Coin coin;
        if (!cursor->GetKey(key) || !cursor->GetValue(coin)) {
            return false;
        }
        if (output_scripts.contains(coin.out.scriptPubKey)) {
            coins.emplace(std::move(key), std::move(coin));
        }
        cursor->Next();
    }
    return true;
}
} // namespace node
