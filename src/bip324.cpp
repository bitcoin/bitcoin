// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bip324.h>

#include <chainparams.h>
#include <crypto/chacha20.h>
#include <crypto/chacha20poly1305.h>
#include <crypto/hkdf_sha256_32.h>
#include <key.h>
#include <pubkey.h>
#include <random.h>
#include <span.h>
#include <support/cleanse.h>
#include <uint256.h>

#include <algorithm>
#include <assert.h>
#include <cstdint>
#include <cstddef>
#include <iterator>
#include <string>

BIP324Cipher::BIP324Cipher(const CKey& key, std::span<const std::byte> ent32) noexcept
    : m_key(key)
{
    m_our_pubkey = m_key.EllSwiftCreate(ent32);
}

BIP324Cipher::BIP324Cipher(const CKey& key, const EllSwiftPubKey& pubkey) noexcept :
    m_key(key), m_our_pubkey(pubkey) {}

void BIP324Cipher::Initialize(const EllSwiftPubKey& their_pubkey, bool initiator, bool self_decrypt) noexcept
{
    // Determine salt (fixed string + network magic bytes)
    const auto& message_header = Params().MessageStart();
    std::string salt = std::string{"bitcoin_v2_shared_secret"} + std::string(std::begin(message_header), std::end(message_header));

    // Perform ECDH to derive shared secret.
    ECDHSecret ecdh_secret = m_key.ComputeBIP324ECDHSecret(their_pubkey, m_our_pubkey, initiator);

    // Derive encryption keys from shared secret, and initialize stream ciphers and AEADs.
    bool side = (initiator != self_decrypt);
    CHKDF_HMAC_SHA256_L32 hkdf(UCharCast(ecdh_secret.data()), ecdh_secret.size(), salt);
    std::array<std::byte, 32> hkdf_32_okm;
    hkdf.Expand32("initiator_L", UCharCast(hkdf_32_okm.data()));
    (side ? m_send_l_cipher : m_recv_l_cipher).emplace(hkdf_32_okm, REKEY_INTERVAL);
    hkdf.Expand32("initiator_P", UCharCast(hkdf_32_okm.data()));
    (side ? m_send_p_cipher : m_recv_p_cipher).emplace(hkdf_32_okm, REKEY_INTERVAL);
    hkdf.Expand32("responder_L", UCharCast(hkdf_32_okm.data()));
    (side ? m_recv_l_cipher : m_send_l_cipher).emplace(hkdf_32_okm, REKEY_INTERVAL);
    hkdf.Expand32("responder_P", UCharCast(hkdf_32_okm.data()));
    (side ? m_recv_p_cipher : m_send_p_cipher).emplace(hkdf_32_okm, REKEY_INTERVAL);

    // Derive garbage terminators from shared secret.
    hkdf.Expand32("garbage_terminators", UCharCast(hkdf_32_okm.data()));
    std::copy(std::begin(hkdf_32_okm), std::begin(hkdf_32_okm) + GARBAGE_TERMINATOR_LEN,
        (initiator ? m_send_garbage_terminator : m_recv_garbage_terminator).begin());
    std::copy(std::end(hkdf_32_okm) - GARBAGE_TERMINATOR_LEN, std::end(hkdf_32_okm),
        (initiator ? m_recv_garbage_terminator : m_send_garbage_terminator).begin());

    // Derive session id from shared secret.
    hkdf.Expand32("session_id", UCharCast(m_session_id.data()));

    // Wipe all variables that contain information which could be used to re-derive encryption keys.
    memory_cleanse(ecdh_secret.data(), ecdh_secret.size());
    memory_cleanse(hkdf_32_okm.data(), sizeof(hkdf_32_okm));
    memory_cleanse(&hkdf, sizeof(hkdf));
    m_key = CKey();
}

void BIP324Cipher::Encrypt(std::span<const std::byte> contents, std::span<const std::byte> aad, bool ignore, std::span<std::byte> output) noexcept
{
    assert(output.size() == contents.size() + EXPANSION);

    // Encrypt length.
    std::byte len[LENGTH_LEN];
    len[0] = std::byte{(uint8_t)(contents.size() & 0xFF)};
    len[1] = std::byte{(uint8_t)((contents.size() >> 8) & 0xFF)};
    len[2] = std::byte{(uint8_t)((contents.size() >> 16) & 0xFF)};
    m_send_l_cipher->Crypt(len, output.first(LENGTH_LEN));

    // Encrypt plaintext.
    std::byte header[HEADER_LEN] = {ignore ? IGNORE_BIT : std::byte{0}};
    m_send_p_cipher->Encrypt(header, contents, aad, output.subspan(LENGTH_LEN));
}

uint32_t BIP324Cipher::DecryptLength(std::span<const std::byte> input) noexcept
{
    assert(input.size() == LENGTH_LEN);

    std::byte buf[LENGTH_LEN];
    // Decrypt length
    m_recv_l_cipher->Crypt(input, buf);
    // Convert to number.
    return uint32_t(buf[0]) + (uint32_t(buf[1]) << 8) + (uint32_t(buf[2]) << 16);
}

bool BIP324Cipher::Decrypt(std::span<const std::byte> input, std::span<const std::byte> aad, bool& ignore, std::span<std::byte> contents) noexcept
{
    assert(input.size() + LENGTH_LEN == contents.size() + EXPANSION);

    std::byte header[HEADER_LEN];
    if (!m_recv_p_cipher->Decrypt(input, aad, header, contents)) return false;

    ignore = (header[0] & IGNORE_BIT) == IGNORE_BIT;
    return true;
}
