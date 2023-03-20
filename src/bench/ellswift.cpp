// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <key.h>
#include <random.h>

#include <array>
#include <cstddef>

static void EllSwiftEncode(benchmark::Bench& bench)
{
    ECC_Start();

    CKey key;
    key.MakeNewKey(true);

    bench.batch(1).unit("pubkey").run([&] {
        std::array<std::byte, 32> rnd32;
        GetRandBytes({reinterpret_cast<unsigned char*>(rnd32.data()), 32});
        key.EllSwiftEncode(rnd32);
    });
    ECC_Stop();
}

BENCHMARK(EllSwiftEncode, benchmark::PriorityLevel::HIGH);
