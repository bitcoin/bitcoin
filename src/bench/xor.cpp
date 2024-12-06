// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bench/bench.h>
#include <random.h>
#include <span.h>
#include <streams.h>

#include <cmath>
#include <cstddef>
#include <map>
#include <vector>

static void Xor(benchmark::Bench& bench)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    auto test_data{rng.randbytes<std::byte>(1 << 20)};

    std::vector key_bytes{rng.randbytes<std::byte>(8)};
    uint64_t key;
    std::memcpy(&key, key_bytes.data(), 8);

    size_t offset{0};
    bench.batch(test_data.size()).unit("byte").run([&] {
        util::Xor(test_data, key_bytes, offset++);
        ankerl::nanobench::doNotOptimizeAway(test_data);
    });
}

BENCHMARK(Xor, benchmark::PriorityLevel::HIGH);
