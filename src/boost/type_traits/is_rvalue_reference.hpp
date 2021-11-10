
//  (C) Copyright John Maddock 2010. 
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_IS_RVALUE_REFERENCE_HPP_INCLUDED
#define BOOST_TT_IS_RVALUE_REFERENCE_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/type_traits/integral_constant.hpp>

namespace boost {

template <class T> struct is_rvalue_reference : public false_type {};
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
template <class T> struct is_rvalue_reference<T&&> : public true_type {};
#endif

} // namespace boost

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && defined(BOOST_MSVC) && BOOST_WORKAROUND(BOOST_MSVC, <= 1700)
#include <boost/type_traits/detail/is_rvalue_reference_msvc10_fix.hpp>
#endif

#endif // BOOST_TT_IS_REFERENCE_HPP_INCLUDED

