/*
Copyright 2017-2018 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_TT_IS_DETECTED_CONVERTIBLE_HPP_INCLUDED
#define BOOST_TT_IS_DETECTED_CONVERTIBLE_HPP_INCLUDED

#include <boost/type_traits/detected.hpp>
#include <boost/type_traits/is_convertible.hpp>

namespace boost {

template<class To, template<class...> class Op, class... Args>
using is_detected_convertible = is_convertible<detected_t<Op, Args...>, To>;

#if !defined(BOOST_NO_CXX14_VARIABLE_TEMPLATES)
template<class To, template<class...> class Op, class... Args>
constexpr bool is_detected_convertible_v = is_detected_convertible<To, Op,
    Args...>::value;
#endif

} /* boost */

#endif
