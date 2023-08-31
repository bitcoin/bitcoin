// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_PLUS_AMOUNT_RECOVERY_REQUEST_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_PLUS_AMOUNT_RECOVERY_REQUEST_H

#include <blsct/arith/elements.h>
#include <blsct/range_proof/bulletproofs_plus/range_proof.h>
#include <ctokens/tokenid.h>

namespace bulletproofs_plus {

template <typename T>
struct AmountRecoveryRequest
{
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;
    using Points = Elements<Point>;

    size_t id;
    TokenId token_id;
    Scalar y;
    Scalar z;
    Scalar alpha_hat;
    Scalar tau_x;
    Points Vs;
    Points Ls;
    Points Rs;
    size_t m;
    size_t n;
    size_t mn;
    Point nonce;

    static AmountRecoveryRequest<T> of(RangeProof<T>& proof, Point& nonce);
};

} // namespace bulletproofs_plus

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_PLUS_AMOUNT_RECOVERY_REQUEST_H