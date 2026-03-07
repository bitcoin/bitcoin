// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <kernel/chainstatemanager_opts.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <primitives/transaction_identifier.h>
#include <txdb.h>
#include <uint256.h>
#include <util/byte_units.h>
#include <util/hasher.h>
#include <util/threadpool.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <cstring>
#include <memory>
#include <ranges>
#include <unordered_set>

BOOST_AUTO_TEST_SUITE(coinsviewoverlay_tests)

namespace {

CBlock CreateBlock() noexcept
{
    static constexpr auto NUM_TXS{100};
    CBlock block;
    CMutableTransaction coinbase;
    coinbase.vin.emplace_back();
    block.vtx.push_back(MakeTransactionRef(coinbase));

    Txid prevhash{Txid::FromUint256(uint256{1})};

    for (const auto i : std::views::iota(1, NUM_TXS)) {
        CMutableTransaction tx;
        const Txid txid{i % 2 == 0 ? Txid::FromUint256(uint256(i)) : prevhash};
        tx.vin.emplace_back(txid, 0);
        prevhash = tx.GetHash();
        block.vtx.push_back(MakeTransactionRef(tx));
    }

    return block;
}

void PopulateView(const CBlock& block, CCoinsView& view, bool spent = false)
{
    CCoinsViewCache cache{&view};
    cache.SetBestBlock(uint256::ONE);

    std::unordered_set<Txid, SaltedTxidHasher> txids{};
    txids.reserve(block.vtx.size() - 1);
    for (const auto& tx : block.vtx | std::views::drop(1)) {
        for (const auto& in : tx->vin) {
            if (txids.contains(in.prevout.hash)) continue;
            Coin coin{};
            if (!spent) coin.out.nValue = 1;
            cache.EmplaceCoinInternalDANGER(COutPoint{in.prevout}, std::move(coin));
        }
        txids.emplace(tx->GetHash());
    }

    cache.Flush();
}

//! Returns a started thread pool shared across tests, mirroring how production reuses pools.
std::shared_ptr<ThreadPool> StartedThreadPool()
{
    static const auto thread_pool{[] {
        auto pool{std::make_shared<ThreadPool>("fetch_test")};
        pool->Start(DEFAULT_INPUTFETCH_THREADS);
        return pool;
    }()};
    return thread_pool;
}

void CheckCache(const CBlock& block, const CCoinsViewCache& cache)
{
    uint32_t counter{0};
    std::unordered_set<Txid, SaltedTxidHasher> txids{};
    txids.reserve(block.vtx.size() - 1);

    for (const auto& tx : block.vtx) {
        if (tx->IsCoinBase()) {
            BOOST_CHECK(!cache.HaveCoinInCache(tx->vin[0].prevout));
        } else {
            for (const auto& in : tx->vin) {
                const auto& outpoint{in.prevout};
                const auto& first{cache.AccessCoin(outpoint)};
                const auto& second{cache.AccessCoin(outpoint)};
                BOOST_CHECK_EQUAL(&first, &second);
                const auto should_have{!txids.contains(outpoint.hash)};
                if (should_have) ++counter;
                const auto have{cache.HaveCoinInCache(outpoint)};
                BOOST_CHECK_EQUAL(should_have, have);
            }
            txids.emplace(tx->GetHash());
        }
    }
    BOOST_CHECK_EQUAL(cache.GetCacheSize(), counter);
}

} // namespace

BOOST_AUTO_TEST_CASE(fetch_inputs_from_db)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    PopulateView(block, db);
    CCoinsViewCache main_cache{&db};
    CoinsViewOverlay view{&main_cache, StartedThreadPool()};
    const auto reset_guard{view.StartFetching(block)};
    const auto& outpoint{block.vtx[1]->vin[0].prevout};

    BOOST_CHECK(view.HaveCoin(outpoint));
    BOOST_CHECK(view.GetCoin(outpoint).has_value());
    BOOST_CHECK(!main_cache.HaveCoinInCache(outpoint));

    CheckCache(block, view);
    // Check that no coins have been moved up to main cache from db
    for (const auto& tx : block.vtx) {
        for (const auto& in : tx->vin) {
            BOOST_CHECK(!main_cache.HaveCoinInCache(in.prevout));
        }
    }

    view.SetBestBlock(uint256::ONE);
    BOOST_CHECK(view.SpendCoin(outpoint));
    view.Flush();
    BOOST_CHECK(!main_cache.PeekCoin(outpoint).has_value());
}

