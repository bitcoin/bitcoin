/* Copyright 2016-2017 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_DETAIL_IS_INVOCABLE_HPP
#define BOOST_POLY_COLLECTION_DETAIL_IS_INVOCABLE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <functional>
#include <type_traits>

/* technique explained at
 * http://bannalia.blogspot.com/2016/09/compile-time-checking-existence-of.html 
 */

namespace boost{
namespace poly_collection{
namespace detail{
namespace is_invocable_fallback{

template <typename F,typename... Args>
struct is_invocable:
  std::is_constructible<
    std::function<void(Args...)>,
    std::reference_wrapper<typename std::remove_reference<F>::type>
  >
{};

template <typename R,typename F,typename... Args>
struct is_invocable_r:
  std::is_constructible<
    std::function<R(Args...)>,
    std::reference_wrapper<typename std::remove_reference<F>::type>
  >
{};

struct hook{};

}}}}

namespace std{

template<>
struct is_void< ::boost::poly_collection::detail::is_invocable_fallback::hook>:
  std::false_type
{      
  template<typename F,typename... Args>
  static constexpr bool is_invocable_f()
  {
    using namespace ::boost::poly_collection::detail::is_invocable_fallback;
    return is_invocable<F,Args...>::value;
  }

  template<typename R,typename F,typename... Args>
  static constexpr bool is_invocable_r_f()
  {
    using namespace ::boost::poly_collection::detail::is_invocable_fallback;
    return is_invocable_r<R,F,Args...>::value;
  }
};

} /* namespace std */

namespace boost{

namespace poly_collection{

namespace detail{

template<typename F,typename... Args>
struct is_invocable:std::integral_constant<
  bool,
  std::is_void<is_invocable_fallback::hook>::template
    is_invocable_f<F,Args...>()
>{};

template<typename R,typename F,typename... Args>
struct is_invocable_r:std::integral_constant<
  bool,
  std::is_void<is_invocable_fallback::hook>::template
    is_invocable_r_f<R,F,Args...>()
>{};

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#endif
