// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <hash_x11.h>

#include <crypto/x11/sph_blake.h>
#include <crypto/x11/sph_bmw.h>
#include <crypto/x11/sph_cubehash.h>
#include <crypto/x11/sph_echo.h>
#include <crypto/x11/sph_groestl.h>
#include <crypto/x11/sph_jh.h>
#include <crypto/x11/sph_keccak.h>
#include <crypto/x11/sph_luffa.h>
#include <crypto/x11/sph_shavite.h>
#include <crypto/x11/sph_simd.h>
#include <crypto/x11/sph_skein.h>

namespace {
//! Bytes to hash per iteration
static constexpr size_t BUFFER_SIZE{1000*1000};
//! Bytes required to represent 512-bit unsigned integers (uint512)
static constexpr size_t OUTPUT_SIZE{64};

inline void Pow_X11(benchmark::Bench& bench, const size_t bytes)
{
    uint256 hash{};
    std::vector<uint8_t> in(bytes, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        hash = HashX11(in.begin(), in.end());
    });
}

inline void Pow_Blake512(benchmark::Bench& bench, const size_t bytes)
{
    sph_blake512_context ctx;
    uint8_t hash[OUTPUT_SIZE];
    std::vector<uint8_t> in(bytes, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        sph_blake512_init(&ctx);
        sph_blake512(&ctx, in.data(), in.size());
        sph_blake512_close(&ctx, &hash);
    });
}

inline void Pow_Bmw512(benchmark::Bench& bench, const size_t bytes)
{
    sph_bmw512_context ctx;
    uint8_t hash[OUTPUT_SIZE];
    std::vector<uint8_t> in(bytes, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        sph_bmw512_init(&ctx);
        sph_bmw512(&ctx, in.data(), in.size());
        sph_bmw512_close(&ctx, &hash);
    });
}

inline void Pow_Cubehash512(benchmark::Bench& bench, const size_t bytes)
{
    sph_cubehash512_context ctx;
    uint8_t hash[OUTPUT_SIZE];
    std::vector<uint8_t> in(bytes, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        sph_cubehash512_init(&ctx);
        sph_cubehash512(&ctx, in.data(), in.size());
        sph_cubehash512_close(&ctx, &hash);
    });
}

inline void Pow_Echo512(benchmark::Bench& bench, const size_t bytes)
{
    sph_echo512_context ctx;
    uint8_t hash[OUTPUT_SIZE];
    std::vector<uint8_t> in(bytes, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        sph_echo512_init(&ctx);
        sph_echo512(&ctx, in.data(), in.size());
        sph_echo512_close(&ctx, &hash);
    });
}

inline void Pow_Groestl512(benchmark::Bench& bench, const size_t bytes)
{
    sph_groestl512_context ctx;
    uint8_t hash[OUTPUT_SIZE];
    std::vector<uint8_t> in(bytes, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        sph_groestl512_init(&ctx);
        sph_groestl512(&ctx, in.data(), in.size());
        sph_groestl512_close(&ctx, &hash);
    });
}

inline void Pow_Jh512(benchmark::Bench& bench, const size_t bytes)
{
    sph_jh512_context ctx;
    uint8_t hash[OUTPUT_SIZE];
    std::vector<uint8_t> in(bytes, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        sph_jh512_init(&ctx);
        sph_jh512(&ctx, in.data(), in.size());
        sph_jh512_close(&ctx, &hash);
    });
}

inline void Pow_Keccak512(benchmark::Bench& bench, const size_t bytes)
{
    sph_keccak512_context ctx;
    uint8_t hash[OUTPUT_SIZE];
    std::vector<uint8_t> in(bytes, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        sph_keccak512_init(&ctx);
        sph_keccak512(&ctx, in.data(), in.size());
        sph_keccak512_close(&ctx, &hash);
    });
}

inline void Pow_Luffa512(benchmark::Bench& bench, const size_t bytes)
{
    sph_luffa512_context ctx;
    uint8_t hash[OUTPUT_SIZE];
    std::vector<uint8_t> in(bytes, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        sph_luffa512_init(&ctx);
        sph_luffa512(&ctx, in.data(), in.size());
        sph_luffa512_close(&ctx, &hash);
    });
}

