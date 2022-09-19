// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_PROOF_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_PROOF_H

#include <blsct/arith/elements.h>
#include <blsct/arith/g1point.h>
#include <blsct/arith/scalar.h>

struct Proof
{
    G1Points Vs;
    G1Point A;
    G1Point S;
    G1Point T1;
    G1Point T2;
    Scalar t_hat;
    Scalar a;
    Scalar b;
    G1Points Ls;
    G1Points Rs;
    Scalar mu;
    Scalar tau_x;
    Scalar t;
};

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_PROOF_H