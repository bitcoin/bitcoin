#pragma once

#include "secp256k1-zkp.h"
#include <mw/models/crypto/Commitment.h>
#include <mw/models/crypto/PublicKey.h>
#include <mw/models/crypto/Signature.h>

class ConversionUtil
{
public:
    static PublicKey ToPublicKey(const Commitment& commitment);
    static PublicKey ToPublicKey(const secp256k1_pubkey& pubkey);
    static Commitment ToCommitment(const secp256k1_pedersen_commitment& commitment);
    static CompactSignature ToCompact(const Signature& signature);
    static CompactSignature ToCompact(const secp256k1_ecdsa_signature& signature);

    static secp256k1_pubkey ToSecp256k1(const PublicKey& publicKey);
    static std::vector<secp256k1_pubkey> ToSecp256k1(const std::vector<PublicKey>& publicKeys);

    static secp256k1_pedersen_commitment ToSecp256k1(const Commitment& commitment);
    static std::vector<secp256k1_pedersen_commitment> ToSecp256k1(const std::vector<Commitment>& commitments);

    static secp256k1_ecdsa_signature ToSecp256k1(const CompactSignature& signature);
    static std::vector<secp256k1_ecdsa_signature> ToSecp256k1(const std::vector<CompactSignature>& signatures);

    static secp256k1_schnorrsig ToSecp256k1(const Signature& signature);
    static std::vector<secp256k1_schnorrsig> ToSecp256k1(const std::vector<const Signature*>& signatures);
    static Signature ToSignature(const secp256k1_schnorrsig& signature);
};