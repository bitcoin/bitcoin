// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl_g1point.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/range_proof/lazy_g1point.h>
#include <bls/bls384_256.h> // must include this before bls/bls.h
#include <bls/bls.h>

template <typename T>
LazyG1Point<T>::LazyG1Point(const typename T::Point& base, const typename T::Scalar& exp): m_base(base.m_p), m_exp(exp.m_fr)
{
}
template LazyG1Point<Mcl>::LazyG1Point(const Mcl::Point& base, const Mcl::Scalar& exp);

template <typename T>
LazyG1Points<T>::LazyG1Points(const Elements<typename T::Point>& bases, const Elements<typename T::Scalar>& exps) {
    if (bases.Size() != exps.Size()) {
        throw std::runtime_error("number of bases and exps don't match");
    }
    for (size_t i=0; i<bases.Size(); ++i) {
        points.push_back(LazyG1Point<T>(bases[i], exps[i]));
    }
}
template LazyG1Points<Mcl>::LazyG1Points(const Elements<Mcl::Point>& bases, const Elements<Mcl::Scalar>& exps);

template <typename T>
void LazyG1Points<T>::Add(const LazyG1Point<T>& point) {
    points.push_back(point);
}
template void LazyG1Points<Mcl>::Add(const LazyG1Point<Mcl>& point);

template <typename T>
typename T::Point LazyG1Points<T>::Sum() const {
    using Point = typename T::Point;
    using Scalar = typename T::Scalar;

    std::vector<typename Point::UnderlyingType> bases;
    std::vector<typename Scalar::UnderlyingType> exps;

    for (auto point: points) {
        bases.push_back(point.m_base.Underlying());
        exps.push_back(point.m_exp.Underlying());
    }
    typename Point::UnderlyingType pv;
    mclBnG1_mulVec(&pv, bases.data(), exps.data(), points.size());
    return Point(pv);
}
template Mcl::Point LazyG1Points<Mcl>::Sum() const;

template <typename T>
LazyG1Points<T> LazyG1Points<T>::operator+(const LazyG1Points<T>& rhs) const {
    Elements<typename T::Point> bases;
    Elements<typename T::Scalar> exps;

    for (auto p: points) {
        bases.Add(p.m_base);
        exps.Add(p.m_exp);
    }
    for (auto p: rhs.points) {
        bases.Add(p.m_base);
        exps.Add(p.m_exp);
    }

    return LazyG1Points<T>(bases, exps);
}
template LazyG1Points<Mcl> LazyG1Points<Mcl>::operator+(const LazyG1Points<Mcl>& rhs) const;

template <typename T>
LazyG1Points<T> LazyG1Points<T>::operator+(const LazyG1Point<T>& rhs) const {
    Elements<typename T::Point> bases;
    Elements<typename T::Scalar> exps;

    for (auto p: points) {
        bases.Add(p.m_base);
        exps.Add(p.m_exp);
    }
    bases.Add(rhs.m_base);
    exps.Add(rhs.m_exp);

    return LazyG1Points<T>(bases, exps);
}
template LazyG1Points<Mcl> LazyG1Points<Mcl>::operator+(const LazyG1Point<Mcl>& rhs) const;
