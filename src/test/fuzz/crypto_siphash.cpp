// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compat/endian.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cstdint>
#include <vector>

constexpr uint8_t sep = 0x07;

extern "C" {
int siphash(const uint8_t *in, size_t inlen, const uint8_t *k, uint8_t *out, size_t outlen);
}


FUZZ_TARGET(crypto_siphash)
{

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    uint64_t k[2] = {
        htole64(fuzzed_data_provider.ConsumeIntegral<uint64_t>()),
        htole64(fuzzed_data_provider.ConsumeIntegral<uint64_t>())
    };

    auto gradual_siphash = CSipHasher(k[0], k[1]);
    auto input = fuzzed_data_provider.ConsumeRemainingBytes<uint8_t>();

    auto iterators = std::vector<std::vector<uint8_t>::const_iterator>{};
    for (auto it = input.cbegin(); it != input.cend(); it = std::find(std::next(it), input.cend(), sep)) {
        iterators.push_back(it);
    }
    iterators.push_back(input.cend());

    for (size_t i = 0; i < iterators.size(); i++) {
        size_t size;
        if (i < iterators.size() - 1) {
            size = std::distance(iterators[i], iterators[i + 1]);
        } else {
            size = std::distance(iterators[i], input.cend());
        }
        gradual_siphash.Write(iterators[i].base(), size);
    }
    uint64_t our_gradual_hash = gradual_siphash.Finalize();
    uint64_t our_direct_hash = CSipHasher(k[0], k[1]).Write(input.data(), input.size()).Finalize();
    uint64_t ref_hash;
    siphash(input.data(), input.size(), reinterpret_cast<const uint8_t*>(&k[0]), reinterpret_cast<uint8_t*>(&ref_hash), sizeof(ref_hash));
    ref_hash = htole64(ref_hash);

    assert(ref_hash == our_gradual_hash);
    assert(our_direct_hash == our_gradual_hash);
}
