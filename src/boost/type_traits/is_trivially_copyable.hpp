/*
Copyright 2020 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_TT_IS_TRIVIALLY_COPYABLE_HPP_INCLUDED
#define BOOST_TT_IS_TRIVIALLY_COPYABLE_HPP_INCLUDED

#include <boost/type_traits/has_trivial_assign.hpp>
#include <boost/type_traits/has_trivial_copy.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/type_traits/has_trivial_move_assign.hpp>
#include <boost/type_traits/has_trivial_move_constructor.hpp>

namespace boost {

template<class T>
struct is_trivially_copyable
    : integral_constant<bool, has_trivial_copy<T>::value &&
        has_trivial_assign<T>::value &&
        has_trivial_move_constructor<T>::value &&
        has_trivial_move_assign<T>::value &&
        has_trivial_destructor<T>::value> { };

} /* boost */

#endif
