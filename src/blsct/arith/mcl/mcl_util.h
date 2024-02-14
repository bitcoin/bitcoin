// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_MCL_MCL_UTIL_H
#define NAVCOIN_BLSCT_ARITH_MCL_MCL_UTIL_H

#include <bls/bls384_256.h>
#include <blsct/arith/mcl/mcl_g1point.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <blsct/building_block/lazy_point.h>

struct MclUtil {
    // using template to avoid circular dependency problem with Mcl class
    template <typename T>
    static MclG1Point MultiplyLazyPoints(const std::vector<LazyPoint<T>>& points)
    {
        std::vector<MclG1Point::Underlying> bases;
        std::vector<MclScalar::Underlying> exps;

        for (auto point: points) {
            bases.push_back(point.m_base.GetUnderlying());
            exps.push_back(point.m_exp.GetUnderlying());
        }
        MclG1Point::Underlying pv;
        mclBnG1_mulVec(&pv, bases.data(), exps.data(), points.size());
        return MclG1Point(pv);
    }
};

#endif // NAVCOIN_BLSCT_ARITH_MCL_MCL_UTIL_H
