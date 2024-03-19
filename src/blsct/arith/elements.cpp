// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/elements.h>
#include <blsct/arith/mcl/mcl_g1point.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <deque>
#include <sstream>
#include <tinyformat.h>
#include <util/strencodings.h>

template <typename T>
OrderedElements<T>::OrderedElements(){};
template OrderedElements<MclG1Point>::OrderedElements();

template <typename T>
OrderedElements<T>::OrderedElements(const std::set<T>& set)
{
    m_set = set;
};
template OrderedElements<MclG1Point>::OrderedElements(const std::set<MclG1Point>& set);

template <typename T>
Elements<T> OrderedElements<T>::GetElements() const
{
    if (Size() == 0) return Elements<T>();
    std::vector<T> ret;
    std::copy(m_set.begin(), m_set.end(), std::back_inserter(ret));

    return ret;
};
template Elements<MclG1Point> OrderedElements<MclG1Point>::GetElements() const;

template <typename T>
size_t OrderedElements<T>::Size() const
{
    return m_set.size();
}
template size_t OrderedElements<MclG1Point>::Size() const;

template <typename T>
void OrderedElements<T>::Add(const T& x)
{
    if (m_set.count(x) > 0)
        return;
    m_set.insert(x);
}
template void OrderedElements<MclG1Point>::Add(const MclG1Point&);

template <typename T>
void OrderedElements<T>::Add(const OrderedElements<T>& x)
{
    auto list = x.GetElements();

    for (size_t i = 0; i < list.Size(); ++i) {
        Add(list[i]);
    }
}
template void OrderedElements<MclG1Point>::Add(const OrderedElements<MclG1Point>&);

template <typename T>
bool OrderedElements<T>::Exists(const T& x) const
{
    return m_set.count(x) > 0;
}
template bool OrderedElements<MclG1Point>::Exists(const MclG1Point&) const;

template <typename T>
void OrderedElements<T>::Clear()
{
    m_set.clear();
}
template void OrderedElements<MclG1Point>::Clear();

template <typename T>
bool OrderedElements<T>::Empty() const
{
    return m_set.empty();
}
template bool OrderedElements<MclG1Point>::Empty() const;

template <typename T>
bool OrderedElements<T>::Remove(const T& x)
{
    return m_set.erase(x) > 0;
}
template bool OrderedElements<MclG1Point>::Remove(const MclG1Point& x);

template <typename T>
std::vector<uint8_t> OrderedElements<T>::GetVch() const
{
    std::vector<uint8_t> aggr_vec;
    for (T x : m_set) {
        auto vec = x.GetVch();
        aggr_vec.insert(aggr_vec.end(), vec.begin(), vec.end());
    }
    return aggr_vec;
}
template std::vector<uint8_t> OrderedElements<MclG1Point>::GetVch() const;

template <typename T>
std::string OrderedElements<T>::GetString(const uint8_t& radix) const
{
    std::stringstream ss;
    ss << "[";
    auto it = m_set.begin();

    while (it != m_set.end()) {
        ss << it->GetString(radix);
        ++it;
        if (it != m_set.end()) ss << ", ";
    }
    ss << "]";

    return ss.str();
}
template std::string OrderedElements<MclG1Point>::GetString(const uint8_t& radix) const;


// Elements

template <typename T>
Elements<T>::Elements()
{
}
template Elements<MclG1Point>::Elements();
template Elements<MclScalar>::Elements();

template <typename T>
Elements<T>::Elements(const std::vector<T>& vec)
{
    m_vec = vec;
}
template Elements<MclG1Point>::Elements(const std::vector<MclG1Point>& vec);
template Elements<MclScalar>::Elements(const std::vector<MclScalar>& vec);

template <typename T>
Elements<T>::Elements(const size_t& size, const T& default_value)
{
    std::vector<T> vec(size, default_value);
    m_vec = vec;
}
template Elements<MclScalar>::Elements(const size_t&, const MclScalar&);
template Elements<MclG1Point>::Elements(const size_t&, const MclG1Point&);

template <typename T>
Elements<T>::Elements(const Elements<T>& other)
{
    m_vec = other.m_vec;
}
template Elements<MclScalar>::Elements(const Elements<MclScalar>& x);
template Elements<MclG1Point>::Elements(const Elements<MclG1Point>& x);

