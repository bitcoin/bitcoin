// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Diverse arithmetic operations in the bls curve
// inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
// and https://github.com/monero-project/monero/blob/master/src/ringct/bulletproofs.cc

#ifndef NAVCOIN_BLSCT_ARITH_ELEMENTS_H
#define NAVCOIN_BLSCT_ARITH_ELEMENTS_H

#include <serialize.h>

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>


template <typename T>
class Elements
{
public:
    using value_type = T;

    Elements();
    Elements(const std::vector<T>& vec);
    Elements(const size_t& size, const T& default_value);
    Elements(const Elements& other);

    T Sum() const;
    T& operator[](const size_t& index);
    T operator[](const size_t& index) const;
    size_t Size() const;
    void Add(const T& x);
    void Clear();
    bool Empty() const;
    std::vector<uint8_t> GetVch() const;

    void ConfirmIndexInsideRange(const uint32_t& index) const;
    void ConfirmSizesMatch(const size_t& other_size) const;

    /**
     * returns a geometric series of k with n terms i.e.
     * k^0, k^1, ..., k^{n-1}
     * if from_index is specified, it returns a geometrix series
     * k^{from_index}, ..., k^{from_index + n - 1}
     */
    static Elements<T> FirstNPow(const T& k, const size_t& n, const size_t& from_index = 0);

    static Elements<T> RepeatN(const T& k, const size_t& n);
    static Elements<T> RandVec(const size_t& n, const bool exclude_zero = false);

    /**
     * Scalars x Scalars
     * [a1, a2] * [b1, b2] = [a1*b1, a2*b2]
     *
     * G1Points x Scalars
     * [p1, p2] * [a1, ba] = [p1*a1, p2*a2]
     */
    template <typename Scalar>
    Elements<T> operator*(const Elements<Scalar>& rhs) const;

    /**
     * Scalars x Scalar
     * [s1, s2] * s = [s1*s, s2*s]
     *
     * G1Points x Scalar
     * [p1, p2] ^ s = [p1*s, p2*s]
     */
    template <typename Scalar>
    Elements<T> operator*(const Scalar& rhs) const;

    /**
     * [p1, p2] + [q1, q2] = [p1+q1, p2+q2]
     */
    Elements<T> operator+(const Elements<T>& rhs) const;

    /**
     * [p1, p2] - [q1, q2] = [p1-q1, p2-q2]
     */
    Elements<T> operator-(const Elements<T>& rhs) const;

    void operator=(const Elements<T>& rhs);

    bool operator==(const Elements<T>& rhs) const;

    bool operator!=(const Elements<T>& rhs) const;

    /**
     * Returns elements slice [fromIndex, vec.size())
     */
    Elements<T> From(const size_t from_index) const;

    /**
     * Returns elements slice [0, toIndex)
     */
    Elements<T> To(const size_t to_index) const;

    Elements<T> Negate() const;

    Elements<T> Invert() const;

    Elements<T> Reverse() const;

    T Product() const;

    Elements<T> Square() const;

    std::string GetString(const uint8_t& radix = 16) const;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        ::WriteCompactSize(s, m_vec.size());
        for (auto& it : m_vec) {
            ::Serialize(s, it);
        }
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        size_t v_size;
        v_size = ::ReadCompactSize(s);
        m_vec.resize(v_size);
        Clear();
        for (size_t i = 0; i < v_size; i++) {
            T n;
            ::Unserialize(s, n);
            Add(n);
        }
    }

    std::vector<T> m_vec;
};

template <typename T>
class OrderedElements
{
public:
    using value_type = T;

    OrderedElements();
    OrderedElements(const std::set<T>& vec);
    OrderedElements(const OrderedElements& other);

    Elements<T> GetElements() const;

    size_t Size() const;
    void Add(const T& x);
    bool Remove(const T& x);
    void Clear();
    bool Empty() const;
    bool Exists(const T& x) const;
    std::vector<uint8_t> GetVch() const;

    std::string GetString(const uint8_t& radix = 16) const;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        ::WriteCompactSize(s, m_set.size());
        for (auto& it : m_set) {
            ::Serialize(s, it);
        }
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        size_t v_size;
        v_size = ::ReadCompactSize(s);
        Clear();
        for (size_t i = 0; i < v_size; i++) {
            T n;
            ::Unserialize(s, n);
            Add(n);
        }
    }

    std::set<T> m_set;
};


#endif // NAVCOIN_BLSCT_ARITH_ELEMENTS_H
