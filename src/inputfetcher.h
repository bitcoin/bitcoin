// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INPUTFETCHER_H
#define BITCOIN_INPUTFETCHER_H

#include <coins.h>
#include <sync.h>
#include <tinyformat.h>
#include <txdb.h>
#include <util/threadnames.h>

#include <algorithm>
#include <iterator>
#include <set>
#include <vector>

/**
 * Input fetcher for fetching inputs from the CoinsDB and inserting
 * into the CoinsTip.
 *
 * The main thread pushes batches of outpoints
 * onto the queue, where they are fetched by N worker threads. The resulting
 * coins are pushed onto another queue after they are read from disk. When
 * the main is done adding outpoints, it starts writing the results of the read
 * queue to the cache.
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

    //! The queue of outpoints to be fetched from disk.
    //! As the order of outpoints doesn't matter, it is used as a LIFO (stack)
    std::vector<COutPoint> m_outpoints GUARDED_BY(m_mutex){};

    //! The queue of pairs to be written to the cache.
    std::vector<std::pair<COutPoint, Coin>> m_pairs GUARDED_BY(m_mutex){};

    /**
     * Number of outpoint fetches that haven't completed yet.
     * This includes outpoints that are no longer queued, but still in the
     * worker's own batches.
     */
    size_t m_in_flight_fetches_count GUARDED_BY(m_mutex){0};

    //! The maximum number of outpoints to be processed in one batch
    const size_t m_batch_size;

    //! DB to fetch from.
    const CCoinsViewDB* m_db{nullptr};

    std::vector<std::thread> m_worker_threads;
    bool m_request_stop GUARDED_BY(m_mutex){false};

    /** Internal function that does the fetching from disk. */
    void Loop() noexcept EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        size_t in_flight_fetches_count{0};
        std::vector<std::pair<COutPoint, Coin>> pairs{};
        do {
            std::vector<COutPoint> outpoints{};
            outpoints.reserve(m_batch_size);
            {
                WAIT_LOCK(m_mutex, lock);
                // first do the clean-up of the previous loop run (allowing us to do it in the same critsect)
                // in_flight_fetches_count will only be truthy after first run.
                if (in_flight_fetches_count) {
                    if (m_pairs.empty()) {
                        m_pairs = std::move(pairs);
                    } else {
                        m_pairs.reserve(m_pairs.size() + pairs.size());
                        m_pairs.insert(m_pairs.end(), std::make_move_iterator(pairs.begin()),
                                       std::make_move_iterator(pairs.end()));
                    }
                    m_in_flight_fetches_count -= in_flight_fetches_count;
                    m_main_cv.notify_one();
                }

                // logically, the do loop starts here
                while (m_outpoints.empty() && !m_request_stop) {
                    m_worker_cv.wait(lock);
                }
                if (m_request_stop) {
                    return;
                }

                const auto even_bucket{m_in_flight_fetches_count / m_worker_threads.size()};
                in_flight_fetches_count = std::max(static_cast<size_t>(1),
                                                   std::min(std::min(m_outpoints.size(), m_batch_size), even_bucket));
                auto start_it = m_outpoints.end() - in_flight_fetches_count;
                outpoints.assign(std::make_move_iterator(start_it), std::make_move_iterator(m_outpoints.end()));
                m_outpoints.erase(start_it, m_outpoints.end());
            }

            pairs.clear();
            pairs.reserve(outpoints.size());
            for (COutPoint& outpoint : outpoints) {
                if (auto coin{m_db->GetCoin(outpoint)}; coin) {
                    pairs.emplace_back(std::move(outpoint), std::move(*coin));
                } else {
                    // Missing an input, just break. This block will fail validation, so no point in continuing.
                    break;
                }
            }
        } while (true);
    }

    //! Add a batch of outpoints to the queue
    void Add(std::vector<COutPoint>&& outpoints) noexcept EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        if (outpoints.empty()) {
            return;
        }

        const auto size{outpoints.size()};
        {
            LOCK(m_mutex);
            m_in_flight_fetches_count += outpoints.size();
            if (m_outpoints.empty()) {
                m_outpoints = std::move(outpoints);
            } else {
                m_outpoints.insert(m_outpoints.end(), std::make_move_iterator(outpoints.begin()), std::make_move_iterator(outpoints.end()));
            }
        }

        if (size == 1) {
            m_worker_cv.notify_one();
        } else {
            m_worker_cv.notify_all();
        }
    }


public:
    //! Create a new input fetcher
    explicit InputFetcher(size_t batch_size, size_t worker_thread_count) noexcept
        : m_batch_size(batch_size)
    {
        m_worker_threads.reserve(worker_thread_count);
        for (size_t n = 0; n < worker_thread_count; ++n) {
            m_worker_threads.emplace_back([this, n]() {
                util::ThreadRename(strprintf("inputfetch.%i", n));
                Loop();
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
    void FetchInputs(CCoinsViewCache& cache, const CCoinsViewDB& db, const CBlock& block) noexcept EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        m_db = &db;

        std::vector<COutPoint> buffer{};
        buffer.reserve(m_batch_size);
        std::set<Txid> txids{};
        for (const auto& tx : block.vtx) {
            if (tx->IsCoinBase()) continue;
            for (const auto& in : tx->vin) {
                const auto& outpoint = in.prevout;
                // If an input references an outpoint from earlier in the
                // block, it won't be in the cache yet but it also won't be
                // in the db either.
                if (txids.contains(outpoint.hash)) {
                    continue;
                }
                if (cache.HaveCoinInCache(outpoint)) {
                    continue;
                }

                buffer.emplace_back(outpoint);
                if (buffer.size() == m_batch_size) {
                    Add(std::move(buffer));
                    buffer.clear();
                    buffer.reserve(m_batch_size);
                }
            }
            txids.insert(tx->GetHash());
        }

        Add(std::move(buffer));

        std::vector<std::pair<COutPoint, Coin>> pairs{};
        do {
            {
                WAIT_LOCK(m_mutex, lock);
                while (m_pairs.empty() && !m_request_stop) {
                    if (m_in_flight_fetches_count == 0) {
                        return;
                    }
                    m_main_cv.wait(lock);
                }

                if (m_request_stop) {
                    return;
                }

                pairs = std::move(m_pairs);
                m_pairs.clear();
            }

            for (auto& pair : pairs) {
                cache.EmplaceCoinInternalDANGER(std::move(pair.first), std::move(pair.second), /*set_dirty=*/false);
            }
        } while (true);
    }


    ~InputFetcher()
    {
        WITH_LOCK(m_mutex, m_request_stop = true);
        m_worker_cv.notify_all();
        for (std::thread& t : m_worker_threads) {
            t.join();
        }
    }

    bool HasThreads() const noexcept { return !m_worker_threads.empty(); }
};

#endif // BITCOIN_INPUTFETCHER_H