template <typename T>
bool Elements<T>::Empty() const
{
    return m_vec.empty();
}
template bool Elements<MclScalar>::Empty() const;
template bool Elements<MclG1Point>::Empty() const;

template <typename T>
bool Elements<T>::Find(const T& x) const
{
    for (size_t i = 0; i < Size(); ++i) {
        if (operator[](i) == x)
            return true;
    }
    return false;
}
template bool Elements<MclG1Point>::Find(const MclG1Point& x) const;

template <typename T>
std::vector<uint8_t> Elements<T>::GetVch() const
{
    std::vector<uint8_t> aggr_vec;
    for (T x: m_vec) {
        auto vec = x.GetVch();
        aggr_vec.insert(aggr_vec.end(), vec.begin(), vec.end());
    }
    return aggr_vec;
}
template std::vector<uint8_t> Elements<MclScalar>::GetVch() const;
template std::vector<uint8_t> Elements<MclG1Point>::GetVch() const;

template <typename T>
T Elements<T>::Sum() const
{
    T ret;
    for (T s : m_vec) {
        ret = ret + s;
    }
    return ret;
}
template MclScalar Elements<MclScalar>::Sum() const;
template MclG1Point Elements<MclG1Point>::Sum() const;

template <typename T>
void Elements<T>::ConfirmIndexInsideRange(const uint32_t& index) const
{
    if (index >= m_vec.size()) {
        auto s = strprintf("index %d is out of range [0..%d]", index, m_vec.size() - 1ul);
        throw std::runtime_error(s);
    }
}
template void Elements<MclScalar>::ConfirmIndexInsideRange(const uint32_t&) const;
template void Elements<MclG1Point>::ConfirmIndexInsideRange(const uint32_t&) const;

template <typename T>
T& Elements<T>::operator[](const size_t& index)
{
    ConfirmIndexInsideRange(index);
    return m_vec[index];
}
template MclScalar& Elements<MclScalar>::operator[](const size_t&);
template MclG1Point& Elements<MclG1Point>::operator[](const size_t&);

template <typename T>
T Elements<T>::operator[](const size_t& index) const
{
    ConfirmIndexInsideRange(index);
    return m_vec[index];
}
template MclScalar Elements<MclScalar>::operator[](const size_t&) const;
template MclG1Point Elements<MclG1Point>::operator[](const size_t&) const;

template <typename T>
size_t Elements<T>::Size() const
{
    return m_vec.size();
}
template size_t Elements<MclScalar>::Size() const;
template size_t Elements<MclG1Point>::Size() const;

template <typename T>
void Elements<T>::Add(const T& x)
{
    m_vec.push_back(x);
}
template void Elements<MclScalar>::Add(const MclScalar&);
template void Elements<MclG1Point>::Add(const MclG1Point&);

template <typename T>
void Elements<T>::Clear()
{
    m_vec.clear();
}
template void Elements<MclScalar>::Clear();
template void Elements<MclG1Point>::Clear();

template <typename T>
inline void Elements<T>::ConfirmSizesMatch(const size_t& other_size) const
{
    if (m_vec.size() != other_size) {
        throw std::runtime_error(std::string(__func__) + ": Sizes of elements are expected to be the same, but different");
    }
}
template void Elements<MclScalar>::ConfirmSizesMatch(const size_t&) const;
template void Elements<MclG1Point>::ConfirmSizesMatch(const size_t&) const;

