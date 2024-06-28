// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/sv2_noise.h>

#include <crypto/chacha20poly1305.h>
#include <crypto/hmac_sha256.h>
#include <logging.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/time.h>

Sv2SignatureNoiseMessage::Sv2SignatureNoiseMessage(uint16_t version, uint32_t valid_from, uint32_t valid_to, const XOnlyPubKey& static_key, const CKey& authority_key) : m_version{version}, m_valid_from{valid_from}, m_valid_to{valid_to}, m_static_key{static_key}
{
    SignSchnorr(authority_key, m_sig);
}

uint256 Sv2SignatureNoiseMessage::GetHash()
{
    DataStream ss{};
    ss << m_version
       << m_valid_from
       << m_valid_to
       << m_static_key;

    LogTrace(BCLog::SV2, "Certificate hashed data: %s\n", HexStr(ss));

    CSHA256 hasher;
    hasher.Write(reinterpret_cast<unsigned char*>(&(*ss.begin())), ss.end() - ss.begin());

    uint256 hash_output;
    hasher.Finalize(hash_output.begin());
    return hash_output;
}

bool Sv2SignatureNoiseMessage::Validate(XOnlyPubKey authority_key)
{
    if (m_version > 0) {
        LogTrace(BCLog::SV2, "Invalid certificate version: %d\n", m_version);
        return false;
    }
    auto now{GetTime<std::chrono::seconds>()};
    if (std::chrono::seconds{m_valid_from} > now) {
        LogTrace(BCLog::SV2, "Certificate valid from is in the future: %d\n", m_valid_from);
        return false;
    }
    if (std::chrono::seconds{m_valid_to} < now) {
        LogTrace(BCLog::SV2, "Certificate expired: %d\n", m_valid_to);
        return false;
    }

    if (!authority_key.VerifySchnorr(this->GetHash(), m_sig)) {
        LogTrace(BCLog::SV2, "Certificate signature is invalid\n");
        return false;
    }
    return true;
}

void Sv2SignatureNoiseMessage::SignSchnorr(const CKey& authority_key, Span<unsigned char> sig)
{
    authority_key.SignSchnorr(this->GetHash(), sig, nullptr, {});
}

Sv2CipherState::Sv2CipherState(uint8_t key[HASHLEN])
{
    std::copy(key, key + HASHLEN, m_key);
}

bool Sv2CipherState::DecryptWithAd(Span<const std::byte> associated_data, Span<std::byte> ciphertext, Span<std::byte> plain)
{
    Assume(Sv2Cipher::EncryptedMessageSize(plain.size()) == ciphertext.size());

    if (m_nonce == UINT64_MAX) {
        // This nonce value is reserved, see chapter 5.1 of the Noise paper.
        LogTrace(BCLog::SV2, "Nonce exceeds maximum value\n");
        return false;
    }
    AEADChaCha20Poly1305::Nonce96 nonce = {0, m_nonce};
    auto key = MakeByteSpan(m_key);
    AEADChaCha20Poly1305 aead{key};
    if (!aead.Decrypt(ciphertext, associated_data, nonce, plain)) {
        LogTrace(BCLog::SV2, "Message decryption failed\n");
        return false;
    }
    // Only increase nonce if decryption succeeded
    m_nonce++;
    return true;
}

bool Sv2CipherState::EncryptWithAd(Span<const std::byte> associated_data, Span<const std::byte> plain, Span<std::byte> ciphertext)
{
    Assume(Sv2Cipher::EncryptedMessageSize(plain.size()) == ciphertext.size());

    if (m_nonce == UINT64_MAX) {
        // This nonce value is reserved, see chapter 5.1 of the Noise paper.
        LogTrace(BCLog::SV2, "Nonce exceeds maximum value\n");
        return false;
    }
    AEADChaCha20Poly1305::Nonce96 nonce = {0, m_nonce++};
    auto key = MakeByteSpan(m_key);
    AEADChaCha20Poly1305 aead{key};
    aead.Encrypt(plain, associated_data, nonce, ciphertext);
    return true;
}

