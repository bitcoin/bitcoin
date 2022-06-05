// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_VECTOR_H
#define BITCOIN_TEST_UTIL_VECTOR_H

#include <test/util/traits.h>
#include <uint256.h>

#include <algorithm>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/**
 * A `std::vector` with a few extra operations to make it nice for unit tests,
 * especially of byte vectors (`vector<unsigned char>`).
 */
template <typename E>
struct ex_vector : public std::vector<E> {
    using std::vector<E>::vector;
    using std::vector<E>::operator[];

    explicit ex_vector(const std::vector<E>& other) : std::vector<E>(other)
    {
    }

    explicit ex_vector(std::vector<E>&& other) : std::vector<E>(std::move(other))
    {
    }

    /**
     * Return half-open subrange from byte vector, use notation `v[{i,j}]` for
     * the subrange `[i,j)`.  N.B.: `first`, `one-past-last`, _NOT_ `first` + `length`.
     *
     * (Not sure why `operator[]` overloads must be a member function, but that's
     * the way it is.))
     */
    ex_vector operator[](std::tuple<size_t, size_t> range) const
    {
        auto [i, j] = range;
        assert(i <= j && j <= this->size());
        ex_vector r(j - i, 0);
        std::copy(this->begin() + i, this->begin() + j, r.begin());
        return r;
    }
};

/**
 * Make an `ex_vector` from a `uint160` or `uint256`
 */
template <unsigned int BITS>
ex_vector<unsigned char> from_base_blob(const base_blob<BITS>& bb)
{
    return ex_vector<unsigned char>(bb.begin(), bb.end());
}

namespace test::util::vector_ops {

/**
 * Convert a `std::vector` into an `ex_vector` via unary `+`
 */
template <typename E>
ex_vector<E> operator+(const std::vector<E>& v)
{
    return ex_vector<E>(v);
}

/**
 * Concatenate two vectors (producing an `ex_vector` for further operations)
 */
template <typename E>
ex_vector<E> operator||(const std::vector<E>& lhs, const std::vector<E>& rhs)
{
    ex_vector<E> r;
    r.resize(lhs.size() + rhs.size());
    std::copy(lhs.begin(), lhs.end(), r.begin());
    std::copy(rhs.begin(), rhs.end(), r.begin() + lhs.size());
    return r;
}

} // namespace test::util::vector_ops

#endif // BITCOIN_TEST_UTIL_VECTOR_H
