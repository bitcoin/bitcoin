#ifndef BOOST_RANGE_VALUE_HPP
#define BOOST_RANGE_VALUE_HPP

//  Copyright (c) 2015 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iosfwd>

// print range and value to a standard stream
// make a simple wrapper
template<typename T>
struct range_value {
    // type requirement - a numeric type
    const T & m_t;

    constexpr range_value(const T & t)
    :   m_t(t)
    {}

};

template<typename T>
constexpr range_value<T> make_range_value(const T & t){
    return range_value<T>(t);
}

#include "interval.hpp"

template<
    class CharT,
    class Traits,
    class T
>
std::basic_ostream<CharT, Traits> & operator<<(
    std::basic_ostream<CharT, Traits> & os,
    const range_value<T> & t
){
    return os
        << boost::safe_numerics::make_interval<T>()
        << t.m_t;
}

template<typename T>
struct result_display {
    const T & m_t;
    result_display(const T & t) :
        m_t(t)
    {}
};

template<typename T>
inline result_display<T> make_result_display(const T & t){
    return result_display<T>(t);
}

template<typename CharT, typename Traits, typename T>
inline std::basic_ostream<CharT, Traits> &
operator<<(
    std::basic_ostream<CharT, Traits> & os,
    const result_display<T> & r
){
    return os
        << std::hex
        << make_range_value(r.m_t)
        << "(" << std::dec << r.m_t << ")";
}

#endif  // BOOST_RANGE_VALUE_HPP
