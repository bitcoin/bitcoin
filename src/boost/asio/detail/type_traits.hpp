//
// detail/type_traits.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_TYPE_TRAITS_HPP
#define BOOST_ASIO_DETAIL_TYPE_TRAITS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_STD_TYPE_TRAITS)
# include <type_traits>
#else // defined(BOOST_ASIO_HAS_STD_TYPE_TRAITS)
# include <boost/type_traits/add_const.hpp>
# include <boost/type_traits/add_lvalue_reference.hpp>
# include <boost/type_traits/aligned_storage.hpp>
# include <boost/type_traits/alignment_of.hpp>
# include <boost/type_traits/conditional.hpp>
# include <boost/type_traits/decay.hpp>
# include <boost/type_traits/has_nothrow_copy.hpp>
# include <boost/type_traits/has_nothrow_destructor.hpp>
# include <boost/type_traits/integral_constant.hpp>
# include <boost/type_traits/is_base_of.hpp>
# include <boost/type_traits/is_class.hpp>
# include <boost/type_traits/is_const.hpp>
# include <boost/type_traits/is_convertible.hpp>
# include <boost/type_traits/is_constructible.hpp>
# include <boost/type_traits/is_copy_constructible.hpp>
# include <boost/type_traits/is_destructible.hpp>
# include <boost/type_traits/is_function.hpp>
# include <boost/type_traits/is_object.hpp>
# include <boost/type_traits/is_same.hpp>
# include <boost/type_traits/remove_cv.hpp>
# include <boost/type_traits/remove_pointer.hpp>
# include <boost/type_traits/remove_reference.hpp>
# include <boost/utility/declval.hpp>
# include <boost/utility/enable_if.hpp>
# include <boost/utility/result_of.hpp>
#endif // defined(BOOST_ASIO_HAS_STD_TYPE_TRAITS)

namespace boost {
namespace asio {

#if defined(BOOST_ASIO_HAS_STD_TYPE_TRAITS)
using std::add_const;
using std::add_lvalue_reference;
using std::aligned_storage;
using std::alignment_of;
using std::conditional;
using std::decay;
using std::declval;
using std::enable_if;
using std::false_type;
using std::integral_constant;
using std::is_base_of;
using std::is_class;
using std::is_const;
using std::is_constructible;
using std::is_convertible;
using std::is_copy_constructible;
using std::is_destructible;
using std::is_function;
using std::is_move_constructible;
using std::is_nothrow_copy_constructible;
using std::is_nothrow_destructible;
using std::is_object;
using std::is_reference;
using std::is_same;
using std::is_scalar;
using std::remove_cv;
template <typename T>
struct remove_cvref : remove_cv<typename std::remove_reference<T>::type> {};
using std::remove_pointer;
using std::remove_reference;
#if defined(BOOST_ASIO_HAS_STD_INVOKE_RESULT)
template <typename> struct result_of;
template <typename F, typename... Args>
struct result_of<F(Args...)> : std::invoke_result<F, Args...> {};
#else // defined(BOOST_ASIO_HAS_STD_INVOKE_RESULT)
using std::result_of;
#endif // defined(BOOST_ASIO_HAS_STD_INVOKE_RESULT)
using std::true_type;
#else // defined(BOOST_ASIO_HAS_STD_TYPE_TRAITS)
using boost::add_const;
using boost::add_lvalue_reference;
using boost::aligned_storage;
using boost::alignment_of;
template <bool Condition, typename Type = void>
struct enable_if : boost::enable_if_c<Condition, Type> {};
using boost::conditional;
using boost::decay;
using boost::declval;
using boost::false_type;
using boost::integral_constant;
using boost::is_base_of;
using boost::is_class;
using boost::is_const;
using boost::is_constructible;
using boost::is_convertible;
using boost::is_copy_constructible;
using boost::is_destructible;
using boost::is_function;
#if defined(BOOST_ASIO_HAS_MOVE)
template <typename T>
struct is_move_constructible : false_type {};
#else // defined(BOOST_ASIO_HAS_MOVE)
template <typename T>
struct is_move_constructible : is_copy_constructible<T> {};
#endif // defined(BOOST_ASIO_HAS_MOVE)
template <typename T>
struct is_nothrow_copy_constructible : boost::has_nothrow_copy<T> {};
template <typename T>
struct is_nothrow_destructible : boost::has_nothrow_destructor<T> {};
using boost::is_object;
using boost::is_reference;
using boost::is_same;
using boost::is_scalar;
using boost::remove_cv;
template <typename T>
struct remove_cvref : remove_cv<typename boost::remove_reference<T>::type> {};
using boost::remove_pointer;
using boost::remove_reference;
using boost::result_of;
using boost::true_type;
#endif // defined(BOOST_ASIO_HAS_STD_TYPE_TRAITS)

template <typename> struct void_type { typedef void type; };

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename...> struct conjunction : true_type {};
template <typename T> struct conjunction<T> : T {};
template <typename Head, typename... Tail> struct conjunction<Head, Tail...> :
  conditional<Head::value, conjunction<Tail...>, Head>::type {};

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

struct defaulted_constraint
{
  BOOST_ASIO_CONSTEXPR defaulted_constraint() {}
};

template <bool Condition, typename Type = int>
struct constraint : enable_if<Condition, Type> {};

} // namespace asio
} // namespace boost

#endif // BOOST_ASIO_DETAIL_TYPE_TRAITS_HPP
