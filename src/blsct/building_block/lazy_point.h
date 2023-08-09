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

#endif // NAVCOIN_BLSCT_BUILDING_BLOCK_LAZY_POINT_H
