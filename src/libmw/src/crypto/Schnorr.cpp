#include <mw/crypto/Schnorr.h>
#include "Context.h"
#include "ConversionUtil.h"

#include <caches/Cache.h>
#include <mw/common/Logger.h>
#include <mw/exceptions/CryptoException.h>
#include <mw/util/VectorUtil.h>

static Locked<LRUCache<SignedMessage, bool>> CACHE(std::make_shared<LRUCache<SignedMessage, bool>>(3000));
static Locked<Context> SCHNORR_CONTEXT(std::make_shared<Context>());

static constexpr uint64_t MAX_WIDTH = 1 << 20;
static constexpr size_t SCRATCH_SPACE_SIZE = 256 * MAX_WIDTH;

Signature Schnorr::Sign(
    const uint8_t* secretKey,
    const mw::Hash& message)
{
    secp256k1_schnorrsig signature;
    const int signedResult = secp256k1_schnorrsig_sign(
        SCHNORR_CONTEXT.Write()->Randomized(),
        &signature,
        nullptr,
        message.data(),
        secretKey,
        nullptr,
        nullptr
    );
    if (signedResult != 1) {
        ThrowCrypto("Failed to sign message.");
    }

    return ConversionUtil::ToSignature(signature);
}

SignedMessage Schnorr::SignMessage(
    const SecretKey& secretKey,
    const mw::Hash& message)
{
    PublicKey pubkey = PublicKey::From(secretKey);
    Signature sig = Sign(secretKey.data(), message);

    return SignedMessage(message, pubkey, sig);
}

bool Schnorr::Verify(
    const Signature& signature,
    const PublicKey& sumPubKeys,
    const mw::Hash& message)
{
    SignedMessage signed_message(message, sumPubKeys, signature);
    if (CACHE.Write()->Cached(signed_message)) {
        return true;
    }

    secp256k1_pubkey parsedPubKey = ConversionUtil::ToSecp256k1(sumPubKeys);

    const int verifyResult = secp256k1_aggsig_verify_single(
        SCHNORR_CONTEXT.Read()->Get(),
        signature.data(),
        message.data(),
        nullptr,
        &parsedPubKey,
        &parsedPubKey,
        nullptr,
        false
    );
    if (verifyResult == 1) {
        CACHE.Write()->Put(signed_message, true);
    }

    return verifyResult == 1;
}

bool Schnorr::BatchVerify(const std::vector<SignedMessage>& signatures)
{
    std::vector<SignedMessage> unverified_messages;
    std::vector<secp256k1_pubkey> parsedPubKeys;
    std::vector<secp256k1_schnorrsig> parsedSignatures;
    std::vector<const uint8_t*> messageData;

    for (const SignedMessage& signed_message : signatures) {
        if (CACHE.Write()->Cached(signed_message)) {
            continue;
        }

        unverified_messages.push_back(signed_message);
        parsedPubKeys.push_back(ConversionUtil::ToSecp256k1(signed_message.GetPublicKey()));
        parsedSignatures.push_back(ConversionUtil::ToSecp256k1(signed_message.GetSignature()));
        messageData.push_back(signed_message.GetMsgHash().data());
    }

    if (unverified_messages.empty()) {
        return true;
    }

    std::vector<secp256k1_pubkey*> pubKeyPtrs = VectorUtil::ToPointerVec(parsedPubKeys);
    std::vector<secp256k1_schnorrsig*> signaturePtrs = VectorUtil::ToPointerVec(parsedSignatures);

    secp256k1_scratch_space* pScratchSpace = secp256k1_scratch_space_create(
        SCHNORR_CONTEXT.Read()->Get(),
        SCRATCH_SPACE_SIZE
    );
    const int verifyResult = secp256k1_schnorrsig_verify_batch(
        SCHNORR_CONTEXT.Read()->Get(),
        pScratchSpace,
        signaturePtrs.data(),
        messageData.data(),
        pubKeyPtrs.data(),
        unverified_messages.size()
    );
    secp256k1_scratch_space_destroy(pScratchSpace);

    if (verifyResult == 1) {
        for (const SignedMessage& message : unverified_messages) {
            CACHE.Write()->Put(message, true);
        }
    }

    return verifyResult == 1;
}