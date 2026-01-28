// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <pubkey.h>
#include <random.h>
#include <span.h>
#include <util/strencodings.h>

#include <cstdint>
#include <vector>

static void HexStrBench(benchmark::Bench& bench)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    auto data{rng.randbytes<uint8_t>(CPubKey::COMPRESSED_SIZE)};
    bench.run([&] {
        auto hex = HexStr(data);
        ankerl::nanobench::doNotOptimizeAway(hex);
    });
}

BENCHMARK(HexStrBench);
