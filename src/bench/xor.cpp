// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bench/bench.h>
#include <random.h>
#include <span.h>
#include <streams.h>

#include <cstddef>
#include <vector>

static void Xor(benchmark::Bench& bench)
{
    FastRandomContext frc{/*fDeterministic=*/true};
    auto data{frc.randbytes<std::byte>(1024)};
    auto key{frc.randbytes<std::byte>(31)};

    bench.batch(data.size()).unit("byte").run([&] {
        util::Xor(data, key);
    });
}

BENCHMARK(Xor, benchmark::PriorityLevel::HIGH);
