// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinsviewcacheasync.h>
#include <consensus/amount.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <primitives/transaction_identifier.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <txdb.h>
#include <uint256.h>
#include <util/hasher.h>

#include <cstdint>
#include <map>
#include <optional>
#include <unordered_set>
#include <utility>

std::optional<CoinsViewCacheAsync> g_async_cache{};
std::optional<CCoinsViewDB> g_db{};

static void setup_threadpool_test()
{
    LogInstance().DisableLogging();
    auto db_params = DBParams{
        .path = "",
        .cache_bytes = 1_MiB,
        .memory_only = true,
    };
    g_db.emplace(std::move(db_params), CoinsViewOptions{});
    CCoinsViewCache cache{nullptr};
    g_async_cache.emplace(cache, *g_db);
}

FUZZ_TARGET(coinsviewcacheasync, .init = setup_threadpool_test)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        CBlock block;
        Txid prevhash{Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider))};

        std::map<const COutPoint, const Coin> db_map{};
        std::map<const COutPoint, const Coin> cache_map{};
        std::vector<COutPoint> input_outpoints{};

        CCoinsViewCache main_cache(&*g_db);
        // Used for writing to the db and erasing between iterations
        CCoinsViewCache dummy_cache(&*g_db);
        dummy_cache.SetBestBlock(uint256::ONE);

        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
        {
            CMutableTransaction tx;

            LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
            {
                Txid txid;
                if (fuzzed_data_provider.ConsumeBool()) {
                    txid = Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider));
                } else if (fuzzed_data_provider.ConsumeBool()) {
                    txid = prevhash;
                } else {
                    // Test shortid collisions
                    uint256 u{ConsumeUInt256(fuzzed_data_provider)};
                    std::memcpy(u.begin(), prevhash.ToUint256().begin(), 8);
                    txid = Txid::FromUint256(u);
                }
                const auto index{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
                const COutPoint outpoint{txid, index};

                tx.vin.emplace_back(outpoint);

                if (fuzzed_data_provider.ConsumeBool()) {
                    Coin coin{};
                    coin.fCoinBase = fuzzed_data_provider.ConsumeBool();
                    coin.nHeight =
                        fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(
                            0, std::numeric_limits<int32_t>::max());
                    coin.out.nValue = ConsumeMoney(fuzzed_data_provider);
                    assert(!coin.IsSpent());
                    db_map.try_emplace(outpoint, coin);
                    dummy_cache.EmplaceCoinInternalDANGER(
                        COutPoint(outpoint),
                        std::move(coin));
                }

                // Add a different coin to the cache
                if (fuzzed_data_provider.ConsumeBool()) {
                    Coin coin{};
                    coin.fCoinBase = fuzzed_data_provider.ConsumeBool();
                    coin.nHeight =
                        fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(
                            0, std::numeric_limits<int32_t>::max());
                    coin.out.nValue =
                        fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(
                            -1, MAX_MONEY);
                    cache_map.try_emplace(outpoint, coin);
                    main_cache.EmplaceCoinInternalDANGER(
                        COutPoint(outpoint),
                        std::move(coin));
                }

                input_outpoints.emplace_back(outpoint);
            }

            prevhash = tx.GetHash();
            block.vtx.push_back(MakeTransactionRef(tx));
        }

        (void)dummy_cache.Sync();
        CoinsViewCacheAsync& cache(*g_async_cache);
        cache.SetBackend(main_cache);
        cache.StartFetching(block);

        std::unordered_set<COutPoint, SaltedOutpointHasher> outpoints_in_cache{};
        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), static_cast<uint32_t>(input_outpoints.size() * 10))
        {
            COutPoint outpoint;
            if (fuzzed_data_provider.ConsumeBool()) {
                const auto index{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, input_outpoints.size() - 1)};
                outpoint = input_outpoints[index];
            } else {
                const auto txid{Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider))};
                const auto index{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
                outpoint = COutPoint(txid, index);
            }
            cache.AccessCoin(outpoint);
            const auto coin{cache.GetPossiblySpentCoinFromCache(outpoint)};
            if (coin) {
                assert(!coin->IsSpent());
                outpoints_in_cache.emplace(outpoint);
            }
            const auto& db_it{db_map.find(outpoint)};
            const auto cache_it{cache_map.find(outpoint)};
            if (!coin) {
                // If we don't have a coin, then it's either spent in cache or missing
                const auto spent_cache_coin{cache_it != cache_map.end() && cache_it->second.IsSpent()};
                const auto no_coin{cache_it == cache_map.end() && db_it == db_map.end()};
                assert(spent_cache_coin || no_coin);
            } else if (cache_it != cache_map.end()) {
                // Make sure we have the main cache coin if it exists instead of db
                const auto& cache_coin{cache_it->second};
                assert(!cache_coin.IsSpent());
                assert(coin->fCoinBase == cache_coin.fCoinBase);
                assert(coin->nHeight == cache_coin.nHeight);
                assert(coin->out == cache_coin.out);
            } else {
                assert(db_it != db_map.end());
                // Check any coins not in the main cache are the same as the db
                const auto& db_coin{db_it->second};
                assert(coin->fCoinBase == db_coin.fCoinBase);
                assert(coin->nHeight == db_coin.nHeight);
                assert(coin->out == db_coin.out);
            }
        }
        assert(cache.GetCacheSize() == outpoints_in_cache.size());
        fuzzed_data_provider.ConsumeBool() ? (void)cache.Flush() : cache.Reset();
        for (const auto& pair : db_map) {
            dummy_cache.SpendCoin(pair.first);
        }
        (void)dummy_cache.Flush();
    }
}
