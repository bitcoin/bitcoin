// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BLS_ETH 1

#include <blsct/arith/mcl/mcl.h>
#include <blsct/building_block/lazy_points.h>
#include <bls/bls384_256.h>

template <typename T>
LazyPoints<T>::LazyPoints(const Elements<typename T::Point>& bases, const Elements<typename T::Scalar>& exps) {
    if (bases.Size() != exps.Size()) {
        throw std::runtime_error("sizes of bases and exps don't match");
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
void LazyPoints<T>::Add(const typename T::Point& p)
{
    LazyPoint<T> lp(p, Scalar(1));
    Add(lp);
}
template void LazyPoints<Mcl>::Add(const Mcl::Point& p);

template <typename T>
void LazyPoints<T>::Add(const typename T::Point& p, const typename T::Scalar& s)
{
    LazyPoint<T> lp(p, s);
    Add(lp);
}
template void LazyPoints<Mcl>::Add(const Mcl::Point& p, const Mcl::Scalar& s);

template <typename T>
void LazyPoints<T>::Add(const Elements<typename T::Point>& ps, const typename T::Scalar& s)
{
    for (size_t i=0; i<ps.Size(); ++i) {
        LazyPoint<T> lp(ps[i], s);
        Add(lp);
    }
}
template void LazyPoints<Mcl>::Add(const Elements<Mcl::Point>& ps, const Mcl::Scalar& s);

template <typename T>
void LazyPoints<T>::Add(
    const Elements<typename T::Point>& ps,
    const Elements<typename T::Scalar>& ss
)
{
    if (ps.Size() != ss.Size()) {
        throw std::runtime_error(std::string(__func__) + ": Sizes of points and scalars don't match");
    }
    for (size_t i=0; i<ps.Size(); ++i) {
        LazyPoint<T> lp(ps[i], ss[i]);
        Add(lp);
    }
}
template void LazyPoints<Mcl>::Add(
    const Elements<Mcl::Point>& ps,
    const Elements<Mcl::Scalar>& ss
);

template <typename T>
typename T::Point LazyPoints<T>::Sum() const {
    return T::Util::template MultiplyLazyPoints(m_points);
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
