// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_RANGE_PROOF_BULLETPROOFS_PLUS_RANGE_PROOF_H
#define NAVCOIN_BLSCT_RANGE_PROOF_BULLETPROOFS_PLUS_RANGE_PROOF_H

#include <blsct/arith/elements.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/mcl/mcl_g1point.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <blsct/range_proof/proof_base.h>
#include <span.h>
#include <streams.h>

namespace bulletproofs_plus {

template <typename T>
struct RangeProof: public range_proof::ProofBase<T> {
    using Point = typename T::Point;
    using Scalar = typename T::Scalar;
    using Points = Elements<Point>;
    using Scalars = Elements<Scalar>;

    Point A;      // A = Gi^{aL} + Hi^{aR} + h^{alpha}; required to set up transcript
    Point A_wip;  // A in the last round of wip
    Point B;      // B in the last round of wip
    Scalar r_prime;
    Scalar s_prime;
    Scalar delta_prime;

    Scalar alpha_hat;  // used only for amount recovery

    Scalar tau_x;  // value to embed msg2

    bool operator==(const RangeProof<T>& other) const;
    bool operator!=(const RangeProof<T>& other) const;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        range_proof::ProofBase<T>::Serialize(s);
        ::Serialize(s, A);
        ::Serialize(s, A_wip);
        ::Serialize(s, B);
        ::Serialize(s, r_prime);
        ::Serialize(s, s_prime);
        ::Serialize(s, delta_prime);
        ::Serialize(s, alpha_hat);
        ::Serialize(s, tau_x);
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        range_proof::ProofBase<T>::Unserialize(s);
        ::Unserialize(s, A);
        ::Unserialize(s, A_wip);
        ::Unserialize(s, B);
        ::Unserialize(s, r_prime);
        ::Unserialize(s, s_prime);
        ::Unserialize(s, delta_prime);
        ::Unserialize(s, alpha_hat);
        ::Unserialize(s, tau_x);
    }
};

} // namespace bulletproofs_plus

#endif // NAVCOIN_BLSCT_RANGE_PROOF_BULLETPROOFS_PLUS_RANGE_PROOF_H
