// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINSVIEWCACHEASYNC_H
#define BITCOIN_COINSVIEWCACHEASYNC_H

#include <coins.h>
#include <primitives/transaction.h>

/**
 * CCoinsViewCache subclass that does not call GetCoin on base via GetCoinFromBase, so base will not be mutated before
 * Flush.
 * Only used in ConnectBlock to pass as an ephemeral view that can be reset if the block is invalid.
 * It provides the same interface as CCoinsViewCache, overriding the FetchCoin private method.
 */
class CoinsViewCacheAsync : public CCoinsViewCache
{
private:
    std::optional<Coin> GetCoinFromBase(const COutPoint& outpoint) const override
    {
        return base->PeekCoin(outpoint);
    }

public:
    using CCoinsViewCache::CCoinsViewCache;
};

#endif // BITCOIN_COINSVIEWCACHEASYNC_H
