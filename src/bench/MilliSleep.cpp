// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"
#include "main.h"
#include "utiltime.h"

static void Sleep100ms(benchmark::State& state)
{
    while (state.KeepRunning()) {
        MilliSleep(100);
    }
}

BENCHMARK(Sleep100ms);
