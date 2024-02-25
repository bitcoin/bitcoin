// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_AMOUNT_RECOVERY_REQUEST_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_AMOUNT_RECOVERY_REQUEST_H

#include <blsct/arith/elements.h>
#include <blsct/building_block/generator_deriver.h>
#include <blsct/range_proof/bulletproofs/range_proof.h>

namespace bulletproofs {

template <typename T>
struct AmountRecoveryRequest
{
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;
    using Points = Elements<Point>;

    size_t id;
    typename GeneratorDeriver<T>::Seed seed;
    Scalar x;
    Scalar z;
    Points Vs;
    Points Ls;
    Points Rs;
    Scalar mu;
    Scalar tau_x;
    Point nonce;

    static AmountRecoveryRequest<T> of(
        RangeProof<T>& proof,
        Point& nonce
    );
};

} // namespace bulletproofs

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_AMOUNT_RECOVERY_REQUEST_H

