//
// traits/set_error_member.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_TRAITS_SET_ERROR_MEMBER_HPP
#define BOOST_ASIO_TRAITS_SET_ERROR_MEMBER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>

#if defined(BOOST_ASIO_HAS_DECLTYPE) \
  && defined(BOOST_ASIO_HAS_NOEXCEPT) \
  && defined(BOOST_ASIO_HAS_WORKING_EXPRESSION_SFINAE)
# define BOOST_ASIO_HAS_DEDUCED_SET_ERROR_MEMBER_TRAIT 1
#endif // defined(BOOST_ASIO_HAS_DECLTYPE)
       //   && defined(BOOST_ASIO_HAS_NOEXCEPT)
       //   && defined(BOOST_ASIO_HAS_WORKING_EXPRESSION_SFINAE)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace traits {

template <typename T, typename E, typename = void>
struct set_error_member_default;

template <typename T, typename E, typename = void>
struct set_error_member;

} // namespace traits
namespace detail {

struct no_set_error_member
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = false);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
};

#if defined(BOOST_ASIO_HAS_DEDUCED_SET_ERROR_MEMBER_TRAIT)

template <typename T, typename E, typename = void>
struct set_error_member_trait : no_set_error_member
{
};

template <typename T, typename E>
struct set_error_member_trait<T, E,
  typename void_type<
    decltype(declval<T>().set_error(declval<E>()))
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  using result_type = decltype(
    declval<T>().set_error(declval<E>()));

  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = noexcept(
    declval<T>().set_error(declval<E>())));
};

#else // defined(BOOST_ASIO_HAS_DEDUCED_SET_ERROR_MEMBER_TRAIT)

template <typename T, typename E, typename = void>
struct set_error_member_trait :
  conditional<
    is_same<T, typename remove_reference<T>::type>::value
      && is_same<E, typename decay<E>::type>::value,
    typename conditional<
      is_same<T, typename add_const<T>::type>::value,
      no_set_error_member,
      traits::set_error_member<typename add_const<T>::type, E>
    >::type,
    traits::set_error_member<
      typename remove_reference<T>::type,
      typename decay<E>::type>
  >::type
{
};

#endif // defined(BOOST_ASIO_HAS_DEDUCED_SET_ERROR_MEMBER_TRAIT)

} // namespace detail
namespace traits {

template <typename T, typename E, typename>
struct set_error_member_default :
  detail::set_error_member_trait<T, E>
{
};

template <typename T, typename E, typename>
struct set_error_member :
  set_error_member_default<T, E>
{
};

} // namespace traits
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_TRAITS_SET_ERROR_MEMBER_HPP
