// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINSVIEWOVERLAY_H
#define BITCOIN_COINSVIEWOVERLAY_H

#include <coins.h>
#include <primitives/transaction.h>

/**
 * CCoinsViewCache overlay that avoids populating/mutating parent cache layers on cache misses.
 *
 * This is achieved by fetching coins from the base view using PeekCoin() instead of GetCoin(),
 * so intermediate CCoinsViewCache layers are not filled.
 *
 * Used during ConnectBlock() as an ephemeral, resettable top-level view that is flushed only
 * on success, so invalid blocks don't pollute the underlying cache.
 */
class CoinsViewOverlay : public CCoinsViewCache
{
private:
    std::optional<Coin> GetCoinFromBase(const COutPoint& outpoint) const override
    {
        return base->PeekCoin(outpoint);
    }

public:
    using CCoinsViewCache::CCoinsViewCache;
};

#endif // BITCOIN_COINSVIEWOVERLAY_H
