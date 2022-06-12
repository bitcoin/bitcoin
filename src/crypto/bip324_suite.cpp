// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/bip324_suite.h>

#include <crypto/common.h>
#include <crypto/poly1305.h>
#include <hash.h>
#include <support/cleanse.h>

#include <assert.h>
#include <cstring>
#include <string.h>

#ifndef HAVE_TIMINGSAFE_BCMP

int timingsafe_bcmp(const unsigned char* b1, const unsigned char* b2, size_t n)
{
    const unsigned char *p1 = b1, *p2 = b2;
    int ret = 0;

    for (; n > 0; n--)
        ret |= *p1++ ^ *p2++;
    return (ret != 0);
}

#endif // TIMINGSAFE_BCMP

BIP324CipherSuite::~BIP324CipherSuite()
{
    memory_cleanse(payload_key.data(), payload_key.size());
    memory_cleanse(rekey_salt.data(), rekey_salt.size());
}

bool BIP324CipherSuite::Crypt(Span<const std::byte> input, Span<std::byte> output,
                              BIP324HeaderFlags& flags, bool encrypt)
{
    // check buffer boundaries
    if (
        // if we encrypt, make sure the destination has the space for the length field, header, ciphertext and MAC
        (encrypt && (output.size() < BIP324_LENGTH_FIELD_LEN + BIP324_HEADER_LEN + input.size() + RFC8439_TAGLEN)) ||
        // if we decrypt, make sure the source contains at least the header + mac and the destination has the space for the source - MAC - header
        (!encrypt && (input.size() < BIP324_HEADER_LEN + RFC8439_TAGLEN || output.size() < input.size() - BIP324_HEADER_LEN - RFC8439_TAGLEN))) {
        return false;
    }

    if (encrypt) {
        // input is just the payload
        // output will be encrypted length + encrypted (header and payload) + mac tag
        uint32_t ciphertext_len = BIP324_HEADER_LEN + input.size();
        ciphertext_len = htole32(ciphertext_len);
        auto write_pos = output.data();
        fsc20.Crypt({reinterpret_cast<std::byte*>(&ciphertext_len), BIP324_LENGTH_FIELD_LEN}, {write_pos, BIP324_LENGTH_FIELD_LEN});
        write_pos += BIP324_LENGTH_FIELD_LEN;

        std::vector<Span<const std::byte>> ins{Span{reinterpret_cast<std::byte*>(&flags), BIP324_HEADER_LEN}, input};
        auto encrypted = RFC8439Encrypt({}, payload_key, nonce, ins);

        memcpy(write_pos, encrypted.ciphertext.data(), encrypted.ciphertext.size());
        write_pos += encrypted.ciphertext.size();
        memcpy(write_pos, encrypted.tag.data(), encrypted.tag.size());
        write_pos += encrypted.tag.size();
    } else {
        // we must use BIP324CipherSuite::DecryptLength before calling BIP324CipherSuite::Crypt
        // input is encrypted (header + payload) and the mac tag
        // decrypted header will be put in flags and output will be payload.
        auto ciphertext_size = input.size() - RFC8439_TAGLEN;
        RFC8439Encrypted encrypted;
        encrypted.ciphertext.resize(ciphertext_size);
        memcpy(encrypted.ciphertext.data(), input.data(), ciphertext_size);
        memcpy(encrypted.tag.data(), input.data() + ciphertext_size, RFC8439_TAGLEN);
        auto decrypted = RFC8439Decrypt({}, payload_key, nonce, encrypted);
        if (!decrypted.success) {
            return false;
        }

        memcpy(&flags, decrypted.plaintext.data(), BIP324_HEADER_LEN);
        if (!output.empty()) {
            memcpy(output.data(), decrypted.plaintext.data() + BIP324_HEADER_LEN, ciphertext_size - BIP324_HEADER_LEN);
        }
    }

    msg_ctr++;
    if (msg_ctr == REKEY_INTERVAL) {
        auto new_key = Hash(rekey_salt, payload_key);
        memcpy(payload_key.data(), &new_key, BIP324_KEY_LEN);
        rekey_ctr++;
        msg_ctr = 0;
    }
    set_nonce();
    return true;
}

uint32_t BIP324CipherSuite::DecryptLength(const std::array<std::byte, BIP324_LENGTH_FIELD_LEN>& ciphertext)
{
    std::array<uint8_t, BIP324_LENGTH_FIELD_LEN> length_buffer;
    fsc20.Crypt(ciphertext, MakeWritableByteSpan(length_buffer));

    return (uint32_t{length_buffer[0]}) |
           (uint32_t{length_buffer[1]} << 8) |
           (uint32_t{length_buffer[2]} << 16);
}
