#pragma once

#include <mw/models/crypto/Commitment.h>
#include <mw/models/crypto/ProofData.h>
#include <mw/models/crypto/ProofMessage.h>
#include <mw/models/crypto/RangeProof.h>
#include <mw/models/crypto/RewoundProof.h>
#include <mw/models/crypto/SecretKey.h>
#include <memory>

class Bulletproofs
{
public:
    static bool BatchVerify(
        const std::vector<ProofData>& rangeProofs
    );

    static RangeProof::CPtr Generate(
        const uint64_t amount,
        const SecretKey& key,
        const SecretKey& privateNonce,
        const SecretKey& rewindNonce,
        const ProofMessage& proofMessage,
        const std::vector<uint8_t>& extraData
    );

    static std::unique_ptr<RewoundProof> Rewind(
        const Commitment& commitment,
        const RangeProof& rangeProof,
        const std::vector<uint8_t>& extraData,
        const SecretKey& nonce
    );
};