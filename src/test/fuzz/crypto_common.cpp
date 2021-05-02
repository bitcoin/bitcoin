// Copyright (c) 2020 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/common.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const uint16_t random_u16 = fuzzed_data_provider.ConsumeIntegral<uint16_t>();
    const uint32_t random_u32 = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    const uint64_t random_u64 = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
    const std::vector<uint8_t> random_bytes_2 = ConsumeFixedLengthByteVector(fuzzed_data_provider, 2);
    const std::vector<uint8_t> random_bytes_4 = ConsumeFixedLengthByteVector(fuzzed_data_provider, 4);
    const std::vector<uint8_t> random_bytes_8 = ConsumeFixedLengthByteVector(fuzzed_data_provider, 8);

    std::array<uint8_t, 2> writele16_arr;
    WriteLE16(writele16_arr.data(), random_u16);
    assert(ReadLE16(writele16_arr.data()) == random_u16);

    std::array<uint8_t, 4> writele32_arr;
    WriteLE32(writele32_arr.data(), random_u32);
    assert(ReadLE32(writele32_arr.data()) == random_u32);

    std::array<uint8_t, 8> writele64_arr;
    WriteLE64(writele64_arr.data(), random_u64);
    assert(ReadLE64(writele64_arr.data()) == random_u64);

    std::array<uint8_t, 4> writebe32_arr;
    WriteBE32(writebe32_arr.data(), random_u32);
    assert(ReadBE32(writebe32_arr.data()) == random_u32);

    std::array<uint8_t, 8> writebe64_arr;
    WriteBE64(writebe64_arr.data(), random_u64);
    assert(ReadBE64(writebe64_arr.data()) == random_u64);

    const uint16_t readle16_result = ReadLE16(random_bytes_2.data());
    std::array<uint8_t, 2> readle16_arr;
    WriteLE16(readle16_arr.data(), readle16_result);
    assert(std::memcmp(random_bytes_2.data(), readle16_arr.data(), 2) == 0);

    const uint32_t readle32_result = ReadLE32(random_bytes_4.data());
    std::array<uint8_t, 4> readle32_arr;
    WriteLE32(readle32_arr.data(), readle32_result);
    assert(std::memcmp(random_bytes_4.data(), readle32_arr.data(), 4) == 0);

    const uint64_t readle64_result = ReadLE64(random_bytes_8.data());
    std::array<uint8_t, 8> readle64_arr;
    WriteLE64(readle64_arr.data(), readle64_result);
    assert(std::memcmp(random_bytes_8.data(), readle64_arr.data(), 8) == 0);

    const uint32_t readbe32_result = ReadBE32(random_bytes_4.data());
    std::array<uint8_t, 4> readbe32_arr;
    WriteBE32(readbe32_arr.data(), readbe32_result);
    assert(std::memcmp(random_bytes_4.data(), readbe32_arr.data(), 4) == 0);

    const uint64_t readbe64_result = ReadBE64(random_bytes_8.data());
    std::array<uint8_t, 8> readbe64_arr;
    WriteBE64(readbe64_arr.data(), readbe64_result);
    assert(std::memcmp(random_bytes_8.data(), readbe64_arr.data(), 8) == 0);
}
