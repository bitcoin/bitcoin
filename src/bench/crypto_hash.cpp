// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <bench/bench.h>
#include <crypto/muhash.h>
#include <crypto/ripemd160.h>
#include <crypto/sha1.h>
#include <crypto/sha256.h>
#include <crypto/sha3.h>
#include <crypto/sha512.h>
#include <crypto/siphash.h>
#include <random.h>
#include <span.h>
#include <tinyformat.h>
#include <uint256.h>

#include <cstdint>
#include <vector>

/* Number of bytes to hash per iteration */
static const uint64_t BUFFER_SIZE = 1000*1000;

static void BenchRIPEMD160(benchmark::Bench& bench)
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

static void SHA256_STANDARD(benchmark::Bench& bench)
{
    bench.name(strprintf("%s using the '%s' SHA256 implementation", __func__, SHA256AutoDetect(sha256_implementation::STANDARD)));
    uint8_t hash[CSHA256::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CSHA256().Write(in.data(), in.size()).Finalize(hash);
    });
    SHA256AutoDetect();
}

static void SHA256_SSE4(benchmark::Bench& bench)
{
    bench.name(strprintf("%s using the '%s' SHA256 implementation", __func__, SHA256AutoDetect(sha256_implementation::USE_SSE4)));
    uint8_t hash[CSHA256::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CSHA256().Write(in.data(), in.size()).Finalize(hash);
    });
    SHA256AutoDetect();
}

static void SHA256_AVX2(benchmark::Bench& bench)
{
    bench.name(strprintf("%s using the '%s' SHA256 implementation", __func__, SHA256AutoDetect(sha256_implementation::USE_SSE4_AND_AVX2)));
    uint8_t hash[CSHA256::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CSHA256().Write(in.data(), in.size()).Finalize(hash);
    });
    SHA256AutoDetect();
}

static void SHA256_SHANI(benchmark::Bench& bench)
{
    bench.name(strprintf("%s using the '%s' SHA256 implementation", __func__, SHA256AutoDetect(sha256_implementation::USE_SSE4_AND_SHANI)));
    uint8_t hash[CSHA256::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CSHA256().Write(in.data(), in.size()).Finalize(hash);
    });
    SHA256AutoDetect();
}

static void SHA3_256_1M(benchmark::Bench& bench)
{
    uint8_t hash[SHA3_256::OUTPUT_SIZE];
    std::vector<uint8_t> in(BUFFER_SIZE,0);
    bench.batch(in.size()).unit("byte").run([&] {
        SHA3_256().Write(in).Finalize(hash);
    });
}

static void SHA256_32b_STANDARD(benchmark::Bench& bench)
{
    bench.name(strprintf("%s using the '%s' SHA256 implementation", __func__, SHA256AutoDetect(sha256_implementation::STANDARD)));
    std::vector<uint8_t> in(32,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CSHA256()
            .Write(in.data(), in.size())
            .Finalize(in.data());
    });
    SHA256AutoDetect();
}

static void SHA256_32b_SSE4(benchmark::Bench& bench)
{
    bench.name(strprintf("%s using the '%s' SHA256 implementation", __func__, SHA256AutoDetect(sha256_implementation::USE_SSE4)));
    std::vector<uint8_t> in(32,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CSHA256()
            .Write(in.data(), in.size())
            .Finalize(in.data());
    });
    SHA256AutoDetect();
}

static void SHA256_32b_AVX2(benchmark::Bench& bench)
{
    bench.name(strprintf("%s using the '%s' SHA256 implementation", __func__, SHA256AutoDetect(sha256_implementation::USE_SSE4_AND_AVX2)));
    std::vector<uint8_t> in(32,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CSHA256()
            .Write(in.data(), in.size())
            .Finalize(in.data());
    });
    SHA256AutoDetect();
}

static void SHA256_32b_SHANI(benchmark::Bench& bench)
{
    bench.name(strprintf("%s using the '%s' SHA256 implementation", __func__, SHA256AutoDetect(sha256_implementation::USE_SSE4_AND_SHANI)));
    std::vector<uint8_t> in(32,0);
    bench.batch(in.size()).unit("byte").run([&] {
        CSHA256()
            .Write(in.data(), in.size())
            .Finalize(in.data());
    });
    SHA256AutoDetect();
}

