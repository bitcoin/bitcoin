// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_RANGE_PROOF_BULLETPROOFS_RANGE_PROOF_H
#define NAVCOIN_BLSCT_RANGE_PROOF_BULLETPROOFS_RANGE_PROOF_H

#include <blsct/arith/elements.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/mcl/mcl_g1point.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <blsct/range_proof/proof_base.h>
#include <span.h>
#include <streams.h>

namespace bulletproofs {

template <typename T>
struct RangeProof: public range_proof::ProofBase<T> {
    using Point = typename T::Point;
    using Scalar = typename T::Scalar;
    using Points = Elements<Point>;

    // intermediate values used to derive random values later
    Point A;
    Point S;
    Point T1;
    Point T2;
    Scalar mu;
    Scalar tau_x;

    // proof results
    Scalar a;     // result of inner product argument
    Scalar b;     // result of inner product argument
    Scalar t_hat; // inner product of l and r

    bool operator==(const RangeProof<T>& other) const;
    bool operator!=(const RangeProof<T>& other) const;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        range_proof::ProofBase<T>::Serialize(s);
        ::Serialize(s, A);
        ::Serialize(s, S);
        ::Serialize(s, T1);
        ::Serialize(s, T2);
        ::Serialize(s, tau_x);
        ::Serialize(s, mu);
        ::Serialize(s, a);
        ::Serialize(s, b);
        ::Serialize(s, t_hat);
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        range_proof::ProofBase<T>::Unserialize(s);
        ::Unserialize(s, A);
        ::Unserialize(s, S);
        ::Unserialize(s, T1);
        ::Unserialize(s, T2);
        ::Unserialize(s, tau_x);
        ::Unserialize(s, mu);
        ::Unserialize(s, a);
        ::Unserialize(s, b);
        ::Unserialize(s, t_hat);
    }
};

} // namespace bulletproofs

#endif // NAVCOIN_BLSCT_RANGE_PROOF_BULLETPROOFS_RANGE_PROOF_H