inline void Pow_Shavite512(benchmark::Bench& bench, const size_t bytes)
{
    sph_shavite512_context ctx;
    uint8_t hash[OUTPUT_SIZE];
    std::vector<uint8_t> in(bytes, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        sph_shavite512_init(&ctx);
        sph_shavite512(&ctx, in.data(), in.size());
        sph_shavite512_close(&ctx, &hash);
    });
}

inline void Pow_Simd512(benchmark::Bench& bench, const size_t bytes)
{
    sph_simd512_context ctx;
    uint8_t hash[OUTPUT_SIZE];
    std::vector<uint8_t> in(bytes, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        sph_simd512_init(&ctx);
        sph_simd512(&ctx, in.data(), in.size());
        sph_simd512_close(&ctx, &hash);
    });
}

inline void Pow_Skein512(benchmark::Bench& bench, const size_t bytes)
{
    sph_skein512_context ctx;
    uint8_t hash[OUTPUT_SIZE];
    std::vector<uint8_t> in(bytes, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        sph_skein512_init(&ctx);
        sph_skein512(&ctx, in.data(), in.size());
        sph_skein512_close(&ctx, &hash);
    });
}
} // anonymous namespace

static void Pow_X11_0032b(benchmark::Bench& bench) { return Pow_X11(bench, 32); }
static void Pow_X11_0080b(benchmark::Bench& bench) { return Pow_X11(bench, 80); }
static void Pow_X11_0128b(benchmark::Bench& bench) { return Pow_X11(bench, 128); }
static void Pow_X11_0512b(benchmark::Bench& bench) { return Pow_X11(bench, 512); }
static void Pow_X11_1024b(benchmark::Bench& bench) { return Pow_X11(bench, 1024); }
static void Pow_X11_2048b(benchmark::Bench& bench) { return Pow_X11(bench, 2048); }
static void Pow_X11_1M(benchmark::Bench& bench) { return Pow_X11(bench, BUFFER_SIZE); }

static void Pow_Blake512_0032b(benchmark::Bench& bench) { return Pow_Blake512(bench, 32); }
static void Pow_Blake512_0080b(benchmark::Bench& bench) { return Pow_Blake512(bench, 80); }
static void Pow_Blake512_0128b(benchmark::Bench& bench) { return Pow_Blake512(bench, 128); }
static void Pow_Blake512_0512b(benchmark::Bench& bench) { return Pow_Blake512(bench, 512); }
static void Pow_Blake512_1024b(benchmark::Bench& bench) { return Pow_Blake512(bench, 1024); }
static void Pow_Blake512_2048b(benchmark::Bench& bench) { return Pow_Blake512(bench, 2048); }
static void Pow_Blake512_1M(benchmark::Bench& bench) { return Pow_Blake512(bench, BUFFER_SIZE); }

static void Pow_Bmw512_0032b(benchmark::Bench& bench) { return Pow_Bmw512(bench, 32); }
static void Pow_Bmw512_0080b(benchmark::Bench& bench) { return Pow_Bmw512(bench, 80); }
static void Pow_Bmw512_0128b(benchmark::Bench& bench) { return Pow_Bmw512(bench, 128); }
static void Pow_Bmw512_0512b(benchmark::Bench& bench) { return Pow_Bmw512(bench, 512); }
static void Pow_Bmw512_1024b(benchmark::Bench& bench) { return Pow_Bmw512(bench, 1024); }
static void Pow_Bmw512_2048b(benchmark::Bench& bench) { return Pow_Bmw512(bench, 2048); }
static void Pow_Bmw512_1M(benchmark::Bench& bench) { return Pow_Bmw512(bench, BUFFER_SIZE); }

