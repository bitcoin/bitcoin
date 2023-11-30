// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/sv2_noise.h>

#include <crypto/chacha20poly1305.h>
#include <crypto/hmac_sha256.h>
#include <crypto/poly1305.h>
#include <logging.h>
#include <util/check.h>
#include <util/strencodings.h>

Sv2SignatureNoiseMessage::Sv2SignatureNoiseMessage(uint16_t version, uint32_t valid_from, uint32_t valid_to, const XOnlyPubKey& static_key, const CKey& authority_key) : m_version{version}, m_valid_from{valid_from}, m_valid_to{valid_to}, m_static_key{static_key}
{
    std::vector<unsigned char> sig;
    const auto sig_size = 64;
    sig.resize(sig_size);

    SignSchnorr(authority_key, sig);
    m_sig = std::move(sig);
}

uint256 Sv2SignatureNoiseMessage::GetHash()
{
    DataStream ss{};
    ss  << m_version
        << m_valid_from
        << m_valid_to
        // TODO: Stratum v2 spec requires signing the static key, but SRI currently
        //       implements this incorrectly.
        // << m_static_key
        ;

    CSHA256 hasher;
    hasher.Write(reinterpret_cast<unsigned char*>(&(*ss.begin())), ss.end() - ss.begin());

    uint256 hash_output;
    hasher.Finalize(hash_output.begin());
    return hash_output;
}

bool Sv2SignatureNoiseMessage::Validate(XOnlyPubKey authority_key)
{
    Assume(m_sig.size() == 64);
    if (m_version > 0) return false;
    auto epoch_now = std::chrono::system_clock::now().time_since_epoch();
    uint32_t now = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(epoch_now).count());
    if (m_valid_from > now + 3600) return false;
    if (m_valid_to < now - 3600) return false;

    return authority_key.VerifySchnorr(this->GetHash(), m_sig);
}

void Sv2SignatureNoiseMessage::SignSchnorr(const CKey& authority_key, Span<unsigned char> sig)
{
    authority_key.SignSchnorr(this->GetHash(), sig, nullptr, {});
}

Sv2CipherState::Sv2CipherState(uint8_t key[KEY_SIZE])
{
    std::copy(key, key + KEY_SIZE, m_key);
}

bool Sv2CipherState::DecryptWithAd(Span<const std::byte> associated_data, Span<std::byte> msg)
{
    AEADChaCha20Poly1305::Nonce96 nonce = {0, ++m_nonce};

    auto key = MakeByteSpan(Span(m_key));
    AEADChaCha20Poly1305 aead{key};
    return aead.Decrypt(msg, associated_data, nonce, Span(msg.begin(), msg.end() - POLY1305_TAGLEN));
}

// The encryption assumes that the msg variable has sufficient space for a 16 byte MAC.
void Sv2CipherState::EncryptWithAd(Span<const std::byte> associated_data, Span<std::byte> msg)
{
    AEADChaCha20Poly1305::Nonce96 nonce = {0, ++m_nonce};

    auto key = MakeByteSpan(Span(m_key));
    AEADChaCha20Poly1305 aead{key};
    aead.Encrypt(Span(msg.begin(), msg.end() - POLY1305_TAGLEN), associated_data, nonce, msg);
}

void Sv2CipherState::EncryptMessage(Span<const std::byte> input, Span<std::byte> output) {
    Assume(output.size() == Sv2Cipher::EncryptedMessageSize(input.size()));
    Assume(output.begin() != input.begin());

    std::vector<std::byte> ad; // No associated data

    const size_t max_chunk_size = NOISE_MAX_CHUNK_SIZE - POLY1305_TAGLEN;
    size_t num_chunks = input.size() / max_chunk_size;
    if (input.size() % max_chunk_size != 0) {
        num_chunks++;
    }
    if (num_chunks > 1) {
        LogPrintLevel(BCLog::SV2, BCLog::Level::Trace,
            "Split into %d chunks (max %d bytes)\n",
            num_chunks, max_chunk_size);
    }

    // Copy input bytes into output buffer
    const std::vector<std::byte> padding(POLY1305_TAGLEN, std::byte(0));
    for (size_t i = 0; i < num_chunks; ++i) {
        size_t chunk_start = i * max_chunk_size;
        size_t chunk_end = std::min(chunk_start + max_chunk_size, input.size());
        size_t chunk_size = chunk_end - chunk_start;
        const auto encrypted_chunk_start = output.begin() + i * NOISE_MAX_CHUNK_SIZE;
        std::copy(input.begin() + chunk_start, input.begin() + chunk_start + chunk_size, encrypted_chunk_start);
        std::copy(padding.begin(), padding.end(), encrypted_chunk_start + chunk_size);
    }

    // Encrypt each chunk
    size_t bytes_written = 0;
    for (size_t i = 0; i < num_chunks; ++i) {
        size_t chunk_size = std::min(output.size() - bytes_written, NOISE_MAX_CHUNK_SIZE);
        Span<std::byte> chunk = output.subspan(bytes_written, chunk_size);
        EncryptWithAd(ad, chunk);
        bytes_written += chunk.size();
    }

    Assume(bytes_written == output.size());
}

