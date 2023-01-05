// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_RANGE_PROOF_RANGE_PROOF_H
#define NAVCOIN_BLSCT_RANGE_PROOF_RANGE_PROOF_H

#include <blsct/arith/elements.h>

template <typename T>
struct RangeProof
{
    using Point = typename T::Point;
    using Scalar = typename T::Scalar;
    using Points = Elements<Point>;

    // intermediate values used to derive random values later
    Points Vs;
    Point A;
    Point S;
    Point T1;
    Point T2;
    Scalar mu;
    Scalar tau_x;
    Points Ls;
    Points Rs;

    // proof results
    Scalar t_hat;   // inner product of l and r
    Scalar a;       // result of inner product argument
    Scalar b;       // result of inner product argument
};

#endif // NAVCOIN_BLSCT_RANGE_PROOF_RANGE_PROOF_H
