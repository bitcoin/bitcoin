// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINSVIEWCACHENONMUTATING_H
#define BITCOIN_COINSVIEWCACHENONMUTATING_H

#include <coins.h>
#include <primitives/transaction.h>

/**
 * CCoinsViewCache subclass that uses PeekCoin instead of GetCoin when fetching from base, avoiding FetchCoin mutation
 * on parent cache layers.
 * Only used in ConnectBlock to pass as an ephemeral view that can be reset if the block is invalid.
 * It provides the same interface as CCoinsViewCache, overriding the GetCoinFromBase protected method.
 */
class CoinsViewCacheNonMutating : public CCoinsViewCache
{
private:
    std::optional<Coin> GetCoinFromBase(const COutPoint& outpoint) const noexcept override
    {
        return base->PeekCoin(outpoint);
    }

public:
    using CCoinsViewCache::CCoinsViewCache;
};

#endif // BITCOIN_COINSVIEWCACHENONMUTATING_H
