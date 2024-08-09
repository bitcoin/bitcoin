// Copyright (c) 2019-2021 The Bitcoin Core developers
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
    for (auto& coin : coins) {
        if (!mempool_view.GetCoin(coin.first, coin.second)) {
            // Either the coin is not in the CCoinsViewCache or is spent. Clear it.
            coin.second.Clear();
        }
    }
}

void GetCoins(const NodeContext& node, std::set<CScript>& output_scripts, std::map<COutPoint, Coin>& coins)
{
    assert(node.chainman);
    LOCK(cs_main);
    std::unique_ptr<CCoinsViewCursor> cursor = node.chainman->ActiveChainstate().CoinsDB().Cursor();
    while (cursor->Valid()) {
        COutPoint key;
        Coin coin;
        if (cursor->GetKey(key) && cursor->GetValue(coin)) {
            if (output_scripts.count(coin.out.scriptPubKey)) {
                coins.emplace(key, coin);
            }
        }
        cursor->Next();
    }
}
} // namespace node
