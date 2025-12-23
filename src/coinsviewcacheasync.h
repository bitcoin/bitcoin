// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINSVIEWCACHEASYNC_H
#define BITCOIN_COINSVIEWCACHEASYNC_H

#include <attributes.h>
#include <coins.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <util/check.h>

#include <algorithm>
#include <atomic>
#include <barrier>
#include <cstdint>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

/**
 * CCoinsViewCache subclass that fetches all block inputs during ConnectBlock without mutating the base cache.
 * Only used in ConnectBlock to pass as an ephemeral view that can be reset if the block is invalid.
 * It provides the same interface as CCoinsViewCache. It overrides all methods that mutate cacheCoins or base,
 * stopping threads before calling superclass.
 * It adds an additional StartFetching method to provide the block.
 */
class CoinsViewCacheAsync : public CCoinsViewCache
{
private:
    //! The latest input not yet being fetched. Workers atomically increment this when fetching.
    mutable std::atomic_uint32_t m_input_head{0};
    //! The latest input not yet accessed by a consumer. Only the main thread increments this.
    mutable uint32_t m_input_tail{0};

    //! The inputs of the block which is being fetched.
    struct InputToFetch {
        //! Workers set this after setting the coin. The main thread tests this before reading the coin.
        std::atomic_flag ready{};
        //! The outpoint of the input to fetch.
        const COutPoint& outpoint;
        //! The coin that workers will fetch and main thread will insert into cache.
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
     * The first 8 bytes of txids of all txs in the block being fetched. This is used to filter out inputs that
     * are created earlier in the same block, since they will not be in the db or the cache.
     * Using only the first 8 bytes is a performance improvement, versus storing the entire 32 bytes. In case of a
     * collision of an input being spent having the same first 8 bytes as a txid of a tx elsewhere in the block,
     * the input will not be fetched in the background. The input will still be fetched later on the main thread.
     * Using a sorted vector and binary search lookups is a performance improvement. It is faster than
     * using std::unordered_set with salted hash or std::set.
     */
    std::vector<uint64_t> m_txids{};

    /**
     * Claim and fetch the next input in the queue. Safe to call from any thread once inside the barrier.
     *
     * @return true if there are more inputs in the queue to fetch
     * @return false if there are no more inputs in the queue to fetch
     */
    bool ProcessInputInBackground() const noexcept
    {
        const auto i{m_input_head.fetch_add(1, std::memory_order_relaxed)};
        if (i >= m_inputs.size()) [[unlikely]] return false;

        auto& input{m_inputs[i]};
        // Inputs spending a coin from a tx earlier in the block won't be in the cache or db
        if (std::ranges::binary_search(m_txids, input.outpoint.hash.ToUint256().GetUint64(0))) {
            // We can use relaxed ordering here since we don't write the coin.
            input.ready.test_and_set(std::memory_order_relaxed);
            input.ready.notify_one();
            return true;
        }

        if (auto coin{FetchCoinWithoutMutating(input.outpoint)}) [[likely]] input.coin.emplace(std::move(*coin));
        // We need release here, so writing coin in the line above happens before the main thread acquires.
        input.ready.test_and_set(std::memory_order_release);
        input.ready.notify_one();
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
            // Check if the coin is ready to be read. We need to acquire to match the worker thread's release.
            while (!input.ready.test(std::memory_order_acquire)) {
                // Work instead of waiting if the coin is not ready
                if (!ProcessInputInBackground()) {
                    // No more work, just wait
                    input.ready.wait(/*old=*/false, std::memory_order_acquire);
                    break;
                }
            }
            if (input.coin) [[likely]] ret->second.coin = std::move(*input.coin);
        }

        if (ret->second.coin.IsSpent()) [[unlikely]] {
            // We will only get in here for BIP30 checks, shorttxid collisions, or a block with missing or spent inputs.
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

    std::barrier<> m_barrier{1};

    //! Stop all worker threads.
    void StopFetching() noexcept
    {
        if (m_inputs.empty()) return;
        // Skip fetching the rest of the inputs by moving the head to the end.
        m_input_head.store(m_inputs.size(), std::memory_order_relaxed);
        // Wait for all threads to stop.
        m_barrier.arrive_and_wait();
        m_inputs.clear();
        m_input_head.store(0, std::memory_order_relaxed);
        m_input_tail = 0;
        m_txids.clear();
    }

public:
    //! Fetch all block inputs.
    void StartFetching(const CBlock& block) noexcept
    {
        Assume(m_inputs.empty());
        // Loop through the inputs of the block and set them in the queue. Also construct the set of txids to filter.
        for (const auto& tx : block.vtx | std::views::drop(1)) [[likely]] {
            for (const auto& input : tx->vin) [[likely]] m_inputs.emplace_back(input.prevout);
            m_txids.emplace_back(tx->GetHash().ToUint256().GetUint64(0));
        }
        // Don't start threads if there's nothing to fetch.
        if (m_inputs.empty()) [[unlikely]] return;
        // Sort txids so we can do binary search lookups.
        std::ranges::sort(m_txids);
        // Start workers by entering the barrier.
        m_barrier.arrive_and_wait();
    }

    bool Flush(bool will_reuse_cache) override
    {
        StopFetching();
        return CCoinsViewCache::Flush(will_reuse_cache);
    }

    bool Sync() override
    {
        StopFetching();
        return CCoinsViewCache::Sync();
    }

    void SetBackend(CCoinsView &viewIn) override
    {
        StopFetching();
        return CCoinsViewCache::SetBackend(viewIn);
    }

    void Reset() noexcept override
    {
        StopFetching();
        CCoinsViewCache::Reset();
    }

    using CCoinsViewCache::CCoinsViewCache;

    ~CoinsViewCacheAsync() override
    {
        StopFetching();
        m_barrier.arrive_and_drop();
    }
};

#endif // BITCOIN_COINSVIEWCACHEASYNC_H