bool Sv2CipherState::DecryptMessage(Span<std::byte> message) {
    size_t processed = 0;
    std::vector<std::byte> ad; // No associated data

    while (processed < message.size()) {
        size_t chunk_size = std::min(message.size() - processed, NOISE_MAX_CHUNK_SIZE);
        Span<std::byte> chunk = message.subspan(processed, chunk_size);
        if (!DecryptWithAd(ad, chunk)) return false;
        processed += chunk_size;
    }

    return true;
}

void Sv2SymmetricState::MixHash(const Span<const std::byte> input)
{
    m_hash_output = (HashWriter{} << m_hash_output << input).GetSHA256();
}

void Sv2SymmetricState::MixKey(const Span<const uint8_t> input_key_material)
{
    uint8_t out0[KEY_SIZE], out1[KEY_SIZE];

    HKDF2(input_key_material, out0, out1);

    std::memset(m_chaining_key, 0, sizeof(m_chaining_key));
    std::copy(out0, out0 + KEY_SIZE, m_chaining_key);
    m_cipher_state = Sv2CipherState{out1};
}

std::string Sv2SymmetricState::GetChainingKey()
{
    return HexStr(m_chaining_key);
}

void Sv2SymmetricState::LogChainingKey()
{
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Chaining key: %s\n", GetChainingKey());
}

void Sv2SymmetricState::HKDF2(const Span<const uint8_t> input_key_material, uint8_t out0[KEY_SIZE], uint8_t out1[KEY_SIZE])
{
    uint8_t tmp_key[KEY_SIZE];
    CHMAC_SHA256 tmp_mac(m_chaining_key, KEY_SIZE);
    tmp_mac.Write(input_key_material.begin(), input_key_material.size());
    tmp_mac.Finalize(tmp_key);

    CHMAC_SHA256 out0_mac(tmp_key, KEY_SIZE);
    uint8_t one[1]{0x1};
    out0_mac.Write(one, 1);
    out0_mac.Finalize(out0);

    std::vector<uint8_t> in1;
    in1.reserve(KEY_SIZE + 1);
    std::copy(out0, out0 + KEY_SIZE, std::back_inserter(in1));
    in1.push_back(0x02);

    CHMAC_SHA256 out1_mac(tmp_key, KEY_SIZE);
    out1_mac.Write(&in1[0], in1.size());
    out1_mac.Finalize(out1);
}

void Sv2SymmetricState::EncryptAndHash(Span<std::byte> data)
{
    m_cipher_state.EncryptWithAd(MakeByteSpan(m_hash_output), data);
    MixHash(data);
}

bool Sv2SymmetricState::DecryptAndHash(Span<std::byte> data)
{
    // The handshake requires mix hashing the cipher text NOT the decrypted
    // plaintext.
    std::vector<std::byte> cipher_text(data.begin(), data.end());
    bool res = m_cipher_state.DecryptWithAd(MakeByteSpan(m_hash_output), data);
    if (!res) return false;
    MixHash(cipher_text);
    return true;
}

