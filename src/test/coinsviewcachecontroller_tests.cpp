// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <coinsviewcachecontroller.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <txdb.h>
#include <uint256.h>
#include <util/byte_units.h>

#include <boost/test/unit_test.hpp>

#include <ranges>

BOOST_AUTO_TEST_SUITE(coinsviewcachecontroller_tests)

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
        Txid txid{Txid::FromUint256(uint256(i))};
        tx.vin.emplace_back(txid, 0);
        prevhash = tx.GetHash();
        block.vtx.push_back(MakeTransactionRef(tx));
    }

    return block;
}

void PopulateView(const CBlock& block, CCoinsView& view)
{
    CCoinsViewCache cache{&view};
    cache.SetBestBlock(uint256::ONE);

    for (const auto& tx : block.vtx | std::views::drop(1)) {
        for (const auto& in : tx->vin) {
            Coin coin{};
            coin.out.nValue = 1;
            cache.EmplaceCoinInternalDANGER(COutPoint{in.prevout}, std::move(coin));
        }
    }

    cache.Flush();
}

// Test that Handle destruction resets the cache
BOOST_AUTO_TEST_CASE(handle_scope_resets_cache)
{
    const auto block{CreateBlock()};
    CCoinsViewDB db{{.path = "", .cache_bytes = 1_MiB, .memory_only = true}, {}};
    CCoinsViewCache main_cache{&db};
    PopulateView(block, main_cache);
    CoinsViewCacheController controller{&main_cache};

    CCoinsViewCache* first_cache_ptr{nullptr};
    {
        auto handle{controller.Start()};
        first_cache_ptr = &*handle;
        // Access some coins to populate the cache
        for (const auto& tx : block.vtx | std::views::drop(1)) {
            for (const auto& in : tx->vin) {
                (void)handle->AccessCoin(in.prevout);
            }
        }
        BOOST_CHECK_GT(handle->GetCacheSize(), 0);
    }

    {
        auto handle{controller.Start()};
        CCoinsViewCache* second_cache_ptr{&*handle};
        // Same cache instance is reused
        BOOST_CHECK_EQUAL(first_cache_ptr, second_cache_ptr);
        // Cache was reset when previous handle went out of scope
        BOOST_CHECK_EQUAL(handle->GetCacheSize(), 0);

        // Can still access coins after reset
        const auto& coin{handle->AccessCoin(block.vtx[1]->vin[0].prevout)};
        BOOST_CHECK(!coin.IsSpent());
        BOOST_CHECK_EQUAL(handle->GetCacheSize(), 1);
    }
}

BOOST_AUTO_TEST_SUITE_END()
