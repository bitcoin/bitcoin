// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>

#include <base58.h>
#include <psbt.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <string>
#include <vector>
#include <ranges>

using util::TrimStringView;

FUZZ_TARGET(base58_encode_decode)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const std::string random_string{provider.ConsumeRandomLengthString(1000)};
    const int max_ret_len{provider.ConsumeIntegralInRange<int>(-1, 1000)};

    // Decode/Encode roundtrip
    std::vector<unsigned char> decoded;
    if (DecodeBase58(random_string, decoded, max_ret_len)) {
        const auto encoded_string{EncodeBase58(decoded)};
        assert(encoded_string == TrimStringView(random_string));
        assert(encoded_string.empty() || !DecodeBase58(encoded_string, decoded, provider.ConsumeIntegralInRange<int>(0, decoded.size() - 1)));
    }
    // Encode/Decode roundtrip
    const auto encoded{EncodeBase58(buffer)};
    std::vector<unsigned char> roundtrip_decoded;
    assert(DecodeBase58(encoded, roundtrip_decoded, buffer.size())
        && std::ranges::equal(roundtrip_decoded, buffer));
}

FUZZ_TARGET(base58check_encode_decode)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    const std::string random_string{provider.ConsumeRandomLengthString(1000)};
    const int max_ret_len{provider.ConsumeIntegralInRange<int>(-1, 1000)};

    // Decode/Encode roundtrip
    std::vector<unsigned char> decoded;
    if (DecodeBase58Check(random_string, decoded, max_ret_len)) {
        const auto encoded_string{EncodeBase58Check(decoded)};
        assert(encoded_string == TrimStringView(random_string));
        assert(encoded_string.empty() || !DecodeBase58Check(encoded_string, decoded, provider.ConsumeIntegralInRange<int>(0, decoded.size() - 1)));
    }
    // Encode/Decode roundtrip
    const auto encoded{EncodeBase58Check(buffer)};
    std::vector<unsigned char> roundtrip_decoded;
    assert(DecodeBase58Check(encoded, roundtrip_decoded, buffer.size())
        && std::ranges::equal(roundtrip_decoded, buffer));
}

FUZZ_TARGET(base32_encode_decode)
{
    const std::string random_string{buffer.begin(), buffer.end()};

    // Decode/Encode roundtrip
    if (auto result{DecodeBase32(random_string)}) {
        const auto encoded_string{EncodeBase32(*result)};
        assert(encoded_string == ToLower(TrimStringView(random_string)));
    }
    // Encode/Decode roundtrip
    const auto encoded{EncodeBase32(buffer)};
    const auto decoded{DecodeBase32(encoded)};
    assert(decoded && std::ranges::equal(*decoded, buffer));
}

FUZZ_TARGET(base64_encode_decode)
{
    const std::string random_string{buffer.begin(), buffer.end()};

    // Decode/Encode roundtrip
    if (auto result{DecodeBase64(random_string)}) {
        const auto encoded_string{EncodeBase64(*result)};
        assert(encoded_string == TrimStringView(random_string));
    }
    // Encode/Decode roundtrip
    const auto encoded{EncodeBase64(buffer)};
    const auto decoded{DecodeBase64(encoded)};
    assert(decoded && std::ranges::equal(*decoded, buffer));
}

FUZZ_TARGET(psbt_base64_decode)
{
    const std::string random_string{buffer.begin(), buffer.end()};

    PartiallySignedTransaction psbt;
    std::string error;
    assert(DecodeBase64PSBT(psbt, random_string, error) == error.empty());
}
