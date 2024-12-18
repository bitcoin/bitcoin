// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bench/bench.h>
#include <random.h>
#include <span.h>
#include <streams.h>
#include <test/util/script.h>

#include <vector>

static void DataStreamAlloc(benchmark::Bench& bench)
{
    FastRandomContext frc{/*fDeterministic=*/true};
    // Mimicks the initialization of CNetMessage's m_recv in V2Transport::GetReceivedMessage
    std::vector<uint8_t> data{frc.randbytes<uint8_t>(8192)};
    Span<const uint8_t> data_view{data};

    bench.batch(data.size()).unit("byte").run([&] {
        DataStream stream{data_view};
        ankerl::nanobench::doNotOptimizeAway(stream);
    });
}

static void DataStreamSerializeScript(benchmark::Bench& bench)
{
    FastRandomContext frc{/*fDeterministic=*/true};
    auto script = RandScript(frc, /*length=*/1000);

    bench.batch(script.size()).unit("byte").run([&] {
        DataStream stream{};
        stream << script;
        ankerl::nanobench::doNotOptimizeAway(stream);
    });
}

BENCHMARK(DataStreamAlloc, benchmark::PriorityLevel::HIGH);
BENCHMARK(DataStreamSerializeScript, benchmark::PriorityLevel::HIGH);
