// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <coinsviewcacheasync.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <primitives/transaction_identifier.h>
#include <txdb.h>
#include <uint256.h>
#include <util/byte_units.h>
#include <util/hasher.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <cstring>
#include <ranges>
#include <unordered_set>

BOOST_AUTO_TEST_SUITE(coinsviewcacheasync_tests)

static CBlock CreateBlock() noexcept
{
    static constexpr auto NUM_TXS{100};
    CBlock block;
    CMutableTransaction coinbase;
    coinbase.vin.emplace_back();
    block.vtx.push_back(MakeTransactionRef(coinbase));

    Txid prevhash{Txid::FromUint256(uint256{1})};

    for (const auto i : std::views::iota(1, NUM_TXS)) {
        CMutableTransaction tx;
        Txid txid;
        if (i % 3 == 0) {
            // External input
            txid = Txid::FromUint256(uint256(i));
        } else if (i % 3 == 1) {
            // Internal spend (prev tx)
            txid = prevhash;
        } else {
            // Test shortxid collisions (looks internal, but is external)
            uint256 u{};
            std::memcpy(u.begin(), prevhash.ToUint256().begin(), 8);
            txid = Txid::FromUint256(u);
        }
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
            if (!txids.contains(in.prevout.hash)) {
                Coin coin{};
                if (!spent) coin.out.nValue = 1;
                cache.EmplaceCoinInternalDANGER(COutPoint{in.prevout}, std::move(coin));
            }
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
                const auto should_have{!txids.contains(outpoint.hash)};
                if (should_have) ++counter;
                const auto have{cache.HaveCoinInCache(outpoint)};
                BOOST_CHECK_EQUAL(should_have, !!have);
            }
            txids.emplace(tx->GetHash());
        }
    }
    BOOST_CHECK_EQUAL(cache.GetCacheSize(), counter);
}

BOOST_AUTO_TEST_CASE(fetch_inputs_from_db)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    PopulateView(block, db);
    CCoinsViewCache main_cache{&db};
    CoinsViewCacheAsync view{&main_cache};
    for (auto i{0}; i < 3; ++i) {
        auto reset_guard{view.StartFetching(block)};
        CheckCache(block, view);
        // Check that no coins have been moved up to main cache from db
        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                BOOST_CHECK(!main_cache.HaveCoinInCache(in.prevout));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(fetch_inputs_from_cache)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    PopulateView(block, main_cache);
    CoinsViewCacheAsync view{&main_cache};
    for (auto i{0}; i < 3; ++i) {
        auto reset_guard{view.StartFetching(block)};
        CheckCache(block, view);
    }
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
    CoinsViewCacheAsync view{&main_cache};
    for (auto i{0}; i < 3; ++i) {
        auto reset_guard{view.StartFetching(block)};
        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                const auto& c{view.AccessCoin(in.prevout)};
                BOOST_CHECK(c.IsSpent());
            }
        }
        // Coins are not added to the view, even though they exist unspent in the parent db
        BOOST_CHECK_EQUAL(view.GetCacheSize(), 0);
    }
}

BOOST_AUTO_TEST_CASE(fetch_no_inputs)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    CoinsViewCacheAsync view{&main_cache};
    for (auto i{0}; i < 3; ++i) {
        auto reset_guard{view.StartFetching(block)};
        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                const auto& c{view.AccessCoin(in.prevout)};
                BOOST_CHECK(c.IsSpent());
            }
        }
        BOOST_CHECK_EQUAL(view.GetCacheSize(), 0);
    }
}

// Access coin that is not a block's input
BOOST_AUTO_TEST_CASE(access_non_input_coin)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    Coin coin{};
    coin.out.nValue = 1;
    const COutPoint outpoint{Txid::FromUint256(uint256::ZERO), 0};
    main_cache.EmplaceCoinInternalDANGER(COutPoint{Txid::FromUint256(uint256::ZERO), 0}, std::move(coin));
    CoinsViewCacheAsync view{&main_cache};
    for (auto i{0}; i < 3; ++i) {
        auto reset_guard{view.StartFetching(block)};
        const auto& accessed_coin{view.AccessCoin(outpoint)};
        BOOST_CHECK(!accessed_coin.IsSpent());
    }
}

// Test that the main thread can make progress with no workers
BOOST_AUTO_TEST_CASE(fetch_main_thread)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    PopulateView(block, main_cache);
    CoinsViewCacheAsync async_cache{&main_cache, /*deterministic=*/false, /*num_workers=*/0};
    for (auto i{0}; i < 3; ++i) {
        const auto reset_guard{async_cache.StartFetching(block)};
        CheckCache(block, async_cache);
    }
}

BOOST_AUTO_TEST_SUITE_END()
