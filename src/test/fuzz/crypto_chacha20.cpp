// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    ChaCha20 chacha20;
    if (fuzzed_data_provider.ConsumeBool()) {
        const std::vector<unsigned char> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(16, 32));
        chacha20 = ChaCha20{key.data(), key.size()};
    }
    while (fuzzed_data_provider.ConsumeBool()) {
        switch (fuzzed_data_provider.ConsumeIntegralInRange(0, 4)) {
        case 0: {
            const std::vector<unsigned char> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(16, 32));
            chacha20.SetKey(key.data(), key.size());
            break;
        }
        case 1: {
            chacha20.SetIV(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            break;
        }
        case 2: {
            chacha20.Seek(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            break;
        }
        case 3: {
            std::vector<uint8_t> output(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
            chacha20.Keystream(output.data(), output.size());
            break;
        }
        case 4: {
            std::vector<uint8_t> output(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
            const std::vector<uint8_t> input = ConsumeFixedLengthByteVector(fuzzed_data_provider, output.size());
            chacha20.Crypt(input.data(), output.data(), input.size());
            break;
        }
        }
    }
}
