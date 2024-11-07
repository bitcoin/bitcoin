// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <coins.h>
#include <common/system.h>
#include <inputfetcher.h>
#include <primitives/block.h>
#include <serialize.h>
#include <streams.h>
#include <util/time.h>

static constexpr auto DELAY{2ms};

//! Simulates a DB by adding a delay when calling GetCoin
class DelayedCoinsView : public CCoinsView
{
private:
    std::chrono::milliseconds m_delay;

public:
    DelayedCoinsView(std::chrono::milliseconds delay) : m_delay(delay) {}

    std::optional<Coin> GetCoin(const COutPoint&) const override
    {
        UninterruptibleSleep(m_delay);
        return Coin{};
    }

    bool BatchWrite(CoinsViewCacheCursor&, const uint256&) override { return true; }
};

static void InputFetcherBenchmark(benchmark::Bench& bench)
{
    DataStream stream{benchmark::data::block413567};
    CBlock block;
    stream >> TX_WITH_WITNESS(block);

    DelayedCoinsView db(DELAY);
    CCoinsViewCache cache(&db);

    InputFetcher fetcher{};

    bench.run([&] {
        const auto ok{cache.Flush()};
        assert(ok);
        fetcher.FetchInputs(cache, db, block);
    });
}

BENCHMARK(InputFetcherBenchmark, benchmark::PriorityLevel::HIGH);
