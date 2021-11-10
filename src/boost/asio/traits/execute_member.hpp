//
// traits/execute_member.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_TRAITS_EXECUTE_MEMBER_HPP
#define BOOST_ASIO_TRAITS_EXECUTE_MEMBER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>

#if defined(BOOST_ASIO_HAS_DECLTYPE) \
  && defined(BOOST_ASIO_HAS_NOEXCEPT) \
  && defined(BOOST_ASIO_HAS_WORKING_EXPRESSION_SFINAE)
# define BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT 1
#endif // defined(BOOST_ASIO_HAS_DECLTYPE)
       //   && defined(BOOST_ASIO_HAS_NOEXCEPT)
       //   && defined(BOOST_ASIO_HAS_WORKING_EXPRESSION_SFINAE)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace traits {

template <typename T, typename F, typename = void>
struct execute_member_default;

template <typename T, typename F, typename = void>
struct execute_member;

} // namespace traits
namespace detail {

struct no_execute_member
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = false);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
};

#if defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

template <typename T, typename F, typename = void>
struct execute_member_trait : no_execute_member
{
};

template <typename T, typename F>
struct execute_member_trait<T, F,
  typename void_type<
    decltype(declval<T>().execute(declval<F>()))
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  using result_type = decltype(
    declval<T>().execute(declval<F>()));

  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = noexcept(
    declval<T>().execute(declval<F>())));
};

#else // defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

template <typename T, typename F, typename = void>
struct execute_member_trait :
  conditional<
    is_same<T, typename decay<T>::type>::value
      && is_same<F, typename decay<F>::type>::value,
    no_execute_member,
    traits::execute_member<
      typename decay<T>::type,
      typename decay<F>::type>
  >::type
{
};

#endif // defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

} // namespace detail
namespace traits {

template <typename T, typename F, typename>
struct execute_member_default :
  detail::execute_member_trait<T, F>
{
};

template <typename T, typename F, typename>
struct execute_member :
  execute_member_default<T, F>
{
};

} // namespace traits
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_TRAITS_EXECUTE_MEMBER_HPP
