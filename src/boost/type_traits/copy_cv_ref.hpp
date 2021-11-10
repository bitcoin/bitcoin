/*
Copyright 2019 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_TT_COPY_CV_REF_HPP_INCLUDED
#define BOOST_TT_COPY_CV_REF_HPP_INCLUDED

#include <boost/type_traits/copy_cv.hpp>
#include <boost/type_traits/copy_reference.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace boost {

template<class T, class U>
struct copy_cv_ref {
    typedef typename copy_reference<typename copy_cv<T,
        typename remove_reference<U>::type >::type, U>::type type;
};

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)
template<class T, class U>
using copy_cv_ref_t = typename copy_cv_ref<T, U>::type;
#endif

} /* boost */

#endif
