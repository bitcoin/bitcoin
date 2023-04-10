// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_BUILDING_BLOCK_G_H_GI_HI_ZERO_VERIFIER_H
#define NAVCOIN_BLSCT_BUILDING_BLOCK_G_H_GI_HI_ZERO_VERIFIER_H

#include <blsct/arith/elements.h>
#include <blsct/building_block/lazy_point.h>

template <typename T>
struct G_H_Gi_Hi_ZeroVerifier {
    using Scalar = typename T::Scalar;
    using Scalars = Elements<typename T::Scalar>;
    using Point = typename T::Point;
    using Points = Elements<typename T::Point>;

    G_H_Gi_Hi_ZeroVerifier(const size_t& n):
        m_n{n}, m_gi_exps{Scalars(n, 0)}, m_hi_exps{Scalars(n, 0)} {}

    void AddPoint(const LazyPoint<T>& p);
    void AddPositiveG(const Scalar& exp);
    void AddPositiveH(const Scalar& exp);
    void AddNegativeG(const Scalar& exp);
    void AddNegativeH(const Scalar& exp);
    void SetGiExp(const size_t& i, const Scalar& s);
    void SetHiExp(const size_t& i, const Scalar& s);
    bool Verify(const Point& g, const Point&h, const Points& Gi, const Points& Hi);

private:
    size_t m_n;

    Scalar m_h_pos_exp = Scalar(0);
    Scalar m_g_neg_exp = Scalar(0);
    Scalar m_h_neg_exp = Scalar(0);
    Scalar m_g_pos_exp = Scalar(0);
    Scalars m_gi_exps;
    Scalars m_hi_exps;

    LazyPoints<T> m_points;
};

#endif // NAVCOIN_BLSCT_BUILDING_BLOCK_G_H_GI_HI_ZERO_VERIFIER_H

