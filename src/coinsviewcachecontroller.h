// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINSVIEWCACHECONTROLLER_H
#define BITCOIN_COINSVIEWCACHECONTROLLER_H

#include <coins.h>

/**
 * Controller that manages a CCoinsViewCache and provides scoped access through a Handle.
 * The Handle dereferences to the internal cache and automatically resets it on destruction.
 */
class CoinsViewCacheController
{
private:
    CCoinsViewCache m_cache;

public:
    class Handle
    {
    private:
        friend class CoinsViewCacheController;

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

    explicit CoinsViewCacheController(CCoinsView* base_in) noexcept : m_cache{base_in} {}

    [[nodiscard]] Handle Start() noexcept { return Handle{m_cache}; }
};

#endif // BITCOIN_COINSVIEWCACHECONTROLLER_H