template <typename T>
Elements<T> Elements<T>::FirstNPow(const T& k, const size_t& n, const size_t& from_index)
{
    Elements<T> ret;
    T x(1);
    for (size_t i = 0; i < n + from_index; ++i) {
        if (i >= from_index) {
            ret.m_vec.push_back(x);
        }
        x = x * k;
    }
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::FirstNPow(const MclScalar&, const size_t&, const size_t& from_index);

template <typename T>
Elements<T> Elements<T>::RepeatN(const T& k, const size_t& n)
{
    Elements<T> ret;
    for (size_t i = 0; i < n; ++i) {
        ret.m_vec.push_back(k);
    }
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::RepeatN(const MclScalar&, const size_t&);
template Elements<MclG1Point> Elements<MclG1Point>::RepeatN(const MclG1Point&, const size_t&);

template <typename T>
Elements<T> Elements<T>::RandVec(const size_t& n, const bool exclude_zero)
{
    Elements<T> ret;
    for (size_t i = 0; i < n; ++i) {
        auto x = T::Rand(exclude_zero);
        ret.m_vec.push_back(x);
    }
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::RandVec(const size_t&, const bool);

template <typename T>
template <typename Scalar>
Elements<T> Elements<T>::operator*(const Elements<Scalar>& rhs) const
{
    ConfirmSizesMatch(rhs.Size());

    Elements<T> ret;
    for (size_t i = 0; i < m_vec.size(); ++i) {
        ret.m_vec.push_back(m_vec[i] * rhs[i]);
    }
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::operator*(const Elements<MclScalar>&) const;
template Elements<MclG1Point> Elements<MclG1Point>::operator*(const Elements<MclScalar>&) const;

template <typename T>
template <typename Scalar>
Elements<T> Elements<T>::operator*(const Scalar& rhs) const
{
    Elements<T> ret;
    for (size_t i = 0; i < m_vec.size(); ++i) {
        ret.m_vec.push_back(m_vec[i] * rhs);
    }
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::operator*(const MclScalar&) const;
template Elements<MclG1Point> Elements<MclG1Point>::operator*(const MclScalar&) const;

template <typename T>
Elements<T> Elements<T>::operator+(const Elements<T>& rhs) const
{
    ConfirmSizesMatch(rhs.Size());

    Elements<T> ret;
    for (size_t i = 0; i < m_vec.size(); ++i) {
        ret.m_vec.push_back(m_vec[i] + rhs.m_vec[i]);
    }
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::operator+(const Elements<MclScalar>&) const;
template Elements<MclG1Point> Elements<MclG1Point>::operator+(const Elements<MclG1Point>&) const;

template <typename T>
Elements<T> Elements<T>::operator-(const Elements<T>& rhs) const
{
    ConfirmSizesMatch(rhs.Size());

    Elements<T> ret;
    for (size_t i = 0; i < m_vec.size(); ++i) {
        ret.m_vec.push_back(m_vec[i] - rhs.m_vec[i]);
    }
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::operator-(const Elements<MclScalar>&) const;
template Elements<MclG1Point> Elements<MclG1Point>::operator-(const Elements<MclG1Point>&) const;

template <typename T>
Elements<T> Elements<T>::operator-(const T& rhs) const
{
    Elements<T> ret;
    for (size_t i = 0; i < m_vec.size(); ++i) {
        ret.m_vec.push_back(m_vec[i] - rhs);
    }
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::operator-(const MclScalar&) const;
template Elements<MclG1Point> Elements<MclG1Point>::operator-(const MclG1Point&) const;

template <typename T>
bool Elements<T>::operator<=(const T& rhs) const
{
    for (size_t i = 0; i < m_vec.size(); ++i) {
        if (m_vec[i] > rhs) return false;
    }
    return true;
}
template bool Elements<MclScalar>::operator<=(const MclScalar&) const;

template <typename T>
bool Elements<T>::operator>=(const T& rhs) const
{
    for (size_t i = 0; i < m_vec.size(); ++i) {
        if (m_vec[i] < rhs) return false;
    }
    return true;
}
template bool Elements<MclScalar>::operator>=(const MclScalar&) const;

template <typename T>
void Elements<T>::operator=(const Elements<T>& rhs)
{
    m_vec.clear();
    for (size_t i = 0; i < rhs.m_vec.size(); ++i) {
        auto copy = T(rhs.m_vec[i]);
        m_vec.push_back(copy);
    }
}
template void Elements<MclScalar>::operator=(const Elements<MclScalar>&);
template void Elements<MclG1Point>::operator=(const Elements<MclG1Point>&);

template <typename T>
bool Elements<T>::operator==(const Elements<T>& rhs) const
{
    if (m_vec.size() != rhs.Size()) {
        return false;
    }

    for (size_t i = 0; i < m_vec.size(); ++i) {
        if (m_vec[i] != rhs[i]) return false;
    }
    return true;
}
template bool Elements<MclScalar>::operator==(const Elements<MclScalar>&) const;
template bool Elements<MclG1Point>::operator==(const Elements<MclG1Point>&) const;

template <typename T>
bool Elements<T>::operator!=(const Elements<T>& rhs) const
{
    return !operator==(rhs);
}
template bool Elements<MclScalar>::operator!=(const Elements<MclScalar>&) const;
template bool Elements<MclG1Point>::operator!=(const Elements<MclG1Point>&) const;

template <typename T>
Elements<T> Elements<T>::From(const size_t from_index) const
{
    if (from_index >= Size()) {
        throw std::runtime_error(std::string(__func__) + ": 'From' index out of range");
    }

    Elements<T> ret;
    for (size_t i = from_index; i < m_vec.size(); ++i) {
        ret.m_vec.push_back(m_vec[i]);
    }
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::From(const size_t from_index) const;
template Elements<MclG1Point> Elements<MclG1Point>::From(const size_t from_index) const;

template <typename T>
Elements<T> Elements<T>::To(const size_t to_index) const
{
    if (to_index > Size()) {
        throw std::runtime_error(std::string(__func__) + ": 'To' index out of range");
    }

    Elements<T> ret;
    for (size_t i = 0; i < to_index; ++i) {
        ret.m_vec.push_back(m_vec[i]);
    }
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::To(const size_t to_index) const;
template Elements<MclG1Point> Elements<MclG1Point>::To(const size_t to_index) const;

template <typename T>
Elements<T> Elements<T>::Negate() const
{
    Elements<T> ret;
    for (auto& x : m_vec) {
        ret.Add(x.Negate());
    }
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::Negate() const;

template <typename T>
Elements<T> Elements<T>::Invert() const
{
    // build:
    // - elem_inverses = (x_1, x_2, ..., x_n)^-1
    // - extract_factors = [1, x_1, x_1*x_2, ..., x_1*...*x_n]
    Elements<T> extract_factors;  // cumulative product sequence used to cancel out inverses
    T elem_inverse_prod;  // product of all element inverses
    {
        T n(1);
        for (auto& x: m_vec) {
            extract_factors.Add(n);
            n = n * x;
        }
        elem_inverse_prod = n.Invert();
    }

    // calculate inverses of all elements
    std::deque<T> q;
    size_t i = m_vec.size()-1;
    for (;;) {
        // extract x_i^-1 by multiplying x_1*...*x_{i-1}
        T x = elem_inverse_prod * extract_factors[i];
        q.push_front(x);

        // drop the inverse just extracted
        elem_inverse_prod = elem_inverse_prod * m_vec[i];

        if (i == 0) break;
        --i;
    }

    Elements<T> ret({ q.begin(), q.end() });
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::Invert() const;

template <typename T>
Elements<T> Elements<T>::Reverse() const
{
    std::vector<T> rev_vec(m_vec.rbegin(), m_vec.rend());
    Elements<T> ret(rev_vec);
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::Reverse() const;

template <typename T>
T Elements<T>::Product() const
{
    if (m_vec.size() == 0) {
        throw std::runtime_error(std::string(__func__) + ": Cannot compute the product of empty vector");
    }
    T ret = m_vec[0];
    for (size_t i=1; i<m_vec.size(); ++i) {
        ret = ret * m_vec[i];
    }
    return ret;
}
template MclScalar Elements<MclScalar>::Product() const;

template <typename T>
Elements<T> Elements<T>::Square() const
{
    Elements<T> ret;
    std::transform(m_vec.begin(), m_vec.end(), std::back_inserter(ret.m_vec), [](const T& x) {
        return x.Square();
    });
    return ret;
}
template Elements<MclScalar> Elements<MclScalar>::Square() const;

template <typename T>
std::string Elements<T>::GetString(const uint8_t& radix) const
{
    std::stringstream ss;
    ss << "[";
    for (size_t i=0; i<m_vec.size(); ++i) {
        ss << HexStr(m_vec[i].GetVch());
        if (i != m_vec.size() - 1) ss << ", ";
    }
    ss << "]";

    return ss.str();
}
template std::string Elements<MclG1Point>::GetString(const uint8_t& radix) const;
template std::string Elements<MclScalar>::GetString(const uint8_t& radix) const;
