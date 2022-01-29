#include "Context.h"
#include "ConversionUtil.h"

#include <mw/crypto/Pedersen.h>
#include <mw/exceptions/CryptoException.h>
#include <mw/util/VectorUtil.h>

static Locked<Context> PEDERSEN_CONTEXT(std::make_shared<Context>());

Commitment Pedersen::CommitTransparent(const uint64_t value)
{
    return Commit(value, BigInt<32>());
}

Commitment Pedersen::Commit(const uint64_t value, const BlindingFactor& blindingFactor)
{
    secp256k1_pedersen_commitment commitment;
    const int result = secp256k1_pedersen_commit(
        PEDERSEN_CONTEXT.Read()->Get(),
        &commitment,
        blindingFactor.data(),
        value,
        &secp256k1_generator_const_h,
        &secp256k1_generator_const_g
    );
    if (result != 1) {
        ThrowCrypto("Failed to create commitment.");
    }

    return ConversionUtil::ToCommitment(commitment);
}

Commitment Pedersen::AddCommitments(
    const std::vector<Commitment>& positive,
    const std::vector<Commitment>& negative)
{
    std::vector<Commitment> sanitizedPositive;
    std::copy_if(
        positive.cbegin(), positive.cend(), std::back_inserter(sanitizedPositive),
        [](const auto& positiveCommitment) { return !positiveCommitment.IsZero(); });

    std::vector<Commitment> sanitizedNegative;
    std::copy_if(
        negative.cbegin(), negative.cend(), std::back_inserter(sanitizedNegative),
        [](const auto& negativeCommitment) { return !negativeCommitment.IsZero(); });

    if (sanitizedPositive.empty() && sanitizedNegative.empty()) {
        return Commitment{};
    }

    return PedersenCommitSum(sanitizedPositive, sanitizedNegative);
}

BlindingFactor Pedersen::AddBlindingFactors(
    const std::vector<BlindingFactor>& positive,
    const std::vector<BlindingFactor>& negative)
{
    std::vector<BlindingFactor> sanitizedPositive;
    std::copy_if(
        positive.cbegin(), positive.cend(), std::back_inserter(sanitizedPositive),
        [](const auto& positiveBlind) { return !positiveBlind.IsZero(); });

    std::vector<BlindingFactor> sanitizedNegative;
    std::copy_if(
        negative.cbegin(), negative.cend(), std::back_inserter(sanitizedNegative),
        [](const auto& negativeBlind) { return !negativeBlind.IsZero(); });

    if (sanitizedPositive.empty() && sanitizedNegative.empty()) {
        return BlindingFactor{};
    }

    return PedersenBlindSum(sanitizedPositive, sanitizedNegative);
}

Commitment Pedersen::PedersenCommitSum(const std::vector<Commitment>& positive, const std::vector<Commitment>& negative)
{
    std::vector<secp256k1_pedersen_commitment> positiveCommitments = ConversionUtil::ToSecp256k1(positive);
    std::vector<secp256k1_pedersen_commitment*> positivePtrs = VectorUtil::ToPointerVec(positiveCommitments);

    std::vector<secp256k1_pedersen_commitment> negativeCommitments = ConversionUtil::ToSecp256k1(negative);
    std::vector<secp256k1_pedersen_commitment*> negativePtrs = VectorUtil::ToPointerVec(negativeCommitments);

    secp256k1_pedersen_commitment commitment;
    const int result = secp256k1_pedersen_commit_sum(
        PEDERSEN_CONTEXT.Read()->Get(),
        &commitment,
        positivePtrs.empty() ? nullptr : positivePtrs.data(),
        positivePtrs.size(),
        negativePtrs.empty() ? nullptr : negativePtrs.data(),
        negativePtrs.size()
    );
    if (result != 1) {
        ThrowCrypto("secp256k1_pedersen_commit_sum error");
    }

    return ConversionUtil::ToCommitment(commitment);
}

BlindingFactor Pedersen::PedersenBlindSum(const std::vector<BlindingFactor>& positive, const std::vector<BlindingFactor>& negative)
{
    std::vector<const uint8_t*> blindingFactors;
    for (const BlindingFactor& positiveFactor : positive) {
        blindingFactors.push_back(positiveFactor.data());
    }

    for (const BlindingFactor& negativeFactor : negative) {
        blindingFactors.push_back(negativeFactor.data());
    }

    BlindingFactor blindingFactor;
    const int result = secp256k1_pedersen_blind_sum(
        PEDERSEN_CONTEXT.Read()->Get(),
        blindingFactor.data(),
        blindingFactors.data(),
        blindingFactors.size(),
        positive.size()
    );
    if (result != 1) {
        ThrowCrypto("secp256k1_pedersen_blind_sum error");
    }

    return blindingFactor;
}

BlindingFactor Pedersen::BlindSwitch(const BlindingFactor& blindingFactor, const uint64_t amount)
{
    BlindingFactor blindSwitch;
    const int result = secp256k1_blind_switch(
        PEDERSEN_CONTEXT.Read()->Get(),
        blindSwitch.data(),
        blindingFactor.data(),
        amount,
        &secp256k1_generator_const_h,
        &secp256k1_generator_const_g,
        &GENERATOR_J_PUB
    );
    if (result != 1) {
        ThrowCrypto("secp256k1_blind_switch failed");
    }

    return blindSwitch;
}