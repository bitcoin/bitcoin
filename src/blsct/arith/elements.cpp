// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/elements.h>
#include <blsct/arith/g1point.h>
#include <blsct/arith/scalar.h>
#include <tinyformat.h>

template <typename T>
Elements<T>::Elements(const size_t& size, const T& default_value)
{
    std::vector<T> vec(size, default_value);
    m_vec = vec;
}
template Elements<Scalar>::Elements(const size_t&, const Scalar&);
template Elements<G1Point>::Elements(const size_t&, const G1Point&);

template <typename T>
Elements<T>::Elements(const Elements &x)
{
    m_vec = x.m_vec;
}
template Elements<Scalar>::Elements(const Elements &x);
template Elements<G1Point>::Elements(const Elements &x);

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
void Elements<T>::ConfirmIndexInsideRange(const uint32_t& index) const {
    if (index >= m_vec.size()) {
        auto s = strprintf("index %d is out of range [0..%d]", index, m_vec.size() - 1ul);
        throw std::runtime_error(s);
    }
}
template void Elements<Scalar>::ConfirmIndexInsideRange(const uint32_t&) const;
template void Elements<G1Point>::ConfirmIndexInsideRange(const uint32_t&) const;

template <typename T>
T& Elements<T>::operator[](const size_t& index)
{
    ConfirmIndexInsideRange(index);
    return m_vec[index];
}
template Scalar& Elements<Scalar>::operator[](const size_t&);
template G1Point& Elements<G1Point>::operator[](const size_t&);

template <typename T>
T Elements<T>::operator[](const size_t& index) const
{
    ConfirmIndexInsideRange(index);
    return m_vec[index];
}
template Scalar Elements<Scalar>::operator[](const size_t&) const;
template G1Point Elements<G1Point>::operator[](const size_t&) const;

template <typename T>
size_t Elements<T>::Size() const
{
    return m_vec.size();
}
template size_t Elements<Scalar>::Size() const;
template size_t Elements<G1Point>::Size() const;

template <typename T>
bool Elements<T>::Empty() const
{
    return m_vec.empty();
}
template bool Elements<Scalar>::Empty() const;
template bool Elements<G1Point>::Empty() const;

template <typename T>
void Elements<T>::Add(const T& x)
{
    m_vec.push_back(x);
}
template void Elements<Scalar>::Add(const Scalar&);
template void Elements<G1Point>::Add(const G1Point&);

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
Elements<T> Elements<T>::FirstNPow(const Scalar& k, const size_t& n, const size_t& from_index)
{
    if constexpr (std::is_same_v<T, Scalar>) {
        Elements<Scalar> ret;
        Scalar x(1);
        for (size_t i = 0; i < n + from_index; ++i) {
            if (i >= from_index) {
                ret.m_vec.push_back(x);
            }
            x = x * k;
        }
        return ret;
    } else {
        throw std::runtime_error("Not implemented");
    }
}
template Elements<Scalar> Elements<Scalar>::FirstNPow(const Scalar&, const size_t&, const size_t& from_index);

template <typename T>
Elements<T> Elements<T>::RepeatN(const T& k, const size_t& n)
{
    Elements<T> ret;
    for (size_t i = 0; i < n; ++i) {
        ret.m_vec.push_back(k);
    }
    return ret;
}
template Elements<Scalar> Elements<Scalar>::RepeatN(const Scalar&, const size_t&);
template Elements<G1Point> Elements<G1Point>::RepeatN(const G1Point&, const size_t&);

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
template Elements<Scalar> Elements<Scalar>::operator*(const Scalar& s) const;
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
void Elements<T>::operator=(const Elements<T>& other)
{
    if constexpr (std::is_same_v<T, Scalar>) {
        m_vec.clear();
        for (size_t i = 0; i < other.m_vec.size(); ++i) {
            auto copy = Scalar(other.m_vec[i]);
            m_vec.push_back(copy);
        }
    } else if constexpr (std::is_same_v<T, G1Point>) {
        m_vec.clear();
        for (size_t i = 0; i < other.m_vec.size(); ++i) {
            auto copy = G1Point(other.m_vec[i]);
            m_vec.push_back(copy);
        }
    } else {
        throw std::runtime_error("Not implemented");
    }
}
template void Elements<Scalar>::operator=(const Elements<Scalar>& other);
template void Elements<G1Point>::operator=(const Elements<G1Point>& other);

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
        throw std::runtime_error("'From' index out of range");
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
        throw std::runtime_error("'To' index out of range");
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

template <typename T>
Elements<T> Elements<T>::Negate() const
{
    if constexpr (std::is_same_v<T, Scalar>) {
        Scalars ret;
        for(auto& x: m_vec) {
            ret.Add(x.Negate());
        }
        return ret;

    } else {
        throw std::runtime_error("Not implemented");
    }
}
template Elements<Scalar> Elements<Scalar>::Negate() const;
