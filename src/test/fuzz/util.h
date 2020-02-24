// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_H
#define BITCOIN_TEST_FUZZ_UTIL_H

#include <attributes.h>
#include <optional.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <version.h>

#include <cstdint>
#include <string>
#include <vector>

NODISCARD inline std::vector<uint8_t> ConsumeRandomLengthByteVector(FuzzedDataProvider& fuzzed_data_provider, size_t max_length = 4096) noexcept
{
    const std::string s = fuzzed_data_provider.ConsumeRandomLengthString(max_length);
    return {s.begin(), s.end()};
}

template <typename T>
NODISCARD inline Optional<T> ConsumeDeserializable(FuzzedDataProvider& fuzzed_data_provider, size_t max_length = 4096) noexcept
{
    const std::vector<uint8_t>& buffer = ConsumeRandomLengthByteVector(fuzzed_data_provider, max_length);
    CDataStream ds{buffer, SER_NETWORK, INIT_PROTO_VERSION};
    T obj;
    try {
        ds >> obj;
    } catch (const std::ios_base::failure&) {
        return nullopt;
    }
    return obj;
}

#endif // BITCOIN_TEST_FUZZ_UTIL_H
