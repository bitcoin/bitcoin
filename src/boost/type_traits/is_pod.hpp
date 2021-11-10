
//  (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_IS_POD_HPP_INCLUDED
#define BOOST_TT_IS_POD_HPP_INCLUDED

#include <cstddef> // size_t
#include <boost/type_traits/detail/config.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/type_traits/is_scalar.hpp>
#include <boost/type_traits/intrinsics.hpp>

#ifdef __SUNPRO_CC
#include <boost/type_traits/is_function.hpp>
#endif

#include <cstddef>

#ifndef BOOST_IS_POD
#define BOOST_INTERNAL_IS_POD(T) false
#else
#define BOOST_INTERNAL_IS_POD(T) BOOST_IS_POD(T)
#endif

namespace boost {

// forward declaration, needed by 'is_pod_array_helper' template below
template< typename T > struct is_POD;

template <typename T> struct is_pod
: public integral_constant<bool, ::boost::is_scalar<T>::value || ::boost::is_void<T>::value || BOOST_INTERNAL_IS_POD(T)>
{};

#if !defined(BOOST_NO_ARRAY_TYPE_SPECIALIZATIONS)
template <typename T, std::size_t sz> struct is_pod<T[sz]> : public is_pod<T>{};
#endif


// the following help compilers without partial specialization support:
template<> struct is_pod<void> : public true_type{};

#ifndef BOOST_NO_CV_VOID_SPECIALIZATIONS
template<> struct is_pod<void const> : public true_type{};
template<> struct is_pod<void const volatile> : public true_type{};
template<> struct is_pod<void volatile> : public true_type{};
#endif

template<class T> struct is_POD : public is_pod<T>{};

} // namespace boost

#undef BOOST_INTERNAL_IS_POD

#endif // BOOST_TT_IS_POD_HPP_INCLUDED
