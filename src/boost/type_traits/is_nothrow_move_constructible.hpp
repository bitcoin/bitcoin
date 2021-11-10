
//  (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  (C) Copyright Eric Friedman 2002-2003.
//  (C) Copyright Antony Polukhin 2013.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_IS_NOTHROW_MOVE_CONSTRUCTIBLE_HPP_INCLUDED
#define BOOST_TT_IS_NOTHROW_MOVE_CONSTRUCTIBLE_HPP_INCLUDED

#include <cstddef> // size_t
#include <boost/config.hpp>
#include <boost/type_traits/intrinsics.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/type_traits/is_complete.hpp>
#include <boost/static_assert.hpp>

#ifdef BOOST_IS_NOTHROW_MOVE_CONSTRUCT

namespace boost {

template <class T>
struct is_nothrow_move_constructible : public integral_constant<bool, BOOST_IS_NOTHROW_MOVE_CONSTRUCT(T)>
{
   BOOST_STATIC_ASSERT_MSG(boost::is_complete<T>::value, "Arguments to is_nothrow_move_constructible must be complete types");
};

template <class T> struct is_nothrow_move_constructible<volatile T> : public ::boost::false_type {};
template <class T> struct is_nothrow_move_constructible<const volatile T> : public ::boost::false_type{};

#elif !defined(BOOST_NO_CXX11_NOEXCEPT) && !defined(BOOST_NO_SFINAE_EXPR) && !BOOST_WORKAROUND(BOOST_GCC_VERSION, < 40700)

#include <boost/type_traits/declval.hpp>
#include <boost/type_traits/enable_if.hpp>

namespace boost{ namespace detail{

template <class T, class Enable = void>
struct false_or_cpp11_noexcept_move_constructible: public ::boost::false_type {};

template <class T>
struct false_or_cpp11_noexcept_move_constructible <
        T,
        typename ::boost::enable_if_<sizeof(T) && BOOST_NOEXCEPT_EXPR(T(::boost::declval<T>()))>::type
    > : public ::boost::integral_constant<bool, BOOST_NOEXCEPT_EXPR(T(::boost::declval<T>()))>
{};

}

template <class T> struct is_nothrow_move_constructible
   : public integral_constant<bool, ::boost::detail::false_or_cpp11_noexcept_move_constructible<T>::value>
{
   BOOST_STATIC_ASSERT_MSG(boost::is_complete<T>::value, "Arguments to is_nothrow_move_constructible must be complete types");
};

template <class T> struct is_nothrow_move_constructible<volatile T> : public ::boost::false_type {};
template <class T> struct is_nothrow_move_constructible<const volatile T> : public ::boost::false_type{};
template <class T, std::size_t N> struct is_nothrow_move_constructible<T[N]> : public ::boost::false_type{};
template <class T> struct is_nothrow_move_constructible<T[]> : public ::boost::false_type{};

#else

#include <boost/type_traits/has_trivial_move_constructor.hpp>
#include <boost/type_traits/has_nothrow_copy.hpp>
#include <boost/type_traits/is_array.hpp>

namespace boost{

template <class T>
struct is_nothrow_move_constructible
   : public integral_constant<bool,
   (::boost::has_trivial_move_constructor<T>::value || ::boost::has_nothrow_copy<T>::value) && !::boost::is_array<T>::value>
{
   BOOST_STATIC_ASSERT_MSG(boost::is_complete<T>::value, "Arguments to is_nothrow_move_constructible must be complete types");
};

#endif

template <> struct is_nothrow_move_constructible<void> : false_type{};
#ifndef BOOST_NO_CV_VOID_SPECIALIZATIONS
template <> struct is_nothrow_move_constructible<void const> : false_type{};
template <> struct is_nothrow_move_constructible<void volatile> : false_type{};
template <> struct is_nothrow_move_constructible<void const volatile> : false_type{};
#endif
// References are always trivially constructible, even if the thing they reference is not:
template <class T> struct is_nothrow_move_constructible<T&> : public ::boost::true_type{};
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
template <class T> struct is_nothrow_move_constructible<T&&> : public ::boost::true_type{};
#endif

} // namespace boost

#endif // BOOST_TT_IS_NOTHROW_MOVE_CONSTRUCTIBLE_HPP_INCLUDED
