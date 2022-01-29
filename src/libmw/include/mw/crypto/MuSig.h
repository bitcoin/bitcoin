#pragma once

#include <mw/models/crypto/SecretKey.h>
#include <mw/models/crypto/Signature.h>
#include <mw/models/crypto/PublicKey.h>
#include <mw/models/crypto/Hash.h>

class MuSig
{
public:
    static SecretKey GenerateSecureNonce();

    //
    // Builds one party's share of a Schnorr signature.
    // Returns a CompactSignature if successful.
    //
    static CompactSignature CalculatePartial(
        const SecretKey& secretKey,
        const SecretKey& secretNonce,
        const PublicKey& sumPubKeys,
        const PublicKey& sumPubNonces,
        const mw::Hash& message
    );

    //
    // Verifies one party's share of a Schnorr signature.
    // Returns true if valid.
    //
    static bool VerifyPartial(
        const CompactSignature& partialSignature,
        const PublicKey& publicKey,
        const PublicKey& sumPubKeys,
        const PublicKey& sumPubNonces,
        const mw::Hash& message
    );

    //
    // Combines multiple partial signatures to build the final aggregate signature.
    // Returns the raw aggregate signature.
    //
    static Signature Aggregate(
        const std::vector<CompactSignature>& signatures,
        const PublicKey& sumPubNonces
    );
};