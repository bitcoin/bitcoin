#include <mw/models/tx/Input.h>
#include <mw/crypto/Schnorr.h>
#include <mw/crypto/SecretKeys.h>

Input Input::Create(const mw::Hash& output_id, const Commitment& commitment, const SecretKey& input_key, const SecretKey& output_key)
{
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

    return Input(
        output_id,
        commitment,
        std::move(input_pubkey),
        std::move(output_pubkey),
        Schnorr::Sign(sig_key.data(), output_id)
    );
}

SignedMessage Input::BuildSignedMsg() const noexcept
{
    // Hash keys (K_i||K_o)
    Hasher key_hasher;
    key_hasher << GetInputPubKey() << GetOutputPubKey();
    SecretKey key_hash = key_hasher.hash();

    // Calculate aggregated key K_agg = K_i + HASH(K_i||K_o) * K_o
    PublicKey public_key = GetOutputPubKey()
        .Mul(key_hash)
        .Add(GetInputPubKey());

    return SignedMessage{GetOutputID(), public_key, GetSignature()};
}