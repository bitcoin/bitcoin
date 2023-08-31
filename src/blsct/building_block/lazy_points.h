// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_BUILDING_BLOCK_LAZY_POINTS_H
#define NAVCOIN_BLSCT_BUILDING_BLOCK_LAZY_POINTS_H

#include <blsct/arith/elements.h>
#include <blsct/building_block/lazy_point.h>

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
    void Add(const typename T::Point& p);  // Add Point * 1

    // Add a single point
    void Add(
        const typename T::Point& p,
        const typename T::Scalar& s
    );

    // Add each point * s,
    void Add(
        const Elements<typename T::Point>& ps,
        const typename T::Scalar& s
    );

    void Add(
        const Elements<typename T::Point>& ps,
        const Elements<typename T::Scalar>& ss
    );

    Point Sum() const;

    LazyPoints<T> operator+(const LazyPoints<T>& rhs) const;
    LazyPoints<T> operator+(const LazyPoint<T>& rhs) const;

private:
    std::vector<LazyPoint<T>> m_points;
};

#endif // NAVCOIN_BLSCT_BUILDING_BLOCK_LAZY_POINTS_H
