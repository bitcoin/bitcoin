// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <bench/bench.h>
#include <crypto/chacha20.h>

/* Number of bytes to process per iteration */
static const uint64_t BUFFER_SIZE_TINY  = 64;
static const uint64_t BUFFER_SIZE_SMALL = 256;
static const uint64_t BUFFER_SIZE_LARGE = 1024*1024;

static void CHACHA20(benchmark::Bench& bench, size_t buffersize)
{
    std::vector<uint8_t> key(32,0);
    ChaCha20 ctx(key.data(), key.size());
    ctx.SetIV(0);
    ctx.Seek(0);
    std::vector<uint8_t> in(buffersize,0);
    std::vector<uint8_t> out(buffersize,0);
    bench.batch(in.size()).unit("byte").run([&] {
        ctx.Crypt(in.data(), out.data(), in.size());
    });
}

static void CHACHA20_64BYTES(benchmark::Bench& bench)
{
    CHACHA20(bench, BUFFER_SIZE_TINY);
}

static void CHACHA20_256BYTES(benchmark::Bench& bench)
{
    CHACHA20(bench, BUFFER_SIZE_SMALL);
}

static void CHACHA20_1MB(benchmark::Bench& bench)
{
    CHACHA20(bench, BUFFER_SIZE_LARGE);
}

BENCHMARK(CHACHA20_64BYTES);
BENCHMARK(CHACHA20_256BYTES);
BENCHMARK(CHACHA20_1MB);
