// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/range_proof/lazy_g1point.h>

LazyG1Points::LazyG1Points(const G1Points& bases, const Scalars& exps) {
    if (bases.Size() != exps.Size()) {
        throw std::runtime_error("number of bases and exps don't match");
    }
    for (size_t i=0; i<bases.Size(); ++i) {
        points.push_back(LazyG1Point(bases[i].m_p, exps[i].m_fr));
    }
}

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

LazyG1Points LazyG1Points::operator+(const LazyG1Points& rhs) const {
    std::vector<G1Point> bases;
    std::vector<Scalar> exps;

    for (auto p: points) {
        bases.push_back(p.m_base);
        exps.push_back(p.m_exp);
    }
    for (auto p: rhs.points) {
        bases.push_back(p.m_base);
        exps.push_back(p.m_exp);
    }

    return LazyG1Points(bases, exps);
}

LazyG1Points LazyG1Points::operator+(const LazyG1Point& rhs) const {
    std::vector<G1Point> bases;
    std::vector<Scalar> exps;

    for (auto p: points) {
        bases.push_back(p.m_base);
        exps.push_back(p.m_exp);
    }
    bases.push_back(rhs.m_base);
    exps.push_back(rhs.m_exp);

    return LazyG1Points(bases, exps);
}