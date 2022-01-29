#include <mw/models/crypto/Commitment.h>
#include <mw/models/crypto/BlindingFactor.h>
#include <mw/crypto/Pedersen.h>
#include <amount.h>
#include <random.h>

Commitment Commitment::Random()
{
    return Commitment::Blinded(BlindingFactor::Random(), GetRand(MAX_MONEY));
}

Commitment Commitment::Switch(const BlindingFactor& blind, const uint64_t value)
{
    return Pedersen::Commit(value, Pedersen::BlindSwitch(blind, value));
}

Commitment Commitment::Blinded(const BlindingFactor& blind, const uint64_t value)
{
    return Pedersen::Commit(value, blind);
}

Commitment Commitment::Transparent(const uint64_t value)
{
    return Pedersen::CommitTransparent(value);
}