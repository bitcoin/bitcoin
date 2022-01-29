#include <mw/crypto/Bulletproofs.h>
#include "Context.h"
#include "ConversionUtil.h"

#include <caches/Cache.h>
#include <mw/exceptions/CryptoException.h>
#include <mw/util/VectorUtil.h>

static constexpr uint64_t MAX_WIDTH = 1 << 20;
static constexpr size_t SCRATCH_SPACE_SIZE = 256 * MAX_WIDTH;
static constexpr size_t PROOF_LEN = 675;
static constexpr size_t NUM_BITS_PROVEN = 64;

static Locked<LRUCache<Commitment, ProofData>> CACHE(std::make_shared<LRUCache<Commitment, ProofData>>(3000));
static Locked<Context> BP_CONTEXT(std::make_shared<Context>());

bool Bulletproofs::BatchVerify(const std::vector<ProofData>& proofs)
{
    std::vector<secp256k1_pedersen_commitment> secpCommitments;
    secpCommitments.reserve(proofs.size());

    std::vector<const uint8_t*> bulletproofPointers;
    bulletproofPointers.reserve(proofs.size());

    std::vector<const uint8_t*> extraData;
    extraData.reserve(proofs.size());

    std::vector<size_t> extraDataLen;
    extraDataLen.reserve(proofs.size());

    for (const auto& proof : proofs)
    {
        auto cache_writer = CACHE.Write();
        if (!cache_writer->Cached(proof.commitment) || proof != cache_writer->Get(proof.commitment)) {
            secpCommitments.push_back(ConversionUtil::ToSecp256k1(proof.commitment));
            bulletproofPointers.emplace_back(proof.pRangeProof->data());

            if (!proof.extraData.empty()) {
                extraData.push_back(proof.extraData.data());
                extraDataLen.push_back(proof.extraData.size());
            } else {
                extraData.push_back(nullptr);
                extraDataLen.push_back(0);
            }
        }
    }

    if (secpCommitments.empty()) {
        return true;
    }

    // array of generator multiplied by value in pedersen commitments (cannot be NULL)
    std::vector<secp256k1_generator> valueGenerators;
    for (size_t i = 0; i < secpCommitments.size(); i++)
    {
        valueGenerators.push_back(secp256k1_generator_const_h);
    }

    std::vector<secp256k1_pedersen_commitment*> commitmentPointers = VectorUtil::ToPointerVec(secpCommitments);

    secp256k1_scratch_space* pScratchSpace = secp256k1_scratch_space_create(
        BP_CONTEXT.Read()->Get(),
        SCRATCH_SPACE_SIZE
    );
    const int result = secp256k1_bulletproof_rangeproof_verify_multi(
        BP_CONTEXT.Read()->Get(),
        pScratchSpace,
        BP_CONTEXT.Read()->GetGenerators(),
        bulletproofPointers.data(),
        secpCommitments.size(),
        PROOF_LEN,
        NULL,
        commitmentPointers.data(),
        1,
        NUM_BITS_PROVEN,
        valueGenerators.data(),
        extraData.data(),
        extraDataLen.data()
    );
    secp256k1_scratch_space_destroy(pScratchSpace);

    if (result == 1) {
        auto cache_writer = CACHE.Write();
        for (const auto& proof : proofs)
        {
            cache_writer->Put(proof.commitment, proof);
        }
    }

    return result == 1;
}

RangeProof::CPtr Bulletproofs::Generate(
    const uint64_t amount,
    const SecretKey& key,
    const SecretKey& privateNonce,
    const SecretKey& rewindNonce,
    const ProofMessage& proofMessage,
    const std::vector<uint8_t>& extraData)
{
    auto contextWriter = BP_CONTEXT.Write();
    secp256k1_context* pContext = contextWriter->Randomized();

    std::vector<uint8_t> proofBytes(RangeProof::SIZE, 0);
    size_t proofLen = RangeProof::SIZE;

    secp256k1_scratch_space* pScratchSpace = secp256k1_scratch_space_create(pContext, SCRATCH_SPACE_SIZE);

    std::vector<const uint8_t*> blindingFactors({ key.data() });
    int result = secp256k1_bulletproof_rangeproof_prove(
        pContext,
        pScratchSpace,
        contextWriter->GetGenerators(),
        &proofBytes[0],
        &proofLen,
        NULL,
        NULL,
        NULL,
        &amount,
        NULL,
        blindingFactors.data(),
        NULL,
        1,
        &secp256k1_generator_const_h,
        NUM_BITS_PROVEN,
        rewindNonce.data(),
        privateNonce.data(),
        extraData.data(),
        extraData.size(),
        proofMessage.data()
    );
    secp256k1_scratch_space_destroy(pScratchSpace);

    if (result != 1) {
        ThrowCrypto_F("secp256k1_bulletproof_rangeproof_prove failed with error: {}", result);
    }

    proofBytes.resize(proofLen);
    return std::make_shared<RangeProof>(std::move(proofBytes));
}

std::unique_ptr<RewoundProof> Bulletproofs::Rewind(
    const Commitment& commitment,
    const RangeProof& rangeProof,
    const std::vector<uint8_t>& extraData,
    const SecretKey& nonce)
{
    secp256k1_pedersen_commitment secpCommitment = ConversionUtil::ToSecp256k1(commitment);

    uint64_t value;
    SecretKey blindingFactor;
    std::vector<uint8_t> message(20, 0);

    int result = secp256k1_bulletproof_rangeproof_rewind(
        BP_CONTEXT.Read()->Get(),
        &value,
        blindingFactor.data(),
        rangeProof.data(),
        rangeProof.size(),
        0,
        &secpCommitment,
        &secp256k1_generator_const_h,
        nonce.data(),
        extraData.data(),
        extraData.size(),
        message.data()
    );

    if (result == 1) {
        return std::make_unique<RewoundProof>(
            value, 
            std::make_unique<SecretKey>(std::move(blindingFactor)),
            ProofMessage(BigInt<20>(std::move(message)))
        );
    }

    return std::unique_ptr<RewoundProof>(nullptr);
}