/*
Copyright 2017-2018 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_TT_IS_DETECTED_EXACT_HPP_INCLUDED
#define BOOST_TT_IS_DETECTED_EXACT_HPP_INCLUDED

#include <boost/type_traits/detected.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost {

template<class Expected, template<class...> class Op, class... Args>
using is_detected_exact = is_same<Expected, detected_t<Op, Args...> >;

#if !defined(BOOST_NO_CXX14_VARIABLE_TEMPLATES)
template<class Expected, template<class...> class Op, class... Args>
constexpr bool is_detected_exact_v = is_detected_exact<Expected, Op,
    Args...>::value;
#endif

} /* boost */

#endif
