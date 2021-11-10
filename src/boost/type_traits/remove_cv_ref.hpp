
//  (C) Copyright Peter Dimov 2017. 
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.


#ifndef BOOST_TT_REMOVE_CV_REF_HPP_INCLUDED
#define BOOST_TT_REMOVE_CV_REF_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace boost {

   template <class T> struct remove_cv_ref: remove_cv<typename remove_reference<T>::type> {};


#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

   template <class T> using remove_cv_ref_t = typename remove_cv_ref<T>::type;

#endif

} // namespace boost

#endif // BOOST_TT_REMOVE_CV_REF_HPP_INCLUDED
