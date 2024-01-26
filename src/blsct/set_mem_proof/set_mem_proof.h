// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_SET_MEM_PROOF_SET_MEM_PROOF_H
#define NAVCOIN_BLSCT_SET_MEM_PROOF_SET_MEM_PROOF_H

#include <blsct/arith/elements.h>
#include <streams.h>

template <typename T>
struct SetMemProof {
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;
    using Scalars = Elements<Scalar>;
    using Points = Elements<Point>;

    // sigma excluding l and r
    Point phi;
    Point A1;
    Point A2;
    Point S1;
    Point S2;
    Point S3;
    Point T1;
    Point T2;
    Scalar tau_x;
    Scalar mu;
    Scalar z_alpha;
    Scalar z_tau;
    Scalar z_beta;
    Scalar t;

    // improved inner product argument
    Points Ls;
    Points Rs;
    Scalar a;
    Scalar b;
    Scalar omega;

    // for unserialization
    SetMemProof() {}

    SetMemProof(
        // sigma
        const Point& phi,
        const Point& A1,
        const Point& A2,
        const Point& S1,
        const Point& S2,
        const Point& S3,
        const Point& T1,
        const Point& T2,
        const Scalar& tau_x,
        const Scalar& mu,
        const Scalar& z_alpha,
        const Scalar& z_tau,
        const Scalar& z_beta,
        const Scalar& t,

        // for improved inner product argument
        const Points& Ls,
        const Points& Rs,
        const Scalar& a,
        const Scalar& b,
        const Scalar& omega
    ): phi{phi}, A1{A1}, A2{A2}, S1{S1}, S2{S2},
        S3{S3}, T1{T1}, T2{T2}, tau_x{tau_x},
        mu{mu}, z_alpha{z_alpha}, z_tau{z_tau}, z_beta{z_beta}, t{t},
        Ls{Ls}, Rs{Rs}, a{a}, b{b},
        omega{omega}
        {}

    bool operator==(const SetMemProof& other) const;
    bool operator!=(const SetMemProof& other) const;

    template <typename Stream>
    void Serialize(Stream& st) const
    {
        st << phi
           << A1
           << A2
           << S1
           << S2
           << S3
           << T1
           << T2
           << tau_x
           << mu
           << z_alpha
           << z_tau
           << z_beta
           << t
           << Ls
           << Rs
           << a
           << b
           << omega;
    };

    template <typename Stream>
    void Unserialize(Stream& st)
    {
        st >> phi >> A1 >> A2 >> S1 >> S2 >> S3 >> T1 >> T2 >> tau_x >> mu >> z_alpha >> z_tau >> z_beta >> t >> Ls >> Rs >> a >> b >> omega;
    };
};

#endif // NAVCOIN_BLSCT_SET_MEM_PROOF_SET_MEM_PROOF_H
