//
// traits/bulk_execute_free.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_TRAITS_BULK_EXECUTE_FREE_HPP
#define BOOST_ASIO_TRAITS_BULK_EXECUTE_FREE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>

#if defined(BOOST_ASIO_HAS_DECLTYPE) \
  && defined(BOOST_ASIO_HAS_NOEXCEPT) \
  && defined(BOOST_ASIO_HAS_WORKING_EXPRESSION_SFINAE)
# define BOOST_ASIO_HAS_DEDUCED_BULK_EXECUTE_FREE_TRAIT 1
#endif // defined(BOOST_ASIO_HAS_DECLTYPE)
       //   && defined(BOOST_ASIO_HAS_NOEXCEPT)
       //   && defined(BOOST_ASIO_HAS_WORKING_EXPRESSION_SFINAE)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace traits {

template <typename T, typename F, typename N, typename = void>
struct bulk_execute_free_default;

template <typename T, typename F, typename N, typename = void>
struct bulk_execute_free;

} // namespace traits
namespace detail {

struct no_bulk_execute_free
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = false);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
};

#if defined(BOOST_ASIO_HAS_DEDUCED_BULK_EXECUTE_FREE_TRAIT)

template <typename T, typename F, typename N, typename = void>
struct bulk_execute_free_trait : no_bulk_execute_free
{
};

template <typename T, typename F, typename N>
struct bulk_execute_free_trait<T, F, N,
  typename void_type<
    decltype(bulk_execute(declval<T>(), declval<F>(), declval<N>()))
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  using result_type = decltype(
    bulk_execute(declval<T>(), declval<F>(), declval<N>()));

  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = noexcept(
    bulk_execute(declval<T>(), declval<F>(), declval<N>())));
};

#else // defined(BOOST_ASIO_HAS_DEDUCED_BULK_EXECUTE_FREE_TRAIT)

template <typename T, typename F, typename N, typename = void>
struct bulk_execute_free_trait :
  conditional<
    is_same<T, typename remove_reference<T>::type>::value
      && is_same<F, typename decay<F>::type>::value
      && is_same<N, typename decay<N>::type>::value,
    typename conditional<
      is_same<T, typename add_const<T>::type>::value,
      no_bulk_execute_free,
      traits::bulk_execute_free<typename add_const<T>::type, F, N>
    >::type,
    traits::bulk_execute_free<
      typename remove_reference<T>::type,
      typename decay<F>::type,
      typename decay<N>::type>
  >::type
{
};

#endif // defined(BOOST_ASIO_HAS_DEDUCED_BULK_EXECUTE_FREE_TRAIT)

} // namespace detail
namespace traits {

template <typename T, typename F, typename N, typename>
struct bulk_execute_free_default :
  detail::bulk_execute_free_trait<T, F, N>
{
};

template <typename T, typename F, typename N, typename>
struct bulk_execute_free :
  bulk_execute_free_default<T, F, N>
{
};

} // namespace traits
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_TRAITS_BULK_EXECUTE_FREE_HPP
