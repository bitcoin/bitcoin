
// (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_IS_EMPTY_HPP_INCLUDED
#define BOOST_TT_IS_EMPTY_HPP_INCLUDED

#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/detail/config.hpp>
#include <boost/type_traits/intrinsics.hpp>

#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/add_reference.hpp>

#ifndef BOOST_INTERNAL_IS_EMPTY
#define BOOST_INTERNAL_IS_EMPTY(T) false
#else
#define BOOST_INTERNAL_IS_EMPTY(T) BOOST_IS_EMPTY(T)
#endif

namespace boost {

namespace detail {


#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4624) // destructor could not be generated
#endif

template <typename T>
struct empty_helper_t1 : public T
{
    empty_helper_t1();  // hh compiler bug workaround
    int i[256];
private:
   // suppress compiler warnings:
   empty_helper_t1(const empty_helper_t1&);
   empty_helper_t1& operator=(const empty_helper_t1&);
};

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

struct empty_helper_t2 { int i[256]; };

#if !BOOST_WORKAROUND(BOOST_BORLANDC, < 0x600)

template <typename T, bool is_a_class = false>
struct empty_helper
{
    BOOST_STATIC_CONSTANT(bool, value = false);
};

template <typename T>
struct empty_helper<T, true>
{
    BOOST_STATIC_CONSTANT(
        bool, value = (sizeof(empty_helper_t1<T>) == sizeof(empty_helper_t2))
        );
};

template <typename T>
struct is_empty_impl
{
    typedef typename remove_cv<T>::type cvt;
    BOOST_STATIC_CONSTANT(
        bool, 
        value = ( ::boost::detail::empty_helper<cvt,::boost::is_class<T>::value>::value || BOOST_INTERNAL_IS_EMPTY(cvt)));
};

#else // BOOST_BORLANDC

template <typename T, bool is_a_class, bool convertible_to_int>
struct empty_helper
{
    BOOST_STATIC_CONSTANT(bool, value = false);
};

template <typename T>
struct empty_helper<T, true, false>
{
    BOOST_STATIC_CONSTANT(bool, value = (
        sizeof(empty_helper_t1<T>) == sizeof(empty_helper_t2)
        ));
};

template <typename T>
struct is_empty_impl
{
   typedef typename remove_cv<T>::type cvt;
   typedef typename add_reference<T>::type r_type;

   BOOST_STATIC_CONSTANT(
       bool, value = (
              ::boost::detail::empty_helper<
                  cvt
                , ::boost::is_class<T>::value
                , ::boost::is_convertible< r_type,int>::value
              >::value || BOOST_INTERNAL_IS_EMPTY(cvt)));
};

#endif // BOOST_BORLANDC

} // namespace detail

template <class T> struct is_empty : integral_constant<bool, ::boost::detail::is_empty_impl<T>::value> {};

} // namespace boost

#undef BOOST_INTERNAL_IS_EMPTY

#endif // BOOST_TT_IS_EMPTY_HPP_INCLUDED