static void SHA256D64_1024_STANDARD(benchmark::Bench& bench)
{
    bench.name(strprintf("%s using the '%s' SHA256 implementation", __func__, SHA256AutoDetect(sha256_implementation::STANDARD)));
    std::vector<uint8_t> in(64 * 1024, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        SHA256D64(in.data(), in.data(), 1024);
    });
    SHA256AutoDetect();
}

static void SHA256D64_1024_SSE4(benchmark::Bench& bench)
{
    bench.name(strprintf("%s using the '%s' SHA256 implementation", __func__, SHA256AutoDetect(sha256_implementation::USE_SSE4)));
    std::vector<uint8_t> in(64 * 1024, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        SHA256D64(in.data(), in.data(), 1024);
    });
    SHA256AutoDetect();
}

static void SHA256D64_1024_AVX2(benchmark::Bench& bench)
{
    bench.name(strprintf("%s using the '%s' SHA256 implementation", __func__, SHA256AutoDetect(sha256_implementation::USE_SSE4_AND_AVX2)));
    std::vector<uint8_t> in(64 * 1024, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        SHA256D64(in.data(), in.data(), 1024);
    });
    SHA256AutoDetect();
}

static void SHA256D64_1024_SHANI(benchmark::Bench& bench)
{
    bench.name(strprintf("%s using the '%s' SHA256 implementation", __func__, SHA256AutoDetect(sha256_implementation::USE_SSE4_AND_SHANI)));
    std::vector<uint8_t> in(64 * 1024, 0);
    bench.batch(in.size()).unit("byte").run([&] {
        SHA256D64(in.data(), in.data(), 1024);
    });
    SHA256AutoDetect();
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
    FastRandomContext rng{/*fDeterministic=*/true};
    PresaltedSipHasher presalted_sip_hasher(rng.rand64(), rng.rand64());
    auto val{rng.rand256()};
    auto i{0U};
    bench.run([&] {
        ankerl::nanobench::doNotOptimizeAway(presalted_sip_hasher(val));
        ++i;
        val.data()[i % uint256::size()] ^= i & 0xFF;
    });
}

static void MuHash(benchmark::Bench& bench)
{
    MuHash3072 acc;
    unsigned char key[32] = {0};
    uint32_t i = 0;
    bench.run([&] {
        key[0] = ++i & 0xFF;
        acc *= MuHash3072(key);
    });
}

static void MuHashMul(benchmark::Bench& bench)
{
    MuHash3072 acc;
    FastRandomContext rng(true);
    MuHash3072 muhash{rng.randbytes(32)};

    bench.run([&] {
        acc *= muhash;
    });
}

static void MuHashDiv(benchmark::Bench& bench)
{
    MuHash3072 acc;
    FastRandomContext rng(true);
    MuHash3072 muhash{rng.randbytes(32)};

    bench.run([&] {
        acc /= muhash;
    });
}

static void MuHashPrecompute(benchmark::Bench& bench)
{
    MuHash3072 acc;
    FastRandomContext rng(true);
    std::vector<unsigned char> key{rng.randbytes(32)};

    bench.run([&] {
        MuHash3072{key};
    });
}

static void MuHashFinalize(benchmark::Bench& bench)
{
    FastRandomContext rng(true);
    MuHash3072 acc{rng.randbytes(32)};
    acc /= MuHash3072{rng.rand256()};

    bench.run([&] {
        uint256 out;
        acc.Finalize(out);
        acc /= MuHash3072{out};
    });
}

BENCHMARK(BenchRIPEMD160, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA1, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256_STANDARD, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256_SSE4, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256_AVX2, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256_SHANI, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA512, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA3_256_1M, benchmark::PriorityLevel::HIGH);

BENCHMARK(SHA256_32b_STANDARD, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256_32b_SSE4, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256_32b_AVX2, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256_32b_SHANI, benchmark::PriorityLevel::HIGH);
BENCHMARK(SipHash_32b, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256D64_1024_STANDARD, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256D64_1024_SSE4, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256D64_1024_AVX2, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256D64_1024_SHANI, benchmark::PriorityLevel::HIGH);

BENCHMARK(MuHash, benchmark::PriorityLevel::HIGH);
BENCHMARK(MuHashMul, benchmark::PriorityLevel::HIGH);
BENCHMARK(MuHashDiv, benchmark::PriorityLevel::HIGH);
BENCHMARK(MuHashPrecompute, benchmark::PriorityLevel::HIGH);
BENCHMARK(MuHashFinalize, benchmark::PriorityLevel::HIGH);
