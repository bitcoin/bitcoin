#ifndef BOOST_NUMERIC_UTILITY_HPP
#define BOOST_NUMERIC_UTILITY_HPP

//  Copyright (c) 2015 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <cstdint> // intmax_t, uintmax_t, uint8_t, ...
#include <algorithm>
#include <type_traits> // conditional
#include <limits>
#include <cassert>
#include <utility> // pair

#include <boost/integer.hpp> // (u)int_t<>::least, exact

namespace boost {
namespace safe_numerics {
namespace utility {

///////////////////////////////////////////////////////////////////////////////
// used for debugging

// provokes warning message with names of type T
// usage - print_types<T, ...>;
// see https://cukic.co/2019/02/19/tmp-testing-and-debugging-templates

/*
template<typename Tx>
using print_type = typename Tx::error_message;
*/
template <typename... Ts>
struct [[deprecated]] print_types {};

// display value of constexpr during compilation
// usage print_value(N) pn;
template<int N> 
struct print_value {
    enum test : char {
        value = N < 0 ? N - 256 : N + 256
    };
};

// static warning - same as static_assert but doesn't
// stop compilation. 
template <typename T>
struct static_test{};

template <>
struct static_test<std::false_type>{
    [[deprecated]] static_test(){}
};

template<typename T>
using static_warning = static_test<T>;

/*
// can be called by constexpr to produce a compile time
// trap of parameter passed is false.
// usage constexpr_assert(bool)
constexpr int constexpr_assert(const bool tf){
    return 1 / tf;
}
*/

///////////////////////////////////////////////////////////////////////////////
// return an integral constant equal to the the number of bits
// held by some integer type (including the sign bit)

template<typename T>
using bits_type = std::integral_constant<
    int,
    std::numeric_limits<T>::digits
    + (std::numeric_limits<T>::is_signed ? 1 : 0)
>;

/*
From http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious
Find the log base 2 of an integer with a lookup table

    static const char LogTable256[256] =
    {
    #define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
        -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
        LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
    };

    unsigned int v; // 32-bit word to find the log of
    unsigned r;     // r will be lg(v)
    register unsigned int t, tt; // temporaries

    if (tt = v >> 16)
    {
      r = (t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
    }
    else 
    {
      r = (t = v >> 8) ? 8 + LogTable256[t] : LogTable256[v];
    }

The lookup table method takes only about 7 operations to find the log of a 32-bit value. 
If extended for 64-bit quantities, it would take roughly 9 operations. Another operation
can be trimmed off by using four tables, with the possible additions incorporated into each.
Using int table elements may be faster, depending on your architecture.
*/

namespace ilog2_detail {

    template<int N>
    constexpr inline unsigned int ilog2(const typename boost::uint_t<N>::exact & t){
        using half_type = typename boost::uint_t<N/2>::exact;
        const half_type upper_half = static_cast<half_type>(t >> N/2);
        const half_type lower_half = static_cast<half_type>(t);
        return upper_half == 0 ? ilog2<N/2>(lower_half) : N/2 + ilog2<N/2>(upper_half);
    }
    template<>
    constexpr inline unsigned int ilog2<8>(const typename boost::uint_t<8>::exact & t){
        #define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
        const char LogTable256[256] = {
            static_cast<char>(-1), 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
            LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
            LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
        };
        return LogTable256[t];
    }

} // ilog2_detail

template<typename T>
constexpr inline unsigned int ilog2(const T & t){
//  log not defined for negative numbers
//    assert(t > 0);
    if(t == 0)
        return 0;
    return ilog2_detail::ilog2<bits_type<T>::value>(
        static_cast<
            typename boost::uint_t<
                bits_type<T>::value
            >::least
        >(t)
    );
}

// the number of bits required to render the value in x
// including sign bit
template<typename T>
constexpr inline unsigned int significant_bits(const T & t){
    return 1 + ((t < 0) ? ilog2(~t) : ilog2(t));
}

/*
// give the value t, return the number which corresponds
// to all 1's which is higher than that number
template<typename T>
constexpr unsigned int bits_value(const T & t){
    const unsigned int sb = significant_bits(t);
    const unsigned int sb_max = significant_bits(std::numeric_limits<T>::max());
    return sb < sb_max ? ((sb << 1) - 1) : std::numeric_limits<T>::max();
}
*/

///////////////////////////////////////////////////////////////////////////////
// meta functions returning types

// If we use std::max in here we get internal compiler errors
// with MSVC (tested VC2017) ...

// Notes from https://en.cppreference.com/w/cpp/algorithm/max
// Capturing the result of std::max by reference if one of the parameters
// is rvalue produces a dangling reference if that parameter is returned.

template <class T>
// turns out this problem crashes all versions of gcc compilers.  So
// make sure we return by value
//constexpr const T & max(
constexpr inline T max(
    const T & lhs,
    const T & rhs
){
    return lhs > rhs ? lhs : rhs;
}

// given a signed range, return type required to hold all the values
// in the range
template<
    std::intmax_t Min,
    std::intmax_t Max
>
using signed_stored_type = typename boost::int_t<
    max(
        significant_bits(Min),
        significant_bits(Max)
    ) + 1
>::least ;

// given an unsigned range, return type required to hold all the values
// in the range
template<
    std::uintmax_t Min,
    std::uintmax_t Max
>
// unsigned range
using unsigned_stored_type = typename boost::uint_t<
    significant_bits(Max)
>::least;

///////////////////////////////////////////////////////////////////////////////
// constexpr functions

// need our own version because official version
// a) is not constexpr
// b) is not guarenteed to handle non-assignable types
template<typename T>
constexpr inline std::pair<T, T>
minmax(const std::initializer_list<T> & l){
    assert(l.size() > 0);
    const T * minimum = l.begin();
    const T * maximum = l.begin();
    for(const T * i = l.begin(); i != l.end(); ++i){
        if(*i < * minimum)
            minimum = i;
        else
        if(* maximum < *i)
            maximum = i;
    }
    return std::pair<T, T>{* minimum, * maximum};
}

// for any given t
// a) figure number of significant bits
// b) return a value with all significant bits set
// so for example:
// 3 == round_out(2) because
// 2 == 10 and 3 == 11
template<typename T>
constexpr inline T round_out(const T & t){
    if(t >= 0){
        const std::uint8_t sb = utility::significant_bits(t);
        return (sb < sizeof(T) * 8)
            ? ((T)1 << sb) - 1
            : std::numeric_limits<T>::max();
    }
    else{
        const std::uint8_t sb = utility::significant_bits(~t);
        return (sb < sizeof(T) * 8)
            ? ~(((T)1 << sb) - 1)
            : std::numeric_limits<T>::min();
    }
}

} // utility
} // safe_numerics
} // boost

#endif  // BOOST_NUMERIC_UTILITY_HPP
