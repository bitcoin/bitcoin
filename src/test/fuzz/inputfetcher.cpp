// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <inputfetcher.h>
#include <primitives/transaction_identifier.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>

#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>

using DbMap = std::map<const COutPoint, std::pair<std::optional<const Coin>, bool>>;

class DbCoinsView : public CCoinsView
{
private:
    DbMap& m_map;

public:
    DbCoinsView(DbMap& map) : m_map(map) {}

    std::optional<Coin> GetCoin(const COutPoint& outpoint) const override
    {
        const auto it{m_map.find(outpoint)};
        assert(it != m_map.end());
        const auto [coin, err] = it->second;
        if (err) {
            throw std::runtime_error("database error");
        }
        return coin;
    }
};

class NoAccessCoinsView : public CCoinsView
{
public:
    std::optional<Coin> GetCoin(const COutPoint&) const override
    {
        abort();
    }
};

FUZZ_TARGET(inputfetcher)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const auto worker_threads{
        fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(2, 4)};
    InputFetcher fetcher{static_cast<size_t>(worker_threads)};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        CBlock block;
        Txid prevhash{Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider))};

        DbMap db_map{};
        std::map<const COutPoint, const Coin> cache_map{};

        DbCoinsView db(db_map);

        NoAccessCoinsView back;
        CCoinsViewCache cache(&back);

        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
            CMutableTransaction tx;

            LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10) {
                const auto txid{fuzzed_data_provider.ConsumeBool()
                    ? Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider))
                    : prevhash};
                const auto index{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
                const COutPoint outpoint(txid, index);

                tx.vin.emplace_back(outpoint);

                std::optional<Coin> maybe_coin;
                if (fuzzed_data_provider.ConsumeBool()) {
                    Coin coin{};
                    coin.fCoinBase = fuzzed_data_provider.ConsumeBool();
                    coin.nHeight =
                        fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(
                            0, std::numeric_limits<int32_t>::max());
                    coin.out.nValue = ConsumeMoney(fuzzed_data_provider);
                    maybe_coin = coin;
                } else {
                    maybe_coin = std::nullopt;
                }
                db_map.try_emplace(outpoint, std::make_pair(
                    maybe_coin,
                    fuzzed_data_provider.ConsumeBool()));

                // Add the coin to the cache
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
                    cache.EmplaceCoinInternalDANGER(
                        COutPoint(outpoint),
                        std::move(coin));
                }
            }

            prevhash = tx.GetHash();
            block.vtx.push_back(MakeTransactionRef(tx));
        }

        fetcher.FetchInputs(cache, db, block);

        for (const auto& [outpoint, pair] : db_map) {
            // Check pre-existing coins in the cache have not been updated
            const auto it{cache_map.find(outpoint)};
            if (it != cache_map.end()) {
                const auto& cache_coin{it->second};
                const auto& coin{cache.AccessCoin(outpoint)};
                assert(coin.IsSpent() == cache_coin.IsSpent());
                assert(coin.fCoinBase == cache_coin.fCoinBase);
                assert(coin.nHeight == cache_coin.nHeight);
                assert(coin.out == cache_coin.out);
                continue;
            }

            if (!cache.HaveCoinInCache(outpoint)) {
                continue;
            }

            const auto& [maybe_coin, err] = pair;
            assert(maybe_coin && !err);

            // Check any newly added coins in the cache are the same as the db
            const auto& coin{cache.AccessCoin(outpoint)};
            assert(!coin.IsSpent());
            assert(coin.fCoinBase == maybe_coin->fCoinBase);
            assert(coin.nHeight == maybe_coin->nHeight);
            assert(coin.out == maybe_coin->out);
        }
    }
}
