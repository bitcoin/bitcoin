// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/bip324_suite.h>

#include <crypto/common.h>
#include <crypto/poly1305.h>
#include <crypto/sha256.h>
#include <support/cleanse.h>

#include <assert.h>
#include <cstring>
#include <string.h>

BIP324CipherSuite::~BIP324CipherSuite()
{
    memory_cleanse(key_P.data(), key_P.size());
}

void BIP324CipherSuite::Rekey()
{
    ChaCha20 rekey_c20(UCharCast(key_P.data()));
    std::array<std::byte, NONCE_LENGTH> rekey_nonce;
    memset(rekey_nonce.data(), 0xFF, 4);
    memcpy(rekey_nonce.data() + 4, nonce.data() + 4, NONCE_LENGTH - 4);
    rekey_c20.SetRFC8439Nonce(rekey_nonce);
    rekey_c20.SeekRFC8439(1);
    rekey_c20.Keystream(reinterpret_cast<unsigned char*>(key_P.data()), BIP324_KEY_LEN);
}

bool BIP324CipherSuite::Crypt(const Span<const std::byte> aad,
                              const Span<const std::byte> input,
                              Span<std::byte> output,
                              BIP324HeaderFlags& flags, bool encrypt)
{
    // check buffer boundaries
    if (
        // if we encrypt, make sure the destination has the space for the encrypted length field, header, contents and MAC
        (encrypt && (output.size() < BIP324_LENGTH_FIELD_LEN + BIP324_HEADER_LEN + input.size() + RFC8439_EXPANSION)) ||
        // if we decrypt, make sure the source contains at least the encrypted header + mac and the destination has the space for the input - MAC - header
        (!encrypt && (input.size() < BIP324_HEADER_LEN + RFC8439_EXPANSION || output.size() < input.size() - BIP324_HEADER_LEN - RFC8439_EXPANSION))) {
        return false;
    }

    if (encrypt) {
        // input is just the contents
        // output will be encrypted contents length + encrypted (header and contents) + mac tag
        uint32_t contents_len = input.size();
        WriteLE32(reinterpret_cast<unsigned char*>(&contents_len), contents_len);

        std::vector<std::byte> header_and_contents(BIP324_HEADER_LEN + input.size());

        memcpy(header_and_contents.data(), &flags, BIP324_HEADER_LEN);
        if (!input.empty()) {
            memcpy(header_and_contents.data() + BIP324_HEADER_LEN, input.data(), input.size());
        }

        auto write_pos = output.data();
        fsc20.Crypt({reinterpret_cast<std::byte*>(&contents_len), BIP324_LENGTH_FIELD_LEN},
                    {write_pos, BIP324_LENGTH_FIELD_LEN});
        write_pos += BIP324_LENGTH_FIELD_LEN;
        RFC8439Encrypt(aad, key_P, nonce, header_and_contents, {write_pos, BIP324_HEADER_LEN + input.size() + RFC8439_EXPANSION});
    } else {
        // we must use BIP324CipherSuite::DecryptLength before calling BIP324CipherSuite::Crypt
        // input is encrypted (header + contents) and the MAC tag i.e. the RFC8439 ciphertext blob
        // decrypted header will be put in flags and output will be plaintext contents.
        std::vector<std::byte> decrypted_header_and_contents(input.size() - RFC8439_EXPANSION);
        auto authenticated = RFC8439Decrypt(aad, key_P, nonce, input, decrypted_header_and_contents);
        if (!authenticated) {
            return false;
        }

        memcpy(&flags, decrypted_header_and_contents.data(), BIP324_HEADER_LEN);
        if (!output.empty()) {
            memcpy(output.data(),
                   decrypted_header_and_contents.data() + BIP324_HEADER_LEN,
                   input.size() - BIP324_HEADER_LEN - RFC8439_EXPANSION);
        }
    }

    packet_counter++;
    if (packet_counter % REKEY_INTERVAL == 0) {
        Rekey();
    }
    set_nonce();
    return true;
}

uint32_t BIP324CipherSuite::DecryptLength(const std::array<std::byte, BIP324_LENGTH_FIELD_LEN>& encrypted_length)
{
    std::array<uint8_t, BIP324_LENGTH_FIELD_LEN> length_buffer;
    fsc20.Crypt(encrypted_length, MakeWritableByteSpan(length_buffer));

    return (uint32_t{length_buffer[0]}) |
           (uint32_t{length_buffer[1]} << 8) |
           (uint32_t{length_buffer[2]} << 16);
}
