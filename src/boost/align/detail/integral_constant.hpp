/*
Copyright 2014-2016 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_DETAIL_INTEGRAL_CONSTANT_HPP
#define BOOST_ALIGN_DETAIL_INTEGRAL_CONSTANT_HPP

#include <boost/config.hpp>

#if !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)
#include <type_traits>
#endif

namespace boost {
namespace alignment {
namespace detail {

#if !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)
using std::integral_constant;
using std::true_type;
using std::false_type;
#else
template<class T, T Value>
struct integral_constant {
    typedef T value_type;
    typedef integral_constant type;

    BOOST_CONSTEXPR operator value_type() const BOOST_NOEXCEPT {
        return Value;
    }

    BOOST_CONSTEXPR value_type operator()() const BOOST_NOEXCEPT {
        return Value;
    }

    BOOST_STATIC_CONSTEXPR T value = Value;
};

template<class T, T Value>
BOOST_CONSTEXPR_OR_CONST T integral_constant<T, Value>::value;

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;
#endif

} /* detail */
} /* alignment */
} /* boost */

#endif
