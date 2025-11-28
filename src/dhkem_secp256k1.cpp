// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dhkem_secp256k1.h>

#include <crypto/hmac_sha256.h>
#include <random.h>
#include <secp256k1_ecdh.h>
#include <span>
#include <array>
#include <cassert>
#include <cstring>
#include <support/allocators/secure.h>
#include <optional>
#include <crypto/chacha20poly1305.h>
#include <mutex>
#include <support/cleanse.h>

namespace dhkem_secp256k1 {

static secp256k1_context* g_secp256k1_ctx = nullptr;
static std::once_flag g_ctx_init_flag;

/** Ensure global secp256k1 context is initialized (call InitContext() before use). */
static void InitCtx() {
    std::call_once(g_ctx_init_flag, [](){
        // Create context (no need for signing in this usage, verification is implicit in pubkey parsing)
        g_secp256k1_ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
        assert(g_secp256k1_ctx != nullptr);
        // Randomize the secp256k1 context with a blinding seed
        std::vector<uint8_t, secure_allocator<uint8_t>> vseed(32);
        GetRandBytes(vseed);
        bool ret = secp256k1_context_randomize(g_secp256k1_ctx, vseed.data());
        assert(ret);
    });
}

void InitContext() {
    InitCtx();
}

// Basic HKDF (SHA-256) functions
void HKDF_Extract(const uint8_t* salt, size_t salt_len,
                  const uint8_t* ikm, size_t ikm_len,
                  uint8_t out_prk[32]) {
    // If no salt, use a 32-byte array of zeroes (per RFC5869) instead of nullptr.
    static const uint8_t zero_salt[32] = {0};
    const uint8_t*  hmac_key    = salt_len > 0 ? salt : zero_salt;
    size_t          hmac_keylen = salt_len > 0 ? salt_len : 32;
    CHMAC_SHA256 hmac(hmac_key, hmac_keylen);
    hmac.Write(ikm, ikm_len);
    hmac.Finalize(out_prk);
}

void HKDF_Expand32(const uint8_t prk[32], const uint8_t* info_data, size_t info_len,
                   uint8_t out_okm[], size_t L) {
    // Only a single round is needed since L <= 32 bytes
    unsigned char counter = 0x01;
    CHMAC_SHA256 hmac(prk, 32);
    hmac.Write(info_data, info_len);
    hmac.Write(&counter, 1);
    unsigned char output_block[32];
    hmac.Finalize(output_block);
    std::memcpy(out_okm, output_block, L);
}

// -------------------- Static Helpers for Clarity and Reuse --------------------

static const uint8_t KEM_SUITE_ID[] = {'K','E','M', 0x00, 0x16};  // Suite ID for secp256k1 KEM

// ECDH hash function that outputs the 32-byte X coordinate (ignoring Y)
static int EcdhHashFunctionRaw(unsigned char* output, const unsigned char* x32, const unsigned char* /*y32*/, void* /*data*/) {
    memcpy(output, x32, 32);
    return 1;
}

// Compute the 32-byte X-coordinate shared secret for ECDH (priv * pub)
static bool EcdhXCoordinate(const secp256k1_pubkey& pub, const uint8_t priv[32], uint8_t out_shared[32]) {
    return secp256k1_ecdh(g_secp256k1_ctx, out_shared, &pub, priv, EcdhHashFunctionRaw, nullptr) == 1;
}

// Compute a secp256k1 public key (65-byte uncompressed) from a 32-byte private key
static bool ComputePublicKey(const uint8_t priv_key[32], uint8_t out_pubkey[65]) {
    if (secp256k1_ec_seckey_verify(g_secp256k1_ctx, priv_key) != 1) {
        return false;
    }
    secp256k1_pubkey pub;
    if (secp256k1_ec_pubkey_create(g_secp256k1_ctx, &pub, priv_key) != 1) {
        return false;
    }
    size_t pub_len = 65;
    secp256k1_ec_pubkey_serialize(g_secp256k1_ctx, out_pubkey, &pub_len, &pub, SECP256K1_EC_UNCOMPRESSED);
    assert(pub_len == 65);
    return true;
}

// Build KEM context for Base mode: kem_context = enc || pkR (130 bytes total)
static void BuildKemContext(const uint8_t enc[65], const uint8_t pkR[65], uint8_t out_context[130]) {
    memcpy(out_context,       enc, 65);
    memcpy(out_context + 65,  pkR, 65);
}

// Build KEM context for Auth modes: kem_context = enc || pkR || pkS (195 bytes total)
static void BuildKemContext(const uint8_t enc[65], const uint8_t pkR[65], const uint8_t pkS[65], uint8_t out_context[195]) {
    memcpy(out_context,        enc, 65);
    memcpy(out_context + 65,   pkR, 65);
    memcpy(out_context + 130,  pkS, 65);
}

// LabeledExtract: HKDF-Extract with salt="", labeled IKM = "HPKE-v1" || KEM_SUITE_ID || label || ikm
static void LabeledExtract(const char* label, const uint8_t* ikm, size_t ikm_len, uint8_t out_prk[32]) {
    // Build labeled IKM
    size_t prefix_len = sizeof(LABEL_PREFIX);    // "HPKE-v1" (7 bytes)
    size_t suite_len  = sizeof(KEM_SUITE_ID);    // "KEM<id>" (5 bytes)
    size_t label_len  = std::strlen(label);
    std::vector<uint8_t> labeled_ikm;
    labeled_ikm.reserve(prefix_len + suite_len + label_len + ikm_len);
    labeled_ikm.insert(labeled_ikm.end(), LABEL_PREFIX, LABEL_PREFIX + prefix_len);
    labeled_ikm.insert(labeled_ikm.end(), KEM_SUITE_ID, KEM_SUITE_ID + suite_len);
    labeled_ikm.insert(labeled_ikm.end(), reinterpret_cast<const uint8_t*>(label), reinterpret_cast<const uint8_t*>(label) + label_len);
    labeled_ikm.insert(labeled_ikm.end(), ikm, ikm + ikm_len);
    // HKDF-Extract with empty salt
    HKDF_Extract(nullptr, 0, labeled_ikm.data(), labeled_ikm.size(), out_prk);
}

// LabeledExpand: HKDF-Expand to length out_len with info = length||"HPKE-v1"||KEM_SUITE_ID||label||info_data
static void LabeledExpand(const uint8_t prk[32], const char* label, const uint8_t* info, size_t info_len, uint8_t out[], size_t out_len) {
    // Build labeled info
    uint8_t length_bytes[2] = { static_cast<uint8_t>((out_len >> 8) & 0xFF), static_cast<uint8_t>(out_len & 0xFF) };
    size_t prefix_len = sizeof(LABEL_PREFIX);
    size_t suite_len  = sizeof(KEM_SUITE_ID);
    size_t label_len  = std::strlen(label);
    std::vector<uint8_t> labeled_info;
    labeled_info.reserve(2 + prefix_len + suite_len + label_len + info_len);
    labeled_info.insert(labeled_info.end(), length_bytes, length_bytes + 2);
    labeled_info.insert(labeled_info.end(), LABEL_PREFIX, LABEL_PREFIX + prefix_len);
    labeled_info.insert(labeled_info.end(), KEM_SUITE_ID, KEM_SUITE_ID + suite_len);
    labeled_info.insert(labeled_info.end(), reinterpret_cast<const uint8_t*>(label), reinterpret_cast<const uint8_t*>(label) + label_len);
    labeled_info.insert(labeled_info.end(), info, info + info_len);
    // HKDF-Expand (single-block, since out_len <= 32)
    HKDF_Expand32(prk, labeled_info.data(), labeled_info.size(), out, out_len);
}

// DeriveSharedSecret: derive the 32-byte KEM shared secret from ECDH output and kem_context
// (uses HKDF-Extract "eae_prk" and HKDF-Expand "shared_secret" as defined in RFC 9180)
static std::array<uint8_t, 32> DeriveSharedSecret(const uint8_t* dh, size_t dh_len,
                                                 const uint8_t* kem_context, size_t kem_context_len) {
    uint8_t prk_eae[32];
    LabeledExtract("eae_prk", dh, dh_len, prk_eae);
    std::array<uint8_t, 32> shared;
    LabeledExpand(prk_eae, "shared_secret", kem_context, kem_context_len, shared.data(), shared.size());
    memory_cleanse(prk_eae, 32);
    return shared;
}

// -------------------- Public API Functions --------------------

bool DeriveKeyPair(std::span<const uint8_t> ikm,
                   std::array<uint8_t, 32>& outPrivKey,
                   std::array<uint8_t, 65>& outPubKey)
{
    // Domain-separate the input keying material (label "dkp_prk") and derive a valid secp256k1 key pair (RFC 9180 §7.1.3)
    uint8_t prk[32];
    LabeledExtract("dkp_prk", ikm.data(), ikm.size(), prk);
    const char* candidate_label = "candidate";
    uint8_t candidate[32];
    // Try up to 256 candidates by expanding "candidate" with a counter until a valid secret scalar is found (rejection sampling)
    bool success = false;
    for (int counter = 0; counter < 256; ++counter) {
        uint8_t ctr = static_cast<uint8_t>(counter);
        LabeledExpand(prk, candidate_label, &ctr, 1, candidate, 32);
        candidate[0] &= 0xFF; // Masking (no effect for 0xFF, included for spec completeness)
        if (ComputePublicKey(candidate, outPubKey.data())) {
            // Found a valid secret scalar; output the private key and corresponding public key
            std::memcpy(outPrivKey.data(), candidate, 32);
            success = true;
            break;
        }
    }
    memory_cleanse(prk, 32);
    memory_cleanse(candidate, 32);
    return success;
}

std::optional<std::array<uint8_t, 32>>
Encap(std::span<const uint8_t> pkR, const std::array<uint8_t, 32>& skE, const std::array<uint8_t, 65>& enc) {
    if (pkR.size() != NPK) {
        return std::nullopt; // Recipient public key must be 65 bytes (uncompressed)
    }
    InitCtx();
    // Note: Using provided ephemeral key pair (skE, enc); no new random key is generated here.

    // 1. Parse the recipient's public key bytes into a secp256k1_pubkey object
    secp256k1_pubkey pubR;
    if (!secp256k1_ec_pubkey_parse(g_secp256k1_ctx, &pubR, pkR.data(), pkR.size())) {
        return std::nullopt; // Invalid recipient public key format
    }

    // 1.5. Validate that enc corresponds to skE
    std::array<uint8_t, 65> comp_enc;
    if (!ComputePublicKey(skE.data(), comp_enc.data())) {
        return std::nullopt; // Invalid ephemeral private key
    }
    if (std::memcmp(comp_enc.data(), enc.data(), NENC) != 0) {
        return std::nullopt; // Ephemeral pubkey mismatch
    }

    // 2. Compute ECDH shared secret: dh = x-coordinate of (skE * pkR)
    std::array<uint8_t, 32> dh;
    if (!EcdhXCoordinate(pubR, skE.data(), dh.data())) {
        return std::nullopt; // ECDH failed (unexpected if inputs are valid)
    }

    // 3. Build KEM context = enc || pkR (130 bytes)
    uint8_t kem_context[130];
    BuildKemContext(enc.data(), pkR.data(), kem_context);

    // 4. Derive the 32-byte shared secret via HKDF
    std::array<uint8_t, 32> shared_secret = DeriveSharedSecret(dh.data(), dh.size(), kem_context, sizeof(kem_context));
    memory_cleanse(dh.data(), dh.size());
    return shared_secret;
}

std::optional<std::array<uint8_t, 32>>
Decap(std::span<const uint8_t> enc, std::span<const uint8_t> skR) {
    if (enc.size() != NENC || skR.size() != NSK) {
        return std::nullopt; // Inputs must be 65-byte enc and 32-byte skR
    }
    InitCtx();

    // 1. Parse the encapsulated ephemeral public key
    secp256k1_pubkey pubE;
    if (!secp256k1_ec_pubkey_parse(g_secp256k1_ctx, &pubE, enc.data(), enc.size())) {
        return std::nullopt; // Invalid ephemeral public key
    }

    // 2. Verify recipient's private key (skR) is valid
    if (secp256k1_ec_seckey_verify(g_secp256k1_ctx, skR.data()) != 1) {
        return std::nullopt; // Invalid recipient private key
    }

    // 3. Compute ECDH shared secret: dh = x-coordinate of (skR * pubE)
    std::array<uint8_t, 32> dh;
    if (!EcdhXCoordinate(pubE, skR.data(), dh.data())) {
        return std::nullopt; // ECDH failed (invalid inputs)
    }

    // 4. Derive pkR (recipient's public key) from skR for KEM context
    std::array<uint8_t, 65> pkR_bytes;
    if (!ComputePublicKey(skR.data(), pkR_bytes.data())) {
        return std::nullopt; // Should not happen if skR is valid
    }

    // 5. Build KEM context = enc || pkR (130 bytes total)
    uint8_t kem_context[130];
    BuildKemContext(enc.data(), pkR_bytes.data(), kem_context);

    // 6. Derive the 32-byte shared secret via HKDF
    std::array<uint8_t, 32> shared_secret = DeriveSharedSecret(dh.data(), dh.size(), kem_context, sizeof(kem_context));
    memory_cleanse(dh.data(), dh.size());
    return shared_secret;
}

std::vector<uint8_t> LabeledExpand(const std::vector<uint8_t>& prk,
                                   const std::vector<uint8_t>& label,
                                   const std::vector<uint8_t>& info,
                                   size_t L)
{
    // 1. Construct labeled_info = length (2 bytes, big-endian) || label_prefix || suite_id || label || info
    uint8_t length_bytes[2];
    length_bytes[0] = static_cast<uint8_t>((L >> 8) & 0xff);
    length_bytes[1] = static_cast<uint8_t>(L & 0xff);

    std::vector<uint8_t> labeled_info;
    labeled_info.insert(labeled_info.end(), length_bytes, length_bytes + 2);
    labeled_info.insert(labeled_info.end(), std::begin(LABEL_PREFIX), std::end(LABEL_PREFIX));
    labeled_info.insert(labeled_info.end(), std::begin(SUITE_ID), std::end(SUITE_ID));
    labeled_info.insert(labeled_info.end(), label.begin(), label.end());
    labeled_info.insert(labeled_info.end(), info.begin(), info.end());

    // 2. Expand to L bytes
    std::vector<uint8_t> out_okm(L);
    HKDF_Expand32(prk.data(), labeled_info.data(), labeled_info.size(), out_okm.data(), L);
    return out_okm;
}

std::vector<uint8_t> LabeledExtract(const std::vector<uint8_t>& salt,
                                    const std::vector<uint8_t>& label,
                                    const std::vector<uint8_t>& ikm)
{
    // 1. Construct labeled_ikm = label_prefix || suite_id || label || ikm
    std::vector<uint8_t> labeled_ikm;
    labeled_ikm.insert(labeled_ikm.end(), std::begin(LABEL_PREFIX), std::end(LABEL_PREFIX));
    labeled_ikm.insert(labeled_ikm.end(), std::begin(SUITE_ID), std::end(SUITE_ID));
    labeled_ikm.insert(labeled_ikm.end(), label.begin(), label.end());
    labeled_ikm.insert(labeled_ikm.end(), ikm.begin(), ikm.end());

    // 2. Extract to obtain PRK
    uint8_t out_prk[32];
    HKDF_Extract(salt.data(), salt.size(), labeled_ikm.data(), labeled_ikm.size(), out_prk);
    // 3. Return the PRK as a 32-byte vector
    return std::vector<uint8_t>(out_prk, out_prk + 32);
}

std::optional<std::array<uint8_t, 32>>
AuthEncap(const std::array<uint8_t, 65>& pkR,
          const std::array<uint8_t, 32>& skS,
          const std::array<uint8_t, 32>& skE,
          const std::array<uint8_t, 65>& enc)
{
    InitCtx();
    // Authenticated KEM Encapsulation (Auth mode) - uses static sender key and ephemeral key

    // 1. Parse recipient's public key (pkR) into secp256k1_pubkey
    secp256k1_pubkey pubR;
    if (!secp256k1_ec_pubkey_parse(g_secp256k1_ctx, &pubR, pkR.data(), pkR.size())) {
        return std::nullopt; // Invalid recipient public key
    }

    // 2. Compute sender's public key (pkS) from sender's private key (skS)
    std::array<uint8_t, 65> pkS_bytes;
    if (!ComputePublicKey(skS.data(), pkS_bytes.data())) {
        return std::nullopt; // Invalid sender private key
    }

    // 2.5. Verify that enc matches skE
    std::array<uint8_t, 65> comp_enc;
    if (!ComputePublicKey(skE.data(), comp_enc.data())) {
        return std::nullopt;
    }
    if (std::memcmp(comp_enc.data(), enc.data(), NENC) != 0) {
        return std::nullopt; // Provided enc does not correspond to skE
    }

    // 4. Compute two ECDH shared secrets:
    //    DH1 = x-coordinate of (skE * pkR), DH2 = x-coordinate of (skS * pkR)
    std::array<uint8_t, 32> dh1, dh2;
    if (!EcdhXCoordinate(pubR, skE.data(), dh1.data())) {
        return std::nullopt;
    }
    if (!EcdhXCoordinate(pubR, skS.data(), dh2.data())) {
        return std::nullopt;
    }

    // 5. Concatenate DH1 || DH2 (64 bytes total)
    uint8_t dh_concat[64];
    std::memcpy(dh_concat,      dh1.data(), 32);
    std::memcpy(dh_concat + 32, dh2.data(), 32);

    // 6. Assemble HPKE KEM context: enc || pkR || pkS (195 bytes)
    uint8_t kem_context[65 * 3];
    BuildKemContext(enc.data(), pkR.data(), pkS_bytes.data(), kem_context);

    // 7. Derive the shared secret via HKDF (using dh_concat and kem_context)
    std::array<uint8_t, 32> shared = DeriveSharedSecret(dh_concat, sizeof(dh_concat), kem_context, sizeof(kem_context));
    memory_cleanse(dh1.data(), dh1.size());
    memory_cleanse(dh2.data(), dh2.size());
    memory_cleanse(dh_concat, 64);
    return shared;
}

std::optional<std::array<uint8_t, 32>>
AuthDecap(const std::array<uint8_t, 65>& enc,
          const std::array<uint8_t, 32>& skR,
          const std::array<uint8_t, 65>& pkS)
{
    InitCtx();
    // Authenticated KEM Decapsulation (Auth mode) - uses sender's static public key

    // 1. Parse sender's static public key (pkS) and encapsulated ephemeral public key (enc)
    secp256k1_pubkey pubS, pubE;
    if (!secp256k1_ec_pubkey_parse(g_secp256k1_ctx, &pubS, pkS.data(), pkS.size())) {
        return std::nullopt; // Invalid sender public key
    }
    if (!secp256k1_ec_pubkey_parse(g_secp256k1_ctx, &pubE, enc.data(), enc.size())) {
        return std::nullopt; // Invalid encapsulated public key
    }

    // 2. Verify receiver's private key (skR) is valid
    if (secp256k1_ec_seckey_verify(g_secp256k1_ctx, skR.data()) != 1) {
        return std::nullopt; // Invalid receiver private key
    }

    // 2. Perform two ECDH operations with receiver's private key (skR):
    //    DH1 = x-coordinate of (skR * pubE), DH2 = x-coordinate of (skR * pubS)
    std::array<uint8_t, 32> dh1, dh2;
    if (!EcdhXCoordinate(pubE, skR.data(), dh1.data())) {
        return std::nullopt;
    }
    if (!EcdhXCoordinate(pubS, skR.data(), dh2.data())) {
        return std::nullopt;
    }

    // 3. Concatenate DH1 || DH2 (64 bytes)
    uint8_t dh_concat[64];
    std::memcpy(dh_concat,      dh1.data(), 32);
    std::memcpy(dh_concat + 32, dh2.data(), 32);

    // 4. Derive receiver's public key (pkR) from skR for KEM context
    std::array<uint8_t, 65> pkR_bytes;
    if (!ComputePublicKey(skR.data(), pkR_bytes.data())) {
        return std::nullopt; // Invalid receiver private key
    }

    // 5. Build KEM context = enc || pkR || pkS (195 bytes)
    uint8_t kem_context[65 * 3];
    BuildKemContext(enc.data(), pkR_bytes.data(), pkS.data(), kem_context);

    // 6. Derive the 32-byte shared secret via HKDF
    std::array<uint8_t, 32> shared = DeriveSharedSecret(dh_concat, sizeof(dh_concat), kem_context, sizeof(kem_context));
    memory_cleanse(dh1.data(), dh1.size());
    memory_cleanse(dh2.data(), dh2.size());
    memory_cleanse(dh_concat, 64);
    return shared;
}

std::vector<uint8_t> Seal(std::span<const std::byte> key, ChaCha20::Nonce96 nonce,
                          std::span<const std::byte> aad, std::span<const std::byte> plaintext)
{
    assert(key.size() == AEADChaCha20Poly1305::KEYLEN);
    AEADChaCha20Poly1305 aead(key);  // Initialize AEAD with the 32-byte key
    // Allocate output: plaintext length + 16-byte tag
    std::vector<uint8_t> ciphertext(plaintext.size() + AEADChaCha20Poly1305::EXPANSION);
    // Encrypt plaintext; result = ciphertext || tag
    std::span<std::byte> cipher_span(reinterpret_cast<std::byte*>(ciphertext.data()), ciphertext.size());
    aead.Encrypt(plaintext, aad, nonce, cipher_span);
    return ciphertext;
}

std::optional<std::vector<uint8_t>> Open(std::span<const std::byte> key, ChaCha20::Nonce96 nonce,
                                        std::span<const std::byte> aad, std::span<const std::byte> ciphertext)
{
    assert(key.size() == AEADChaCha20Poly1305::KEYLEN);
    // Ciphertext must include at least the 16-byte tag
    if (ciphertext.size() < AEADChaCha20Poly1305::EXPANSION) {
        return std::nullopt;
    }
    size_t plaintext_len = ciphertext.size() - AEADChaCha20Poly1305::EXPANSION;
    std::vector<uint8_t> plaintext(plaintext_len);
    std::span<std::byte> plain_span(reinterpret_cast<std::byte*>(plaintext.data()), plaintext.size());
    AEADChaCha20Poly1305 aead(key); // Initialize AEAD with the 32-byte key
    // Decrypt and verify tag (Decrypt returns false if authentication fails)
    bool ok = aead.Decrypt(ciphertext, aad, nonce, plain_span);
    if (!ok) {
        return std::nullopt; // Tag mismatch or decryption failure
    }
    return plaintext;
}

std::vector<uint8_t> ComputeNonce(const std::vector<uint8_t>& base_nonce, size_t seq) {
    const size_t nonce_size = base_nonce.size();
    const size_t seq_size   = sizeof(seq);

    // 1) Build a zero-initialized buffer the same length as the nonce
    std::vector<uint8_t> seq_buf(nonce_size, 0);

    // 2) Write seq in big-endian into the last seq_size bytes
    for (size_t i = 0; i < seq_size; ++i) {
        // take the (seq_size-1-i)th byte of seq
        seq_buf[nonce_size - seq_size + i] =
            static_cast<uint8_t>((seq >> ((seq_size - 1 - i) * 8)) & 0xFF);
    }

    // 3) XOR base_nonce with seq_buf byte-wise
    std::vector<uint8_t> out(nonce_size);
    for (size_t i = 0; i < nonce_size; ++i) {
        out[i] = base_nonce[i] ^ seq_buf[i];
    }

    return out;
}

//---------------------------------------------------------------------------
// KeySchedule(mode, shared_secret, info)  (RFC 9180 §7.1)
//  - PSK functionality not supported. Only Base (0x00) and Auth (0x02) are supported.
//---------------------------------------------------------------------------

std::optional<Context> KeySchedule(
    uint8_t mode,
    const std::vector<uint8_t>& shared_secret,
    const std::vector<uint8_t>& info)
{
    // 0. Reject unsupported modes (PSK modes are not supported).
    if (mode != 0x00 && mode != 0x02) {
        return std::nullopt; // Only Base and Auth supported
    }

    // 1. psk_id_hash = LabeledExtract("", "psk_id_hash", "")  // PSK not supported -> empty psk_id
    const std::vector<uint8_t> label_psk_id_hash{'p','s','k','_','i','d','_','h','a','s','h'};
    const std::vector<uint8_t> empty_vec;
    auto psk_id_hash = LabeledExtract({}, label_psk_id_hash, empty_vec);

    // 2. info_hash = LabeledExtract("", "info_hash", info)
    const std::vector<uint8_t> label_info_hash{'i','n','f','o','_','h','a','s','h'};
    auto info_hash = LabeledExtract({}, label_info_hash, info);

    // 3. key_schedule_context = mode || psk_id_hash || info_hash
    std::vector<uint8_t> key_schedule_context;
    key_schedule_context.push_back(mode);
    key_schedule_context.insert(key_schedule_context.end(), psk_id_hash.begin(), psk_id_hash.end());
    key_schedule_context.insert(key_schedule_context.end(), info_hash.begin(), info_hash.end());

    // 4. secret = LabeledExtract(shared_secret, "secret", "")  // PSK not supported -> empty psk
    const std::vector<uint8_t> label_secret{'s','e','c','r','e','t'};
    auto secret = LabeledExtract(shared_secret, label_secret, empty_vec);

    // 5. Derive key, base_nonce, exporter_secret with LabeledExpand
    const size_t Nk = 32;
    const size_t Nn = 12;
    const size_t Nh = 32;
    static const std::vector<uint8_t> label_key{'k','e','y'};
    static const std::vector<uint8_t> label_base_nonce{'b','a','s','e','_','n','o','n','c','e'};
    static const std::vector<uint8_t> label_exp{'e','x','p'};

    auto got_key      = LabeledExpand(secret, label_key, key_schedule_context, Nk);
    auto got_nonce    = LabeledExpand(secret, label_base_nonce, key_schedule_context, Nn);
    auto got_exporter = LabeledExpand(secret, label_exp, key_schedule_context, Nh);

    memory_cleanse(secret.data(), secret.size());
    return Context(got_key, got_nonce, got_exporter);
}

} // namespace dhkem_secp256k1
