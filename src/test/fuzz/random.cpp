// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

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

    std::vector<int64_t> integrals = ConsumeRandomLengthIntegralVector<int64_t>(fuzzed_data_provider);
    std::shuffle(integrals.begin(), integrals.end(), fast_random_context);
}
