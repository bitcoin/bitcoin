// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINSVIEWCACHEASYNC_H
#define BITCOIN_COINSVIEWCACHEASYNC_H

#include <attributes.h>
#include <coins.h>
#include <primitives/block.h>
#include <primitives/transaction.h>

#include <cstdint>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

/**
 * CCoinsViewCache subclass that fetches all block inputs before ConnectBlock without mutating the base cache.
 * Only used in ConnectBlock to pass as an ephemeral view that can be reset if the block is invalid.
 * It provides the same interface as CCoinsViewCache, overriding the FetchCoin private method and Reset.
 * It adds an additional StartFetching method to provide the block.
 */
class CoinsViewCacheAsync : public CCoinsViewCache
{
private:
    //! The latest input not yet being fetched.
    mutable uint32_t m_input_head{0};
    //! The latest input not yet accessed by a consumer.
    mutable uint32_t m_input_tail{0};

    //! The inputs of the block which is being fetched.
    struct InputToFetch {
        //! The outpoint of the input to fetch.
        const COutPoint& outpoint;
        //! The coin that will be fetched.
        std::optional<Coin> coin{std::nullopt};

        /**
         * We only move when m_inputs reallocates during setup.
         * We never move after work begins, so we don't have to copy other members.
         */
        InputToFetch(InputToFetch&& other) noexcept : outpoint{other.outpoint} {}
        explicit InputToFetch(const COutPoint& o LIFETIMEBOUND) noexcept : outpoint{o} {}
    };
    mutable std::vector<InputToFetch> m_inputs{};

    /**
     * Claim and fetch the next input in the queue.
     *
     * @return true if there are more inputs in the queue to fetch
     * @return false if there are no more inputs in the queue to fetch
     */
    bool ProcessInput() const noexcept
    {
        const auto i{m_input_head++};
        if (i >= m_inputs.size()) [[unlikely]] return false;

        auto& input{m_inputs[i]};
        if (auto coin{FetchCoinWithoutMutating(input.outpoint)}) [[likely]] input.coin.emplace(std::move(*coin));
        return true;
    }

    //! Get the index in m_inputs for the given outpoint. Advances m_input_tail if found.
    std::optional<uint32_t> GetInputIndex(const COutPoint& outpoint) const noexcept
    {
        // This assumes ConnectBlock accesses all inputs in the same order as they are added to m_inputs
        // in StartFetching. Some outpoints are not accessed because they are created by the block, so we scan until we
        // come across the requested input. We advance the tail since the input will be cached and not accessed through
        // this method again.
        for (const auto i : std::views::iota(m_input_tail, m_inputs.size())) [[likely]] {
            if (m_inputs[i].outpoint == outpoint) {
                m_input_tail = i + 1;
                return i;
            }
        }
        return std::nullopt;
    }

    CCoinsMap::iterator FetchCoin(const COutPoint& outpoint) const override
    {
        auto [ret, inserted]{cacheCoins.try_emplace(outpoint)};
        if (!inserted) return ret;

        if (const auto i{GetInputIndex(outpoint)}) [[likely]] {
            auto& input{m_inputs[*i]};
            if (input.coin) [[likely]] ret->second.coin = std::move(*input.coin);
        }

        if (ret->second.coin.IsSpent()) [[unlikely]] {
            // We will only get in here for BIP30 checks or a block with missing or spent inputs.
            // TODO: Remove spent checks once we no longer return spent coins in coinscache_sim CoinsViewBottom.
            if (auto coin{FetchCoinWithoutMutating(outpoint)}; coin && !coin->IsSpent()) {
                ret->second.coin = std::move(*coin);
            } else {
                cacheCoins.erase(ret);
                return cacheCoins.end();
            }
        }

        cachedCoinsUsage += ret->second.coin.DynamicMemoryUsage();
        return ret;
    }

public:
    //! Fetch all block inputs.
    void StartFetching(const CBlock& block) noexcept
    {
        // Loop through the inputs of the block and set them in the queue.
        for (const auto& tx : block.vtx | std::views::drop(1)) [[likely]] {
            for (const auto& input : tx->vin) [[likely]] m_inputs.emplace_back(input.prevout);
        }
        // Don't start if there's nothing to fetch.
        if (m_inputs.empty()) [[unlikely]] return;
        while (ProcessInput()) [[likely]] {}
    }

    void Reset() noexcept override
    {
        m_inputs.clear();
        m_input_head = 0;
        m_input_tail = 0;
        CCoinsViewCache::Reset();
    }

    using CCoinsViewCache::CCoinsViewCache;
};

#endif // BITCOIN_COINSVIEWCACHEASYNC_H
