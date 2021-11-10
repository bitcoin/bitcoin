//
// traits/submit_member.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_TRAITS_SUBMIT_MEMBER_HPP
#define BOOST_ASIO_TRAITS_SUBMIT_MEMBER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>

#if defined(BOOST_ASIO_HAS_DECLTYPE) \
  && defined(BOOST_ASIO_HAS_NOEXCEPT) \
  && defined(BOOST_ASIO_HAS_WORKING_EXPRESSION_SFINAE)
# define BOOST_ASIO_HAS_DEDUCED_SUBMIT_MEMBER_TRAIT 1
#endif // defined(BOOST_ASIO_HAS_DECLTYPE)
       //   && defined(BOOST_ASIO_HAS_NOEXCEPT)
       //   && defined(BOOST_ASIO_HAS_WORKING_EXPRESSION_SFINAE)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace traits {

template <typename S, typename R, typename = void>
struct submit_member_default;

template <typename S, typename R, typename = void>
struct submit_member;

} // namespace traits
namespace detail {

struct no_submit_member
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = false);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
};

#if defined(BOOST_ASIO_HAS_DEDUCED_SUBMIT_MEMBER_TRAIT)

template <typename S, typename R, typename = void>
struct submit_member_trait : no_submit_member
{
};

template <typename S, typename R>
struct submit_member_trait<S, R,
  typename void_type<
    decltype(declval<S>().submit(declval<R>()))
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  using result_type = decltype(
    declval<S>().submit(declval<R>()));

  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = noexcept(
    declval<S>().submit(declval<R>())));
};

#else // defined(BOOST_ASIO_HAS_DEDUCED_SUBMIT_MEMBER_TRAIT)

template <typename S, typename R, typename = void>
struct submit_member_trait :
  conditional<
    is_same<S, typename remove_reference<S>::type>::value
      && is_same<R, typename decay<R>::type>::value,
    typename conditional<
      is_same<S, typename add_const<S>::type>::value,
      no_submit_member,
      traits::submit_member<typename add_const<S>::type, R>
    >::type,
    traits::submit_member<
      typename remove_reference<S>::type,
      typename decay<R>::type>
  >::type
{
};

#endif // defined(BOOST_ASIO_HAS_DEDUCED_SUBMIT_MEMBER_TRAIT)

} // namespace detail
namespace traits {

template <typename S, typename R, typename>
struct submit_member_default :
  detail::submit_member_trait<S, R>
{
};

template <typename S, typename R, typename>
struct submit_member :
  submit_member_default<S, R>
{
};

} // namespace traits
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_TRAITS_SUBMIT_MEMBER_HPP
