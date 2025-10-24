// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INPUTFETCHER_H
#define BITCOIN_INPUTFETCHER_H

#include <coins.h>
#include <logging.h>
#include <primitives/transaction_identifier.h>
#include <tinyformat.h>
#include <txdb.h>
#include <util/hasher.h>
#include <util/threadnames.h>

#include <atomic>
#include <barrier>
#include <cstdint>
#include <functional>
#include <semaphore>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

/**
 * Helper for fetching inputs from the CoinsDB and inserting into the CoinsTip.
 *
 * The main thread loops through the block and writes all input indexes to a
 * global vector. It then wakes all workers and starts working as well. Each
 * thread assigns itself an input from the shared vector, and
 * fetches the coin from disk. The outpoint and coin pairs are written to a
 * thread local vector. Once all inputs are fetched, the last thread to arrive
 * at the completion barrier loops through all thread local vectors and writes
 * the coins to the cache.
 */
class InputFetcher
{
private:
    /**
     * The flattened indexes to each input in the block. The first item in the
     * pair is the index of the tx, and the second is the index of the vin.
     */
    std::vector<std::pair<size_t, size_t>> m_inputs{};

    /**
     * The set of txids of all txs in the block being fetched.
     * This is used to filter out inputs that are created in the block,
     * since they will not be in the db or the cache.
     */
    std::unordered_set<Txid, SaltedTxidHasher> m_txids{};

    /**
     * The latest index in m_inputs that is not yet being fetched.
     * Workers increment this counter when they assign themselves an input
     * from m_inputs to fetch.
     */
    std::atomic<size_t> m_input_counter{0};

    /**
     * The vector of vectors of outpoint:coin pairs.
     * Each thread writes the coins it fetches to the vector at its thread
     * index. This way multiple threads can write concurrently to different
     * vectors in a thread safe way. After all threads are finished, the main
     * thread can loop through all vectors and write the coins to the cache.
     */
    std::vector<std::vector<std::pair<COutPoint, Coin>>> m_coins{};

    //! DB coins view to fetch from.
    const CCoinsView* m_db{nullptr};
    //! The cache to write to.
    CCoinsViewCache* m_cache{nullptr};
    //! The block whose prevouts we are fetching.
    const CBlock* m_block{nullptr};

    std::vector<std::thread> m_worker_threads;
    std::counting_semaphore<> m_start_semaphore{0};
    struct OnCompletionWrapper { // Necessary to get this to compile on Windows
        InputFetcher* self;
        void operator()() noexcept { self->OnCompletion(); }
    };
    std::barrier<OnCompletionWrapper> m_complete_barrier;
    std::atomic<bool> m_request_stop{false};

    void ThreadLoop(size_t thread_index) noexcept
    {
        util::ThreadRename(strprintf("inputfetch.%i", thread_index));

        while (true) {
            m_start_semaphore.acquire();
            if (m_request_stop.load(std::memory_order_relaxed)) {
                return;
            }
            Work(thread_index);
            [[maybe_unused]] const auto arrival_token{m_complete_barrier.arrive()};
        }
    }

    void Work(size_t thread_index) noexcept
    {
        try {
            while (true) {
                const auto input_index{m_input_counter.fetch_add(1, std::memory_order_relaxed)};
                if (input_index >= m_inputs.size()) {
                    return;
                }
                const auto [tx_index, vin_index] = m_inputs[input_index];
                const auto& outpoint{m_block->vtx[tx_index]->vin[vin_index].prevout};
                // If an input spends an outpoint from earlier in the block,
                // it won't be in the cache yet but it also won't be in the db either.
                if (m_txids.contains(outpoint.hash)) {
                    continue;
                }
                if (m_cache->HaveCoinInCache(outpoint)) {
                    continue;
                }
                if (auto coin{m_db->GetCoin(outpoint)}; coin) {
                    m_coins[thread_index].emplace_back(outpoint, std::move(*coin));
                } else {
                    // Missing an input. This block will fail validation.
                    // Skip remaining inputs.
                    m_input_counter.store(m_inputs.size(), std::memory_order_relaxed);
                    return;
                }
            }
        } catch (const std::runtime_error& e) {
            // Database error. This will be handled later in validation.
            // Skip remaining inputs.
            LogPrintLevel(BCLog::VALIDATION, BCLog::Level::Warning, "InputFetcher failed to fetch input: %s.\n", e.what());
            m_input_counter.store(m_inputs.size(), std::memory_order_relaxed);
        }
    }

    void OnCompletion() noexcept
    {
        // At this point all threads are done writing to m_coins and reading from m_cache,
        // so we can safely read from m_coins and write to m_cache.
        for (auto& coins : m_coins) {
            for (auto&& [outpoint, coin] : coins) {
                m_cache->EmplaceCoinInternalDANGER(std::move(outpoint), std::move(coin));
            }
            coins.clear();
        }
    }

public:
    explicit InputFetcher(size_t worker_thread_count) noexcept
        : m_complete_barrier{static_cast<int32_t>(worker_thread_count + 1), OnCompletionWrapper{this}}
    {
        if (worker_thread_count == 0) {
            return;
        }
        for (size_t n{0}; n < worker_thread_count; ++n) {
            m_coins.emplace_back();
            m_worker_threads.emplace_back(std::bind(&InputFetcher::ThreadLoop, this, n));
        }
        // One more coins vector for the main thread
        m_coins.emplace_back();
    }

    //! Fetch all block inputs from db, and insert into cache.
    void FetchInputs(CCoinsViewCache& cache, const CCoinsView& db, const CBlock& block) noexcept
    {
        if (block.vtx.size() <= 1 || m_worker_threads.size() == 0) {
            return;
        }

        m_db = &db;
        m_cache = &cache;
        m_block = &block;
        m_txids.clear();
        m_inputs.clear();

        // Loop through the inputs of the block and add them to the queue.
        // Construct the set of txids to filter.
        for (size_t i{1}; i < block.vtx.size(); ++i) {
            for (size_t j{0}; j < block.vtx[i]->vin.size(); ++j) {
                m_inputs.emplace_back(i, j);
            }
            m_txids.emplace(block.vtx[i]->GetHash());
        }

        // Set the input counter and wake threads.
        m_input_counter.store(0, std::memory_order_relaxed);
        m_start_semaphore.release(m_worker_threads.size());

        // Have the main thread work too before we wait for other threads
        Work(m_worker_threads.size());
        m_complete_barrier.arrive_and_wait();
    }

    ~InputFetcher()
    {
        m_request_stop.store(true, std::memory_order_relaxed);
        m_start_semaphore.release(m_worker_threads.size());
        for (auto& t : m_worker_threads) {
            t.join();
        }
    }
};

#endif // BITCOIN_INPUTFETCHER_H
