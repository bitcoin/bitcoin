// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INPUTFETCHER_H
#define BITCOIN_INPUTFETCHER_H

#include <coins.h>
#include <sync.h>
#include <tinyformat.h>
#include <txdb.h>
#include <util/hasher.h>
#include <util/threadnames.h>
#include <util/transaction_identifier.h>

#include <cstdint>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <vector>

/**
 * Input fetcher for fetching inputs from the CoinsDB and inserting
 * into the CoinsTip.
 *
 * The main thread loops through the block and writes all input prevouts to a
 * global vector. It then wakes all workers and starts working as well. Each
 * thread assigns itself a range of outpoints from the shared vector, and
 * fetches the coins from disk. The outpoint and coin pairs are written to a
 * thread local vector of pairs. Once all outpoints are fetched, the main thread
 * loops through all thread local vectors and writes the pairs to the cache.
 */
class InputFetcher
{
private:
    //! Mutex to protect the inner state
    Mutex m_mutex{};
    //! Worker threads block on this when out of work
    std::condition_variable m_worker_cv{};
    //! Main thread blocks on this when out of work
    std::condition_variable m_main_cv{};

    /**
     * The outpoints to be fetched from disk.
     * This is written to on the main thread, then read from all worker
     * threads only after the main thread is done writing. Hence, it doesn't
     * need to be guarded by a lock.
     */
    std::vector<COutPoint> m_outpoints{};
    /**
     * The index of the last outpoint that is being fetched. Workers assign
     * themselves a range of outpoints to fetch from m_outpoints. They will use
     * this index as the end of their range, and then set this index to the
     * beginning of the range they take for the next worker. Once it gets to
     * zero, all outpoints have been assigned and the next worker will wait.
     */
    size_t m_last_outpoint_index GUARDED_BY(m_mutex){0};

    //! The set of txids of the transactions in the current block being fetched.
    std::unordered_set<Txid, SaltedTxidHasher> m_txids{};
    //! The vector of thread local vectors of pairs to be written to the cache.
    std::vector<std::vector<std::pair<COutPoint, Coin>>> m_pairs{};

    /**
     * Number of outpoint fetches that haven't completed yet.
     * This includes outpoints that have already been assigned, but are still in
     * the worker's own batches.
     */
    int32_t m_in_flight_outpoints_count GUARDED_BY(m_mutex){0};
    //! The number of worker threads that are waiting on m_worker_cv
    int32_t m_idle_worker_count GUARDED_BY(m_mutex){0};
    //! The maximum number of outpoints to be assigned in one batch
    const int32_t m_batch_size;
    //! DB coins view to fetch from.
    const CCoinsView* m_db{nullptr};
    //! The cache to check if we already have this input.
    const CCoinsViewCache* m_cache{nullptr};

    std::vector<std::thread> m_worker_threads;
    bool m_request_stop GUARDED_BY(m_mutex){false};

    //! Internal function that does the fetching from disk.
    void Loop(int32_t index, bool is_main_thread = false) noexcept EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        auto local_batch_size{0};
        auto end_index{0};
        auto& cond{is_main_thread ? m_main_cv : m_worker_cv};
        do {
            {
                WAIT_LOCK(m_mutex, lock);
                // first do the clean-up of the previous loop run (allowing us to do
                // it in the same critsect) local_batch_size will only be
                // truthy after first run.
                if (local_batch_size) {
                    m_in_flight_outpoints_count -= local_batch_size;
                    if (!is_main_thread && m_in_flight_outpoints_count == 0) {
                        m_main_cv.notify_one();
                    }
                }

                // logically, the do loop starts here
                while (m_last_outpoint_index == 0) {
                    if ((is_main_thread && m_in_flight_outpoints_count == 0) || m_request_stop) {
                        return;
                    }
                    ++m_idle_worker_count;
                    cond.wait(lock);
                    --m_idle_worker_count;
                }

                // Assign a batch of outpoints to this thread
                local_batch_size = std::max(1, std::min(m_batch_size,
                            static_cast<int32_t>(m_last_outpoint_index /
                            (m_worker_threads.size() + 1 + m_idle_worker_count))));
                end_index = m_last_outpoint_index;
                m_last_outpoint_index -= local_batch_size;
            }

            auto& local_pairs{m_pairs[index]};
            local_pairs.reserve(local_pairs.size() + local_batch_size);
            try {
                for (auto i{end_index - local_batch_size}; i < end_index; ++i) {
                    const auto& outpoint{m_outpoints[i]};
                    // If an input spends an outpoint from earlier in the
                    // block, it won't be in the cache yet but it also won't be
                    // in the db either.
                    if (m_txids.contains(outpoint.hash)) {
                        continue;
                    }
                    if (m_cache->HaveCoinInCache(outpoint)) {
                        continue;
                    }
                    if (auto coin{m_db->GetCoin(outpoint)}; coin) {
                        local_pairs.emplace_back(outpoint, std::move(*coin));
                    } else {
                        // Missing an input. This block will fail validation.
                        // Skip remaining outpoints and continue so main thread
                        // can proceed.
                        LOCK(m_mutex);
                        m_in_flight_outpoints_count -= m_last_outpoint_index;
                        m_last_outpoint_index = 0;
                        break;
                    }
                }
            } catch (const std::runtime_error&) {
                // Database error. This will be handled later in validation.
                // Skip remaining outpoints and continue so main thread
                // can proceed.
                LOCK(m_mutex);
                m_in_flight_outpoints_count -= m_last_outpoint_index;
                m_last_outpoint_index = 0;
            }
        } while (true);
    }

