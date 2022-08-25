// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/elements.h>
#include <blsct/arith/g1point.h>
#include <blsct/arith/scalar.h>

template <typename T>
T Elements<T>::Sum() const
{
    T ret;
    for (T s : m_vec) {
        ret = ret + s;
    }
    return ret;
}
template Scalar Elements<Scalar>::Sum() const;
template G1Point Elements<G1Point>::Sum() const;

template <typename T>
T Elements<T>::operator[](int index) const
{
    return m_vec[index];
}
template Scalar Elements<Scalar>::operator[](int) const;
template G1Point Elements<G1Point>::operator[](int) const;

template <typename T>
size_t Elements<T>::Size() const
{
    return m_vec.size();
}
template size_t Elements<Scalar>::Size() const;
template size_t Elements<G1Point>::Size() const;

template <typename T>
void Elements<T>::Add(const T x)
{
    m_vec.push_back(x);
}
template void Elements<Scalar>::Add(const Scalar);
template void Elements<G1Point>::Add(const G1Point);

template <typename T>
inline void Elements<T>::ConfirmSizesMatch(const size_t& other_size) const
{
    if (m_vec.size() != other_size) {
        throw std::runtime_error("sizes of elements are expected to be the same, but different");
    }
}
template void Elements<Scalar>::ConfirmSizesMatch(const size_t&) const;
template void Elements<G1Point>::ConfirmSizesMatch(const size_t&) const;

template <typename T>
Elements<T> Elements<T>::FirstNPow(const size_t& n, const Scalar& k)
{
    if constexpr (std::is_same_v<T, Scalar>) {
        Elements<Scalar> ret;
        Scalar x(1);
        for (size_t i = 0; i < n; ++i) {
            ret.m_vec.push_back(x);
            x = x * k;
        }
        return ret;
    } else {
        throw std::runtime_error("Not implemented");
    }
}
template Elements<Scalar> Elements<Scalar>::FirstNPow(const size_t&, const Scalar&);

template <typename T>
Elements<T> Elements<T>::RepeatN(const size_t& n, const T& k)
{
    Elements<T> ret;
    for (size_t i = 0; i < n; ++i) {
        ret.m_vec.push_back(k);
    }
    return ret;
}
template Elements<Scalar> Elements<Scalar>::RepeatN(const size_t&, const Scalar&);
template Elements<G1Point> Elements<G1Point>::RepeatN(const size_t&, const G1Point&);

template <typename T>
Elements<T> Elements<T>::RandVec(const size_t& n, const bool exclude_zero)
{
    if constexpr (std::is_same_v<T, Scalar>) {
        Elements<Scalar> ret;
        for (size_t i = 0; i < n; ++i) {
            auto x = Scalar::Rand(exclude_zero);
            ret.m_vec.push_back(x);
        }
        return ret;
    } else {
        throw std::runtime_error("Not implemented");
    }
}
template Elements<Scalar> Elements<Scalar>::RandVec(const size_t&, const bool);

template <typename T>
Elements<T> Elements<T>::operator*(const Elements<Scalar>& other) const
{
    ConfirmSizesMatch(other.Size());

    if constexpr (std::is_same_v<T, Scalar>) {
        Elements<Scalar> ret;
        for (size_t i = 0; i < m_vec.size(); ++i) {
            ret.m_vec.push_back(m_vec[i] * other[i]);
        }
        return ret;

    } else if constexpr (std::is_same_v<T, G1Point>) {
        Elements<G1Point> ret;
        for (size_t i = 0; i < Size(); ++i) {
            ret.m_vec.push_back(m_vec[i] * other[i]);
        }
        return ret;

    } else {
        throw std::runtime_error("Not implemented");
    }
}
template Elements<Scalar> Elements<Scalar>::operator*(const Elements<Scalar>& other) const;
template Elements<G1Point> Elements<G1Point>::operator*(const Elements<Scalar>& other) const;

