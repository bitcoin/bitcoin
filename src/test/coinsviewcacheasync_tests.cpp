// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <coinsviewcacheasync.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <primitives/transaction_identifier.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/hasher.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <memory>
#include <ranges>
#include <unordered_set>

BOOST_AUTO_TEST_SUITE(coinsviewcacheasync_tests)

struct NoAccessCoinsView : CCoinsView {
    std::optional<Coin> GetCoin(const COutPoint&) const override { abort(); }
};

struct CoinsViewCacheAsyncTest : BasicTestingSetup {
private:
    std::unique_ptr<CoinsViewCacheAsync> m_async_cache{nullptr};
    std::unique_ptr<CBlock> m_block{nullptr};

    CBlock CreateBlock(int32_t num_txs) const noexcept
    {
        CBlock block;
        CMutableTransaction coinbase;
        coinbase.vin.emplace_back();
        block.vtx.push_back(MakeTransactionRef(coinbase));

        Txid prevhash{Txid::FromUint256(uint256{1})};

        for (const auto i : std::views::iota(1, num_txs)) {
            CMutableTransaction tx;
            Txid txid;
            if (i % 3 == 0) {
                txid = Txid::FromUint256(uint256(i));
            } else if (i % 3 == 1) {
                txid = prevhash;
            } else {
                // Test shortid collisions
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

public:
    explicit CoinsViewCacheAsyncTest(const ChainType& chainType = ChainType::MAIN,
                              const TestOpts& opts = {})
        : BasicTestingSetup{chainType, opts}
    {
        SeedRandomForTest(SeedRand::FIXED_SEED);

        const auto num_txs{100};
        m_block = std::make_unique<CBlock>(CreateBlock(num_txs));
    }

    const CBlock& getBlock() const noexcept { return *m_block; }
};

void PopulateCache(const CBlock& block, CCoinsViewCache& cache, bool spent = false)
{
    for (const auto& tx : block.vtx) {
        for (const auto& in : tx->vin) {
            auto outpoint{in.prevout};
            Coin coin{};
            if (!spent) coin.out.nValue = 1;
            BOOST_CHECK_EQUAL(coin.IsSpent(), spent);
            cache.EmplaceCoinInternalDANGER(std::move(outpoint), std::move(coin));
        }
    }
}

void CheckCache(const CBlock& block, const CCoinsViewCache& cache)
{
    uint32_t counter{0};
    std::unordered_set<Txid, SaltedTxidHasher> txids{};
    txids.reserve(block.vtx.size() - 1);

    for (const auto& tx : block.vtx) {
        if (tx->IsCoinBase()) {
            BOOST_CHECK(!cache.GetPossiblySpentCoinFromCache(tx->vin[0].prevout));
        } else {
            for (const auto& in : tx->vin) {
                const auto& outpoint{in.prevout};
                const auto should_have{!txids.contains(outpoint.hash)};
                if (should_have) {
                    cache.AccessCoin(outpoint);
                    ++counter;
                }
                const auto have{cache.GetPossiblySpentCoinFromCache(outpoint)};
                BOOST_CHECK_EQUAL(should_have, !!have);
            }
            txids.emplace(tx->GetHash());
        }
    }
    BOOST_CHECK(cache.GetCacheSize() == counter);
}


BOOST_FIXTURE_TEST_CASE(fetch_inputs_from_db, CoinsViewCacheAsyncTest)
{
    const auto& block{getBlock()};
    NoAccessCoinsView dummy;
    CCoinsViewCache db{&dummy};
    PopulateCache(block, db);
    CCoinsViewCache main_cache{&dummy};
    CoinsViewCacheAsync view{main_cache, db};
    for (auto i{0}; i < 3; ++i) {
        view.StartFetching(block);
        CheckCache(block, view);
        view.Reset();
    }
}

BOOST_FIXTURE_TEST_CASE(fetch_inputs_from_cache, CoinsViewCacheAsyncTest)
{
    const auto& block{getBlock()};
    NoAccessCoinsView dummy;
    const CCoinsViewCache db{&dummy};
    CCoinsViewCache main_cache{&dummy};
    PopulateCache(block, main_cache);
    CoinsViewCacheAsync view{main_cache, db};
    for (auto i{0}; i < 3; ++i) {
        view.StartFetching(block);
        CheckCache(block, view);
        view.Reset();
    }
}

// Test for the case where a block spends coins that are spent in the cache, but
// the spentness has not been flushed to the db.
BOOST_FIXTURE_TEST_CASE(fetch_no_double_spend, CoinsViewCacheAsyncTest)
{
    const auto& block{getBlock()};
    NoAccessCoinsView dummy;
    CCoinsViewCache db{&dummy};
    PopulateCache(block, db);
    CCoinsViewCache main_cache{&dummy};
    // Add all inputs as spent already in cache
    PopulateCache(block, main_cache, /*spent=*/true);
    CoinsViewCacheAsync view{main_cache, db};
    for (auto i{0}; i < 3; ++i) {
        view.StartFetching(block);
        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) view.AccessCoin(in.prevout);
        }
        // Coins are not added to the view, even though they exist unspent in the parent db
        BOOST_CHECK_EQUAL(view.GetCacheSize(), 0);
        view.Reset();
    }
}

BOOST_FIXTURE_TEST_CASE(fetch_no_inputs, CoinsViewCacheAsyncTest)
{
    const auto& block{getBlock()};
    CCoinsView db;
    CCoinsViewCache main_cache(&db);
    CoinsViewCacheAsync view{main_cache, db};
    for (auto i{0}; i < 3; ++i) {
        view.StartFetching(block);
        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) view.AccessCoin(in.prevout);
        }
        BOOST_CHECK_EQUAL(view.GetCacheSize(), 0);
        view.Reset();
    }
}

BOOST_AUTO_TEST_SUITE_END()
