/*
Copyright 2019 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_TT_COPY_REFERENCE_HPP_INCLUDED
#define BOOST_TT_COPY_REFERENCE_HPP_INCLUDED

#include <boost/type_traits/add_lvalue_reference.hpp>
#include <boost/type_traits/add_rvalue_reference.hpp>
#include <boost/type_traits/is_lvalue_reference.hpp>
#include <boost/type_traits/is_rvalue_reference.hpp>
#include <boost/type_traits/conditional.hpp>

namespace boost {

template<class T, class U>
struct copy_reference {
    typedef typename conditional<is_rvalue_reference<U>::value,
        typename add_rvalue_reference<T>::type,
        typename conditional<is_lvalue_reference<U>::value,
        typename add_lvalue_reference<T>::type, T>::type>::type type;
};

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)
template<class T, class U>
using copy_reference_t = typename copy_reference<T, U>::type;
#endif

} /* boost */

#endif