std::array<Sv2CipherState, 2> Sv2SymmetricState::Split()
{
    uint8_t send_key[KEY_SIZE], recv_key[KEY_SIZE];

    std::vector<uint8_t> empty;
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

void Sv2HandshakeState::GenerateEphemeralKey(CKey& key) noexcept
{
    Assume(!key.size());
    key.MakeNewKey(true);
    Assume(XOnlyPubKey(key.GetPubKey()).IsFullyValid());
};

void Sv2HandshakeState::WriteMsgEphemeralPK(Span<std::byte> msg)
{
    if (msg.size() < KEY_SIZE) {
        throw std::runtime_error(strprintf("Invalid message size: %d bytes < %d", msg.size(), KEY_SIZE));
    }

    GenerateEphemeralKey(m_ephemeral_key);

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Write our ephemeral key\n");

    auto ephemeral_pk = XOnlyPubKey(m_ephemeral_key.GetPubKey());
    std::transform(ephemeral_pk.begin(), ephemeral_pk.end(), msg.begin(),
               [](unsigned char b) { return static_cast<std::byte>(b); });

    m_symmetric_state.MixHash(Span(msg.begin(), KEY_SIZE));
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    std::vector<std::byte> empty;
    m_symmetric_state.MixHash(empty);
}

void Sv2HandshakeState::ReadMsgEphemeralPK(Span<std::byte> msg) {
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Write their ephemeral key\n");
    auto ucharSpan = UCharSpanCast(msg);
    m_remote_ephemeral_key = XOnlyPubKey(Span(&ucharSpan[0], KEY_SIZE));

    if (!m_remote_ephemeral_key.IsFullyValid()) {
       throw std::runtime_error("Sv2HandshakeState::ReadMsgEphemeralPK(): Received invalid remote ephemeral key");
    }
    m_symmetric_state.MixHash(Span(&msg[0], KEY_SIZE));
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    std::vector<std::byte> empty;
    m_symmetric_state.MixHash(empty);
}

void Sv2HandshakeState::WriteMsgES(Span<std::byte> msg)
{
    ssize_t bytes_written = 0;

    Assume(m_remote_ephemeral_key.IsFullyValid());

    GenerateEphemeralKey(m_ephemeral_key);

    // Send our ephemeral pk.
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Write our ephemeral key\n");
    auto ephemeral_pk = XOnlyPubKey(m_ephemeral_key.GetPubKey());
    Assume(ephemeral_pk.IsFullyValid());
    std::transform(ephemeral_pk.begin(), ephemeral_pk.end(), msg.begin(),
               [](unsigned char b) { return static_cast<std::byte>(b); });

    m_symmetric_state.MixHash(Span(msg.begin(), KEY_SIZE));
    bytes_written += KEY_SIZE;

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Perform ECDH with the remote ephemeral key\n");
    uint8_t ecdh_output[ECDH_OUTPUT_SIZE] = {};
    if (!m_ephemeral_key.ECDH(m_remote_ephemeral_key, ecdh_output)) {
        throw std::runtime_error("Failed to perform ECDH on the remote ephemeral key using our ephemeral key");
    }
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Mix key with ECDH result: ephemeral ours -- remote ephemeral\n");
    m_symmetric_state.MixKey(Span(ecdh_output));
    m_symmetric_state.LogChainingKey();

    // Send our static pk.
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Encrypt and write our static key\n");
    auto static_pk = XOnlyPubKey(m_static_key.GetPubKey());
    Assume(static_pk.IsFullyValid());
    std::transform(static_pk.begin(), static_pk.end(), msg.begin() + KEY_SIZE,
               [](unsigned char b) { return static_cast<std::byte>(b); });
    m_symmetric_state.EncryptAndHash(Span(msg.begin() + KEY_SIZE, KEY_SIZE + POLY1305_TAGLEN));
    bytes_written += KEY_SIZE + POLY1305_TAGLEN;

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Perform ECDH between our static and remote ephemeral key\n");
    uint8_t ecdh_output_remote[ECDH_OUTPUT_SIZE];
    if (!m_static_key.ECDH(m_remote_ephemeral_key, ecdh_output_remote)) {
        throw std::runtime_error("Failed to perform ECDH on the remote ephemeral key using our static key");
    }
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "ECDH result: %s\n", HexStr(ecdh_output_remote));
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Mix key with ECDH result: static ours -- remote ephemeral\n");
    m_symmetric_state.MixKey(Span(ecdh_output_remote));
    m_symmetric_state.LogChainingKey();

    // Serialize our digital signature noise message and encrypt.
    DataStream ss{};
    Assume(m_certificate);
    ss << m_certificate.value();
    Assume(ss.size() == SIGNATURE_NOISE_MESSAGE_SIZE);
    std::copy(ss.begin(), ss.end(), msg.begin() + bytes_written);
    LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Our certificate: %s\n", HexStr(Span(msg.begin() + bytes_written, SIGNATURE_NOISE_MESSAGE_SIZE)));

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Encrypt certificate\n");
    m_symmetric_state.EncryptAndHash(Span(msg.begin() + bytes_written, SIGNATURE_NOISE_MESSAGE_SIZE + POLY1305_TAGLEN));

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    bytes_written += SIGNATURE_NOISE_MESSAGE_SIZE + POLY1305_TAGLEN;
    Assume(bytes_written == INITIATOR_EXPECTED_HANDSHAKE_MESSAGE_LENGTH);
}

