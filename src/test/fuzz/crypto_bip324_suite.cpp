// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/bip324_suite.h>
#include <crypto/rfc8439.h>
#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cstddef>
#include <vector>

void get_key(FuzzedDataProvider& fdp, Span<std::byte> key)
{
    auto key_vec = fdp.ConsumeBytes<std::byte>(key.size());
    key_vec.resize(key.size());
    memcpy(key.data(), key_vec.data(), key.size());
}


FUZZ_TARGET(crypto_bip324_suite)
{
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};

    BIP324Key key_L, key_P;
    get_key(fdp, key_L);
    get_key(fdp, key_P);

    BIP324CipherSuite suite(key_L, key_P);

    size_t contents_size = fdp.ConsumeIntegralInRange<size_t>(0, 4096);
    std::vector<std::byte> in(BIP324_LENGTH_FIELD_LEN + BIP324_HEADER_LEN + contents_size + RFC8439_EXPANSION, std::byte{0x00});
    std::vector<std::byte> out(BIP324_LENGTH_FIELD_LEN + BIP324_HEADER_LEN + contents_size + RFC8439_EXPANSION, std::byte{0x00});
    bool is_encrypt = fdp.ConsumeBool();
    BIP324HeaderFlags flags{fdp.ConsumeIntegralInRange<uint8_t>(0, 255)};
    size_t aad_size = fdp.ConsumeIntegralInRange<size_t>(0, 255);
    auto aad = fdp.ConsumeBytes<std::byte>(aad_size);
    LIMITED_WHILE(fdp.ConsumeBool(), 10000)
    {
        CallOneOf(
            fdp,
            [&] {
                contents_size = fdp.ConsumeIntegralInRange<size_t>(64, 4096);
                in = std::vector<std::byte>(BIP324_LENGTH_FIELD_LEN + BIP324_HEADER_LEN + contents_size + RFC8439_EXPANSION, std::byte{0x00});
                out = std::vector<std::byte>(BIP324_LENGTH_FIELD_LEN + BIP324_HEADER_LEN + contents_size + RFC8439_EXPANSION, std::byte{0x00});
            },
            [&] {
                flags = BIP324HeaderFlags{fdp.ConsumeIntegralInRange<uint8_t>(0, 255)};
            },
            [&] {
                (void)suite.Crypt(aad, in, out, flags, is_encrypt);
            },
            [&] {
                std::array<std::byte, BIP324_LENGTH_FIELD_LEN> encrypted_pkt_len;
                memcpy(encrypted_pkt_len.data(), in.data(), BIP324_LENGTH_FIELD_LEN);
                (void)suite.DecryptLength(encrypted_pkt_len);
            },
            [&] {
                is_encrypt = fdp.ConsumeBool();
            });
    }
}
