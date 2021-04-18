// Copyright (c) 2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/poly1305.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const std::vector<uint8_t> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, POLY1305_KEYLEN);
    const std::vector<uint8_t> in = ConsumeRandomLengthByteVector(fuzzed_data_provider);

    std::vector<uint8_t> tag_out(POLY1305_TAGLEN);
    poly1305_auth(tag_out.data(), in.data(), in.size(), key.data());
}
