// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/bip324_suite.h>
#include <crypto/rfc8439.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cstddef>
#include <vector>

std::array<std::byte, BIP324_KEY_LEN> get_key(FuzzedDataProvider& fdp)
{
    auto key_vec = fdp.ConsumeBytes<std::byte>(BIP324_KEY_LEN);
    key_vec.resize(BIP324_KEY_LEN);
    std::array<std::byte, BIP324_KEY_LEN> key_arr;
    memcpy(key_arr.data(), key_vec.data(), BIP324_KEY_LEN);
    return key_arr;
}


FUZZ_TARGET(crypto_bip324_suite)
{
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};

    const auto key_l = get_key(fdp);
    const auto key_p = get_key(fdp);
    const auto rekey_salt = get_key(fdp);

    BIP324CipherSuite suite(key_l, key_p, rekey_salt);

    size_t buffer_size = fdp.ConsumeIntegralInRange<size_t>(0, 4096);
    std::vector<std::byte> in(BIP324_LENGTH_FIELD_LEN + BIP324_HEADER_LEN + buffer_size + RFC8439_TAGLEN, std::byte{0x00});
    std::vector<std::byte> out(BIP324_LENGTH_FIELD_LEN + BIP324_HEADER_LEN + buffer_size + RFC8439_TAGLEN, std::byte{0x00});
    bool is_encrypt = fdp.ConsumeBool();
    BIP324HeaderFlags flags{fdp.ConsumeIntegralInRange<uint8_t>(0, 255)};
    LIMITED_WHILE(fdp.ConsumeBool(), 10000)
    {
        CallOneOf(
            fdp,
            [&] {
                buffer_size = fdp.ConsumeIntegralInRange<size_t>(64, 4096);
                in = std::vector<std::byte>(BIP324_LENGTH_FIELD_LEN + BIP324_HEADER_LEN + buffer_size + RFC8439_TAGLEN, std::byte{0x00});
                out = std::vector<std::byte>(BIP324_LENGTH_FIELD_LEN + BIP324_HEADER_LEN + buffer_size + RFC8439_TAGLEN, std::byte{0x00});
            },
            [&] {
                flags = BIP324HeaderFlags{fdp.ConsumeIntegralInRange<uint8_t>(0, 255)};
            },
            [&] {
                (void)suite.Crypt(in, out, flags, is_encrypt);
            },
            [&] {
                std::array<std::byte, BIP324_LENGTH_FIELD_LEN> len_ciphertext;
                memcpy(len_ciphertext.data(), in.data(), BIP324_LENGTH_FIELD_LEN);
                (void)suite.DecryptLength(len_ciphertext);
            },
            [&] {
                is_encrypt = fdp.ConsumeBool();
            });
    }
}
