#pragma once

#include <mw/models/crypto/Commitment.h>
#include <mw/models/crypto/RangeProof.h>
#include <cassert>

struct ProofData
{
    Commitment commitment;
    RangeProof::CPtr pRangeProof;
    std::vector<uint8_t> extraData;

    inline bool operator==(const ProofData& rhs) const noexcept
    {
        assert(rhs.pRangeProof != nullptr);

        return commitment == rhs.commitment
            && *pRangeProof == *rhs.pRangeProof
            && extraData == rhs.extraData;
    }

    inline bool operator!=(const ProofData& rhs) const noexcept { return !(rhs == *this); }
};