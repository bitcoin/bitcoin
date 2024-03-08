// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_BULDING_BLOCK_IMP_INNER_PROD_ARG_H
#define NAVCOIN_BLSCT_BULDING_BLOCK_IMP_INNER_PROD_ARG_H

#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/elements.h>
#include <blsct/range_proof/bulletproofs/range_proof.h>
#include <hash.h>
#include <vector>
#include <optional>

template <typename T>
struct ImpInnerProdArgResult {
    Elements<typename T::Point> Ls;
    Elements<typename T::Point> Rs;
    typename T::Scalar a;
    typename T::Scalar b;
};

// Given P in G, the prover proves that it has vectors a, b in Zp
// for which P = Gi^a + Hi^b + u^<a,b>
struct ImpInnerProdArg {
    template <typename T>
    static std::optional<ImpInnerProdArgResult<T>> Run(
        const size_t& N,
        Elements<typename T::Point>& Gi,
        Elements<typename T::Point>& Hi,
        const typename T::Point& u,
        Elements<typename T::Scalar>& a,
        Elements<typename T::Scalar>& b,
        const typename T::Scalar& c_factor,
        const typename T::Scalar& y,
        HashWriter& fiat_shamir
    );

    // Returns list of exponents to be multiplied to G_i and H_i generator vectors
    // to generate G and H at the end of improved inner product argument. i.e.
    // G = G_1 * list[0], G_2 * list[1], ..., G_n * list[n-1]
    // H = H_1 * list[last], H_2 * list[last-1], ..., H_n * list[0]
    template <typename T>
    static std::vector<typename T::Scalar> GenGeneratorExponents(
        const size_t& num_rounds,
        const Elements<typename T::Scalar>& xs
    );

    template <typename T>
    static void LoopWithYPows(
        const size_t& num_loops,
        const typename T::Scalar& y,
        std::function<void(const size_t&, const typename T::Scalar&, const typename T::Scalar&)> f
    );

    // Generates list of x's and x^1's of all rounds of improved
    // inner product argument from a given hasher
    template <typename T>
    static std::optional<Elements<typename T::Scalar>> GenAllRoundXs(
        const Elements<typename T::Point>& Ls,
        const Elements<typename T::Point>& Rs,
        HashWriter& fiat_shamir
    );
};

#endif  // NAVCOIN_BLSCT_BULDING_BLOCK_IMP_INNER_PROD_ARG_H