// This should not be used outside of test code without further scrutiny.
bool Sv2HandshakeState::ReadMsgES(Span<std::byte> msg)
{
    Assume(msg.size() == INITIATOR_EXPECTED_HANDSHAKE_MESSAGE_LENGTH);
    ssize_t bytes_read = 0;

    // Read the remote ephmeral key from the msg and decrypt.
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Read remote ephemeral key\n");
    auto remote_ephemeral_key_span = UCharSpanCast(Span(msg.begin(), KEY_SIZE));
    m_remote_ephemeral_key = XOnlyPubKey(remote_ephemeral_key_span);
    if (!m_remote_ephemeral_key.IsFullyValid()) {
    throw std::runtime_error("Sv2HandshakeState::ReadMsgES(): Received invalid remote ephemeral key");
    }
    bytes_read += KEY_SIZE;

    m_symmetric_state.MixHash(Span(msg.begin(), KEY_SIZE));
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Perform ECDH with the remote ephemeral key\n");
    uint8_t ecdh_output[ECDH_OUTPUT_SIZE];
    m_ephemeral_key.ECDH(m_remote_ephemeral_key, ecdh_output);

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Mix key with ECDH result: ephemeral ours -- remote ephemeral\n");
    m_symmetric_state.MixKey(Span(ecdh_output));
    m_symmetric_state.LogChainingKey();

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Decrypt remote static key\n");
    bool res = m_symmetric_state.DecryptAndHash(Span(msg.begin() + KEY_SIZE, KEY_SIZE + POLY1305_TAGLEN));
    if (!res) return false;
    bytes_read += KEY_SIZE + POLY1305_TAGLEN;

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    // Load remote static key from the decryted msg
    auto remote_static_key_span = UCharSpanCast(Span(msg.begin() + KEY_SIZE, KEY_SIZE));
    m_remote_static_key = XOnlyPubKey(remote_static_key_span);

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Check if remote static key is valid\n");
    if (!m_remote_static_key.IsFullyValid()) {
        throw std::runtime_error("Sv2HandshakeState::ReadMsgES(): Received invalid remote static key");
    }

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Perform ECDH on the remote static key\n");
    uint8_t ecdh_output_remote[ECDH_OUTPUT_SIZE];
    if (!m_ephemeral_key.ECDH(m_remote_static_key, ecdh_output_remote)) {
        throw std::runtime_error("Failed to perform ECDH on the remote static key using our ephemeral key");
    }
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "ECDH result: %s\n", HexStr(ecdh_output_remote));
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Mix key with ECDH result: ephemeral ours -- remote static\n");
    m_symmetric_state.MixKey(Span(ecdh_output_remote));
    m_symmetric_state.LogChainingKey();


    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Decrypt remote certificate\n");
    res = m_symmetric_state.DecryptAndHash(Span(msg.begin() + bytes_read, SIGNATURE_NOISE_MESSAGE_SIZE + POLY1305_TAGLEN));
    if (!res) return false;
    auto cert_span = UCharSpanCast(Span(msg.begin() + bytes_read, SIGNATURE_NOISE_MESSAGE_SIZE));
    bytes_read += (SIGNATURE_NOISE_MESSAGE_SIZE + POLY1305_TAGLEN);

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Validate remote certificate\n");
    DataStream ss_cert(cert_span);
    Sv2SignatureNoiseMessage cert;
    ss_cert >> cert;
    cert.m_static_key = m_remote_static_key;
    Assume(m_authority_pubkey);
    if (!cert.Validate(m_authority_pubkey.value())) {
        LogPrintLevel(BCLog::SV2, BCLog::Level::Error, "Invalid certificate: %s\n", HexStr(cert_span));
        return false;
    }

    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Mix hash: %s\n", HexStr(m_symmetric_state.GetHashOutput()));

    Assume(bytes_read == INITIATOR_EXPECTED_HANDSHAKE_MESSAGE_LENGTH);
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

Sv2Cipher::Sv2Cipher(CKey&& static_key, XOnlyPubKey&& authority_pubkey)
{
    m_handshake_state = std::make_unique<Sv2HandshakeState>(std::move(static_key), std::move(authority_pubkey));
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

size_t Sv2Cipher::EncryptedMessageSize(size_t msg_len) {
    size_t num_chunks = msg_len / (NOISE_MAX_CHUNK_SIZE - POLY1305_TAGLEN);
    if (msg_len % (NOISE_MAX_CHUNK_SIZE - POLY1305_TAGLEN) != 0) {
        num_chunks++;
    }
    return msg_len + (num_chunks * POLY1305_TAGLEN);
}

bool Sv2Cipher::DecryptMessage(Span<std::byte> message)
{
    if (m_initiator) {
        return m_cs2.DecryptMessage(message);
    } else {
        return m_cs1.DecryptMessage(message);
    }
}

void Sv2Cipher::EncryptMessage(Span<const std::byte> input, Span<std::byte> output)
{
    Assume(output.size() == Sv2Cipher::EncryptedMessageSize(input.size()));

    if (m_initiator) {
        m_cs1.EncryptMessage(input, output);
    } else {
        m_cs2.EncryptMessage(input, output);
    }
}

uint256 Sv2Cipher::GetHash() const
{
    return m_hash;
}
