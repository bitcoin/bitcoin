// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_RANGE_PROOF_WITH_TRANSCRIPT_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_RANGE_PROOF_WITH_TRANSCRIPT_H

#include <blsct/arith/elements.h>
#include <blsct/range_proof/bulletproofs/range_proof.h>

namespace bulletproofs {

template <typename T>
class RangeProofWithTranscript
{
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;
    using Scalars = Elements<Scalar>;

public:
    RangeProofWithTranscript(
        const RangeProofWithSeed<T>& proof,
        const Scalar& x,
        const Scalar& y,
        const Scalar& z,
        const Scalar& c_factor,
        const Scalars& xs,
        const size_t& num_input_values_power_2,
        const size_t& concat_input_values_in_bits
    ): proof{proof}, x{x}, y{y}, z{z}, c_factor{c_factor},
        xs(xs), inv_y(y.Invert()),
        num_input_values_power_2(num_input_values_power_2),
        concat_input_values_in_bits(concat_input_values_in_bits) {}

    static RangeProofWithTranscript<T> Build(const RangeProofWithSeed<T>& proof);

    const RangeProofWithSeed<T> proof;

    // transcript
    const Scalar x;  // x used in the main prove procedure
    const Scalar y;
    const Scalar z;
    const Scalar c_factor;  // factor multiplied to cL and cR in inner product argument
    const Scalars xs;      // x used in inner product argument
    const Scalar inv_y;

    const size_t num_input_values_power_2;  // M in old impl
    const size_t concat_input_values_in_bits;  // MN is old impl
};

} // namespace bulletproofs

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_RANGE_PROOF_WITH_TRANSCRIPT_H
