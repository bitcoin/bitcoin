// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>

#include <base58.h>
#include <psbt.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <string>
#include <vector>

using util::TrimString;

FUZZ_TARGET(base58_encode_decode)
{
    std::string random_string(buffer.begin(), buffer.end());
    std::vector<unsigned char> random_data{ToByteVector(random_string)};

    // Decode/Encode roundtrip
    std::vector<unsigned char> decoded;
    if (DecodeBase58(random_string, decoded, 100)) {
        auto encoded_string{EncodeBase58(decoded)};
        assert(encoded_string == TrimString(random_string));
        assert(encoded_string.empty() || !DecodeBase58(encoded_string, decoded, decoded.size() - 1));
    }
    // Encode/Decode roundtrip
    auto encoded{EncodeBase58(random_data)};
    std::vector<unsigned char> roundtrip_decoded;
    assert(DecodeBase58(encoded, roundtrip_decoded, random_string.size())
        && roundtrip_decoded == random_data);
}

FUZZ_TARGET(base58check_encode_decode)
{
    std::string random_string(buffer.begin(), buffer.end());
    std::vector<unsigned char> random_data{ToByteVector(random_string)};

    // Decode/Encode roundtrip
    std::vector<unsigned char> decoded;
    if (DecodeBase58Check(random_string, decoded, 100)) {
        auto encoded_string{EncodeBase58Check(decoded)};
        assert(encoded_string == TrimString(random_string));
        assert(encoded_string.empty() || !DecodeBase58Check(encoded_string, decoded, decoded.size() - 1));
    }
    // Encode/Decode roundtrip
    auto encoded{EncodeBase58Check(random_data)};
    std::vector<unsigned char> roundtrip_decoded;
    assert(DecodeBase58Check(encoded, roundtrip_decoded, random_string.size())
        && roundtrip_decoded == random_data);
}

FUZZ_TARGET(base32_encode_decode)
{
    std::string random_string(buffer.begin(), buffer.end());
    std::vector<unsigned char> random_data{ToByteVector(random_string)};

    // Decode/Encode roundtrip
    if (auto result{DecodeBase32(random_string)}) {
        auto encoded_string{EncodeBase32(*result)};
        assert(encoded_string == ToLower(TrimString(random_string)));
    }
    // Encode/Decode roundtrip
    auto encoded{EncodeBase32(random_data)};
    auto decoded{DecodeBase32(encoded)};
    assert(decoded && random_data == *decoded);
}

FUZZ_TARGET(base64_encode_decode)
{
    std::string random_string(buffer.begin(), buffer.end());
    std::vector<unsigned char> random_data{ToByteVector(random_string)};

    // Decode/Encode roundtrip
    if (auto result{DecodeBase64(random_string)}) {
        auto encoded_string{EncodeBase64(*result)};
        assert(encoded_string == TrimString(random_string));
    }
    // Encode/Decode roundtrip
    auto encoded{EncodeBase64(random_data)};
    auto decoded{DecodeBase64(encoded)};
    assert(decoded && random_data == *decoded);
}

FUZZ_TARGET(psbt_base64_decode)
{
    std::string random_string(buffer.begin(), buffer.end());

    PartiallySignedTransaction psbt;
    std::string error;
    (void) DecodeBase64PSBT(psbt, random_string, error);
}