bool Sv2CipherState::EncryptMessage(Span<const std::byte> plain, Span<std::byte> ciphertext)
{
    Assume(ciphertext.size() == Sv2Cipher::EncryptedMessageSize(plain.size()));

    std::vector<std::byte> ad; // No associated data

    const size_t max_chunk_size = NOISE_MAX_CHUNK_SIZE - Poly1305::TAGLEN;
    size_t num_chunks = plain.size() / max_chunk_size;
    if (plain.size() % max_chunk_size != 0) {
        num_chunks++;
    }
    if (num_chunks > 1) {
        LogTrace(BCLog::SV2,
                 "Split into %d chunks (max %d bytes)\n",
                 num_chunks, max_chunk_size);
    }

    // Copy input bytes into output buffer
    const std::vector<std::byte> padding(Poly1305::TAGLEN, std::byte(0));
    for (size_t i = 0; i < num_chunks; ++i) {
        size_t chunk_start = i * max_chunk_size;
        size_t chunk_end = std::min(chunk_start + max_chunk_size, plain.size());
        size_t chunk_size = chunk_end - chunk_start;
        const auto encrypted_chunk_start = ciphertext.begin() + i * NOISE_MAX_CHUNK_SIZE;
        std::copy(plain.begin() + chunk_start, plain.begin() + chunk_start + chunk_size, encrypted_chunk_start);
        std::copy(padding.begin(), padding.end(), encrypted_chunk_start + chunk_size);
    }

    // Encrypt each chunk
    size_t bytes_written = 0;
    for (size_t i = 0; i < num_chunks; ++i) {
        size_t chunk_size = std::min(ciphertext.size() - bytes_written, NOISE_MAX_CHUNK_SIZE);
        Span<std::byte> chunk = ciphertext.subspan(bytes_written, chunk_size);
        Span<std::byte> chunk_plain = ciphertext.subspan(bytes_written, chunk_size - Poly1305::TAGLEN);
        if (!EncryptWithAd(ad, chunk_plain, chunk)) {
            return false;
        }
        bytes_written += chunk.size();
    }

    Assume(bytes_written == ciphertext.size());
    return true;
}

bool Sv2CipherState::DecryptMessage(Span<std::byte> ciphertext, Span<std::byte> plain)
{
    Assume(Sv2Cipher::EncryptedMessageSize(plain.size()) == ciphertext.size());

    size_t processed = 0;
    size_t plain_position = 0;
    std::vector<std::byte> ad; // No associated data

    while (processed < ciphertext.size()) {
        size_t chunk_size = std::min(ciphertext.size() - processed, NOISE_MAX_CHUNK_SIZE);
        Span<std::byte> chunk_cipher = ciphertext.subspan(processed, chunk_size);
        Span<std::byte> chunk_plain = plain.subspan(plain_position, chunk_size - Poly1305::TAGLEN);
        if (!DecryptWithAd(ad, chunk_cipher, chunk_plain)) return false;
        processed += chunk_size;
        plain_position += chunk_size - Poly1305::TAGLEN;
    }

    return true;
}

void Sv2SymmetricState::MixHash(const Span<const std::byte> input)
{
    m_hash_output = (HashWriter{} << m_hash_output << input).GetSHA256();
}

void Sv2SymmetricState::MixKey(const Span<const std::byte> input_key_material)
{
    uint8_t out0[Sv2CipherState::HASHLEN], out1[Sv2CipherState::HASHLEN];

    HKDF2(input_key_material, out0, out1);

    std::memset(m_chaining_key, 0, sizeof(m_chaining_key));
    std::copy(out0, out0 + Sv2CipherState::HASHLEN, m_chaining_key);
    m_cipher_state = Sv2CipherState{out1};
}

