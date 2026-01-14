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

/**
 * Controller that manages a CoinsViewCacheAsync and provides scoped access through a Handle.
 * The Handle dereferences to the internal cache and automatically resets it on destruction.
 */
class CoinsViewCacheAsyncController
{
private:
    CoinsViewCacheAsync m_cache;

public:
    class Handle
    {
    private:
        friend class CoinsViewCacheAsyncController;

        CCoinsViewCache& m_cache;

        explicit Handle(CCoinsViewCache& cache) noexcept : m_cache{cache} {}

    public:
        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;
        Handle(Handle&&) = delete;
        Handle& operator=(Handle&&) = delete;

        ~Handle() { m_cache.Reset(); }

        CCoinsViewCache* operator->() { return &m_cache; }
        CCoinsViewCache& operator*() { return m_cache; }
    };

    explicit CoinsViewCacheAsyncController(CCoinsView* base_in) noexcept : m_cache{base_in} {}

    [[nodiscard]] Handle Start() noexcept { return Handle{m_cache}; }
};

#endif // BITCOIN_COINSVIEWCACHEASYNC_H
