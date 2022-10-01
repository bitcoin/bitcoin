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

FUZZ_TARGET(crypto_aes256cbc)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<uint8_t> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES256_KEYSIZE);
    const std::vector<uint8_t> iv = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES_BLOCKSIZE);
    const bool pad = fuzzed_data_provider.ConsumeBool();

    AES256CBCEncrypt encrypt{key.data(), iv.data(), pad};
    AES256CBCDecrypt decrypt{key.data(), iv.data(), pad};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        const std::vector<uint8_t> plaintext = ConsumeRandomLengthByteVector(fuzzed_data_provider);
        std::vector<uint8_t> ciphertext(plaintext.size() + AES_BLOCKSIZE);
        const int encrypt_ret = encrypt.Encrypt(plaintext.data(), plaintext.size(), ciphertext.data());
        ciphertext.resize(encrypt_ret);
        std::vector<uint8_t> decrypted_plaintext(ciphertext.size());
        const int decrypt_ret = decrypt.Decrypt(ciphertext.data(), ciphertext.size(), decrypted_plaintext.data());
        decrypted_plaintext.resize(decrypt_ret);
        assert(decrypted_plaintext == plaintext || (!pad && plaintext.size() % AES_BLOCKSIZE != 0 && encrypt_ret == 0 && decrypt_ret == 0));
    }
}