static void Pow_Cubehash512_0032b(benchmark::Bench& bench) { return Pow_Cubehash512(bench, 32); }
static void Pow_Cubehash512_0080b(benchmark::Bench& bench) { return Pow_Cubehash512(bench, 80); }
static void Pow_Cubehash512_0128b(benchmark::Bench& bench) { return Pow_Cubehash512(bench, 128); }
static void Pow_Cubehash512_0512b(benchmark::Bench& bench) { return Pow_Cubehash512(bench, 512); }
static void Pow_Cubehash512_1024b(benchmark::Bench& bench) { return Pow_Cubehash512(bench, 1024); }
static void Pow_Cubehash512_2048b(benchmark::Bench& bench) { return Pow_Cubehash512(bench, 2048); }
static void Pow_Cubehash512_1M(benchmark::Bench& bench) { return Pow_Cubehash512(bench, BUFFER_SIZE); }

static void Pow_Echo512_0032b(benchmark::Bench& bench) { return Pow_Echo512(bench, 32); }
static void Pow_Echo512_0080b(benchmark::Bench& bench) { return Pow_Echo512(bench, 80); }
static void Pow_Echo512_0128b(benchmark::Bench& bench) { return Pow_Echo512(bench, 128); }
static void Pow_Echo512_0512b(benchmark::Bench& bench) { return Pow_Echo512(bench, 512); }
static void Pow_Echo512_1024b(benchmark::Bench& bench) { return Pow_Echo512(bench, 1024); }
static void Pow_Echo512_2048b(benchmark::Bench& bench) { return Pow_Echo512(bench, 2048); }
static void Pow_Echo512_1M(benchmark::Bench& bench) { return Pow_Echo512(bench, BUFFER_SIZE); }

static void Pow_Groestl512_0032b(benchmark::Bench& bench) { return Pow_Groestl512(bench, 32); }
static void Pow_Groestl512_0080b(benchmark::Bench& bench) { return Pow_Groestl512(bench, 80); }
static void Pow_Groestl512_0128b(benchmark::Bench& bench) { return Pow_Groestl512(bench, 128); }
static void Pow_Groestl512_0512b(benchmark::Bench& bench) { return Pow_Groestl512(bench, 512); }
static void Pow_Groestl512_1024b(benchmark::Bench& bench) { return Pow_Groestl512(bench, 1024); }
static void Pow_Groestl512_2048b(benchmark::Bench& bench) { return Pow_Groestl512(bench, 2048); }
static void Pow_Groestl512_1M(benchmark::Bench& bench) { return Pow_Groestl512(bench, BUFFER_SIZE); }

static void Pow_Jh512_0032b(benchmark::Bench& bench) { return Pow_Jh512(bench, 32); }
static void Pow_Jh512_0080b(benchmark::Bench& bench) { return Pow_Jh512(bench, 80); }
static void Pow_Jh512_0128b(benchmark::Bench& bench) { return Pow_Jh512(bench, 128); }
static void Pow_Jh512_0512b(benchmark::Bench& bench) { return Pow_Jh512(bench, 512); }
static void Pow_Jh512_1024b(benchmark::Bench& bench) { return Pow_Jh512(bench, 1024); }
static void Pow_Jh512_2048b(benchmark::Bench& bench) { return Pow_Jh512(bench, 2048); }
static void Pow_Jh512_1M(benchmark::Bench& bench) { return Pow_Jh512(bench, BUFFER_SIZE); }

static void Pow_Keccak512_0032b(benchmark::Bench& bench) { return Pow_Keccak512(bench, 32); }
static void Pow_Keccak512_0080b(benchmark::Bench& bench) { return Pow_Keccak512(bench, 80); }
static void Pow_Keccak512_0128b(benchmark::Bench& bench) { return Pow_Keccak512(bench, 128); }
static void Pow_Keccak512_0512b(benchmark::Bench& bench) { return Pow_Keccak512(bench, 512); }
static void Pow_Keccak512_1024b(benchmark::Bench& bench) { return Pow_Keccak512(bench, 1024); }
static void Pow_Keccak512_2048b(benchmark::Bench& bench) { return Pow_Keccak512(bench, 2048); }
static void Pow_Keccak512_1M(benchmark::Bench& bench) { return Pow_Keccak512(bench, BUFFER_SIZE); }

