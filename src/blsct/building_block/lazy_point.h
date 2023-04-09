// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_BUILDING_BLOCK_LAZY_POINT_H
#define NAVCOIN_BLSCT_BUILDING_BLOCK_LAZY_POINT_H

#include <blsct/arith/elements.h>

template <typename T>
struct LazyPoint {
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;

    LazyPoint(const Point& base, const Scalar& exp);

    const Point m_base;
    const Scalar m_exp;
};

template <typename T>
struct LazyPoints {
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;
    using Scalars = Elements<Scalar>;
    using Points = Elements<Point>;

    LazyPoints() {}
    LazyPoints(const LazyPoints<T>& points): m_points{points.m_points} {}
    LazyPoints(const Points& bases, const Scalars& exps);

    void Add(const LazyPoint<T>& point);

    Point Sum() const;

    LazyPoints<T> operator+(const LazyPoints<T>& rhs) const;
    LazyPoints<T> operator+(const LazyPoint<T>& rhs) const;

private:
    std::vector<LazyPoint<T>> m_points;
};

#endif // NAVCOIN_BLSCT_BUILDING_BLOCK_LAZY_POINT_H
