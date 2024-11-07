// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>

#include <base58.h>
#include <psbt.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

using util::TrimString;
using util::TrimStringView;

FUZZ_TARGET(base_encode_decode)
{
    const std::string random_encoded_string(buffer.begin(), buffer.end());

    std::vector<unsigned char> decoded;
    if (DecodeBase58(random_encoded_string, decoded, 100)) {
        const std::string encoded_string = EncodeBase58(decoded);
        assert(encoded_string == TrimStringView(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    if (DecodeBase58Check(random_encoded_string, decoded, 100)) {
        const std::string encoded_string = EncodeBase58Check(decoded);
        assert(encoded_string == TrimString(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    auto result = DecodeBase32(random_encoded_string);
    if (result) {
        const std::string encoded_string = EncodeBase32(*result);
        assert(encoded_string == TrimStringView(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    result = DecodeBase64(random_encoded_string);
    if (result) {
        const std::string encoded_string = EncodeBase64(*result);
        assert(encoded_string == TrimString(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    PartiallySignedTransaction psbt;
    std::string error;
    (void)DecodeBase64PSBT(psbt, random_encoded_string, error);
}
