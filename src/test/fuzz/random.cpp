// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

using namespace std::chrono_literals;

FUZZ_TARGET(random)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    FastRandomContext fast_random_context{ConsumeUInt256(fuzzed_data_provider)};
    (void)fast_random_context.rand64();
    (void)fast_random_context.randbits(fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 64));
    (void)fast_random_context.randrange(fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(FastRandomContext::min() + 1, FastRandomContext::max()));
    (void)fast_random_context.randbytes(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 1024));
    (void)fast_random_context.rand32();
    (void)fast_random_context.rand256();
    (void)fast_random_context.randbool();
    (void)fast_random_context();

    {
        std::vector integrals{ConsumeRandomLengthIntegralVector<int64_t>(fuzzed_data_provider)};
        std::shuffle(integrals.begin(), integrals.end(), fast_random_context);
        std::ranges::shuffle(integrals, fast_random_context);
    }

    {
        const int bits{fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 10)};
        const uint64_t v{fast_random_context.randbits(bits)};
        assert(v < (1ULL << bits));
    }

    {
        const size_t len{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 1024)};
        const auto v{fast_random_context.randbytes(len)};
        assert(v.size() == len);
    }

    {
        const int64_t v{fast_random_context.rand<int64_t>()};
        assert(v >= 0);
    }

    {
        const uint64_t range{fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(1, std::numeric_limits<uint64_t>::max())};
        const uint64_t v{fast_random_context.randrange(range)};
        assert(v < range);
    }

    {
        const auto dur{std::chrono::milliseconds{fuzzed_data_provider.ConsumeIntegralInRange<int>(1, 10'000)}};
        const auto v{fast_random_context.rand_uniform_duration<SteadyMilliseconds>(dur)};
        assert(v >= 0ms && v < dur);
    }

    {
        const auto min{std::chrono::seconds{fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 5'000)}};
        const auto max{min + std::chrono::seconds{fuzzed_data_provider.ConsumeIntegralInRange<int>(1, 5'000)}};
        const auto v{fast_random_context.randrange<std::chrono::seconds>(min, max)};
        assert(v >= min && v < max);
    }

    {
        const auto base{std::chrono::steady_clock::now()};
        const auto range{std::chrono::seconds{fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 3'600)}};
        const auto tp{fast_random_context.rand_uniform_delay(base, range)};
        assert(tp >= base && tp <= base + range);
    }
}