std::string Sv2SymmetricState::GetChainingKey()
{
    return HexStr(m_chaining_key);
}

void Sv2SymmetricState::LogChainingKey()
{
    LogTrace(BCLog::SV2, "Chaining key: %s\n", GetChainingKey());
}

void Sv2SymmetricState::HKDF2(const Span<const std::byte> input_key_material, uint8_t out0[Sv2CipherState::HASHLEN], uint8_t out1[Sv2CipherState::HASHLEN])
{
    uint8_t tmp_key[Sv2CipherState::HASHLEN];
    CHMAC_SHA256 tmp_mac(m_chaining_key, Sv2CipherState::HASHLEN);
    tmp_mac.Write(UCharCast(input_key_material.data()), input_key_material.size());
    tmp_mac.Finalize(tmp_key);

    CHMAC_SHA256 out0_mac(tmp_key, Sv2CipherState::HASHLEN);
    uint8_t one[1]{0x1};
    out0_mac.Write(one, 1);
    out0_mac.Finalize(out0);

    std::vector<uint8_t> in1;
    in1.reserve(Sv2CipherState::HASHLEN + 1);
    std::copy(out0, out0 + Sv2CipherState::HASHLEN, std::back_inserter(in1));
    in1.push_back(0x02);

    CHMAC_SHA256 out1_mac(tmp_key, Sv2CipherState::HASHLEN);
    out1_mac.Write(&in1[0], in1.size());
    out1_mac.Finalize(out1);
}

bool Sv2SymmetricState::EncryptAndHash(Span<const std::byte> plain, Span<std::byte> ciphertext)
{
    Assume(Sv2Cipher::EncryptedMessageSize(plain.size()) == ciphertext.size());

    if (!m_cipher_state.EncryptWithAd(MakeByteSpan(m_hash_output), plain, ciphertext)) {
        return false;
    }
    MixHash(ciphertext);
    return true;
}

bool Sv2SymmetricState::DecryptAndHash(Span<std::byte> ciphertext, Span<std::byte> plain)
{
    Assume(Sv2Cipher::EncryptedMessageSize(plain.size()) == ciphertext.size());

    // The handshake requires mix hashing the cipher text NOT the decrypted
    // plaintext.
    std::vector<std::byte> ciphertext_copy;
    ciphertext_copy.assign(ciphertext.begin(), ciphertext.end()); // (ciphertext.size(), std::byte(0));
    // std::copy(ciphertext.begin(), ciphertext.end(), ciphertext_copy.begin());

    bool res = m_cipher_state.DecryptWithAd(MakeByteSpan(m_hash_output), ciphertext, plain);
    if (!res) return false;
    MixHash(ciphertext_copy);
    return true;
}

std::array<Sv2CipherState, 2> Sv2SymmetricState::Split()
{
    uint8_t send_key[Sv2CipherState::HASHLEN], recv_key[Sv2CipherState::HASHLEN];

    std::vector<std::byte> empty;
    HKDF2(empty, send_key, recv_key);

    std::array<Sv2CipherState, 2> result;
    result[0] = Sv2CipherState{send_key};
    result[1] = Sv2CipherState{recv_key};

    return result;
}

uint256 Sv2SymmetricState::GetHashOutput()
{
    return m_hash_output;
}

void Sv2HandshakeState::SetEphemeralKey(CKey&& key)
{
    m_ephemeral_key = key;
    m_our_ephemeral_ellswift_pk = m_ephemeral_key.EllSwiftCreate(MakeByteSpan(GetRandHash()));
};

void Sv2HandshakeState::GenerateEphemeralKey() noexcept
{
    Assume(!m_ephemeral_key.size());
    LogTrace(BCLog::SV2, "Generate ephemeral key\n");
    SetEphemeralKey(GenerateRandomKey());
};

