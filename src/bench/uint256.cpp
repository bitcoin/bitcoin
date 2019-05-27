// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <uint256.h>


static void UInt256IsNull(benchmark::State& state)
{
    uint256 hash;
    while (state.KeepRunning()) {
        (void) hash.IsNull();
    }
}

BENCHMARK(UInt256IsNull, 600000 * 1000);
