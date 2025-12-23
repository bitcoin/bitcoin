// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINSVIEWCACHEASYNC_H
#define BITCOIN_COINSVIEWCACHEASYNC_H

#include <attributes.h>
#include <coins.h>
#include <crypto/common.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <util/check.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

/**
 * CCoinsViewCache subclass that fetches all block inputs during ConnectBlock without mutating the base cache.
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
        //! Set this after setting the coin. Test this before reading the coin.
        bool ready{false};
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
     * The sorted first 8 bytes of txids of all txs in the block being fetched. This is used to filter out inputs that
     * are created earlier in the same block, since they will not be in the db or the cache.
     * Using only the first 8 bytes is a performance improvement, versus storing the entire 32 bytes. In case of a
     * collision of an input being spent having the same first 8 bytes as a txid of a tx elsewhere in the block,
     * the input will not be fetched in the background. The input will still be fetched later on the main thread.
     * Using a sorted vector and binary search lookups is a performance improvement. It is faster than
     * using std::unordered_set with salted hash or std::set.
     */
    std::vector<uint64_t> m_txids{};

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
        // Inputs spending a coin from a tx earlier in the block won't be in the cache or db
        if (std::ranges::binary_search(m_txids, ReadLE64(input.outpoint.hash.begin()))) {
            input.ready = true;
            return true;
        }

        if (auto coin{base->PeekCoin(input.outpoint)}) [[likely]] input.coin.emplace(std::move(*coin));
        input.ready = true;
        return true;
    }

    std::optional<Coin> GetCoinFromBase(const COutPoint& outpoint) const override
    {
        // This assumes ConnectBlock accesses all inputs in the same order as they are added to m_inputs
        // in StartFetching. Some outpoints are not accessed because they are created by the block, so we scan until we
        // come across the requested input.
        for (const auto i : std::views::iota(m_input_tail, m_inputs.size())) [[likely]] {
            auto& input{m_inputs[i]};
            if (input.outpoint != outpoint) continue;
            // We advance the tail since the input is cached and not accessed through this method again.
            m_input_tail = i + 1;
            // Check if the coin is ready to be read.
            while (!input.ready) {
                ProcessInput();
            }
            // We can move the coin since we won't access this input again.
            if (input.coin) [[likely]] return std::move(*input.coin);
            // This block has missing or spent inputs or there is a shorttxid collision.
            break;
        }

        // We will only get in here for BIP30 checks, shorttxid collisions or a block with missing or spent inputs.
        return base->PeekCoin(outpoint);
    }

    //! Stop fetching and clear state.
    void StopFetching() noexcept
    {
        m_inputs.clear();
        m_input_head = 0;
        m_input_tail = 0;
        m_txids.clear();
    }

public:
    //! Start fetching all block inputs in the background.
    [[nodiscard]] ResetGuard StartFetching(const CBlock& block LIFETIMEBOUND) noexcept
    {
        Assert(m_inputs.empty());
        // Loop through the inputs of the block and set them in the queue. Also construct the set of txids to filter.
        for (const auto& tx : block.vtx | std::views::drop(1)) [[likely]] {
            for (const auto& input : tx->vin) [[likely]] m_inputs.emplace_back(input.prevout);
            m_txids.emplace_back(ReadLE64(tx->GetHash().begin()));
        }
        // Only start if we have something to fetch.
        if (!m_inputs.empty()) [[likely]] {
            // Sort txids so we can do binary search lookups.
            std::ranges::sort(m_txids);
        } else {
            m_txids.clear();
        }
        return CreateResetGuard();
    }

    void Reset() noexcept override
    {
        StopFetching();
        CCoinsViewCache::Reset();
    }

    using CCoinsViewCache::CCoinsViewCache;
};

#endif // BITCOIN_COINSVIEWCACHEASYNC_H
