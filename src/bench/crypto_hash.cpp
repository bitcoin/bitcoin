// Copyright (c) 2016-2020 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <bench/bench.h>
#include <crypto/ripemd160.h>
#include <crypto/sha1.h>
#include <crypto/sha256.h>
#include <crypto/sha3.h>
#include <crypto/sha512.h>
#include <crypto/siphash.h>
#include <hash.h>
#include <random.h>
#include <uint256.h>

/* Number of bytes to hash per iteration */
static const uint64_t BUFFER_SIZE = 1000*1000;

static void RIPEMD160(benchmark::Bench& bench)
{
    uint8_t hash[CRIPEMD160::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CRIPEMD160().Write(in.data(), in.size()).Finalize(hash);
    });
}

static void SHA1(benchmark::Bench& bench)
{
    uint8_t hash[CSHA1::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CSHA1().Write(in.data(), in.size()).Finalize(hash);
    });
}

static void SHA256(benchmark::Bench& bench)
{
    uint8_t hash[CSHA256::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CSHA256().Write(in.data(), in.size()).Finalize(hash);
    });
}

static void SHA3_256_1M(benchmark::Bench& bench)
{
    uint8_t hash[SHA3_256::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    bench.batch(in.size()).unit("byte").run([&] {
        SHA3_256().Write(in).Finalize(hash);
    });
}

static void SHA256_32b(benchmark::Bench& bench)
{
    std::vector<uint8_t> in(32,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CSHA256()
            .Write(in.data(), in.size())
            .Finalize(in.data());
    });
}

static void SHA256D64_1024(benchmark::Bench& bench)
{
    std::vector<uint8_t> in(64 * 1024, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        SHA256D64(in.data(), in.data(), 1024);
    });
}

static void SHA512(benchmark::Bench& bench)
{
    uint8_t hash[CSHA512::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CSHA512().Write(in.data(), in.size()).Finalize(hash);
    });
}

static void SipHash_32b(benchmark::Bench& bench)
{
    uint256 x;
    uint64_t k1 = 0;
    bench.run([&] {
        *((uint64_t*)x.begin()) = SipHashUint256(0, ++k1, x);
    });
}

static void FastRandom_32bit(benchmark::Bench& bench)
{
    FastRandomContext rng(true);
    bench.run([&] {
        rng.rand32();
    });
}

static void FastRandom_1bit(benchmark::Bench& bench)
{
    FastRandomContext rng(true);
    bench.run([&] {
        rng.randbool();
    });
}

BENCHMARK(RIPEMD160);
BENCHMARK(SHA1);
BENCHMARK(SHA256);
BENCHMARK(SHA512);
BENCHMARK(SHA3_256_1M);

BENCHMARK(SHA256_32b);
BENCHMARK(SipHash_32b);
BENCHMARK(SHA256D64_1024);
BENCHMARK(FastRandom_32bit);
BENCHMARK(FastRandom_1bit);
