// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_BULDING_BLOCK_WEIGHTED_INNER_PROD_ARG_H
#define NAVCOIN_BLSCT_BULDING_BLOCK_WEIGHTED_INNER_PROD_ARG_H

#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/elements.h>
#include <blsct/range_proof/bulletproofs_plus/range_proof.h>
#include <hash.h>
#include <vector>
#include <optional>

template <typename T>
struct WeightedInnerProdArgResult {
    using Point = typename T::Point;
    using Scalar = typename T::Scalar;
    using Points = Elements<Point>;

    Points Ls;
    Points Rs;
    Point A;
    Point B;
    Scalar r_prime;
    Scalar s_prime;
    Scalar delta_prime;
};

struct WeightedInnerProdArg {
    template <typename T>
    static Elements<typename T::Point> ReduceGs(
        const Elements<typename T::Point>& gs1,
        const Elements<typename T::Point>& gs2,
        const typename T::Scalar& e,
        const typename T::Scalar& e_inv,
        const typename T::Scalar& y_inv_to_n
    );

    template <typename T>
    static Elements<typename T::Point> ReduceHs(
        const Elements<typename T::Point>& hs1,
        const Elements<typename T::Point>& hs2,
        const typename T::Scalar& e,
        const typename T::Scalar& e_inv
    );

    template <typename T>
    static typename T::Point UpdateP(
        const typename T::Point& P,
        const typename T::Point& L,
        const typename T::Point& R,
        const typename T::Scalar& e_sq,
        const typename T::Scalar& e_inv_sq
    );

    template <typename T>
    static std::optional<WeightedInnerProdArgResult<T>> Run(
        const size_t& N,
        const typename T::Scalar& y,
        Elements<typename T::Point>& gs,
        Elements<typename T::Point>& hs,
        typename T::Point& g,
        typename T::Point& h,
        typename T::Point& P,
        Elements<typename T::Scalar>& a,
        Elements<typename T::Scalar>& b,
        const typename T::Scalar& alpha_src,  // alpha is a part of the proof and shouldn't be modified
        HashWriter& fiat_shamir
    );
};

#endif  // NAVCOIN_BLSCT_BULDING_BLOCK_WEIGHTED_INNER_PROD_ARG_H
