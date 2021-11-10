
//  (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_REMOVE_REFERENCE_HPP_INCLUDED
#define BOOST_TT_REMOVE_REFERENCE_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

namespace boost {


namespace detail{
//
// We can't filter out rvalue_references at the same level as
// references or we get ambiguities from msvc:
//
template <class T>
struct remove_rvalue_ref
{
   typedef T type;
};
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
template <class T>
struct remove_rvalue_ref<T&&>
{
   typedef T type;
};
#endif

} // namespace detail

template <class T> struct remove_reference{ typedef typename boost::detail::remove_rvalue_ref<T>::type type; };
template <class T> struct remove_reference<T&>{ typedef T type; };

#if defined(BOOST_ILLEGAL_CV_REFERENCES)
// these are illegal specialisations; cv-qualifies applied to
// references have no effect according to [8.3.2p1],
// C++ Builder requires them though as it treats cv-qualified
// references as distinct types...
template <class T> struct remove_reference<T&const>{ typedef T type; };
template <class T> struct remove_reference<T&volatile>{ typedef T type; };
template <class T> struct remove_reference<T&const volatile>{ typedef T type; };
#endif

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

   template <class T> using remove_reference_t = typename remove_reference<T>::type;

#endif

} // namespace boost

#endif // BOOST_TT_REMOVE_REFERENCE_HPP_INCLUDED
