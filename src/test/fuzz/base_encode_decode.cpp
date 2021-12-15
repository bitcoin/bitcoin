// Copyright (c) 2019-2020 The Bitcoin Core developers
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

void initialize_base_encode_decode()
{
    static const ECCVerifyHandle verify_handle;
}

FUZZ_TARGET_INIT(base_encode_decode, initialize_base_encode_decode)
{
    const std::string random_encoded_string(buffer.begin(), buffer.end());

    std::vector<unsigned char> decoded;
    if (DecodeBase58(random_encoded_string, decoded, 100)) {
        const std::string encoded_string = EncodeBase58(decoded);
        assert(encoded_string == TrimString(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    if (DecodeBase58Check(random_encoded_string, decoded, 100)) {
        const std::string encoded_string = EncodeBase58Check(decoded);
        assert(encoded_string == TrimString(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    bool pf_invalid;
    std::string decoded_string = DecodeBase32(random_encoded_string, &pf_invalid);
    if (!pf_invalid) {
        const std::string encoded_string = EncodeBase32(decoded_string);
        assert(encoded_string == TrimString(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    decoded_string = DecodeBase64(random_encoded_string, &pf_invalid);
    if (!pf_invalid) {
        const std::string encoded_string = EncodeBase64(decoded_string);
        assert(encoded_string == TrimString(encoded_string));
        assert(ToLower(encoded_string) == ToLower(TrimString(random_encoded_string)));
    }

    PartiallySignedTransaction psbt;
    std::string error;
    (void)DecodeBase64PSBT(psbt, random_encoded_string, error);
}
