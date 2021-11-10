// Copyright (c) 2016-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_SIZE_ARRAY_HPP
#define BOOST_PFR_DETAIL_SIZE_ARRAY_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <cstddef>      // metaprogramming stuff

namespace boost { namespace pfr { namespace detail {

///////////////////// Array that has the constexpr
template <std::size_t N>
struct size_array {                         // libc++ misses constexpr on operator[]
    typedef std::size_t type;
    std::size_t data[N];

    static constexpr std::size_t size() noexcept { return N; }

    constexpr std::size_t count_nonzeros() const noexcept {
        std::size_t count = 0;
        for (std::size_t i = 0; i < size(); ++i) {
            if (data[i]) {
                ++ count;
            }
        }
        return count;
    }

    constexpr std::size_t count_from_opening_till_matching_parenthis_seq(std::size_t from, std::size_t opening_parenthis, std::size_t closing_parenthis) const noexcept {
        if (data[from] != opening_parenthis) {
            return 0;
        }
        std::size_t unclosed_parnthesis = 0;
        std::size_t count = 0;
        for (; ; ++from) {
            if (data[from] == opening_parenthis) {
                ++ unclosed_parnthesis;
            } else if (data[from] == closing_parenthis) {
                -- unclosed_parnthesis;
            }
            ++ count;

            if (unclosed_parnthesis == 0) {
                return count;
            }
        }

        return count;
    }
};

template <>
struct size_array<0> {                         // libc++ misses constexpr on operator[]
    typedef std::size_t type;
    std::size_t data[1];

    static constexpr std::size_t size() noexcept { return 0; }

    constexpr std::size_t count_nonzeros() const noexcept {
        return 0;
    }
};

template <std::size_t I, std::size_t N>
constexpr std::size_t get(const size_array<N>& a) noexcept {
    static_assert(I < N, "====================> Boost.PFR: Array index out of bounds");
    return a.data[I];
}



}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_SIZE_ARRAY_HPP
