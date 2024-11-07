// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INPUTFETCHER_H
#define BITCOIN_INPUTFETCHER_H

#include <coins.h>
#include <logging.h>
#include <primitives/transaction_identifier.h>
#include <txdb.h>
#include <util/hasher.h>

#include <cstdint>
#include <stdexcept>
#include <unordered_set>
#include <vector>

/**
 * Helper for fetching inputs from the CoinsDB and inserting into the CoinsTip.
 *
 * It loops through the block and writes all input indexes to a
 * vector. It then assigns itself an input from the vector, and
 * fetches the coin from disk. The outpoint and coin pairs are written to a
 * a different vector. Once all inputs are fetched, it writes the coins to the cache.
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
     * The latest index in m_inputs that is not yet being fetched.
     * The inputfetcher ncrements this counter when it assign itself an input
     * from m_inputs to fetch.
     */
    size_t m_input_counter{0};

    /**
     * The vector of outpoint:coin pairs.
     */
    std::vector<std::pair<COutPoint, Coin>> m_coins{};

    /**
     * The set of txids of all txs in the block being fetched.
     * This is used to filter out inputs that are created in the block,
     * since they will not be in the db or the cache.
     */
    std::unordered_set<Txid, SaltedTxidHasher> m_txids{};

    //! DB coins view to fetch from.
    const CCoinsView* m_db{nullptr};
    //! The cache to check if we already have this input.
    const CCoinsViewCache* m_cache{nullptr};
    //! The block whose prevouts we are fetching.
    const CBlock* m_block{nullptr};

    void Work() noexcept
    {
        const auto inputs_count{m_inputs.size()};
        auto& coins{m_coins};
        try {
            while (true) {
                const auto input_index{m_input_counter++};
                if (input_index >= inputs_count) {
                    return;
                }
                const auto [tx_index, vin_index] = m_inputs[input_index];
                const auto& outpoint{m_block->vtx[tx_index]->vin[vin_index].prevout};
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
                    coins.emplace_back(outpoint, std::move(*coin));
                } else {
                    // Missing an input. This block will fail validation.
                    // Skip remaining inputs.
                    return;
                }
            }
        } catch (const std::runtime_error& e) {
            // Database error. This will be handled later in validation.
            // Skip remaining inputs.
            LogPrintLevel(BCLog::VALIDATION, BCLog::Level::Warning, "InputFetcher failed to fetch input: %s.\n", e.what());
        }
    }

public:
    //! Fetch all block inputs from db, and insert into cache.
    void FetchInputs(CCoinsViewCache& cache,
                     const CCoinsView& db,
                     const CBlock& block) noexcept
    {
        if (block.vtx.size() <= 1) {
            return;
        }

        m_db = &db;
        m_cache = &cache;
        m_block = &block;

        // Loop through the inputs of the block and add them to the queue.
        // Construct the set of txids to filter later.
        m_txids.reserve(block.vtx.size() - 1);
        for (size_t i{1}; i < block.vtx.size(); ++i) {
            const auto& tx{block.vtx[i]};
            for (size_t j{0}; j < tx->vin.size(); ++j) {
                m_inputs.emplace_back(i, j);
            }
            m_txids.emplace(tx->GetHash());
        }

        // Set the input counter.
        m_input_counter = 0;

        Work();

        for (auto&& [outpoint, coin] : m_coins) {
            cache.EmplaceCoinInternalDANGER(std::move(outpoint),
                                            std::move(coin),
                                            /*set_dirty=*/false);
        }
        m_coins.clear();
        m_txids.clear();
        m_inputs.clear();
    }
};

#endif // BITCOIN_INPUTFETCHER_H
