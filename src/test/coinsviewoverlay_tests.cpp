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
#include <vector>

namespace {

std::shared_ptr<ThreadPool> MakeStartedThreadPool()
{
    auto pool{std::make_shared<ThreadPool>("fetch_test")};
    pool->Start(DEFAULT_PREVOUTFETCH_THREADS);
    return pool;
}

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
        const Txid txid{i % 20 == 0 ? prevhash : Txid::FromUint256(uint256(i))};
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
                const auto have{cache.HaveCoinInCache(outpoint)};
                BOOST_CHECK_NE(txids.contains(outpoint.hash), have);
                counter += have;
            }
            txids.emplace(tx->GetHash());
        }
    }
    BOOST_CHECK_EQUAL(cache.GetCacheSize(), counter);
}

} // namespace

BOOST_AUTO_TEST_SUITE(coinsviewoverlay_tests)

BOOST_AUTO_TEST_CASE(fetch_inputs_from_db)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    PopulateView(block, db);
    CCoinsViewCache main_cache{&db};
    CoinsViewOverlay view{&main_cache, MakeStartedThreadPool()};
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
    CoinsViewOverlay view{&main_cache, MakeStartedThreadPool()};
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
    CoinsViewOverlay view{&main_cache, MakeStartedThreadPool()};
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
    CoinsViewOverlay view{&main_cache, MakeStartedThreadPool()};
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
    CBlock block;
    CMutableTransaction coinbase;
    coinbase.vin.emplace_back();
    block.vtx.push_back(MakeTransactionRef(coinbase));
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    Coin coin{};
    coin.out.nValue = 1;
    const COutPoint outpoint{Txid::FromUint256(uint256::ZERO), 0};
    main_cache.EmplaceCoinInternalDANGER(COutPoint{outpoint}, std::move(coin));

    CoinsViewOverlay view{&main_cache, MakeStartedThreadPool()};
    const auto reset_guard{view.StartFetching(block)};

    // Non-input fallback hit.
    BOOST_CHECK(!view.AccessCoin(outpoint).IsSpent());

    // Non-input fallback miss.
    const COutPoint missing_outpoint{Txid::FromUint256(uint256::ONE), 0};
    BOOST_CHECK(view.AccessCoin(missing_outpoint).IsSpent());
    BOOST_CHECK(!view.HaveCoinInCache(missing_outpoint));
}

// Access a fetched input out of order (i.e. not the next one in m_inputs).
// FetchCoinFromBase must fall back to base->PeekCoin, and the coin must still
// be inserted into the cache.
BOOST_AUTO_TEST_CASE(fetch_out_of_order_input_uses_normal_lookup)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    PopulateView(block, main_cache);

    std::vector<COutPoint> fetched_inputs;
    std::unordered_set<Txid, SaltedTxidHasher> txids;
    txids.reserve(block.vtx.size() - 1);
    for (const auto& tx : block.vtx | std::views::drop(1)) {
        for (const auto& input : tx->vin) {
            if (!txids.contains(input.prevout.hash)) fetched_inputs.push_back(input.prevout);
        }
        txids.emplace(tx->GetHash());
    }
    BOOST_REQUIRE_GE(fetched_inputs.size(), 2U);

    CoinsViewOverlay view{&main_cache, MakeStartedThreadPool()};
    const auto reset_guard{view.StartFetching(block)};

    const auto& out_of_order_input{fetched_inputs[1]};
    BOOST_CHECK(!view.HaveCoinInCache(out_of_order_input));
    BOOST_CHECK(!view.AccessCoin(out_of_order_input).IsSpent());
    BOOST_CHECK(view.HaveCoinInCache(out_of_order_input));

    CheckCache(block, view);
}

// The ResetGuard returned by StartFetching must clear all per-block state when
// it goes out of scope, so the overlay can be reused for a subsequent block.
// Flush must also clear all per-block state to be reused.
BOOST_AUTO_TEST_CASE(fetch_state_is_reusable_after_teardown)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    PopulateView(block, main_cache);
    CoinsViewOverlay view{&main_cache, MakeStartedThreadPool()};

    for (const bool use_flush : {false, true, false}) {
        {
            const auto reset_guard{view.StartFetching(block)};
            CheckCache(block, view);
            BOOST_CHECK_GT(view.GetCacheSize(), 0U);
            if (use_flush) {
                view.SetBestBlock(uint256::ONE);
                view.Flush();
            }
        }
        BOOST_CHECK_EQUAL(view.GetCacheSize(), 0U);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(coinsviewoverlay_tests_noworkers)

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

// Test that an interrupted thread pool falls back to normal cache lookups via base->PeekCoin.
BOOST_AUTO_TEST_CASE(fetch_interrupted_thread_pool_uses_normal_lookup)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    PopulateView(block, main_cache);

    auto thread_pool{std::make_shared<ThreadPool>("fetch_intr")};
    thread_pool->Start(DEFAULT_PREVOUTFETCH_THREADS);
    thread_pool->Interrupt();
    CoinsViewOverlay view{&main_cache, thread_pool};
    const auto reset_guard{view.StartFetching(block)};
    CheckCache(block, view);
}

BOOST_AUTO_TEST_SUITE_END()
