// Copyright (C) 2017 Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/optional for documentation.
//
// You are welcome to contact the author at:
//  akrzemi1@gmail.com

#ifndef BOOST_OPTIONAL_DETAIL_EXPERIMENTAL_TRAITS_04NOV2017_HPP
#define BOOST_OPTIONAL_DETAIL_EXPERIMENTAL_TRAITS_04NOV2017_HPP

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/predef.h>
#include <boost/type_traits.hpp>
  
// The condition to use POD implementation

#ifdef BOOST_OPTIONAL_CONFIG_NO_POD_SPEC
# define BOOST_OPTIONAL_DETAIL_NO_SPEC_FOR_TRIVIAL_TYPES
#elif defined BOOST_OPTIONAL_CONFIG_NO_SPEC_FOR_TRIVIAL_TYPES
# define BOOST_OPTIONAL_DETAIL_NO_SPEC_FOR_TRIVIAL_TYPES
#elif !defined BOOST_HAS_TRIVIAL_CONSTRUCTOR
# define BOOST_OPTIONAL_DETAIL_NO_SPEC_FOR_TRIVIAL_TYPES
#elif !defined BOOST_HAS_TRIVIAL_MOVE_ASSIGN
# define BOOST_OPTIONAL_DETAIL_NO_SPEC_FOR_TRIVIAL_TYPES
#elif !defined BOOST_HAS_TRIVIAL_MOVE_CONSTRUCTOR
# define BOOST_OPTIONAL_DETAIL_NO_SPEC_FOR_TRIVIAL_TYPES
#elif !defined BOOST_HAS_TRIVIAL_COPY
# define BOOST_OPTIONAL_DETAIL_NO_SPEC_FOR_TRIVIAL_TYPES
#elif !defined BOOST_HAS_TRIVIAL_ASSIGN
# define BOOST_OPTIONAL_DETAIL_NO_SPEC_FOR_TRIVIAL_TYPES
#elif !defined BOOST_HAS_TRIVIAL_DESTRUCTOR
# define BOOST_OPTIONAL_DETAIL_NO_SPEC_FOR_TRIVIAL_TYPES
#elif BOOST_WORKAROUND(BOOST_GCC, < 50000)
# define BOOST_OPTIONAL_DETAIL_NO_SPEC_FOR_TRIVIAL_TYPES
#endif

// GCC 5 or higher, or clang with libc++ or clang with libstdc++ 5 or higher
#if __cplusplus >= 201103L
#  if BOOST_WORKAROUND(BOOST_GCC, >= 50000)
#    define BOOST_OPTIONAL_DETAIL_USE_STD_TYPE_TRAITS
#  elif (defined BOOST_CLANG)
#    if BOOST_LIB_STD_CXX > 0
#      define BOOST_OPTIONAL_DETAIL_USE_STD_TYPE_TRAITS
#    elif BOOST_LIB_STD_GNU >= 441200023 && BOOST_LIB_STD_GNU != 450600023 && BOOST_LIB_STD_GNU != 450600026 && BOOST_LIB_STD_GNU != 460800003 && BOOST_LIB_STD_GNU != 450400026 && BOOST_LIB_STD_GNU != 460700026
#      define BOOST_OPTIONAL_DETAIL_USE_STD_TYPE_TRAITS
#    endif
#  endif
#endif  


#ifndef BOOST_OPTIONAL_DETAIL_USE_STD_TYPE_TRAITS
#  define BOOST_OPTIONAL_DETAIL_HAS_TRIVIAL_CTOR(T) BOOST_HAS_TRIVIAL_CONSTRUCTOR(T)
#else
#  include <type_traits>
#  define BOOST_OPTIONAL_DETAIL_HAS_TRIVIAL_CTOR(T) std::is_trivially_default_constructible<T>::value
#endif
  
  
namespace boost { namespace optional_detail {
  
#ifndef BOOST_OPTIONAL_DETAIL_NO_SPEC_FOR_TRIVIAL_TYPES
template <typename T>
struct is_type_trivially_copyable
  : boost::conditional<(boost::has_trivial_copy_constructor<T>::value &&
                        boost::has_trivial_move_constructor<T>::value &&
                        boost::has_trivial_destructor<T>::value &&
                        boost::has_trivial_move_assign<T>::value &&
                        boost::has_trivial_assign<T>::value),
                        boost::true_type, boost::false_type>::type
{};
#else
template <typename T>
struct is_type_trivially_copyable
: boost::conditional<(boost::is_scalar<T>::value && !boost::is_const<T>::value && !boost::is_volatile<T>::value),
                     boost::true_type, boost::false_type>::type
{};
#endif



#ifndef BOOST_OPTIONAL_DETAIL_NO_SPEC_FOR_TRIVIAL_TYPES  
template <typename T>
struct optional_uses_direct_storage_for_
  : boost::conditional< (is_type_trivially_copyable<T>::value && BOOST_OPTIONAL_DETAIL_HAS_TRIVIAL_CTOR(T)) ||
                        (boost::is_scalar<T>::value && !boost::is_const<T>::value && !boost::is_volatile<T>::value)
                      , boost::true_type, boost::false_type>::type
{};
#else
template <typename T>
struct optional_uses_direct_storage_for_
  : boost::conditional<(boost::is_scalar<T>::value && !boost::is_const<T>::value && !boost::is_volatile<T>::value)
                      , boost::true_type, boost::false_type>::type
{};
#endif


}} // boost::optional_detail

#endif
