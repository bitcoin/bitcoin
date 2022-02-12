// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/aes.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <vector>

FUZZ_TARGET(crypto_aes256)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<uint8_t> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES256_KEYSIZE);

    AES256Encrypt encrypt{key.data()};
    AES256Decrypt decrypt{key.data()};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        const std::vector<uint8_t> plaintext = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES_BLOCKSIZE);
        std::vector<uint8_t> ciphertext(AES_BLOCKSIZE);
        encrypt.Encrypt(ciphertext.data(), plaintext.data());
        std::vector<uint8_t> decrypted_plaintext(AES_BLOCKSIZE);
        decrypt.Decrypt(decrypted_plaintext.data(), ciphertext.data());
        assert(decrypted_plaintext == plaintext);
    }
}
