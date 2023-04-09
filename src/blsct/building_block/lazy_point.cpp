// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl_g1point.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/building_block/lazy_point.h>
#include <bls/bls384_256.h>

template <typename T>
LazyPoint<T>::LazyPoint(const typename T::Point& base, const typename T::Scalar& exp): m_base(base.m_p), m_exp(exp.m_fr)
{
}
template LazyPoint<Mcl>::LazyPoint(const Mcl::Point& base, const Mcl::Scalar& exp);

template <typename T>
LazyPoints<T>::LazyPoints(const Elements<typename T::Point>& bases, const Elements<typename T::Scalar>& exps) {
    if (bases.Size() != exps.Size()) {
        throw std::runtime_error("number of bases and exps don't match");
    }
    for (size_t i=0; i<bases.Size(); ++i) {
        m_points.push_back(LazyPoint<T>(bases[i], exps[i]));
    }
}
template LazyPoints<Mcl>::LazyPoints(const Elements<Mcl::Point>& bases, const Elements<Mcl::Scalar>& exps);

template <typename T>
void LazyPoints<T>::Add(const LazyPoint<T>& point) {
    m_points.push_back(point);
}
template void LazyPoints<Mcl>::Add(const LazyPoint<Mcl>& point);

template <typename T>
typename T::Point LazyPoints<T>::Sum() const {
    using Point = typename T::Point;
    using Scalar = typename T::Scalar;

    std::vector<typename Point::UnderlyingType> bases;
    std::vector<typename Scalar::UnderlyingType> exps;

    for (auto point: m_points) {
        bases.push_back(point.m_base.Underlying());
        exps.push_back(point.m_exp.Underlying());
    }
    typename Point::UnderlyingType pv;
    mclBnG1_mulVec(&pv, bases.data(), exps.data(), m_points.size());
    return Point(pv);
}
template Mcl::Point LazyPoints<Mcl>::Sum() const;

template <typename T>
LazyPoints<T> LazyPoints<T>::operator+(const LazyPoints<T>& rhs) const {
    Elements<typename T::Point> bases;
    Elements<typename T::Scalar> exps;

    for (auto p: m_points) {
        bases.Add(p.m_base);
        exps.Add(p.m_exp);
    }
    for (auto p: rhs.m_points) {
        bases.Add(p.m_base);
        exps.Add(p.m_exp);
    }

    return LazyPoints<T>(bases, exps);
}
template LazyPoints<Mcl> LazyPoints<Mcl>::operator+(const LazyPoints<Mcl>& rhs) const;

template <typename T>
LazyPoints<T> LazyPoints<T>::operator+(const LazyPoint<T>& rhs) const {
    Elements<typename T::Point> bases;
    Elements<typename T::Scalar> exps;

    for (auto p: m_points) {
        bases.Add(p.m_base);
        exps.Add(p.m_exp);
    }
    bases.Add(rhs.m_base);
    exps.Add(rhs.m_exp);

    return LazyPoints<T>(bases, exps);
}
template LazyPoints<Mcl> LazyPoints<Mcl>::operator+(const LazyPoint<Mcl>& rhs) const;
