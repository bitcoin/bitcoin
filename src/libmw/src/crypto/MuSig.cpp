#include <mw/crypto/MuSig.h>
#include "Context.h"
#include "ConversionUtil.h"

#include <mw/common/Logger.h>
#include <mw/exceptions/CryptoException.h>
#include <mw/util/VectorUtil.h>

static Locked<Context> MUSIG_CONTEXT(std::make_shared<Context>());

SecretKey MuSig::GenerateSecureNonce()
{
    SecretKey nonce;
    const int result = secp256k1_aggsig_export_secnonce_single(
        MUSIG_CONTEXT.Read()->Get(),
        nonce.data(),
        SecretKey::Random().data()
    );
    if (result != 1) {
        ThrowCrypto_F("secp256k1_aggsig_export_secnonce_single failed with error: {}", result);
    }

    return nonce;
}

CompactSignature MuSig::CalculatePartial(
    const SecretKey& secretKey,
    const SecretKey& secretNonce,
    const PublicKey& sumPubKeys,
    const PublicKey& sumPubNonces,
    const mw::Hash& message)
{
    secp256k1_pubkey pubKeyForE = ConversionUtil::ToSecp256k1(sumPubKeys);
    secp256k1_pubkey pubNoncesForE = ConversionUtil::ToSecp256k1(sumPubNonces);

    secp256k1_ecdsa_signature signature;
    const int signedResult = secp256k1_aggsig_sign_single(
        MUSIG_CONTEXT.Write()->Randomized(),
        signature.data,
        message.data(),
        secretKey.data(),
        secretNonce.data(),
        nullptr,
        &pubNoncesForE,
        &pubNoncesForE,
        &pubKeyForE,
        SecretKey::Random().data()
    );
    if (signedResult != 1) {
        ThrowCrypto("Failed to calculate partial signature.");
    }

    return ConversionUtil::ToCompact(signature);
}

bool MuSig::VerifyPartial(
    const CompactSignature& partialSignature,
    const PublicKey& publicKey,
    const PublicKey& sumPubKeys,
    const PublicKey& sumPubNonces,
    const mw::Hash& message)
{
    secp256k1_ecdsa_signature signature = ConversionUtil::ToSecp256k1(partialSignature);

    secp256k1_pubkey pubkey = ConversionUtil::ToSecp256k1(publicKey);
    secp256k1_pubkey sumPubKey = ConversionUtil::ToSecp256k1(sumPubKeys);
    secp256k1_pubkey sumNoncesPubKey = ConversionUtil::ToSecp256k1(sumPubNonces);

    const int verifyResult = secp256k1_aggsig_verify_single(
        MUSIG_CONTEXT.Read()->Get(),
        signature.data,
        message.data(),
        &sumNoncesPubKey,
        &pubkey,
        &sumPubKey,
        nullptr,
        true
    );

    return verifyResult == 1;
}

Signature MuSig::Aggregate(
    const std::vector<CompactSignature>& signatures,
    const PublicKey& sumPubNonces)
{
    assert(!signatures.empty());

    secp256k1_pubkey pubNonces = ConversionUtil::ToSecp256k1(sumPubNonces);

    std::vector<secp256k1_ecdsa_signature> parsedSignatures = ConversionUtil::ToSecp256k1(signatures);
    std::vector<secp256k1_ecdsa_signature*> signaturePtrs = VectorUtil::ToPointerVec(parsedSignatures);

    secp256k1_ecdsa_signature aggregatedSignature;
    const int result = secp256k1_aggsig_add_signatures_single(
        MUSIG_CONTEXT.Read()->Get(),
        aggregatedSignature.data,
        (const unsigned char**)signaturePtrs.data(),
        signaturePtrs.size(),
        &pubNonces
    );
    if (result != 1) {
        ThrowCrypto("Failed to aggregate signatures");
    }

    return Signature(aggregatedSignature.data);
}