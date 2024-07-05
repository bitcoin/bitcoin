// Copyright (c) 2022 The Navio developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVIO_BLSCT_BUILDING_BLOCK_LAZY_POINT_H
#define NAVIO_BLSCT_BUILDING_BLOCK_LAZY_POINT_H

template <typename T>
struct LazyPoint {
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;

    LazyPoint(const Point& base, const Scalar& exp);

    const Point m_base;
    const Scalar m_exp;
};

#endif // NAVIO_BLSCT_BUILDING_BLOCK_LAZY_POINT_H
