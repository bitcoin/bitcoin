// Copyright (c) 2016-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_FUNCTIONS_FOR_HPP
#define BOOST_PFR_FUNCTIONS_FOR_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <boost/pfr/ops_fields.hpp>
#include <boost/pfr/io_fields.hpp>

/// \file boost/pfr/functions_for.hpp
/// Contains BOOST_PFR_FUNCTIONS_FOR macro that defined comparison and stream operators for T along with hash_value function.
/// \b Example:
/// \code
///     #include <boost/pfr/functions_for.hpp>
///
///     namespace my_namespace {
///         struct my_struct {      // No operators defined for that structure
///             int i; short s; char data[7]; bool bl; int a,b,c,d,e,f;
///         };
///         BOOST_PFR_FUNCTIONS_FOR(my_struct)
///     }
/// \endcode
///
/// \podops for other ways to define operators and more details.
///
/// \b Synopsis:

/// \def BOOST_PFR_FUNCTIONS_FOR(T)
/// Defines comparison and stream operators for T along with hash_value function.
///
/// \b Example:
/// \code
///     #include <boost/pfr/functions_for.hpp>
///     struct comparable_struct {      // No operators defined for that structure
///         int i; short s; char data[7]; bool bl; int a,b,c,d,e,f;
///     };
///     BOOST_PFR_FUNCTIONS_FOR(comparable_struct)
///     // ...
///
///     comparable_struct s1 {0, 1, "Hello", false, 6,7,8,9,10,11};
///     comparable_struct s2 {0, 1, "Hello", false, 6,7,8,9,10,11111};
///     assert(s1 < s2);
///     std::cout << s1 << std::endl; // Outputs: {0, 1, H, e, l, l, o, , , 0, 6, 7, 8, 9, 10, 11}
/// \endcode
///
/// \podops for other ways to define operators and more details.
///
/// \b Defines \b following \b for \b T:
/// \code
/// bool operator==(const T& lhs, const T& rhs);
/// bool operator!=(const T& lhs, const T& rhs);
/// bool operator< (const T& lhs, const T& rhs);
/// bool operator> (const T& lhs, const T& rhs);
/// bool operator<=(const T& lhs, const T& rhs);
/// bool operator>=(const T& lhs, const T& rhs);
///
/// template <class Char, class Traits>
/// std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, const T& value);
///
/// template <class Char, class Traits>
/// std::basic_istream<Char, Traits>& operator>>(std::basic_istream<Char, Traits>& in, T& value);
///
/// // helper function for Boost unordered containers and boost::hash<>.
/// std::size_t hash_value(const T& value);
/// \endcode

#define BOOST_PFR_FUNCTIONS_FOR(T)                                                                                                          \
    BOOST_PFR_MAYBE_UNUSED inline bool operator==(const T& lhs, const T& rhs) { return ::boost::pfr::eq_fields(lhs, rhs); }                 \
    BOOST_PFR_MAYBE_UNUSED inline bool operator!=(const T& lhs, const T& rhs) { return ::boost::pfr::ne_fields(lhs, rhs); }                 \
    BOOST_PFR_MAYBE_UNUSED inline bool operator< (const T& lhs, const T& rhs) { return ::boost::pfr::lt_fields(lhs, rhs); }                 \
    BOOST_PFR_MAYBE_UNUSED inline bool operator> (const T& lhs, const T& rhs) { return ::boost::pfr::gt_fields(lhs, rhs); }                 \
    BOOST_PFR_MAYBE_UNUSED inline bool operator<=(const T& lhs, const T& rhs) { return ::boost::pfr::le_fields(lhs, rhs); }                 \
    BOOST_PFR_MAYBE_UNUSED inline bool operator>=(const T& lhs, const T& rhs) { return ::boost::pfr::ge_fields(lhs, rhs); }                 \
    template <class Char, class Traits>                                                                                                     \
    BOOST_PFR_MAYBE_UNUSED inline ::std::basic_ostream<Char, Traits>& operator<<(::std::basic_ostream<Char, Traits>& out, const T& value) { \
        return out << ::boost::pfr::io_fields(value);                                                                                       \
    }                                                                                                                                       \
    template <class Char, class Traits>                                                                                                     \
    BOOST_PFR_MAYBE_UNUSED inline ::std::basic_istream<Char, Traits>& operator>>(::std::basic_istream<Char, Traits>& in, T& value) {        \
        return in >> ::boost::pfr::io_fields(value);                                                                                        \
    }                                                                                                                                       \
    BOOST_PFR_MAYBE_UNUSED inline std::size_t hash_value(const T& v) {                                                                      \
        return ::boost::pfr::hash_fields(v);                                                                                                \
    }                                                                                                                                       \
/**/

#endif // BOOST_PFR_FUNCTIONS_FOR_HPP