static void Pow_Luffa512_0032b(benchmark::Bench& bench) { return Pow_Luffa512(bench, 32); }
static void Pow_Luffa512_0080b(benchmark::Bench& bench) { return Pow_Luffa512(bench, 80); }
static void Pow_Luffa512_0128b(benchmark::Bench& bench) { return Pow_Luffa512(bench, 128); }
static void Pow_Luffa512_0512b(benchmark::Bench& bench) { return Pow_Luffa512(bench, 512); }
static void Pow_Luffa512_1024b(benchmark::Bench& bench) { return Pow_Luffa512(bench, 1024); }
static void Pow_Luffa512_2048b(benchmark::Bench& bench) { return Pow_Luffa512(bench, 2048); }
static void Pow_Luffa512_1M(benchmark::Bench& bench) { return Pow_Luffa512(bench, BUFFER_SIZE); }

static void Pow_Shavite512_0032b(benchmark::Bench& bench) { return Pow_Shavite512(bench, 32); }
static void Pow_Shavite512_0080b(benchmark::Bench& bench) { return Pow_Shavite512(bench, 80); }
static void Pow_Shavite512_0128b(benchmark::Bench& bench) { return Pow_Shavite512(bench, 128); }
static void Pow_Shavite512_0512b(benchmark::Bench& bench) { return Pow_Shavite512(bench, 512); }
static void Pow_Shavite512_1024b(benchmark::Bench& bench) { return Pow_Shavite512(bench, 1024); }
static void Pow_Shavite512_2048b(benchmark::Bench& bench) { return Pow_Shavite512(bench, 2048); }
static void Pow_Shavite512_1M(benchmark::Bench& bench) { return Pow_Shavite512(bench, BUFFER_SIZE); }

static void Pow_Simd512_0032b(benchmark::Bench& bench) { return Pow_Simd512(bench, 32); }
static void Pow_Simd512_0080b(benchmark::Bench& bench) { return Pow_Simd512(bench, 80); }
static void Pow_Simd512_0128b(benchmark::Bench& bench) { return Pow_Simd512(bench, 128); }
static void Pow_Simd512_0512b(benchmark::Bench& bench) { return Pow_Simd512(bench, 512); }
static void Pow_Simd512_1024b(benchmark::Bench& bench) { return Pow_Simd512(bench, 1024); }
static void Pow_Simd512_2048b(benchmark::Bench& bench) { return Pow_Simd512(bench, 2048); }
static void Pow_Simd512_1M(benchmark::Bench& bench) { return Pow_Simd512(bench, BUFFER_SIZE); }

static void Pow_Skein512_0032b(benchmark::Bench& bench) { return Pow_Skein512(bench, 32); }
static void Pow_Skein512_0080b(benchmark::Bench& bench) { return Pow_Skein512(bench, 80); }
static void Pow_Skein512_0128b(benchmark::Bench& bench) { return Pow_Skein512(bench, 128); }
static void Pow_Skein512_0512b(benchmark::Bench& bench) { return Pow_Skein512(bench, 512); }
static void Pow_Skein512_1024b(benchmark::Bench& bench) { return Pow_Skein512(bench, 1024); }
static void Pow_Skein512_2048b(benchmark::Bench& bench) { return Pow_Skein512(bench, 2048); }
static void Pow_Skein512_1M(benchmark::Bench& bench) { return Pow_Skein512(bench, BUFFER_SIZE); }

BENCHMARK(Pow_X11_0032b);
BENCHMARK(Pow_X11_0080b);
BENCHMARK(Pow_X11_0128b);
BENCHMARK(Pow_X11_0512b);
BENCHMARK(Pow_X11_1024b);
BENCHMARK(Pow_X11_2048b);
BENCHMARK(Pow_X11_1M);

BENCHMARK(Pow_Blake512_0032b);
BENCHMARK(Pow_Blake512_0080b);
BENCHMARK(Pow_Blake512_0128b);
BENCHMARK(Pow_Blake512_0512b);
BENCHMARK(Pow_Blake512_1024b);
BENCHMARK(Pow_Blake512_2048b);
BENCHMARK(Pow_Blake512_1M);

