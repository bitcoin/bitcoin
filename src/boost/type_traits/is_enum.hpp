
//  (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, Howard
//  Hinnant & John Maddock 2000.  
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.


#ifndef BOOST_TT_IS_ENUM_HPP_INCLUDED
#define BOOST_TT_IS_ENUM_HPP_INCLUDED

#include <boost/type_traits/intrinsics.hpp>
#include <boost/type_traits/integral_constant.hpp>
#ifndef BOOST_IS_ENUM
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_array.hpp>
#ifdef __GNUC__
#include <boost/type_traits/is_function.hpp>
#endif
#include <boost/type_traits/detail/config.hpp>
#if defined(BOOST_TT_HAS_CONFORMING_IS_CLASS_IMPLEMENTATION) 
#  include <boost/type_traits/is_class.hpp>
#  include <boost/type_traits/is_union.hpp>
#endif
#endif

namespace boost {

#ifndef BOOST_IS_ENUM
#if !(defined(BOOST_BORLANDC) && (BOOST_BORLANDC <= 0x551))

namespace detail {

#if defined(BOOST_TT_HAS_CONFORMING_IS_CLASS_IMPLEMENTATION) 

template <typename T>
struct is_class_or_union
{
   BOOST_STATIC_CONSTANT(bool, value = ::boost::is_class<T>::value || ::boost::is_union<T>::value);
};

#else

template <typename T>
struct is_class_or_union
{
# if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x581))// we simply can't detect it this way.
    BOOST_STATIC_CONSTANT(bool, value = false);
# else
    template <class U> static ::boost::type_traits::yes_type is_class_or_union_tester(void(U::*)(void));

#  if BOOST_WORKAROUND(__MWERKS__, <= 0x3000) // no SFINAE
    static ::boost::type_traits::no_type is_class_or_union_tester(...);
    BOOST_STATIC_CONSTANT(
        bool, value = sizeof(is_class_or_union_tester(0)) == sizeof(::boost::type_traits::yes_type));
#  else
    template <class U>
    static ::boost::type_traits::no_type is_class_or_union_tester(...);
    BOOST_STATIC_CONSTANT(
        bool, value = sizeof(is_class_or_union_tester<T>(0)) == sizeof(::boost::type_traits::yes_type));
#  endif
# endif
};
#endif

struct int_convertible
{
    int_convertible(int);
};

// Don't evaluate convertibility to int_convertible unless the type
// is non-arithmetic. This suppresses warnings with GCC.
template <bool is_typename_arithmetic_or_reference = true>
struct is_enum_helper
{
    template <typename T> struct type
    {
        BOOST_STATIC_CONSTANT(bool, value = false);
    };
};

template <>
struct is_enum_helper<false>
{
    template <typename T> struct type
    {
       static const bool value = ::boost::is_convertible<typename boost::add_reference<T>::type, ::boost::detail::int_convertible>::value;
    };
};

template <typename T> struct is_enum_impl
{
   //typedef ::boost::add_reference<T> ar_t;
   //typedef typename ar_t::type r_type;

#if defined(__GNUC__)

#ifdef BOOST_TT_HAS_CONFORMING_IS_CLASS_IMPLEMENTATION
    
   // We MUST check for is_class_or_union on conforming compilers in
   // order to correctly deduce that noncopyable types are not enums
   // (dwa 2002/04/15)...
   BOOST_STATIC_CONSTANT(bool, selector =
           ::boost::is_arithmetic<T>::value
         || ::boost::is_reference<T>::value
         || ::boost::is_function<T>::value
         || is_class_or_union<T>::value
         || is_array<T>::value);
#else
   // ...however, not checking is_class_or_union on non-conforming
   // compilers prevents a dependency recursion.
   BOOST_STATIC_CONSTANT(bool, selector =
           ::boost::is_arithmetic<T>::value
         || ::boost::is_reference<T>::value
         || ::boost::is_function<T>::value
         || is_array<T>::value);
#endif // BOOST_TT_HAS_CONFORMING_IS_CLASS_IMPLEMENTATION

#else // !defined(__GNUC__):
    
   BOOST_STATIC_CONSTANT(bool, selector =
           ::boost::is_arithmetic<T>::value
         || ::boost::is_reference<T>::value
         || is_class_or_union<T>::value
         || is_array<T>::value);
    
#endif

#if BOOST_WORKAROUND(BOOST_BORLANDC, < 0x600)
    typedef ::boost::detail::is_enum_helper<
          ::boost::detail::is_enum_impl<T>::selector
        > se_t;
#else
    typedef ::boost::detail::is_enum_helper<selector> se_t;
#endif

    typedef typename se_t::template type<T> helper;
    BOOST_STATIC_CONSTANT(bool, value = helper::value);
};

} // namespace detail

template <class T> struct is_enum : public integral_constant<bool, ::boost::detail::is_enum_impl<T>::value> {};

#else // BOOST_BORLANDC
//
// buggy is_convertible prevents working
// implementation of is_enum:
template <class T> struct is_enum : public integral_constant<bool, false> {};

#endif

#else // BOOST_IS_ENUM

template <class T> struct is_enum : public integral_constant<bool, BOOST_IS_ENUM(T)> {};

#endif

} // namespace boost

#endif // BOOST_TT_IS_ENUM_HPP_INCLUDED
