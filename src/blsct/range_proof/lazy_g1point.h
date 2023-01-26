// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_RANGE_PROOF_LAZY_G1POINT_H
#define NAVCOIN_BLSCT_RANGE_PROOF_LAZY_G1POINT_H

#include <blsct/arith/elements.h>

template <typename T>
struct LazyG1Point {
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;

public:
    LazyG1Point(const Point& base, const Scalar& exp);

    const Point m_base;
    const Scalar m_exp;
};

template <typename T>
struct LazyG1Points {
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;
    using Scalars = Elements<Scalar>;
    using Points = Elements<Point>;

public:
    LazyG1Points() {}
    LazyG1Points(const Points& bases, const Scalars& exps);

    void Add(const LazyG1Point<T>& point);

    Point Sum() const;

    LazyG1Points<T> operator+(const LazyG1Points<T>& rhs) const;
    LazyG1Points<T> operator+(const LazyG1Point<T>& rhs) const;

private:
    std::vector<LazyG1Point<T>> points;
};

#endif // NAVCOIN_BLSCT_RANGE_PROOF_LAZY_G1POINT_H