template <typename T>
Elements<T> Elements<T>::operator*(const Scalar& s) const
{
    if constexpr (std::is_same_v<T, Scalar>) {
        Elements<Scalar> ret;
        for (size_t i = 0; i < m_vec.size(); ++i) {
            ret.m_vec.push_back(m_vec[i] * s);
        }
        return ret;

    } else if constexpr (std::is_same_v<T, G1Point>) {
        Elements<G1Point> ret;
        for (size_t i = 0; i < m_vec.size(); ++i) {
            ret.m_vec.push_back(m_vec[i] * s);
        }
        return ret;

    } else {
        throw std::runtime_error("Not implemented");
    }
}
template Elements<Scalar> Elements<Scalar>::operator*(const Scalar&) const;
template Elements<G1Point> Elements<G1Point>::operator*(const Scalar& s) const;

template <typename T>
Elements<T> Elements<T>::operator+(const Elements<T>& other) const
{
    ConfirmSizesMatch(other.Size());

    Elements<T> ret;
    for (size_t i = 0; i < m_vec.size(); ++i) {
        ret.m_vec.push_back(m_vec[i] + other.m_vec[i]);
    }
    return ret;
}
template Elements<Scalar> Elements<Scalar>::operator+(const Elements<Scalar>& other) const;
template Elements<G1Point> Elements<G1Point>::operator+(const Elements<G1Point>& other) const;

template <typename T>
Elements<T> Elements<T>::operator-(const Elements<T>& other) const
{
    ConfirmSizesMatch(other.Size());

    Elements<T> ret;
    for (size_t i = 0; i < m_vec.size(); ++i) {
        ret.m_vec.push_back(m_vec[i] - other.m_vec[i]);
    }
    return ret;
}
template Elements<Scalar> Elements<Scalar>::operator-(const Elements<Scalar>& other) const;
template Elements<G1Point> Elements<G1Point>::operator-(const Elements<G1Point>& other) const;

template <typename T>
bool Elements<T>::operator==(const Elements<T>& other) const
{
    if (m_vec.size() != other.Size()) {
        return false;
    }

    for (size_t i = 0; i < m_vec.size(); ++i) {
        if (m_vec[i] != other[i]) return false;
    }
    return true;
}
template bool Elements<Scalar>::operator==(const Elements<Scalar>& other) const;
template bool Elements<G1Point>::operator==(const Elements<G1Point>& other) const;

template <typename T>
bool Elements<T>::operator!=(const Elements<T>& other) const
{
    return !operator==(other);
}
template bool Elements<Scalar>::operator!=(const Elements<Scalar>& other) const;
template bool Elements<G1Point>::operator!=(const Elements<G1Point>& other) const;

template <typename T>
Elements<T> Elements<T>::From(const size_t from_index) const
{
    if (from_index >= Size()) {
        throw std::runtime_error("from index out of range");
    }

    Elements<T> ret;
    for (size_t i = from_index; i < m_vec.size(); ++i) {
        ret.m_vec.push_back(m_vec[i]);
    }
    return ret;
}
template Elements<Scalar> Elements<Scalar>::From(const size_t from_index) const;
template Elements<G1Point> Elements<G1Point>::From(const size_t from_index) const;

template <typename T>
Elements<T> Elements<T>::To(const size_t to_index) const
{
    if (to_index > Size()) {
        throw std::runtime_error("to index out of range");
    }

    Elements<T> ret;
    for (size_t i = 0; i < to_index; ++i) {
        ret.m_vec.push_back(m_vec[i]);
    }
    return ret;
}
template Elements<Scalar> Elements<Scalar>::To(const size_t to_index) const;
template Elements<G1Point> Elements<G1Point>::To(const size_t to_index) const;

template <typename T>
G1Point Elements<T>::MulVec(const Elements<Scalar>& scalars) const
{
    if constexpr (std::is_same_v<T, G1Point>) {
        ConfirmSizesMatch(scalars.Size());

        const size_t vec_count = m_vec.size();

        std::vector<mclBnG1> vec_g1(vec_count);
        std::vector<mclBnFr> vec_fr(vec_count);

        for (size_t i = 0; i < vec_count; ++i) {
            vec_g1[i] = m_vec[i].m_p;
            vec_fr[i] = scalars[i].m_fr;
        }

        G1Point ret;
        mclBnG1_mulVec(&ret.m_p, vec_g1.data(), vec_fr.data(), vec_count);
        return ret;

    } else {
        throw std::runtime_error("Not implemented");
    }
}
template G1Point Elements<G1Point>::MulVec(const Elements<Scalar>&) const;
