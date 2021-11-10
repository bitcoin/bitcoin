//
// traits/set_value_free.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_TRAITS_SET_VALUE_FREE_HPP
#define BOOST_ASIO_TRAITS_SET_VALUE_FREE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/detail/variadic_templates.hpp>

#if defined(BOOST_ASIO_HAS_DECLTYPE) \
  && defined(BOOST_ASIO_HAS_NOEXCEPT) \
  && defined(BOOST_ASIO_HAS_WORKING_EXPRESSION_SFINAE)
# define BOOST_ASIO_HAS_DEDUCED_SET_VALUE_FREE_TRAIT 1
#endif // defined(BOOST_ASIO_HAS_DECLTYPE)
       //   && defined(BOOST_ASIO_HAS_NOEXCEPT)
       //   && defined(BOOST_ASIO_HAS_WORKING_EXPRESSION_SFINAE)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace traits {

template <typename T, typename Vs, typename = void>
struct set_value_free_default;

template <typename T, typename Vs, typename = void>
struct set_value_free;

} // namespace traits
namespace detail {

struct no_set_value_free
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = false);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
};

#if defined(BOOST_ASIO_HAS_DEDUCED_SET_VALUE_FREE_TRAIT)

template <typename T, typename Vs, typename = void>
struct set_value_free_trait : no_set_value_free
{
};

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T, typename... Vs>
struct set_value_free_trait<T, void(Vs...),
  typename void_type<
    decltype(set_value(declval<T>(), declval<Vs>()...))
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  using result_type = decltype(
    set_value(declval<T>(), declval<Vs>()...));

  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = noexcept(
    set_value(declval<T>(), declval<Vs>()...)));
};

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T>
struct set_value_free_trait<T, void(),
  typename void_type<
    decltype(set_value(declval<T>()))
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);

  using result_type = decltype(set_value(declval<T>()));

  BOOST_ASIO_STATIC_CONSTEXPR(bool,
    is_noexcept = noexcept(set_value(declval<T>())));
};

#define BOOST_ASIO_PRIVATE_SET_VALUE_FREE_TRAIT_DEF(n) \
  template <typename T, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct set_value_free_trait<T, void(BOOST_ASIO_VARIADIC_TARGS(n)), \
    typename void_type< \
      decltype(set_value(declval<T>(), BOOST_ASIO_VARIADIC_DECLVAL(n))) \
    >::type> \
  { \
    BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true); \
  \
    using result_type = decltype( \
      set_value(declval<T>(), BOOST_ASIO_VARIADIC_DECLVAL(n))); \
  \
    BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = noexcept( \
      set_value(declval<T>(), BOOST_ASIO_VARIADIC_DECLVAL(n)))); \
  }; \
  /**/
BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_SET_VALUE_FREE_TRAIT_DEF)
#undef BOOST_ASIO_PRIVATE_SET_VALUE_FREE_TRAIT_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#else // defined(BOOST_ASIO_HAS_DEDUCED_SET_VALUE_FREE_TRAIT)

template <typename T, typename Vs, typename = void>
struct set_value_free_trait;

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T, typename... Vs>
struct set_value_free_trait<T, void(Vs...)> :
  conditional<
    is_same<T, typename remove_reference<T>::type>::value
      && conjunction<is_same<Vs, typename decay<Vs>::type>...>::value,
    typename conditional<
      is_same<T, typename add_const<T>::type>::value,
      no_set_value_free,
      traits::set_value_free<typename add_const<T>::type, void(Vs...)>
    >::type,
    traits::set_value_free<
      typename remove_reference<T>::type,
      void(typename decay<Vs>::type...)>
  >::type
{
};

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename T>
struct set_value_free_trait<T, void()> :
  conditional<
    is_same<T, typename remove_reference<T>::type>::value,
    typename conditional<
      is_same<T, typename add_const<T>::type>::value,
      no_set_value_free,
      traits::set_value_free<typename add_const<T>::type, void()>
    >::type,
    traits::set_value_free<typename remove_reference<T>::type, void()>
  >::type
{
};

#define BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME(n) \
  BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_##n

#define BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_1 \
  && is_same<T1, typename decay<T1>::type>::value
#define BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_2 \
  BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_1 \
  && is_same<T2, typename decay<T2>::type>::value
#define BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_3 \
  BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_2 \
  && is_same<T3, typename decay<T3>::type>::value
#define BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_4 \
  BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_3 \
  && is_same<T4, typename decay<T4>::type>::value
#define BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_5 \
  BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_4 \
  && is_same<T5, typename decay<T5>::type>::value
#define BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_6 \
  BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_5 \
  && is_same<T6, typename decay<T6>::type>::value
#define BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_7 \
  BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_6 \
  && is_same<T7, typename decay<T7>::type>::value
#define BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_8 \
  BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_7 \
  && is_same<T8, typename decay<T8>::type>::value

#define BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_TRAIT_DEF(n) \
  template <typename T, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  struct set_value_free_trait<T, void(BOOST_ASIO_VARIADIC_TARGS(n))> : \
    conditional< \
      is_same<T, typename remove_reference<T>::type>::value \
        BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME(n), \
      typename conditional< \
        is_same<T, typename add_const<T>::type>::value, \
        no_set_value_free, \
        traits::set_value_free< \
          typename add_const<T>::type, \
          void(BOOST_ASIO_VARIADIC_TARGS(n))> \
      >::type, \
      traits::set_value_free< \
        typename remove_reference<T>::type, \
        void(BOOST_ASIO_VARIADIC_DECAY(n))> \
    >::type \
  { \
  }; \
  /**/
BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_TRAIT_DEF)
#undef BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_TRAIT_DEF
#undef BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME
#undef BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_1
#undef BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_2
#undef BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_3
#undef BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_4
#undef BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_5
#undef BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_6
#undef BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_7
#undef BOOST_ASIO_PRIVATE_SET_VALUE_MEMBER_IS_SAME_8

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#endif // defined(BOOST_ASIO_HAS_DEDUCED_SET_VALUE_FREE_TRAIT)

} // namespace detail
namespace traits {

template <typename T, typename Vs, typename>
struct set_value_free_default :
  detail::set_value_free_trait<T, Vs>
{
};

template <typename T, typename Vs, typename>
struct set_value_free :
  set_value_free_default<T, Vs>
{
};

} // namespace traits
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_TRAITS_SET_VALUE_FREE_HPP
