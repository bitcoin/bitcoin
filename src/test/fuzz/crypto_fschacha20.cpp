// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

FUZZ_TARGET(crypto_fschacha20)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    auto key = fuzzed_data_provider.ConsumeBytes<std::byte>(FSChaCha20::KEYLEN);
    key.resize(FSChaCha20::KEYLEN);

    auto fsc20 = FSChaCha20{key, fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, 1024)};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        auto input = fuzzed_data_provider.ConsumeBytes<std::byte>(fuzzed_data_provider.ConsumeIntegralInRange(0, 4096));
        std::vector<std::byte> output;
        output.resize(input.size());
        fsc20.Crypt(input, output);
    }
}