BOOST_AUTO_TEST_CASE(fetch_inputs_from_cache)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    PopulateView(block, main_cache);
    CoinsViewOverlay view{&main_cache, StartedThreadPool()};
    const auto reset_guard{view.StartFetching(block)};
    CheckCache(block, view);

    const auto& outpoint{block.vtx[1]->vin[0].prevout};
    view.SetBestBlock(uint256::ONE);
    BOOST_CHECK(view.SpendCoin(outpoint));
    view.Flush();
    BOOST_CHECK(!main_cache.PeekCoin(outpoint).has_value());
}

// Test for the case where a block spends coins that are spent in the cache, but
// the spentness has not been flushed to the db.
BOOST_AUTO_TEST_CASE(fetch_no_double_spend)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    PopulateView(block, db);
    CCoinsViewCache main_cache{&db};
    // Add all inputs as spent already in cache
    PopulateView(block, main_cache, /*spent=*/true);
    CoinsViewOverlay view{&main_cache, StartedThreadPool()};
    const auto reset_guard{view.StartFetching(block)};
    for (const auto& tx : block.vtx) {
        for (const auto& in : tx->vin) {
            const auto& c{view.AccessCoin(in.prevout)};
            BOOST_CHECK(c.IsSpent());
            BOOST_CHECK(!view.HaveCoin(in.prevout));
            BOOST_CHECK(!view.GetCoin(in.prevout));
        }
    }
    // Coins are not added to the view, even though they exist unspent in the parent db
    BOOST_CHECK_EQUAL(view.GetCacheSize(), 0);
}

BOOST_AUTO_TEST_CASE(fetch_no_inputs)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    CoinsViewOverlay view{&main_cache, StartedThreadPool()};
    const auto reset_guard{view.StartFetching(block)};
    for (const auto& tx : block.vtx) {
        for (const auto& in : tx->vin) {
            const auto& c{view.AccessCoin(in.prevout)};
            BOOST_CHECK(c.IsSpent());
            BOOST_CHECK(!view.HaveCoin(in.prevout));
            BOOST_CHECK(!view.GetCoin(in.prevout));
        }
    }
    BOOST_CHECK_EQUAL(view.GetCacheSize(), 0);
}

// Access coins that are not block inputs
BOOST_AUTO_TEST_CASE(access_non_input_coins)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    Coin coin{};
    coin.out.nValue = 1;
    const COutPoint outpoint{Txid::FromUint256(uint256::ZERO), 0};
    main_cache.EmplaceCoinInternalDANGER(COutPoint{outpoint}, std::move(coin));
    const COutPoint missing_outpoint{Txid::FromUint256(uint256::ONE), 0};

    CoinsViewOverlay view{&main_cache, StartedThreadPool()};
    const auto reset_guard{view.StartFetching(block)};

    // Non-input fallback hit.
    const auto& accessed_coin{view.AccessCoin(outpoint)};
    BOOST_CHECK(!accessed_coin.IsSpent());

    // Non-input fallback miss.
    const auto& missing_coin{view.AccessCoin(missing_outpoint)};
    BOOST_CHECK(missing_coin.IsSpent());
    BOOST_CHECK(!view.HaveCoinInCache(missing_outpoint));
}

// Test that disabled input fetching falls back to normal cache lookups via base->PeekCoin.
BOOST_AUTO_TEST_CASE(fetch_unstarted_thread_pool)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    PopulateView(block, main_cache);
    auto thread_pool{std::make_shared<ThreadPool>("fetch_none")};
    CoinsViewOverlay view{&main_cache, thread_pool};
    const auto reset_guard{view.StartFetching(block)};
    CheckCache(block, view);
}

BOOST_AUTO_TEST_CASE(reservation_holds_for_max_inputs)
{
    CBlock block;
    CMutableTransaction coinbase;
    coinbase.vin.emplace_back();
    block.vtx.push_back(MakeTransactionRef(coinbase));

    CMutableTransaction tx;
    tx.vin.reserve(MAX_INPUTS_PER_BLOCK);
    const Txid prev{Txid::FromUint256(uint256::ONE)};
    for (const auto i : std::views::iota(0u, MAX_INPUTS_PER_BLOCK)) {
        tx.vin.emplace_back(prev, i);
    }
    block.vtx.push_back(MakeTransactionRef(tx));

    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    CoinsViewOverlay view{&main_cache, StartedThreadPool()};
    // If the reservation were too small, this would abort
    const auto reset_guard{view.StartFetching(block)};
}

BOOST_AUTO_TEST_SUITE_END()