BENCHMARK(Pow_Bmw512_0032b);
BENCHMARK(Pow_Bmw512_0080b);
BENCHMARK(Pow_Bmw512_0128b);
BENCHMARK(Pow_Bmw512_0512b);
BENCHMARK(Pow_Bmw512_1024b);
BENCHMARK(Pow_Bmw512_2048b);
BENCHMARK(Pow_Bmw512_1M);

BENCHMARK(Pow_Cubehash512_0032b);
BENCHMARK(Pow_Cubehash512_0080b);
BENCHMARK(Pow_Cubehash512_0128b);
BENCHMARK(Pow_Cubehash512_0512b);
BENCHMARK(Pow_Cubehash512_1024b);
BENCHMARK(Pow_Cubehash512_2048b);
BENCHMARK(Pow_Cubehash512_1M);

BENCHMARK(Pow_Echo512_0032b);
BENCHMARK(Pow_Echo512_0080b);
BENCHMARK(Pow_Echo512_0128b);
BENCHMARK(Pow_Echo512_0512b);
BENCHMARK(Pow_Echo512_1024b);
BENCHMARK(Pow_Echo512_2048b);
BENCHMARK(Pow_Echo512_1M);

BENCHMARK(Pow_Groestl512_0032b);
BENCHMARK(Pow_Groestl512_0080b);
BENCHMARK(Pow_Groestl512_0128b);
BENCHMARK(Pow_Groestl512_0512b);
BENCHMARK(Pow_Groestl512_1024b);
BENCHMARK(Pow_Groestl512_2048b);
BENCHMARK(Pow_Groestl512_1M);

BENCHMARK(Pow_Jh512_0032b);
BENCHMARK(Pow_Jh512_0080b);
BENCHMARK(Pow_Jh512_0128b);
BENCHMARK(Pow_Jh512_0512b);
BENCHMARK(Pow_Jh512_1024b);
BENCHMARK(Pow_Jh512_2048b);
BENCHMARK(Pow_Jh512_1M);

BENCHMARK(Pow_Keccak512_0032b);
BENCHMARK(Pow_Keccak512_0080b);
BENCHMARK(Pow_Keccak512_0128b);
BENCHMARK(Pow_Keccak512_0512b);
BENCHMARK(Pow_Keccak512_1024b);
BENCHMARK(Pow_Keccak512_2048b);
BENCHMARK(Pow_Keccak512_1M);

BENCHMARK(Pow_Luffa512_0032b);
BENCHMARK(Pow_Luffa512_0080b);
BENCHMARK(Pow_Luffa512_0128b);
BENCHMARK(Pow_Luffa512_0512b);
BENCHMARK(Pow_Luffa512_1024b);
BENCHMARK(Pow_Luffa512_2048b);
BENCHMARK(Pow_Luffa512_1M);

BENCHMARK(Pow_Shavite512_0032b);
BENCHMARK(Pow_Shavite512_0080b);
BENCHMARK(Pow_Shavite512_0128b);
BENCHMARK(Pow_Shavite512_0512b);
BENCHMARK(Pow_Shavite512_1024b);
BENCHMARK(Pow_Shavite512_2048b);
BENCHMARK(Pow_Shavite512_1M);

BENCHMARK(Pow_Simd512_0032b);
BENCHMARK(Pow_Simd512_0080b);
BENCHMARK(Pow_Simd512_0128b);
BENCHMARK(Pow_Simd512_0512b);
BENCHMARK(Pow_Simd512_1024b);
BENCHMARK(Pow_Simd512_2048b);
BENCHMARK(Pow_Simd512_1M);

BENCHMARK(Pow_Skein512_0032b);
BENCHMARK(Pow_Skein512_0080b);
BENCHMARK(Pow_Skein512_0128b);
BENCHMARK(Pow_Skein512_0512b);
BENCHMARK(Pow_Skein512_1024b);
BENCHMARK(Pow_Skein512_2048b);
BENCHMARK(Pow_Skein512_1M);
