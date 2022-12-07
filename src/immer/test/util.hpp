//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <boost/range/irange.hpp>
#include <boost/range/join.hpp>
#include <cstddef>

namespace {

struct identity_t
{
    template <typename T>
    decltype(auto) operator()(T&& x)
    {
        return std::forward<decltype(x)>(x);
    }
};

template <typename Integer>
auto test_irange(Integer from, Integer to)
{
#if IMMER_SLOW_TESTS
    return boost::irange(from, to);
#else
    if (to - from < Integer{10})
        return boost::join(boost::irange(Integer{}, Integer{}),
                           boost::join(boost::irange(from, to, 1),
                                       boost::irange(Integer{}, Integer{})));
    else
        return boost::join(boost::irange(from, from + Integer{2}),
                           boost::join(boost::irange(from + Integer{2},
                                                     to - Integer{2},
                                                     (to - from) / Integer{5}),
                                       boost::irange(to - Integer{2}, to)));
#endif
}

} // anonymous namespace

#if IMMER_SLOW_TESTS
#define CHECK_SLOW(...) CHECK(__VA_ARGS__)
#else
#define CHECK_SLOW(...)
#endif

#if IMMER_SLOW_TESTS
#define CHECK_VECTOR_EQUALS_RANGE_AUX(v1_, first_, last_, xf_)                 \
    [](auto&& v1, auto&& first, auto&& last, auto&& xf) {                      \
        auto size = std::distance(first, last);                                \
        CHECK(static_cast<std::ptrdiff_t>(v1.size()) == size);                 \
        if (static_cast<std::ptrdiff_t>(v1.size()) != size)                    \
            return;                                                            \
        for (auto j = 0u; j < size; ++j)                                       \
            CHECK(xf(v1[j]) == xf(*first++));                                  \
    }(v1_, first_, last_, xf_) // CHECK_EQUALS
#else
#define CHECK_VECTOR_EQUALS_RANGE_AUX(v1_, first_, last_, ...)                 \
    [](auto&& v1, auto&& first, auto&& last, auto&& xf) {                      \
        auto size = std::distance(first, last);                                \
        CHECK(static_cast<std::ptrdiff_t>(v1.size()) == size);                 \
        if (static_cast<std::ptrdiff_t>(v1.size()) != size)                    \
            return;                                                            \
        if (size > 0) {                                                        \
            CHECK(xf(v1[0]) == xf(*(first + (0))));                            \
            CHECK(xf(v1[size - 1]) == xf(*(first + (size - 1))));              \
            CHECK(xf(v1[size / 2]) == xf(*(first + (size / 2))));              \
            CHECK(xf(v1[size / 3]) == xf(*(first + (size / 3))));              \
            CHECK(xf(v1[size / 4]) == xf(*(first + (size / 4))));              \
            CHECK(xf(v1[size - 1 - size / 2]) ==                               \
                  xf(*(first + (size - 1 - size / 2))));                       \
            CHECK(xf(v1[size - 1 - size / 3]) ==                               \
                  xf(*(first + (size - 1 - size / 3))));                       \
            CHECK(xf(v1[size - 1 - size / 4]) ==                               \
                  xf(*(first + (size - 1 - size / 4))));                       \
        }                                                                      \
        if (size > 1) {                                                        \
            CHECK(xf(v1[1]) == xf(*(first + (1))));                            \
            CHECK(xf(v1[size - 2]) == xf(*(first + (size - 2))));              \
        }                                                                      \
        if (size > 2) {                                                        \
            CHECK(xf(v1[2]) == xf(*(first + (2))));                            \
            CHECK(xf(v1[size - 3]) == xf(*(first + (size - 3))));              \
        }                                                                      \
    }(v1_, first_, last_, __VA_ARGS__) // CHECK_EQUALS
#endif                                 // IMMER_SLOW_TESTS

#define CHECK_VECTOR_EQUALS_AUX(v1_, v2_, ...)                                 \
    [](auto&& v1, auto&& v2, auto&&... xs) {                                   \
        CHECK_VECTOR_EQUALS_RANGE_AUX(v1, v2.begin(), v2.end(), xs...);        \
    }(v1_, v2_, __VA_ARGS__)

#define CHECK_VECTOR_EQUALS_RANGE(v1, b, e)                                    \
    CHECK_VECTOR_EQUALS_RANGE_AUX((v1), (b), (e), identity_t{})

#define CHECK_VECTOR_EQUALS(v1, v2)                                            \
    CHECK_VECTOR_EQUALS_AUX((v1), (v2), identity_t{})