public:

    //! Create a new input fetcher
    explicit InputFetcher(int32_t batch_size, int32_t worker_thread_count) noexcept
        : m_batch_size(batch_size)
    {
        if (worker_thread_count < 1) {
            // Don't do anything if there are no worker threads.
            return;
        }
        m_pairs.reserve(worker_thread_count + 1);
        for (auto n{0}; n < worker_thread_count + 1; ++n) {
            m_pairs.emplace_back();
        }
        m_worker_threads.reserve(worker_thread_count);
        for (auto n{0}; n < worker_thread_count; ++n) {
            m_worker_threads.emplace_back([this, n]() {
                util::ThreadRename(strprintf("inputfetch.%i", n));
                Loop(n);
            });
        }
    }

    // Since this class manages its own resources, which is a thread
    // pool `m_worker_threads`, copy and move operations are not appropriate.
    InputFetcher(const InputFetcher&) = delete;
    InputFetcher& operator=(const InputFetcher&) = delete;
    InputFetcher(InputFetcher&&) = delete;
    InputFetcher& operator=(InputFetcher&&) = delete;

    //! Fetch all block inputs from db, and insert into cache.
    void FetchInputs(CCoinsViewCache& cache,
                     const CCoinsView& db,
                     const CBlock& block) noexcept
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        if (m_worker_threads.empty() || block.vtx.size() <= 1) {
            return;
        }

        // Set the db and cache to use for this block.
        m_db = &db;
        m_cache = &cache;

        // Loop through the inputs of the block and add them to the queue
        m_txids.reserve(block.vtx.size() - 1);
        for (const auto& tx : block.vtx) {
            if (tx->IsCoinBase()) {
                continue;
            }
            m_outpoints.reserve(m_outpoints.size() + tx->vin.size());
            for (const auto& in : tx->vin) {
                m_outpoints.emplace_back(in.prevout);
            }
            m_txids.emplace(tx->GetHash());
        }
        {
            LOCK(m_mutex);
            m_in_flight_outpoints_count = m_outpoints.size();
            m_last_outpoint_index = m_outpoints.size();
        }
        m_worker_cv.notify_all();

        // Have the main thread work too while we wait for other threads
        Loop(m_worker_threads.size(), /*is_main_thread=*/true);

        // At this point all threads are done writing to m_pairs, so we can
        // safely read from it and insert the fetched coins into the cache.
        for (auto& local_pairs : m_pairs) {
            for (auto&& [outpoint, coin] : local_pairs) {
                cache.EmplaceCoinInternalDANGER(std::move(outpoint),
                                                std::move(coin),
                                                /*set_dirty=*/false);
            }
            local_pairs.clear();
        }
        m_txids.clear();
        m_outpoints.clear();
    }

    ~InputFetcher()
    {
        WITH_LOCK(m_mutex, m_request_stop = true);
        m_worker_cv.notify_all();
        for (std::thread& t : m_worker_threads) {
            t.join();
        }
    }
};

#endif // BITCOIN_INPUTFETCHER_H
