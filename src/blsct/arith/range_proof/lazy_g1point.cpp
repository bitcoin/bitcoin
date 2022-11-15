// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/range_proof/lazy_g1point.h>

void LazyG1Points::Add(const LazyG1Point& point) {
    points.push_back(point);
}

G1Point LazyG1Points::Sum() const {
    std::vector<mclBnG1> bases;
    std::vector<mclBnFr> exps;

    for (auto point: points) {
        bases.push_back(point.m_base);
        exps.push_back(point.m_exp);
    }
    G1Point p;
    mclBnG1_mulVec(&p.m_p, bases.data(), exps.data(), points.size());
    return p;
}