void Sv2HandshakeState::WriteMsgEphemeralPK(Span<std::byte> msg)
{
    if (msg.size() < ELLSWIFT_PUB_KEY_SIZE) {
        throw std::runtime_error(strprintf("Invalid message size: %d bytes < %d", msg.size(), ELLSWIFT_PUB_KEY_SIZE));
    }

    if (!m_ephemeral_key.IsValid()) {
        GenerateEphemeralKey();
    }

    LogTrace(BCLog::SV2, "Write our ephemeral key\n");
    std::copy(m_our_ephemeral_ellswift_pk.begin(), m_our_ephemeral_ellswift_pk.end(), msg.begin());

    m_symmetric_state.MixHash(msg.subspan(0, ELLSWIFT_PUB_KEY_SIZE));
    LogTrace(BCLog::SV2, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    std::vector<std::byte> empty;
    m_symmetric_state.MixHash(empty);
}

void Sv2HandshakeState::ReadMsgEphemeralPK(Span<std::byte> msg)
{
    LogTrace(BCLog::SV2, "Read their ephemeral key\n");
    Assume(msg.size() == ELLSWIFT_PUB_KEY_SIZE);
    m_remote_ephemeral_ellswift_pk = EllSwiftPubKey(msg);

    m_symmetric_state.MixHash(msg.subspan(0, ELLSWIFT_PUB_KEY_SIZE));
    LogTrace(BCLog::SV2, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    std::vector<std::byte> empty;
    m_symmetric_state.MixHash(empty);
}

void Sv2HandshakeState::WriteMsgES(Span<std::byte> msg)
{
    if (msg.size() < HANDSHAKE_STEP2_SIZE) {
        throw std::runtime_error(strprintf("Invalid message size: %d bytes < %d", msg.size(), HANDSHAKE_STEP2_SIZE));
    }

    ssize_t bytes_written = 0;

    if (!m_ephemeral_key.IsValid()) {
        GenerateEphemeralKey();
    }

    // Send our ephemeral pk.
    LogTrace(BCLog::SV2, "Write our ephemeral key\n");
    std::copy(m_our_ephemeral_ellswift_pk.begin(), m_our_ephemeral_ellswift_pk.end(), msg.begin());

    m_symmetric_state.MixHash(msg.subspan(0, ELLSWIFT_PUB_KEY_SIZE));
    bytes_written += ELLSWIFT_PUB_KEY_SIZE;

    LogTrace(BCLog::SV2, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    LogTrace(BCLog::SV2, "Perform ECDH with the remote ephemeral key\n");
    ECDHSecret ecdh_secret{m_ephemeral_key.ComputeBIP324ECDHSecret(m_remote_ephemeral_ellswift_pk,
                                                                   m_our_ephemeral_ellswift_pk,
                                                                   /*initiating=*/false)};

    LogTrace(BCLog::SV2, "Mix key with ECDH result: ephemeral ours -- remote ephemeral\n");
    m_symmetric_state.MixKey(ecdh_secret);
    m_symmetric_state.LogChainingKey();

    // Send our static pk.
    LogTrace(BCLog::SV2, "Encrypt and write our static key\n");

    if (!m_symmetric_state.EncryptAndHash(m_our_static_ellswift_pk, msg.subspan(ELLSWIFT_PUB_KEY_SIZE, ELLSWIFT_PUB_KEY_SIZE + Poly1305::TAGLEN))) {
        // This should never happen
        Assume(false);
        throw std::runtime_error("Failed to encrypt our ephemeral key\n");
    }

    bytes_written += ELLSWIFT_PUB_KEY_SIZE + Poly1305::TAGLEN;

    LogTrace(BCLog::SV2, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    LogTrace(BCLog::SV2, "Perform ECDH between our static and remote ephemeral key\n");
    ECDHSecret ecdh_static_secret{m_static_key.ComputeBIP324ECDHSecret(m_remote_ephemeral_ellswift_pk,
                                                                       m_our_static_ellswift_pk,
                                                                       /*initiating=*/false)};
    LogTrace(BCLog::SV2, "ECDH result: %s\n", HexStr(ecdh_static_secret));

    LogTrace(BCLog::SV2, "Mix key with ECDH result: static ours -- remote ephemeral\n");
    m_symmetric_state.MixKey(ecdh_static_secret);
    m_symmetric_state.LogChainingKey();

    // Serialize our digital signature noise message and encrypt.
    DataStream ss{};
    Assume(m_certificate);
    ss << m_certificate.value();
    Assume(ss.size() == Sv2SignatureNoiseMessage::SIZE);

    LogTrace(BCLog::SV2, "Encrypt certificate: %s\n", HexStr(ss));
    if (!m_symmetric_state.EncryptAndHash(ss, msg.subspan(bytes_written, Sv2SignatureNoiseMessage::SIZE + Poly1305::TAGLEN))) {
        // This should never happen
        Assume(false);
        throw std::runtime_error("Failed to encrypt our certificate\n");
    }

    LogTrace(BCLog::SV2, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    bytes_written += Sv2SignatureNoiseMessage::SIZE + Poly1305::TAGLEN;
    Assume(bytes_written == HANDSHAKE_STEP2_SIZE);
}

bool Sv2HandshakeState::ReadMsgES(Span<std::byte> msg)
{
    Assume(msg.size() == HANDSHAKE_STEP2_SIZE);
    ssize_t bytes_read = 0;

    // Read the remote ephmeral key from the msg and decrypt.
    LogTrace(BCLog::SV2, "Read remote ephemeral key\n");
    m_remote_ephemeral_ellswift_pk = EllSwiftPubKey(msg.subspan(0, ELLSWIFT_PUB_KEY_SIZE));
    bytes_read += ELLSWIFT_PUB_KEY_SIZE;

    m_symmetric_state.MixHash(m_remote_ephemeral_ellswift_pk);
    LogTrace(BCLog::SV2, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    LogTrace(BCLog::SV2, "Perform ECDH with the remote ephemeral key\n");
    ECDHSecret ecdh_secret{m_ephemeral_key.ComputeBIP324ECDHSecret(m_remote_ephemeral_ellswift_pk,
                                                                   m_our_ephemeral_ellswift_pk,
                                                                   /*initiating=*/true)};

    LogTrace(BCLog::SV2, "Mix key with ECDH result: ephemeral ours -- remote ephemeral\n");
    m_symmetric_state.MixKey(ecdh_secret);
    m_symmetric_state.LogChainingKey();

    LogTrace(BCLog::SV2, "Decrypt remote static key\n");
    std::array<std::byte, ELLSWIFT_PUB_KEY_SIZE> remote_static_key_bytes;
    bool res = m_symmetric_state.DecryptAndHash(msg.subspan(ELLSWIFT_PUB_KEY_SIZE, ELLSWIFT_PUB_KEY_SIZE + Poly1305::TAGLEN), remote_static_key_bytes);
    if (!res) return false;
    bytes_read += ELLSWIFT_PUB_KEY_SIZE + Poly1305::TAGLEN;

    LogTrace(BCLog::SV2, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    // Load remote static key from the decryted msg
    m_remote_static_ellswift_pk = EllSwiftPubKey(remote_static_key_bytes);

    LogTrace(BCLog::SV2, "Perform ECDH on the remote static key\n");
    ECDHSecret ecdh_static_secret{m_ephemeral_key.ComputeBIP324ECDHSecret(m_remote_static_ellswift_pk,
                                                                          m_our_ephemeral_ellswift_pk,
                                                                          /*initiating=*/true)};
    LogTrace(BCLog::SV2, "ECDH result: %s\n", HexStr(ecdh_static_secret));

    LogTrace(BCLog::SV2, "Mix key with ECDH result: ephemeral ours -- remote static\n");
    m_symmetric_state.MixKey(ecdh_static_secret);
    m_symmetric_state.LogChainingKey();

    LogTrace(BCLog::SV2, "Decrypt remote certificate\n");
    std::array<std::byte, Sv2SignatureNoiseMessage::SIZE> remote_cert_bytes;
    res = m_symmetric_state.DecryptAndHash(msg.subspan(bytes_read, Sv2SignatureNoiseMessage::SIZE + Poly1305::TAGLEN), remote_cert_bytes);
    if (!res) return false;
    bytes_read += (Sv2SignatureNoiseMessage::SIZE + Poly1305::TAGLEN);

    LogTrace(BCLog::SV2, "Validate remote certificate\n");
    DataStream ss_cert(remote_cert_bytes);
    Sv2SignatureNoiseMessage cert;
    ss_cert >> cert;
    cert.m_static_key = XOnlyPubKey(m_remote_static_ellswift_pk.Decode());
    Assume(m_authority_pubkey);
    if (!cert.Validate(m_authority_pubkey.value())) {
        // We initiated the connection, so it's safe to unconditionally log this:
        LogWarning("Invalid certificate: %s\n", HexStr(remote_cert_bytes));
        return false;
    }

    LogTrace(BCLog::SV2, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    Assume(bytes_read == HANDSHAKE_STEP2_SIZE);
    return true;
}

std::array<Sv2CipherState, 2> Sv2HandshakeState::SplitSymmetricState()
{
    return m_symmetric_state.Split();
}

uint256 Sv2HandshakeState::GetHashOutput()
{
    return m_symmetric_state.GetHashOutput();
}

Sv2Cipher::Sv2Cipher(CKey&& static_key, XOnlyPubKey authority_pubkey)
{
    m_handshake_state = std::make_unique<Sv2HandshakeState>(std::move(static_key), authority_pubkey);
    m_initiator = true;
}

Sv2Cipher::Sv2Cipher(CKey&& static_key, Sv2SignatureNoiseMessage&& certificate)
{
    m_handshake_state = std::make_unique<Sv2HandshakeState>(std::move(static_key), std::move(certificate));
    m_initiator = false;
}

Sv2HandshakeState& Sv2Cipher::GetHandshakeState()
{
    Assume(m_handshake_state);
    return *m_handshake_state;
}

void Sv2Cipher::FinishHandshake()
{
    Assume(m_handshake_state);

    auto cipher_state = m_handshake_state->SplitSymmetricState();
    auto cs1 = cipher_state[0];
    auto cs2 = cipher_state[1];

    m_hash = m_handshake_state->GetHashOutput();

    m_cs1 = std::move(cs1);
    m_cs2 = std::move(cs2);

    m_handshake_state.reset();
}

size_t Sv2Cipher::EncryptedMessageSize(size_t msg_len)
{
    size_t num_chunks = msg_len / (NOISE_MAX_CHUNK_SIZE - Poly1305::TAGLEN);
    if (msg_len % (NOISE_MAX_CHUNK_SIZE - Poly1305::TAGLEN) != 0) {
        num_chunks++;
    }
    return msg_len + (num_chunks * Poly1305::TAGLEN);
}

bool Sv2Cipher::DecryptMessage(Span<std::byte> ciphertext, Span<std::byte> plain)
{
    Assume(EncryptedMessageSize(plain.size()) == ciphertext.size());

    if (m_initiator) {
        return m_cs2.DecryptMessage(ciphertext, plain);
    } else {
        return m_cs1.DecryptMessage(ciphertext, plain);
    }
}

bool Sv2Cipher::EncryptMessage(Span<const std::byte> input, Span<std::byte> output)
{
    Assume(output.size() == Sv2Cipher::EncryptedMessageSize(input.size()));

    if (m_initiator) {
        if (!m_cs1.EncryptMessage(input, output)) return false;
    } else {
        if (!m_cs2.EncryptMessage(input, output)) return false;
    }
    return true;
}

uint256 Sv2Cipher::GetHash() const
{
    return m_hash;
}
