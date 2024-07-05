// Copyright (c) 2023 The Navio developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVIO_BLSCT_RANGE_PROOF_PROOF_BASE_H
#define NAVIO_BLSCT_RANGE_PROOF_PROOF_BASE_H

#include <blsct/arith/elements.h>
#include <streams.h>

namespace range_proof {

template <typename T>
struct ProofBase {
    using Point = typename T::Point;
    using Scalar = typename T::Scalar;
    using Points = Elements<Point>;

    ProofBase(){};

    ProofBase(const Points& Vs,
              const Points& Ls,
              const Points& Rs) : Vs(Vs), Ls(Ls), Rs(Rs){};

    Points Vs;
    Points Ls;
    Points Rs;

    bool operator==(const ProofBase<T>& other) const;
    bool operator!=(const ProofBase<T>& other) const;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        ::Serialize(s, Vs);
        ::Serialize(s, Ls);
        ::Serialize(s, Rs);
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        ::Unserialize(s, Vs);
        ::Unserialize(s, Ls);
        ::Unserialize(s, Rs);
    }
};

} // namespace range_proof

#endif // NAVIO_BLSCT_RANGE_PROOF_PROOF_BASE_H
