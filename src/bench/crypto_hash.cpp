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
#include <tinyformat.h>
#include <uint256.h>
#include <primitives/transaction.h>
#include <util/hasher.h>
#include <unordered_set>

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
    uint256 x;
    uint64_t k1 = 0;
    bench.run([&] {
        *((uint64_t*)x.begin()) = SipHashUint256(0, ++k1, x);
    });
}

static void SaltedOutpointHasherBenchmark_hash(benchmark::Bench& bench)
{
    FastRandomContext rng(true);
    constexpr size_t size = 1000;

    std::vector<COutPoint> outpoints(size);
    for (auto& outpoint : outpoints) {
        outpoint = {Txid::FromUint256(rng.rand256()), rng.rand32()};
    }

    const SaltedOutpointHasher hasher;
    bench.batch(size).run([&] {
        size_t result = 0;
        for (const auto& outpoint : outpoints) {
            result ^= hasher(outpoint);
        }
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

static void SaltedOutpointHasherBenchmark_match(benchmark::Bench& bench)
{
    FastRandomContext rng(true);
    constexpr size_t size = 1000;

    std::unordered_set<COutPoint, SaltedOutpointHasher> values;
    std::vector<COutPoint> value_vector;
    values.reserve(size);
    value_vector.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        COutPoint outpoint{Txid::FromUint256(rng.rand256()), rng.rand32()};
        values.emplace(outpoint);
        value_vector.push_back(outpoint);
        assert(values.contains(outpoint));
    }

    bench.batch(size).run([&] {
        bool result = true;
        for (const auto& outpoint : value_vector) {
            result ^= values.contains(outpoint);
        }
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

static void SaltedOutpointHasherBenchmark_mismatch(benchmark::Bench& bench)
{
    FastRandomContext rng(true);
    constexpr size_t size = 1000;

    std::unordered_set<COutPoint, SaltedOutpointHasher> values;
    std::vector<COutPoint> missing_value_vector;
    values.reserve(size);
    missing_value_vector.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        values.emplace(Txid::FromUint256(rng.rand256()), rng.rand32());
        COutPoint missing_outpoint{Txid::FromUint256(rng.rand256()), rng.rand32()};
        missing_value_vector.push_back(missing_outpoint);
        assert(!values.contains(missing_outpoint));
    }

    bench.batch(size).run([&] {
        bool result = false;
        for (const auto& outpoint : missing_value_vector) {
            result ^= values.contains(outpoint);
        }
        ankerl::nanobench::doNotOptimizeAway(result);
    });
}

static void SaltedOutpointHasherBenchmark_create_set(benchmark::Bench& bench)
{
    FastRandomContext rng(true);
    constexpr size_t size = 1000;

    std::vector<COutPoint> outpoints(size);
    for (auto& outpoint : outpoints) {
        outpoint = {Txid::FromUint256(rng.rand256()), rng.rand32()};
    }

    bench.batch(size).run([&] {
        std::unordered_set<COutPoint, SaltedOutpointHasher> set;
        set.reserve(size);
        for (const auto& outpoint : outpoints) {
            set.emplace(outpoint);
        }
        ankerl::nanobench::doNotOptimizeAway(set);
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
BENCHMARK(SaltedOutpointHasherBenchmark_hash, benchmark::PriorityLevel::HIGH);
BENCHMARK(SaltedOutpointHasherBenchmark_match, benchmark::PriorityLevel::HIGH);
BENCHMARK(SaltedOutpointHasherBenchmark_mismatch, benchmark::PriorityLevel::HIGH);
BENCHMARK(SaltedOutpointHasherBenchmark_create_set, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256D64_1024_STANDARD, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256D64_1024_SSE4, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256D64_1024_AVX2, benchmark::PriorityLevel::HIGH);
BENCHMARK(SHA256D64_1024_SHANI, benchmark::PriorityLevel::HIGH);

BENCHMARK(MuHash, benchmark::PriorityLevel::HIGH);
BENCHMARK(MuHashMul, benchmark::PriorityLevel::HIGH);
BENCHMARK(MuHashDiv, benchmark::PriorityLevel::HIGH);
BENCHMARK(MuHashPrecompute, benchmark::PriorityLevel::HIGH);
