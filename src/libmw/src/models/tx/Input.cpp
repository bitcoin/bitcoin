#include <mw/models/tx/Input.h>
#include <mw/crypto/Schnorr.h>
#include <mw/crypto/SecretKeys.h>

// Creates a standard input with a stealth key (feature bit = 1)
Input Input::Create(const mw::Hash& output_id, const Commitment& commitment, const SecretKey& input_key, const SecretKey& output_key)
{
    uint8_t features = (uint8_t)FeatureBit::STEALTH_KEY_FEATURE_BIT;

    PublicKey input_pubkey = PublicKey::From(input_key);
    PublicKey output_pubkey = PublicKey::From(output_key);

    // Hash keys (K_i||K_o)
    Hasher key_hasher;
    key_hasher << input_pubkey << output_pubkey;
    SecretKey key_hash = key_hasher.hash();

    // Calculate aggregated key k_agg = k_i + HASH(K_i||K_o) * k_o
    SecretKey sig_key = SecretKeys::From(output_key)
        .Mul(key_hash)
        .Add(input_key)
        .Total();

    // Hash message
    Hasher msg_hasher;
    msg_hasher << features << output_id;
    mw::Hash msg_hash = msg_hasher.hash();

    return Input(
        features,
        output_id,
        commitment,
        std::move(input_pubkey),
        std::move(output_pubkey),
        Schnorr::Sign(sig_key.data(), msg_hash)
    );
}

SignedMessage Input::BuildSignedMsg() const noexcept
{
    // Calculate message hash
    Hasher msg_hasher;
    msg_hasher << m_features << GetOutputID();
    if (m_features & EXTRA_DATA_FEATURE_BIT) {
        msg_hasher << GetExtraData();
    }
    mw::Hash message = msg_hasher.hash();

    // Calculate public key
    PublicKey public_key = GetOutputPubKey();
    if (m_features & STEALTH_KEY_FEATURE_BIT) {
        // Hash keys (K_i||K_o)
        Hasher key_hasher;
        key_hasher << (*GetInputPubKey()) << GetOutputPubKey();
        SecretKey key_hash = key_hasher.hash();

        // Calculate aggregated key K_agg = K_i + HASH(K_i||K_o) * K_o
        public_key = public_key
            .Mul(key_hash)
            .Add(*GetInputPubKey());
    }

    return SignedMessage{message, public_key, GetSignature()};
}