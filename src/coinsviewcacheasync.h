// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINSVIEWCACHEASYNC_H
#define BITCOIN_COINSVIEWCACHEASYNC_H

#include <coins.h>
#include <primitives/transaction.h>

/**
 * CCoinsViewCache subclass that does not call GetCoin on base via FetchCoin, so base will not be mutated before Flush.
 * Only used in ConnectBlock to pass as an ephemeral view that can be reset if the block is invalid.
 * It provides the same interface as CCoinsViewCache, overriding the FetchCoin private method.
 */
class CoinsViewCacheAsync : public CCoinsViewCache
{
private:
    CCoinsMap::iterator FetchCoin(const COutPoint& outpoint) const override
    {
        auto [ret, inserted]{cacheCoins.try_emplace(outpoint)};
        if (!inserted) return ret;

        // TODO: Remove spent checks once we no longer return spent coins in coinscache_sim CoinsViewBottom.
        if (auto coin{FetchCoinWithoutMutating(outpoint)}; coin && !coin->IsSpent()) {
            ret->second.coin = std::move(*coin);
        } else {
            cacheCoins.erase(ret);
            return cacheCoins.end();
        }

        cachedCoinsUsage += ret->second.coin.DynamicMemoryUsage();
        return ret;
    }

public:
    using CCoinsViewCache::CCoinsViewCache;
};

#endif // BITCOIN_COINSVIEWCACHEASYNC_H
